IMPORTANT
=========
Development of this program has ended.
Please migrate to the program below instead.

- https://github.com/rindow/rindow-opencl-ffi

We stopped using PHP extensions because it was too difficult to prepare binary files for each PHP version and Linux version.


Rindow OpenCL PHP extension
===========================
You can use OpenCL on PHP.
The version of OpenCL is limited to version 1.2(1.1 with restrictions), and we are considering porting to a wide range of environments.

Since our goal is to use it with the Rindow Neural Network Library, we currently only have the minimum required functionality. It will be expanded in the future.

Learn more about the Rindow Neural Network Library [here](https://rindow.github.io/neuralnetworks/).

Requirements
============

- PHP7.2 or PHP7.3 or PHP7.4 or PHP8.0 or PHP8.1 or PHP8.2 or PHP8.3
- interop-phpobjects/polite-math 1.0.3 or later
- LinearBuffer implements for interop-phpobjects (rindow_openblas etc.)
- OpenCL 1.2 ICL loader and OpenCL 1.1/1.2 drivers
- Windows / Linux

AMD GPU/APU drivers for windows are including OpenCL drivers.

Recommend environment
=====================

- PHP8.1 or PHP8.2 or PHP8.3
- OpenBLAS [sources](https://github.com/OpenMathLib/OpenBLAS), [binaries](https://github.com/OpenMathLib/OpenBLAS/releases)
- LinearBuffer implements - rindow-openblas 0.4.0 or later. [sources](https://github.com/rindow/rindow-openblas), [binaries](https://github.com/rindow/rindow-openblas/releases)
- OpenCL binding for PHP - rindow-opencl 0.2.0. [sources](https://github.com/rindow/rindow-opencl), [binaries](https://github.com/rindow-opencl/releases)
- BLAS libray for OpenCL implements - rindow-clblast 0.2.0. [sources](https://github.com/rindow/rindow-clblast), [binaries](https://github.com/rindow-clblast/releases)
- Matrix PHP library - rindow-math-matrix 2.0.0 or later. [sources](https://github.com/rindow/rindow-math-matrix)
- Driver Pack - rindow-math-matrix-matlibext 1.0.0 or later. [sources](https://github.com/rindow/rindow-math-matrix-matlibext)


How to build from source code on Linux
========================================
You can also build and use from source code.

### Install OpenCL ICD and Tool
```shell
$ sudo apt install clinfo
```

### Install Hardware-dependent OpenCL library.
For example, in the case of Ubuntu standard AMD driver, install as follows

```shell
$ sudo apt install mesa-opencl-icd
$ sudo mkdir -p /usr/local/usr/lib
$ sudo ln -s /usr/lib/clc /usr/local/usr/lib/clc
```

In addition, there are the following drivers.

- mesa-opencl-icd
- beignet-opencl-icd
- intel-opencl-icd
- nvidia-opencl-icd-xxx
- pocl-opencl-icd

### Check OpenCL status
How to check the installation status

```shell
$ clinfo
Number of platforms                               1
  Platform Name                                   Clover
  Platform Vendor                                 Mesa
  Platform Version                                OpenCL 1.1 Mesa 21.2.6
  Platform Profile                                FULL_PROFILE
  Platform Extensions                             cl_khr_icd
  Platform Extensions function suffix             MESA
....
...
..
.
```

### Install build tools and libray
Install gcc development environment and opencl library. Then install the php development environment according to the target php version.

```shell
$ sudo apt install build-essential autoconf automake libtool bison re2c
$ sudo apt install pkg-config
$ sudo apt install phpX.X-dev (ex. php8.1-dev)
$ sudo apt install ocl-icd-opencl-dev
$ sudo apt install ./rindow-openblas-phpX.X_X.X.X_amd64.deb (ex. php8.1)
```

### Build
Run the target php version of phpize and build.

```shell
$ git clone https://github.com/rindow/rindow-opencl
$ cd rindow-opencl
$ composer update
$ phpizeX.X (ex. phpize8.1)
$ mv build/Makefile.global build/Makefile.global.orig
$ sed -f Makefile.global.patch < build/Makefile.global.orig > build/Makefile.global
$ ./configure --enable-rindow_opencl --with-php-config=php-configX.X (ex. php-config8.1)
$ make clean
$ make
$ make test
```

### Install from built directory

```shell
$ sudo make install
```
Add the "extension=rindow_opencl" entry to php.ini

If you want an easier install, use the following spell instead of "make install" and creating an ini file.

```shell
$ sh ./packaging.sh X.X   (ex. sh ./packaging.sh 8.1)
$ sudo apt install ./rindow-opencl-phpX.X_X.X.X_amd64.deb (ex. ./rindow-opencl-php8.1_...)
```

How to build from source code on Windows
========================================

### Install VC15 or VS16
Developing PHP extensions from php7.2 to php7.4 requires VC15 instead of the latest VC.

- Install Microsoft Visual Studio 2019 or later installer
- Run Installer with vs2017 build tools option.

Developing PHP extensions from php8.0 requires VS16. You can use Visual Studio 2019.

### php sdk and devel-pack binaries for windows

- You must know that PHP 7.2,7.3 and 7.4 needs environment for the MSVC version vc15 (that means Visual Studio 2017). php-sdk releases 2.1.9 supports vc15.
- For PHP 7.x, Download the php-sdk from https://github.com/microsoft/php-sdk-binary-tools/releases/tag/php-sdk-2.1.9
- If you want to build extensions for PHP 8.0, You have to use php-sdk release 2.2.0. It supports vs16.
- For PHP 8.0,8.1,8.2 Download the php-sdk from https://github.com/microsoft/php-sdk-binary-tools/releases/tag/php-sdk-2.2.0
- Extract to c:\php-sdk
- Download target dev-pack from https://windows.php.net/downloads/releases/
- Extract to /path/to/php-devel-pack-x.x.x-Win32-Vxxx-x64/

### Download OpenCL-SDK

- Download OpenCL SDK form https://github.com/KhronosGroup/OpenCL-SDK/releases
- extract and copy to opencl development directory

### Install and setup rindow_openblas for test

In order to execute rindow_opencl, you need a buffer object that implements the LinearBuffer interface. Please install rindow_openblas to run the tests.
See https://github.com/rindow/rindow-openblas/ for more information.

```shell
C:\tmp>copy rindow_openblas.dll /path/to/php-installation-path/ext
C:\tmp>echo extension=rindow_openblas.dll >> /path/to/php-installation-path/php.ini
C:\tmp>PATH %PATH%;/path/to/OpenBLAS/bin
```

### Start php-sdk for target PHP version

Open Visual Studio Command Prompt for VS for the target PHP version(see stepbystepbuild.)
Note that when you build for PHP7.x, you must explicitly specify the version of vc15 for which php.exe was built.
The -vcvars_ver=14.16 means vc15.

If you want to build for PHP8.0, 8.1, 8.2 you just use Visual Studio Command Prompt.

```shell
C:\visual\studio\path>vcvars64 -vcvars_ver=14.16
or
C:\visual\studio\path>vcvars64

C:\tmp>cd c:\php-sdk
C:\php-sdk>phpsdk-vc15-x64.bat
or
C:\php-sdk>phpsdk-vs16-x64.bat
```

### Build

```shell
$ cd /path/to/here
$ composer update
$ /path/to/php-devel-pack-x.x.x-Win32-VXXX-x64/phpize.bat
$ configure --enable-rindow_opencl --with-prefix=/path/to/php-installation-path --with-opencl=/path/to/OpenCL-SDK-directory
$ nmake clean
$ nmake
$ nmake test
```

### Install from built directory

- Copy the php extension binary(.dll) to the php/ext directory from here/arch/Releases_TS/php_rindow_opencl.dll
- Add the "extension=php_rindow_opencl" entry to php.ini
