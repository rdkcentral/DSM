#ifndef __LCM_DATAMODEL_C__
#define __LCM_DATAMODEL_C__

// If not stated otherwise in this file or this component's license file the
// following copyright and licenses apply:
//
// Copyright 2022 Consult Red
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// #include "usp_err_codes.h"
// #include "vendor_defs.h"
// #include "vendor_api.h"
// #include "usp_api.h"

// Number of elements in an array
#define NUM_ELEM(x) (sizeof((x)) / sizeof((x)[0]))

#include <string.h>
#include <stdio.h>

#include <pthread.h>
#include <sys/time.h>

static pthread_mutex_t LCM_commandLock;

#define LCM_CACHE_SIZE 25

static int LCM_CacheCount = 0;

typedef struct _tag_LCM_CacheElement {

    char command[1024];
    char result[1024];

    struct timeval tv;

}   LCM_CacheElement;

static LCM_CacheElement LCM_Cache[LCM_CACHE_SIZE];
static bool LCM_CacheInitOk = false;

static void LCM_CacheInit()
{
    if (!LCM_CacheInitOk)
    {
        memset(&(LCM_Cache),0,sizeof(LCM_Cache));
        LCM_CacheInitOk = true;
    }
}

static bool LCM_popCached(char *command,char *buf,size_t len)
{
    // Not very efficient but better than maxing out IPC!
    int i;

    LCM_CacheInit();

    fprintf(stdout,"Looking for cached %s\n",command);

    for (i=0;i<LCM_CacheCount;i++)
    {
        if (strcmp(command,LCM_Cache[i].command) == 0)
        {
            struct timeval now;

            gettimeofday(&(now),NULL);
            // Check time stamp is new enough

            fprintf(stdout,"Now %d Was %d\n",(int)(now.tv_sec),(int)(LCM_Cache[i].tv.tv_sec));

            if (now.tv_sec < LCM_Cache[i].tv.tv_sec + 3)
            {
                fprintf(stdout,"Using cached version of %s",command);
                strncpy(buf,LCM_Cache[i].result,len);

                return true;
            }
            else
            {
                fprintf(stdout,"Cached version too old, replace\n");
                return false;
            }
        }
    }

    fprintf(stdout,"Not founds\n");

    return false;
}

static void LCM_pushCached(char *command,char *result)
{
    // Not very efficient but better than maxing out IPC!
    int i;

    LCM_CacheInit();

    fprintf(stdout,"Pushing cached: command=%s, result=%s\n",command,result);

    for (i=0;i<LCM_CacheCount;i++)
    {
        if (strcmp(command,LCM_Cache[i].command) == 0)
        {
            fprintf(stdout,"Replacing existing\n");
            // Found so replace and update timestamp
            strncpy(LCM_Cache[i].result,result,sizeof(LCM_Cache[i].result));
            gettimeofday(&(LCM_Cache[LCM_CacheCount].tv),NULL);  

            return;
        }
    }

    // Not found, insert new
    if (LCM_CacheCount < LCM_CACHE_SIZE)
    {
        // Space for more, add on to the end
        fprintf(stdout,"New cache entry\n");

        strncpy(LCM_Cache[LCM_CacheCount].command,command,sizeof(LCM_Cache[LCM_CacheCount].command));
        strncpy(LCM_Cache[LCM_CacheCount].result ,result, sizeof(LCM_Cache[LCM_CacheCount].result));
        gettimeofday(&(LCM_Cache[LCM_CacheCount].tv),NULL);

        LCM_CacheCount++;
    }
    else
    {
        fprintf(stdout,"Random replace\n");

        // Pick cache entry at random and replace, assume rand is seeded and RAND_MAX > CACHE_SIZE
        int pick = rand() % LCM_CACHE_SIZE;

        strncpy(LCM_Cache[pick].command,command,sizeof(LCM_Cache[pick].command));
        strncpy(LCM_Cache[pick].result ,result, sizeof(LCM_Cache[pick].result));
        gettimeofday(&(LCM_Cache[LCM_CacheCount].tv),NULL);       
    }
}

static int LCM_execCommand(char *command,char *buf,int len,bool cached)
{
    int err = USP_ERR_INVALID_COMMAND_ARGS;

    if (buf != NULL && len != 0)
    {
        memset(buf,0,len);
    }
    else
    {
        // Can't cache without a buffer!
        cached = false;
    }

    pthread_mutex_lock(&LCM_commandLock);

    if (cached)
    {
        // Look for cached value
        if (LCM_popCached(command,buf,len))
        {
            // Early exit so unlock
            pthread_mutex_unlock(&LCM_commandLock);
            return USP_ERR_OK;
        }
    }

    fprintf(stdout,"Popen=%s\n",command);

    FILE *comm  = popen(command,"r");
    char *write = buf;
    int   remaining = len;

    fprintf(stdout,"Pipe is open %p\n",comm);

    if (comm != (FILE *)NULL)
    {
        // Read return until no more chars
        while(buf != NULL && !feof(comm))
        {
            size_t r = fread(write,sizeof(char),remaining,comm);
            write += r;
            remaining -= r;
        }

        fclose(comm);
        
        fprintf(stdout,"Pipe closed\n");

        err = USP_ERR_OK;

        if (cached)
        {
            LCM_pushCached(command,buf);
        }
    }

    pthread_mutex_unlock(&LCM_commandLock);

    return err;
}

static int LCM_getIndexFromPath(char *path)
{
    char buf[16];

    int index = -1;

    int first=-1,second=-1;
    int i = 0;

    fprintf(stdout,"Index from path %s\n",path);

    // Device.SoftwareModules.DeploymentUnit.1.Status, for example
    // Find last pair of ., index = chars between converted to int
    while(path[i] != '\0')
    {
        if (path[i] == '.')
        {
            if (first == -1)
            {
                first = i;
            }
            else if (second == -1)
            {
                second = i;
            }
            else
            {
                first = second;
                second = i;
            }
        }

        i++;
    }

    fprintf(stdout,"First = %d, Second = %d\n",first,second);

    if (first >= 0 && second > first)
    {
        memset(buf,0,sizeof(buf));

        int i = 0; int j=0;

        for (j=first+1;j<second;j++)
        {
            buf[i] = path[j];
            i++;

            if (i>=15) break;
        }

        index = atoi(buf);
    }

    return index;
}

static int LCM_DSMCLI_parseTag(char *in,char *tag,char openC,char closeC,char *buf,size_t bufSize)
{
    int err = -1;

    // e.g. tag = "result":
    // openC = [ or {
    // closeC = ] or }

    int tagLength = strlen(tag);
    int  inLength = strlen(in);

    int i,j,k;

    printf("in=%s, tag=%s, inL=%d, tagL=%d\n",in,tag,inLength,tagLength);

    for (i=0;i<inLength-tagLength;i++)
    {
        k = 0;

        for (j=i;j<i+tagLength;j++)
        {
            if (j>=inLength)
            {
                // Out of bounds!
                break;
            }

            if (in[j] != tag[k])
            {
                break;
            }

            k++;

            if (k==tagLength) // All chars matched
            {
                // Copy to buffer
                k = 0;
                j++;

                if (j>=inLength) return -1;
                if (in[j] != openC) return -1;

                j++;

                while(j<inLength && k<bufSize)
                {
                    if (in[j] == closeC)
                    {
                        buf[k] = '\0';
                        
                        return USP_ERR_OK;
                    }

                    buf[k] = in[j];
                    j++; k++;
                }

                break;
            }
        }
    }

    return err;
}

static int LCM_DSMCLI_parseVectorIndex(char *in,int index,char *buf,size_t bufSize)
{
    int commaCount = 0;
    int i = 0,j = 0;

    while(in[i] != '\0' && j<bufSize)
    {
        if (in[i] == ',')
        {
            commaCount++;

            // Indexes start at 1!
            if (index == commaCount)
            {
                // Found index
                buf[j] = '\0';

                return USP_ERR_OK;
            }
            else
            {
                // Start on the next item
                j = 0;
            }
        }
        else
        {
            if (in[i] != '"')
            {
                buf[j] = in[i]; j++;
            }
        }

        i++;
    }

    if (commaCount + 1 == index) // Last item special case
    {
        // Single item list!
        if (j<bufSize)
        {
            buf[j] = '\0';

            return USP_ERR_OK;
        }
    }
    return -1;
}

static int ValidateAdd_AddExecEnv(dm_req_t *req)
{
    return USP_ERR_OK;
}

static int Notify_AddExecEnv(dm_req_t *req)
{
    return USP_ERR_OK;
}

static int Notify_DeleteExecEnv(dm_req_t *req)
{
    return USP_ERR_OK;
}

// Device.SoftwareModules.ExecEnv.{i}.Status
static int LCM_ExecEnv_Status(dm_req_t *req, char *buf, int len)
{
    // Up, Error, Disabled
    snprintf(buf,len,"Up");
    
    fprintf(stdout,"Path = %s\n",req->path);
    return USP_ERR_OK;
}

static int LCM_ExecEnv_Enable(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"True");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_Alias(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

// Single EE for now, must align with dsm.config
static int LCM_ExecEnv_Name(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"test");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_Type(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_InitialRunLevel(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_CurrentRunLevel(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_Vendor(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Consult RED");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_Version(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"0.01");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_ParentExecEnv(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"<>");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_AllocatedDiskSpace(dm_req_t *req, char *buf, int len)
{
    // Up, Error, Disabled
    snprintf(buf,len,"-1");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_AvailableDiskSpace(dm_req_t *req, char *buf, int len)
{
    // Up, Error, Disabled
    snprintf(buf,len,"-1");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_AllocatedMemory(dm_req_t *req, char *buf, int len)
{
    // Up, Error, Disabled
    snprintf(buf,len,"-1");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_AvailableMemory(dm_req_t *req, char *buf, int len)
{
    // Up, Error, Disabled
    snprintf(buf,len,"-1");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_ActiveExecutionUnits(dm_req_t *req, char *buf, int len)
{
    // Up, Error, Disabled
    snprintf(buf,len,"<>");
    
    return USP_ERR_OK;
}

static int LCM_ExecEnv_ProcessorRefList(dm_req_t *req, char *buf, int len)
{
    // Up, Error, Disabled
    snprintf(buf,len,"<>");
    
    return USP_ERR_OK;
}

#define DEV_SM_ROOT "Device.SoftwareModules."

static void LCM_Register_ExecEnv(void)
{
    // Device.SoftwareModules.ExecEnv.{i}.
    USP_REGISTER_Object(DEV_SM_ROOT "ExecEnv.{i}", 
        ValidateAdd_AddExecEnv, NULL, Notify_AddExecEnv,
        NULL, NULL, Notify_DeleteExecEnv);

    // Device.SoftwareModules.ExecEnvNumberOfEntries
    USP_REGISTER_Param_NumEntries( // ExecEnv table
        "Device.SoftwareModules.ExecEnvNumberOfEntries",
        "Device.SoftwareModules.ExecEnv.{i}");

    // Enable (bool,W)
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.Enable"               ,LCM_ExecEnv_Enable              ,DM_BOOL); 
    // Status
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.Status"               ,LCM_ExecEnv_Status              ,DM_STRING);
    // Alias (W)
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.Alias"                ,LCM_ExecEnv_Alias               ,DM_STRING);
    // Name
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.Name"                 ,LCM_ExecEnv_Name                ,DM_STRING);
    // Type
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.Type"                 ,LCM_ExecEnv_Type                ,DM_STRING);
    // InitialRunLevel
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.InitialRunLevel"      ,LCM_ExecEnv_InitialRunLevel     ,DM_INT);
    // CurrentRunLevel
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.CurrentRunLevel"      ,LCM_ExecEnv_CurrentRunLevel     ,DM_INT);   
    // Vendor
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.Vendor"               ,LCM_ExecEnv_Vendor              ,DM_STRING);
    // Version
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.Version"              ,LCM_ExecEnv_Version             ,DM_STRING);
    // ParentExecEnv (empty)
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.ParentExecEnv"        ,LCM_ExecEnv_ParentExecEnv       ,DM_STRING);
    // AllocatedDiskSpace (-1)
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.AllocatedDiskSpace"   ,LCM_ExecEnv_AllocatedDiskSpace  ,DM_INT);
    // AvailableDiskSpace (-1)
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.AvailableDiskSpace"   ,LCM_ExecEnv_AvailableDiskSpace  ,DM_INT);
    // AllocatedMemory (-1)
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.AllocatedMemory"      ,LCM_ExecEnv_AllocatedMemory     ,DM_INT);
    // AvailableMemory (-1)
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.AvailableMemory"      ,LCM_ExecEnv_AvailableMemory     ,DM_INT);
    // ActiveExecutionUnits , sep list of paths names in EU table
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.ActiveExecutionUnits" ,LCM_ExecEnv_ActiveExecutionUnits,DM_STRING);
    // ProcessorRefList
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecEnv.{i}.ProcessorRefList"     ,LCM_ExecEnv_ProcessorRefList    ,DM_STRING);   

    // SetRunLevel operate(int=requestedRunLevel) SYNC
    // Reset() operate() SYNC

    USP_DM_InformInstance(DEV_SM_ROOT "ExecEnv.1");
    
}

static int ValidateAdd_AddDeploymentUnit(dm_req_t *req)
{
    return USP_ERR_OK;
}

static int Notify_AddDeploymentUnit(dm_req_t *req)
{
    return USP_ERR_OK;
}

static int Notify_DeleteDeploymentUnit(dm_req_t *req)
{
    return USP_ERR_OK;
}

static int LCM_DSM_getDUPropertyTag(dm_req_t *req,char *tag,char *buf,size_t len)
{
    int err = -1;

    char duList[2048];
    int index;

    fprintf(stdout,"Status request on path %s",req->path);
    index = LCM_getIndexFromPath(req->path);
    fprintf(stdout,"Index = %d\n",index);

    snprintf(buf,len,"Unknown");
    
    // dsmcli du.list (cached)
    err = LCM_execCommand("dsmcli du.list",duList,sizeof(duList),true);

    if (err == USP_ERR_OK)
    {
        char urlVector[2048];

        // duVector out of result
        err = LCM_DSMCLI_parseTag(duList,"\"result\":",'[',']',urlVector,sizeof(urlVector));

        if (err == USP_ERR_OK)
        {
            char url[1024];

            fprintf(stdout,"Found duList URL Vector %s\n",urlVector);

            err = LCM_DSMCLI_parseVectorIndex(urlVector,index,url,sizeof(url));

            if (err == USP_ERR_OK)
            {
                char comm[2048];
                char info[2048];

                fprintf(stdout,"Url = %s\n",url);
                snprintf(comm,sizeof(comm),"dsmcli du.detail %s",url);

                err = LCM_execCommand(comm,info,sizeof(info),true);

                if (err == USP_ERR_OK)
                {
                    char props[2048];

                    fprintf(stdout,"Info=%s\n",info);

                    err = LCM_DSMCLI_parseTag(info,"\"result\":",'{','}',props,sizeof(props));

                    if (err == USP_ERR_OK)
                    {
                        fprintf(stdout,"Props = %s\n",props);

                        // And finally!
                        err = LCM_DSMCLI_parseTag(props,tag,'"','"',buf,len);
                    }
                }
            }
        }
    }

    // Vector element (index), URL
    // dsmcli du.info (cached)
    // property(status)
    // MAP

    return err;

}

// dsm du info properties:

// "parent-ee":"test","state":"Installed"
// "URI":"http://127.0.0.1/packages/x86-64/sleepy1.tar",
// "UUID":"http://127.0.0.1/packages/x86-64/sleepy1.tar",
// "eu":"9K",
// "eu.path":"/home/dave/Work/RDKCentral/DSM/testing/destination/sleepy1",
// "parent-ee":"test",
// "state":"Installed", tag="state"

static int LCM_DeploymentUnit_UUID(dm_req_t *req, char *buf, int len)   { return LCM_DSM_getDUPropertyTag(req,"\"UUID\":",buf,len); }
static int LCM_DeploymentUnit_DUID(dm_req_t *req, char *buf, int len)   { return LCM_DSM_getDUPropertyTag(req,"\"UUID\":",buf,len); }

static int LCM_DeploymentUnit_Alias(dm_req_t *req, char *buf, int len) 
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_DeploymentUnit_Name(dm_req_t *req, char *buf, int len)   { return LCM_DSM_getDUPropertyTag(req,"\"UUID\":",buf,len); }

static int LCM_DeploymentUnit_Status(dm_req_t *req, char *buf, int len) { return LCM_DSM_getDUPropertyTag(req,"\"state\":",buf,len); }

static int LCM_DeploymentUnit_Resolved(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"True");
    
    return USP_ERR_OK;
}

static int LCM_DeploymentUnit_URL(dm_req_t *req, char *buf, int len)    { return LCM_DSM_getDUPropertyTag(req,"\"URI\":",buf,len); }

static int LCM_DeploymentUnit_Description(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_DeploymentUnit_Vendor(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_DeploymentUnit_Version(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_DeploymentUnit_VendorLogList(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_DeploymentUnit_VendorConfigList(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_DeploymentUnit_ExecutionUnitList(dm_req_t *req, char *buf, int len) { return LCM_DSM_getDUPropertyTag(req,"\"eu\":",buf,len); }

static int LCM_DeploymentUnit_ExecutionEnvRef(dm_req_t *req, char *buf, int len)   { return LCM_DSM_getDUPropertyTag(req,"\"eu\":",buf,len); }

static char *LCM_DeploymentUnit_Update_inputArgs[] =
{
    "URL",
    "Username",
    "Password"
};

static int LCM_DeploymentUnit_Update(dm_req_t *req, char *command_key, kv_vector_t *input_args, kv_vector_t *output_args)
{
    // NOT YET SUPPORTED
    return USP_ERR_INVALID_ARGUMENTS;
}

// static char *LCM_DeploymentUnit_Uninstall_inputArgs[] = NO ARGS
static int LCM_DeploymentUnit_Uninstall(dm_req_t *req, char *command_key, kv_vector_t *input_args, kv_vector_t *output_args)
{
    int err = USP_ERR_INVALID_ARGUMENTS;
    char url[1024];

    // Ensure that no output arguments are returned for this sync operation
    USP_ARG_Init(output_args);

    err = LCM_DSM_getDUPropertyTag(req,"\"URI\":",url,sizeof(url));

    if (err == USP_ERR_OK)
    {
        char comm[2048];

        snprintf(comm,sizeof(comm),"dsmcli du.uninstall %s",url);
        err = LCM_execCommand(comm,NULL,0,false);
    }

    fprintf(stdout,"Returning from uninstall %d\n",err);

    return err;
}

static void LCM_Register_DeploymentUnit(void)
{
    // Device.SoftwareModules.DeploymentUnit.{i}.
    USP_REGISTER_Object(DEV_SM_ROOT "DeploymentUnit.{i}", 
        ValidateAdd_AddDeploymentUnit, NULL, Notify_AddDeploymentUnit,
        NULL, NULL, Notify_DeleteDeploymentUnit);

    USP_REGISTER_Param_NumEntries( // DeploymentUnit
        "Device.SoftwareModules.DeploymentUnitNumberOfEntries",
        "Device.SoftwareModules.DeploymentUnit.{i}");

    // UUID
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.UUID"             ,LCM_DeploymentUnit_UUID             ,DM_STRING);
    // DUID
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.DUID"             ,LCM_DeploymentUnit_DUID             ,DM_STRING);
    // Alias
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.Alias"            ,LCM_DeploymentUnit_Alias            ,DM_STRING);
    // Name
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.Name"             ,LCM_DeploymentUnit_Name             ,DM_STRING);
    // Status
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.Status"           ,LCM_DeploymentUnit_Status           ,DM_STRING);

    // Resolved (Bool)
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.Resolved"         ,LCM_DeploymentUnit_Resolved         ,DM_STRING);
    // URL
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.URL"              ,LCM_DeploymentUnit_URL              ,DM_STRING);
    // Description
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.Description"      ,LCM_DeploymentUnit_Description      ,DM_STRING);
    // Vendor
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.Vendor"           ,LCM_DeploymentUnit_Vendor           ,DM_STRING);
    // Version
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.Version"          ,LCM_DeploymentUnit_Version          ,DM_STRING);
    // VendorLogList
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.VendorLogList"    ,LCM_DeploymentUnit_VendorLogList    ,DM_STRING);
    // VendorConfigList
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.VendorConfigList" ,LCM_DeploymentUnit_VendorConfigList ,DM_STRING);
    // ExecutionUnitList
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.ExecutionUnitList",LCM_DeploymentUnit_ExecutionUnitList,DM_STRING);
    // ExecutionEnvRef
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "DeploymentUnit.{i}.ExecutionEnvRef"  ,LCM_DeploymentUnit_ExecutionEnvRef  ,DM_STRING);

    // Update() ASYNC (URL,Username,Password)
    USP_REGISTER_SyncOperation       (DEV_SM_ROOT "DeploymentUnit.{i}.Update()"         ,LCM_DeploymentUnit_Update);
    USP_REGISTER_OperationArguments  (DEV_SM_ROOT "DeploymentUnit.{i}.Update()",
        LCM_DeploymentUnit_Update_inputArgs,NUM_ELEM(LCM_DeploymentUnit_Update_inputArgs),NULL,0);

    // Uninstall() ASYNC
    USP_REGISTER_SyncOperation       (DEV_SM_ROOT "DeploymentUnit.{i}.Uninstall()"      ,LCM_DeploymentUnit_Uninstall);
    USP_REGISTER_OperationArguments  (DEV_SM_ROOT "DeploymentUnit.{i}.Uninstall()"      ,NULL,0,NULL,0);

    return;
}

static int ValidateAdd_AddExecutionUnit(dm_req_t *req)
{
    return USP_ERR_OK;
}

static int Notify_AddExecutionUnit(dm_req_t *req)
{
    return USP_ERR_OK;
}

static int Notify_DeleteExecutionUnit(dm_req_t *req)
{
    return USP_ERR_OK;
}

static int LCM_DSM_getEUPropertyTag(dm_req_t *req,char *tag,char *buf,size_t len)
{
    int err = -1;

    char euList[2048];
    int index;

    fprintf(stdout,"Status request on path %s",req->path);
    index = LCM_getIndexFromPath(req->path);
    fprintf(stdout,"Index = %d\n",index);

    snprintf(buf,len,"Unknown");
    
    // dsmcli du.list (cached)
    err = LCM_execCommand("dsmcli eu.list",euList,sizeof(euList),true);

    if (err == USP_ERR_OK)
    {
        char idVector[2048];

        // euVector out of result
        err = LCM_DSMCLI_parseTag(euList,"\"result\":",'[',']',idVector,sizeof(idVector));

        if (err == USP_ERR_OK)
        {
            char id[1024];

            fprintf(stdout,"Found euList ID Vector %s\n",idVector);

            err = LCM_DSMCLI_parseVectorIndex(idVector,index,id,sizeof(id));

            if (err == USP_ERR_OK)
            {
                char comm[2048];
                char info[2048];

                fprintf(stdout,"id = %s\n",id);
                snprintf(comm,sizeof(comm),"dsmcli eu.detail %s",id);

                err = LCM_execCommand(comm,info,sizeof(info),true);

                if (err == USP_ERR_OK)
                {
                    char props[2048];

                    fprintf(stdout,"Info=%s\n",info);

                    err = LCM_DSMCLI_parseTag(info,"\"result\":",'{','}',props,sizeof(props));

                    if (err == USP_ERR_OK)
                    {
                        fprintf(stdout,"Eu Props = %s\n",props);

                        // And finally!
                        err = LCM_DSMCLI_parseTag(props,tag,'"','"',buf,len);
                    }
                }
            }
        }
    }

    return err;
}

// EU detail
// {"du":"http://127.0.0.1/packages/x86-64/sleepy1.tar","ee":"test","path":"/home/dave/Work/RDKCentral/DSM/testing/destination/sleepy1","status":"Idle","uid":"9K"}}

static int LCM_ExecutionUnit_EUID(dm_req_t *req, char *buf, int len)         { return LCM_DSM_getEUPropertyTag(req,"\"uid\":"     ,buf,len); }

static int LCM_ExecutionUnit_Alias(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_Name(dm_req_t *req, char *buf, int len)         { return LCM_DSM_getEUPropertyTag(req,"\"uid\":"    ,buf,len); }

static int LCM_ExecutionUnit_ExecEnvLabel(dm_req_t *req, char *buf, int len) { return LCM_DSM_getEUPropertyTag(req,"\"ee\":"    , buf,len); }

static int LCM_ExecutionUnit_Status(dm_req_t *req, char *buf, int len)       { return LCM_DSM_getEUPropertyTag(req,"\"status\":", buf,len); }

static int LCM_ExecutionUnit_ExecutionFaultCode(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"NoFault");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_ExecutionFaultMessage(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"NoFault");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_AutoStart(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"False");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_RunLevel(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"-1");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_Vendor(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_Version(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_Description(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_DiskSpaceInUse(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"-1");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_MemoryInUse(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"-1");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_References(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_AssociatedProcessList(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_VendorLogList(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_VendorConfigList(dm_req_t *req, char *buf, int len)
{
    snprintf(buf,len,"Unknown");
    
    return USP_ERR_OK;
}

static int LCM_ExecutionUnit_ExecutionEnvRef(dm_req_t *req, char *buf, int len) { return LCM_DSM_getEUPropertyTag(req,"\"ee\":"    , buf,len); }

static char *LCM_ExecutionUnit_SetRequestedState_inputArgs[] =
{
    "RequestedState" // RequestedState=Idle, RequestedState=Active
};

static int LCM_ExecutionUnit_SetRequestedState(dm_req_t *req, char *command_key, kv_vector_t *input_args, kv_vector_t *output_args)
{
    int err = USP_ERR_INVALID_ARGUMENTS;

    USP_ARG_Init(output_args);

    char *Status = USP_ARG_Get(input_args,"RequestedState",NULL);

    if (Status != NULL)
    {
        int index = 0;

        index = LCM_getIndexFromPath(req->path);

        if (index > 0)
        {
            char euList[1024];

            // Get execution unit vector
                // dsmcli du.list (cached)
            err = LCM_execCommand("dsmcli eu.list",euList,sizeof(euList),true);

            if (err == USP_ERR_OK)
            {
                char idVector[2048];

                // euVector out of result
                err = LCM_DSMCLI_parseTag(euList,"\"result\":",'[',']',idVector,sizeof(idVector));

                if (err == USP_ERR_OK)
                {
                    char id[1024];

                    fprintf(stdout,"Found euList ID Vector %s\n",idVector);
                    err = LCM_DSMCLI_parseVectorIndex(idVector,index,id,sizeof(id));

                    if (err == USP_ERR_OK)
                    {
                        char comm[2048];

                        fprintf(stdout,"id = %s Status = %s\n",id,Status);

                        if      (strcmp(Status,"Active") == 0)
                        {
                            snprintf(comm,sizeof(comm),"dsmcli eu.start %s",id);
                        }
                        else if (strcmp(Status,"Idle") == 0)
                        {
                            snprintf(comm,sizeof(comm),"dsmcli eu.stop %s",id);
                        }
                        else
                        {
                            // Invalid Status
                            return -1;
                        }

                        err = LCM_execCommand(comm,NULL,0,false);
                    }
                }
            }
        }
    }

    return err;
}

static void LCM_Register_ExecutionUnit(void)
{
    // Device.SoftwareModules.ExecutionUnit.{i}.
    USP_REGISTER_Object(DEV_SM_ROOT "ExecutionUnit.{i}", 
        ValidateAdd_AddExecutionUnit, NULL, 
        Notify_AddExecutionUnit, NULL, NULL, 
        Notify_DeleteExecutionUnit);

    USP_REGISTER_Param_NumEntries( // DeploymentUnit
        "Device.SoftwareModules.ExecutionUnitNumberOfEntries",
        "Device.SoftwareModules.ExecutionUnit.{i}");

    // EUID
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.EUID"                 ,LCM_ExecutionUnit_EUID                 ,DM_STRING);
    // Alias
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.Alias"                ,LCM_ExecutionUnit_Alias                ,DM_STRING);    
    // Name
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.Name"                 ,LCM_ExecutionUnit_Name                 ,DM_STRING);   
    // ExecEnvLabel
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.ExecEnvLabel"         ,LCM_ExecutionUnit_ExecEnvLabel         ,DM_STRING);   
    // Status
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.Status"               ,LCM_ExecutionUnit_Status               ,DM_STRING);  
    // ExecutionFaultCode
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.ExecutionFaultCode"   ,LCM_ExecutionUnit_ExecutionFaultCode   ,DM_STRING); 
    // ExecutionFaultMessage
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.ExecutionFaultMessage",LCM_ExecutionUnit_ExecutionFaultMessage,DM_STRING); 
    // AutoStart (Bool)
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.AutoStart"            ,LCM_ExecutionUnit_AutoStart            ,DM_BOOL); 
    // RunLevel (Int)
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.RunLevel"             ,LCM_ExecutionUnit_RunLevel             ,DM_INT);
    // Vendor
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.Vendor"               ,LCM_ExecutionUnit_Vendor               ,DM_STRING);
    // Version
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.Version"              ,LCM_ExecutionUnit_Version              ,DM_STRING);
    // Description
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.Description"          ,LCM_ExecutionUnit_Description          ,DM_STRING);
    // DiskSpaceInUse (Int)
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.DiskSpaceInUse"       ,LCM_ExecutionUnit_DiskSpaceInUse       ,DM_INT);
    // MemoryInUse (Int)
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.MemoryInUse"          ,LCM_ExecutionUnit_MemoryInUse          ,DM_INT);
    // References
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.References"           ,LCM_ExecutionUnit_References           ,DM_STRING);
    // AssociatedProcessList
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.AssociatedProcessList",LCM_ExecutionUnit_AssociatedProcessList,DM_STRING);
    // VendorLogList
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.VendorLogList"        ,LCM_ExecutionUnit_VendorLogList        ,DM_STRING);
    // VendorConfigList
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.AssociatedConfigList" ,LCM_ExecutionUnit_VendorConfigList     ,DM_STRING);
    // ExecutionEnvRef
    USP_REGISTER_VendorParam_ReadOnly(DEV_SM_ROOT "ExecutionUnit.{i}.ExecutionEnvRef"      ,LCM_ExecutionUnit_ExecutionEnvRef      ,DM_STRING);

    // SetRequestedState (RequestedState=Idle|Active)
    USP_REGISTER_SyncOperation       (DEV_SM_ROOT "ExecutionUnit.{i}.SetRequestedState()"  ,LCM_ExecutionUnit_SetRequestedState);
    USP_REGISTER_OperationArguments  (DEV_SM_ROOT "ExecutionUnit.{i}.SetRequestedState()",
        LCM_ExecutionUnit_SetRequestedState_inputArgs,NUM_ELEM(LCM_ExecutionUnit_SetRequestedState_inputArgs),NULL,0);

    return;
}

static char *LCM_DeploymentUnit_InstallDU_inputArgs[] =
{
    "URL",
    "UUID",
    "Username",
    "Password",
    "ExecutionEnvRef",
};

static int LCM_DeploymentUnit_InstallDU(dm_req_t *req, char *command_key, kv_vector_t *input_args, kv_vector_t *output_args)
{
    int  err = USP_ERR_INVALID_ARGUMENTS;

    char command[1024];
    char out[1024];

    // Ensure that no output arguments are returned for this sync operation
    USP_ARG_Init(output_args);

    // Get URL --- We will ignore other args for now!
    char *URL   = USP_ARG_Get(input_args,"URL",NULL);
    // char *UUID  = USP_ARG_Get(input_args,"UUID",NULL);

    // char *Username = USP_ARG_Get(input_args,"Username",NULL);
    // char *Password = USP_ARG_Get(input_args,"Password",NULL);

    // char *EEREF = USP_ARG_Get(input_args,"ExecutionEnvRef",NULL);

    if (URL != NULL) // We have a URL
    {
        memset(command,0,sizeof(command));
        memset(out,0,sizeof(out));

        snprintf(command,sizeof(command),"dsmcli du.install test %s",URL);

        err = LCM_execCommand(command,out,sizeof(out),false);
    }

    return err;
}

static void LCM_Register_InstallDU(void)
{
    // Device.SoftwareModules.InstallDU()

    // Not async (FIX)
    USP_REGISTER_SyncOperation     (DEV_SM_ROOT "InstallDU()",LCM_DeploymentUnit_InstallDU);
    USP_REGISTER_OperationArguments(DEV_SM_ROOT "InstallDU()",
        LCM_DeploymentUnit_InstallDU_inputArgs,NUM_ELEM(LCM_DeploymentUnit_InstallDU_inputArgs),NULL,0);

}

#if 0 // For future use

// DSMCLI parsing utils (cheap JSON equiv!)
static int LCM_DSMCLI_parseResponse(char *in,char *out,size_t outSize)
{
    int length = 0;
    int i = 0;

    memset(out,0,outSize);

    length = strlen(in);

    // For for chars in sequence, response: se:<space>
    while(i<length-3)
    {
        if (in[i] == 'e' && in[i+1] == ':' && in[i+2] == ' ')
        {
            // Found
            strncpy(out,&(in[i+3]),outSize);
            return 0;
        }
    }

    return -1;
}

#endif

static int LCM_DSMCLI_countChars(char *in,char toCount)
{
    int count = 0;
    int i = 0;

    // Assume 0 terminated
    while(in[i] != '\0')
    {
        if (in[i] == toCount) count++;
        i++;
    }

    return count;
}

static bool LCM_DSMCLI_isEmpty(char *in)
{
    // Look for []
    int i=0,length;

    length = strlen(in); // Not efficient to keep doing this!

    while(i<length-1)
    {
        if (in[i] == '[')
        {
            if (in[i+1] == ']')
            {   
                return true;
            }
            else
            {
                return false;
            }
        }

        i++;
    }

    return false;
}

#include <pthread.h>
#include <unistd.h>

static int   LCM_DU_monitorCountDUs(char *buf)
{
    int commaCount = 0;

    // An empty vector means no DUs
    if (LCM_DSMCLI_isEmpty(buf))
    {
        // fprintf(stdout,"Empty DU list\n");
        return 0;
    }

    // Count the commas!
    commaCount = LCM_DSMCLI_countChars(buf,',');
    // fprintf(stdout,"Command count is %d",commaCount);
    
    if (commaCount > 0)
    {
        commaCount--;
    }

    return commaCount;
}

// int USP_SIGNAL_ObjectAdded(char *path);
// int USP_SIGNAL_ObjectDeleted(char *path);

static void *LCM_DU_monitorThread(void *arg)
{
    int err;
    static int duCount = 0;

    char buf[1024];
    char object[1024];

    // Get initial count and set up
    int i=0,count=0;

    // Keep trying until we get a first response
    do
    {
        /* code */
        err = LCM_execCommand("dsmcli du.list",buf,sizeof(buf),false);

        if (err == USP_ERR_OK && strlen(buf) > 50) // If there's real data response is longer than 50 chars (trust me!)
        {
            count = LCM_DU_monitorCountDUs(buf);

            if (count > 0)
            {
                for (i=0;i<count;i++)
                {
                    snprintf(object,sizeof(object),DEV_SM_ROOT "DeploymentUnit.%d",i+1);
                    fprintf(stdout,"Signal add on %s\n",object);
                    USP_SIGNAL_ObjectAdded(object);
                }

                duCount = count;
            }
        }
        else
        {
            fprintf(stdout,"DU list retry\n");
            usleep(2 * 1000 * 1000);

            err = -1;
        }
    } while (err != USP_ERR_OK);

    while(true)
    {
        // Poll every 2 seconds!
        usleep(2 * 1000 * 1000);

        if (LCM_execCommand("dsmcli du.list",buf,sizeof(buf),false) == USP_ERR_OK)
        {
            int newCount = LCM_DU_monitorCountDUs(buf);

            // fprintf(stdout,"NUMBER OF DUS=%d\n",duCount);

            if (duCount == newCount - 1)
            {
                // New count
                snprintf(object,sizeof(object),DEV_SM_ROOT "DeploymentUnit.%d",newCount);
                fprintf(stdout,"Signal add on %s\n",object);

                USP_SIGNAL_ObjectAdded(object);
            }

            if (duCount == newCount + 1)
            {
                // Remove old count
                snprintf(object,sizeof(object),DEV_SM_ROOT "DeploymentUnit.%d",duCount);
                fprintf(stdout,"Signal delete on %s\n",object);

                USP_SIGNAL_ObjectDeleted(object);
            }

            duCount = newCount;
        }
        else
        {
            fprintf(stdout,"DU monitor command failed\n");
        }
    }

    return NULL;
}

// Will work the same for EUs
#define LCM_EU_monitorCountEUs(X) LCM_DU_monitorCountDUs(X)

// Monitor threads should be same code with different data!
static void *LCM_EU_monitorThread(void *arg)
{
    int err;
    static int euCount = 0;

    char buf[1024];
    char object[1024];

    // Get initial count and set up
    int i=0,count=0;

    usleep(1 * 1000 * 1000);

    // Keep trying until we get a first response
    do
    {
        /* code */
        err = LCM_execCommand("dsmcli eu.list",buf,sizeof(buf),false);

        if (err == USP_ERR_OK && strlen(buf) > 50)
        {
            count = LCM_EU_monitorCountEUs(buf);

            if (count > 0)
            {
                for (i=0;i<count;i++)
                {
                    snprintf(object,sizeof(object),DEV_SM_ROOT "ExecutionUnit.%d",i+1);
                    USP_SIGNAL_ObjectAdded(object);
                }

                euCount = count;
            }
        }
        else
        {
            usleep(2 * 1000 * 1000);
            fprintf(stdout,"EU list retry\n");

            err = -1;
        }
    } while (err != USP_ERR_OK);

    while(true)
    {
        // Poll every 2 seconds!
        usleep(2 * 1000 * 1000);

        if (LCM_execCommand("dsmcli eu.list",buf,sizeof(buf),false) == USP_ERR_OK)
        {
            int newCount = LCM_EU_monitorCountEUs(buf);

            // fprintf(stdout,"NUMBER OF DUS=%d\n",duCount);

            if (euCount == newCount - 1)
            {
                // New count
                snprintf(object,sizeof(object),DEV_SM_ROOT "ExecutionUnit.%d",newCount);
                // fprintf(stdout,"Signal add on %s\n",object);

                USP_SIGNAL_ObjectAdded(object);
            }

            if (euCount == newCount + 1)
            {
                // Remove old count
                snprintf(object,sizeof(object),DEV_SM_ROOT "ExecutionUnit.%d",euCount);
                // fprintf(stdout,"Signal delete on %s\n",object);

                USP_SIGNAL_ObjectDeleted(object);
            }

            euCount = newCount;
        }
        else
        {
            fprintf(stdout,"EU monitor command failed\n");
        }
    }

    return NULL;
}

static pthread_t duMonitorTID;
static pthread_t euMonitorTID;

static void LCM_RegisterDataModel(void)
{
    // TR-181 Device.SoftwareModules data model

    // Execution Environment Table
    LCM_Register_ExecEnv();
    // Deployment Unit Table
    LCM_Register_DeploymentUnit();
    // Execution Unit Table
    LCM_Register_ExecutionUnit();

    // Install Deployment Unit Operate
    LCM_Register_InstallDU();

    // Start monitor (polling)
    pthread_mutex_init(&LCM_commandLock,NULL);

    pthread_create(&(duMonitorTID), NULL, LCM_DU_monitorThread, NULL);
    pthread_create(&(euMonitorTID), NULL, LCM_EU_monitorThread, NULL);

    return;
}

static int LCM_VENDOR_Init(void)
{
    LCM_RegisterDataModel();
    
    return USP_ERR_OK;
}

static int LCM_VENDOR_Start(void)
{
    // We should split our init into two (Init, Start)
    return USP_ERR_OK;
}

static int LCM_VENDOR_Stop(void)
{
    // Stop threads here!
    return USP_ERR_OK;
}


#endif // #ifndef __LCM_DATAMODEL_C__
