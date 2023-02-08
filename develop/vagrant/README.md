# Recommended Development Workflow
This describes the recommended workflow for developing Dobby and Dsm, although you are obviously free to use whatever tools you are most comfortable with.

### 1. Building the vagrant VM
For development on PC we are using vagrant so that all components can be built and tested in a consistent environment. The Vagrantfile now builds DSM and Dobby.

First time:

```
sudo apt install vagrant
```
You may also want to install virtualbox on your dev machine if you don't already have it:
```
sudo apt install virtualbox
```

### 2. Setup Vagrant
The Vagrant VM in the `vagrant` directory is recommended for Dobby and Dsm development. Follow the `README.md` file to set up the VM.

Once the VM is created, add the VM to your SSH config by running
```
vagrant ssh-config >> ~/.ssh/config
```

### 3. VSCode
Install VSCode if you haven't got it already, and install the Remote SSH extension: https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-ssh

Now in the bottom-left corner of VSCode, click the green arrow button, then `Remote-SSH: Connect to Host` 

It is recommended to then install the following VSCode extensions if you don't already have them:

* C/C++
* CMake
* CMake Tools
* Trailing Spaces

### 4. Connect to Vagrant
```
git clone https://github.com/rdkcentral/DSM.git
cd DSM/develop/vagrant/
To connect to the running VM (note: this must be done from the DSM/develop/vagrant/ directory):
vagrant up
vagrant ssh
```


### 5. Take vagrant snapshot

vagrant has support for taking "snapshots" of the VM filesystem. These can save and restore the state of the VM allowing you to return it to a previous condition

It is recommended that you create a "base" snapshot after the initial VM creation and setup , given the time that it takes to create a fresh VM and the possibility of breaking a VM in one way or another while developing.

#Save a snapshot named "base" using the current name of the VM (lcm-vagrant-focal in this case)
```
vagrant snapshot save lcm-vagrant-focal base
```
to restore the VM back to the base snapshot
```
vagrant snapshot restore base
```
Please be careful when restoring and make sure any changes you want to keep i.e. local commits or changes to files and configs are backed up elsewhere, as the restore command will wipe them when it restores the base image.

### 6. Re-provisioning vagrant 
At some points during development it may be desirable to re-run all the provisioners, i.e. after a change to the vagrant scripts has added new components or updated the versions of existing components

re-running provisioners is possible by running
```
vagrant reload --provision

```

### 7. Build issues you may encounter for vagrant:
The version of vagrant-libvirt that had been installed on my system when I used "sudo apt install vagrant" was old and it failed to update it automagically.
so I had to

```
sudo apt remove vagrant-libvirt
```
After that, use following command to install vagrant libvirt plugin

```
vagrant plugin install vagrant-libvirt
```
Hit an error caused by unknown configuration section 'disksize'

There are errors in the configuration of this machine. Please fix
the following errors and try again:
  
Vagrant:
* Unknown configuration section 'disksize'.
I manually installed the plugin for disksize

```
vagrant plugin install vagrant-disksize
```
If your Linux flavour has Secure Boot enabled, you will be required to sign the modules of the VM libs of the provider. You can use the following command to do so
```
sudo /usr/src/linux-headers-$(uname -r)/scripts/sign-file sha256 ./MOK.priv ./MOK.der $(modinfo -n MODNAME)
```
MOK.priv and MOK.der are the Machine Owner Keys you have to generate for yourself and enroll into the system. The sign-file should be shipped with your distribution or you should be able to download it in one of the packages for your distro.

### 8. Building Dsm inside vagrant.
```
cd srcDsm/DSM/src
mkdir build
cde build
  * build without rbus
    cmake ../..
* build  rbus
    cmake ../.. -D ENABLE_RBUS_PROVIDER=ON
make
sudo make install
```

### 9. Building Dobby inside vagrant.
```
cd ~/srcDobby/build
cmake ../
make
sudo make install
```

### 10. Setting up a server with python to host containers
```
sudo python3 -m http.server 8080

```

### 11. Clean up any installed containers.
```
rm -rf ~/destination/*

```


### 12. Run inside vagrant DSM with Dobby and rbus
```
vagrant ssh
sudo DobbyDaemon --nofork
#with rbus we also need
vagrant ssh
killall -9 rtrouted;rm -fr /tmp/rtroute*;rtrouted -f -l DEBUG
```
In he next terminal start dsm:
```
vagrant ssh
dsm

```


### 13. Deployment containers via rbus:

* DSM rbus provider implements installDU() with URL (mandatory) and name (optional default is "default")


* start processes see #[here](### 10. Run inside vagrant DSM with Dobby and rbus)

```
rbuscli method_values "Device.SoftwareModules.InstallDU()" URL string http://${SERVER_IP}:8080/example_container.tar 
```
ExecutionEnvRef string test
* check state of example_container via

Check container gets deployed .
ls -al ~destination/

```
rbuscli getvalues "Device.SoftwareModules.ExecutionUnit."
```
  It will show status Idle.
* start example_container container dsmcli eu.start <eu_id>
* check state of example_container via

```
rbuscli getvalues "Device.SoftwareModules.ExecutionUnit."
```
  It will show status Active.
* dsmcli eu.detail <eu_id> will show status Active.
* sudo DobbyTool list will show container example_container running.
* Stop example_container container dsmcli eu.stop <eu_id>
* check state of example_container via rbuscli getvalues "Device.SoftwareModules.ExecutionUnit." It will show status Idle


### 14. Set EU Active and Idle state via rbus:

DSM's functions start/stop available over RBUS SetRequestedState
```
rbuscli method_values "Device.SoftwareModules.ExecutionUnit.1.SetRequestedState()" RequestedState string Active
#above line is equivalent to dsmcli eu.start <eu_id>
rbuscli method_values "Device.SoftwareModules.ExecutionUnit.1.SetRequestedState()" RequestedState string Idle
#above line is equivalent to dsmcli eu.stop <eu_id>
```

### 15. DSM du.uninstall available over RBUS as well.


#Check via rbus
```
rbuscli getvalues "Device.SoftwareModules.DeploymentUnit."
```
#uninstalled via rbus
```
rbuscli method_noargs "Device.SoftwareModules.DeploymentUnit.1.Uninstall()"
```
#Check , It will show empty.
```
rbuscli getvalues "Device.SoftwareModules.DeploymentUnit."
```