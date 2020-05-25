How to build AntilatencyIpTrackingDemoProvider
----------------------------------------------

# Raspberry Pi

## First time

Execute:
```
sudo apt install git cmake build-essential wiringpi && \
git clone https://github.com/antilatency/Antilatency.IpTrackingDemo.Provider.RaspberryPi && \
cd Antilatency.IpTrackingDemo.Provider.RaspberryPi && \
git submodule init && git submodule update && \
mkdir build && cd build && \
cmake .. && \
cmake --build . --target all && \
sudo mkdir -p /opt/antilatency/bin /opt/antilatency/lib && \
sudo chown -R pi:pi /opt/antilatency && \
cmake -P ./cmake_install.cmake && \
/opt/antilatency/bin/AntilatencyIpTrackingDemoProvider --help
```


## Next time

Go to build directory and run:
```
cmake .. && cmake --build . --target all
```
to build. Then run:
```
cmake -P ./cmake_install.cmake
```
to install.



# Linux cross build

## Preparation

```
sudo apt install git cmake crossbuild-essential-armhf gdb-multiarch openssh-client && \
git clone https://github.com/antilatency/Antilatency.IpTrackingDemo.Provider.RaspberryPi
```
Place [sysroot](https://yadi.sk/d/e7zrbAj5iRGafw) to the `/opt/rpi3-sysroot` directory.


## Console

```
cd Antilatency.IpTrackingDemo.Provider.RaspberryPi && \
git submodule init && git submodule update && \
mkdir build && cd build && \
cmake -D CMAKE_TOOLCHAIN_FILE=../LinuxToolchainRpi.cmake .. && \
cmake --build . --target all
scp AntilatencyIpTrackingDemoProvider pi@RPI_IP_ADDRESS:/opt/antilatency/bin/
```


## QtCreator

1. `sudo apt install qtcreator`
2. Setup Kit and project for Raspberry Pi:
 * Add device: Tools -> Option -> Devices -> Add Generic Linux Device. Name: rpi3a-plus, Host name: `RPI_IP_ADDRESS`, Private key file: Create new, then Deploy public key, Apply.
 * Tools -> Options -> Kits -> Add. Name: rpi3a-plus-cmake, Device type: Generic Linux Device, Device: rpi3a-plus, Sysroot: /opt/rpi3-sysroot, Compiler: C: GCC (C, arm 32 bit in /usr/bin), C++: GCC (C++, arm 32 bit in /usr/bin), Debugger: gdb-multiarch, Qt version: None, CMake Tool: CMake (Qt).
3. File -> Open file or project -> Antilatency.IpTrackingDemo.Provider.RaspberryPi/CMakeList.txt
4. Projects -> Run: delete "Install into temporary host directory deploy" step.



# Windows cross build

## Preparation

1. Install [SysGCC GNU toolchain](http://sysprogs.com/getfile/566/raspberry-gcc8.3.0.exe) to C:\SysGCC.
2. Use Update sysroot utilitity from [SysGCC 6.3.0](http://sysprogs.com/getfile/478/raspberry-gcc6.3.0-r5.exe) or just download and extract to C:\SysGCC\raspberry\arm-linux-gnueabihf\sysroot [sysroot.7z](https://yadi.sk/d/2N9vsbO_NGfJ2g).
3. Create copy of make.exe with name mingw32-make.exe in the C:\SysGCC\raspberry\bin folder.
4. Install [CMake](https://cmake.org/download/).
5. Enable OpenSSH client (Windows 10 -> Apps & features -> Optional features -> Add a feature: OpenSSH Client) or install [WinSCP](https://winscp.net/eng/download.php).
6. Install [Git](https://winscp.net/eng/download.php).
7. Clone Antilatency.IpTrackingDemo.Provider.RaspberryPi git repo using GitBash, cmd.exe or GUI tool:
```
git clone https://github.com/antilatency/Antilatency.IpTrackingDemo.Provider.RaspberryPi
```


## Command prompt

Run cmd.exe and execute commands:
```
cd Antilatency.IpTrackingDemo.Provider.RaspberryPi
git submodule init && git submodule update
mkdir build && cd build
"C:\Program Files\CMake\bin\cmake.exe" -G"MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=..\WindowsToolchainRpi.cmake ..
"C:\Program Files\CMake\bin\cmake.exe" --build . --target all
scp AntilatencyIpTrackingDemoProvider pi@RPI_IP_ADDRESS:/opt/antilatency/bin/
```


## QtCreator

1. Install [QtCreator](https://www.qt.io/product/development-tools).
2. Add device: Tools -> Option -> Devices -> Add Generic Linux Device. Name: rpi3a-plus, Host name: `RPI_IP_ADDRESS`, Private key file: Create new, then Deploy public key, Apply.
3. Tools -> Options -> Kits -> Add. Name: rpi3a-plus-cmake, Device type: Generic Linux Device, Device: rpi3a-plus, Sysroot: C:/SysGCC/raspberry/arm-linux-gnueabihf/sysroot, Compiler: C: GCC (C, arm 32 bit in C:\SysGCC\raspberry\bin), C++: GCC (C++, arm 32 bit in C:\SysGCC\raspberry\bin), Qt version: None, CMake Tool: CMake (Qt).
4. File -> Open file or project -> Antilatency.IpTrackingDemo.Provider.RaspberryPi/CMakeList.txt
5. Install AntilatencyIpTrackingDemoProvider binary to the Raspberry Pi board using scp command or WinSCP app.

