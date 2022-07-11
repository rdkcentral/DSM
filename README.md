# What is DSM
DSM delegates between package/download components, and container runtime managers. It further provides an "up-facing" interface upon which remote management may be built to control the lifecyle of containerized services.

This is an initial implementation. It utilizes TR-181's datamodel concept of Execution Environment, Deployment Unit and Execution Unit. By executing actions on those entities it translates behavior to providers using adapters:

1. Packager (tar (now), LIRA (future))
2. Runtime (Dobby/crun)

# Build
It assumes Linux based system. Currently tested on ubuntu/debian but has no external dependencies yet

1. Git clone project
2. Enter main directory
3. mkdir build
4. cd build
5. cmake ..
6. make, and optionally make install (usually requires sudo)
7. go to <dsm>/build/bin, or /usr/local/bin (after install)
8. you can see 2 executables
    - dsm - this is main daemon with management logic
    - dsmcli - this is cli connecting to dsm over socket and executing commands

Although to build the project dependencies are not required. Dobby is required in runtime to manage the container. Note: A Vagrant VM provides all dependencies for developers.

# Important folders
1. repo - this is packet of test packages / deployment units
    - sample.tar and hello_world.tar are just simple packaged
    - busybox.tar is the package that has propper bundle definition created from standard busybox OCI image. It executes command `sleep infinity`. It is used for testing of start stop container
    - Packages may also be downloaded over HTTP by specifying http://<package> as the URL
    - With the default packager, packages must be of the form [package-name].tar and have the structure [package-name]/rootfs & [package-name]/config.json (These are OCI Bundles)
2. destination - this is the destination directory used for installation

# Use
export DSM_CONFIG_FILE=<full_path_of_dsm.config> e.g. /home/vagrant/dsm.config (This is set by default in Vagrant VM, see src/dsm.config for an example)

1. Start dsm, % dsm
2. Give commands to dsm
    - % dsmcli eu.list
    - % dsmcli du.list
    - % dsmcli du.install [target execution env] [package name/url]
    - % dsmcli du.list
    - % dsmcli du.detail [package name]
    - % dsmcli eu.list
    - % dsmcli eu.detail [uid]
    - % dsmcli eu.start [uid]
    - % dsmcli du.detail [uid]
    - you can check also use DobbyTool list and DobbyTool info
    - % dsmcli eu.stop [uid]
    - % dsmcli du.detail [uid]
    
# Idea
DSM is a server with controller that manages status with concept of Execution Environments, Deployment Units and Execution Units. It uses Packager and Runtime to translate commands. Each technology e.g. TAR packager, Dobby/crun runtime are attached by adapters

# Repo Structure
1. src/ DSM source code
2. integration/ Materials (patch sets and scripts) for end-to-end integration in, for example, RDK-B 22q1 on RPI with TR-181 Data Model. Integration to RDK-B will ultimately be supported by modification to the relevant layers, and recipes.
3. develop/ Support for developers including Vagrant machine definition for end-to-end development from USP Data Model, to DSM, to Dobby

