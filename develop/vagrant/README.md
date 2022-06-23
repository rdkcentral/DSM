# Recommended Development Workflow
This describes the recommended workflow for developing Dobby and Dsm, although you are obviously free to use whatever tools you are most comfortable with.

## 1. Setup Vagrant
The Vagrant VM in the `vagrant` directory is recommended for Dobby and Dsm development. Follow the `README.md` file to set up the VM.

Once the VM is created, add the VM to your SSH config by running
```
vagrant ssh-config >> ~/.ssh/config
```

## 2. VSCode
Install VSCode if you haven't got it already, and install the Remote SSH extension: https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-ssh

Now in the bottom-left corner of VSCode, click the green arrow button, then `Remote-SSH: Connect to Host` and select `dobby-vagrant`. A new VSCode window will open. In this window, open the `~/srcDobby` and/or 'srcDsm' directories.

It is recommended to then install the following VSCode extensions if you don't already have them:

* C/C++
* CMake
* CMake Tools
* Trailing Spaces

Now make any changes and rebuild from the command line:
### Rebuilding Dobby
```
cd ~/srcDobby/build
cmake -DCMAKE_BUILD_TYPE=Debug -DRDK_PLATFORM=DEV_VM ../
make -j$(nproc)
```
### Rebuilding Dsm
```
cd ~/srcDsm/rdk-smart-router/dsm/build/
make
sudo make install
```

It is possible to add build-time arguments to CMake when using the "Build" button in VSCode (e.g. to make sure the platform is set to DEV_VM). Open/create the file .vscode/settings.
json and add the following JSON section:
```json
"cmake.configureArgs": [
    "-DRDK_PLATFORM=DEV_VM"
]
```
