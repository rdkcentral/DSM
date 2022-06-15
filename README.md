# What is DSM
This is an initial implementation that has been created based on discussion in PRPL enviroment. It utilizes TR-181's datamodel concept of Execution Environment, Deployment Unit and Execution Unit. By executing ations on those entities it translates behavior to providers using adapters:
1. Packager (tar, LISA)
2. Runtime (e.g. crun, Dobby)

# Build
It assumes Linux based system. Currently tested on ubuntu but has no external dependencies yet

1. Git clone project
2. Enter main directory
3. cmake .
4. make
5. go to ${main_dsm_diretgory}/build/bin
6. you can see 2 executables
    - dsm - this is main daemon with management logic
    - dsmcli - this is cli connecting to dsm over socket and executing commands

Althoug to build project dependencies are not required. **crun** is required in runtime to manage container

# Important folders
1. repo - this is packet of test packages / deploymen tunits
    - sample.tar and hello_world.tar are just simple packaged
    - busybox.tar is the package that has propper bundle definition created from standard busybox OCI image. It executes command `sleep infinity`. It is used for testing of start stop container
2. destination - this is destination directory used for installation
3. tests - noit updated for long time... i.e. wont work

# Use
Because of current configuration and fodler structure it assumes CWD is build/bin

1. Start dsm
2. Give commands to dsm
    - ./dsmcli eu.list
    - ./dsmcli du.list
    - ./dsmcli du.install [target execution env] [package name]
    - ./dsmcli du.list
    - ./dsmcli du.detail [package name]
    - ./dsmcli eu.list
    - ./dsmcli eu.detail [uid]
    - ./dsmcli eu.start [uid]
    - ./dsmcli du.detail [uid]
    - you can check also crun list
    - ./dsmcli eu.stop [uid]
    - ./dsmcli du.detail [uid]
    
# Idea
DSM is server with controller that manages status with concept of Execution Environments, Deployment Units and Execution Units. It uses Packager and Runtime to translate commands. Each technology e.g. TAR packager, crun runtime are attached by adapters
