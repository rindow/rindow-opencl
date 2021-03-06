Rindow OpenCL PHP extension
===========================
You can use OpenCL on PHP.
The version of OpenCL is limited to version 1.2, and we are considering porting to a wide range of environments.

Since our goal is to use it with the Rindow Neural Network Library, we currently only have the minimum required functionality. It will be expanded in the future.

Learn more about the Rindow Neural Network Library [here](https://rindow.github.io/).

Requirements
============

- PHP7.2 or PHP7.3 or PHP7.4 or PHP8.0
- interop-phpobjects/polite-math 1.0.3 or later
- LinearBuffer implements for interop-phpobjects (rindow_openblas etc.)
- OpenCL 1.2 drivers/libraries.
- Windows 10

AMD GPU/APU drivers for windows are including OpenCL drivers.
Probably you can use Intel OpenCL dirivers.
Intel OpenCL drivers can be downloaded from site https://software.intel.com/content/www/us/en/develop/articles/opencl-drivers.html

Recommend environment
=====================

- LinearBuffer implements - rindow-openblas 0.2.1 or later. [sources](https://github.com/rindow/rindow-openblas), [binaries](https://github.com/rindow_openblas/releases)
- BLAS libray for OpenCL implements - rindow-clblast 0.1.2. [sources](https://github.com/rindow/rindow-clblast), [binaries](https://github.com/rindow-clblast/releases)
- Matrix PHP library - rindow-math-matrix 1.0.8 or later. [sources](https://github.com/rindow/rindow-math-matrix)
- Machine learning library on PHP - rindow-neuralnetworks [sources](https://github.com/rindow/rindow-neuralnetworks)

How to setup pre-build binaries
===============================
You can download and use pre-built Windows binaries.
Download the binary for your version of PHP.

- https://github.com/rindow/rindow-openblas-binaries
- https://github.com/xianyi/OpenBLAS/releases
- https://github.com/rindow/rindow-opencl/releases
- https://github.com/rindow/rindow-clblast/releases
- https://github.com/CNugteren/CLBlast/releases

Please download the following two binaries and extract.

- The PHP extension of rindow-opencl that matches the php version.
- The PHP extension of rindow-openblas that matches the php version.
- The PHP extension of rindow-clblast that matches the php version.
- DLL of OpenBLAS library.
- DLL of CLBlast library.

Copy the shared library to the PHP extension directory and set it in php.ini.
And OpenBLAS DLL's path to Windows PATH environment variable.

```shell
C:\tmp>copy rindow_openblas.dll /path/to/php-installation-path/ext
C:\tmp>copy rindow_opencl.dll /path/to/php-installation-path/ext
C:\tmp>copy rindow_clblast.dll /path/to/php-installation-path/ext
C:\tmp>echo extension=rindow_openblas.dll >> /path/to/php-installation-path/php.ini
C:\tmp>echo extension=rindow_opencl.dll >> /path/to/php-installation-path/php.ini
C:\tmp>echo extension=rindow_clblast.dll >> /path/to/php-installation-path/php.ini
C:\tmp>PATH %PATH%;/path/to/OpenBLAS/bin;/path/to/CLBlast-Library/lib
C:\tmp>cd /some/app/directory
C:\app\dir>composer require rindow/rindow-math-matrix
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
- For PHP 8.0, Download the php-sdk from https://github.com/microsoft/php-sdk-binary-tools/releases/tag/php-sdk-2.2.0
- Extract to c:\php-sdk
- Download target dev-pack from https://windows.php.net/downloads/releases/
- Extract to /path/to/php-devel-pack-x.x.x-Win32-Vxxx-x64/

### Exports OpenCL binding library

```shell
C:\visual\studio\path>vcvars64 -vcvars_ver=14.16  <=== for PHP7.x
or
C:\visual\studio\path>vcvars64                    <=== for PHP8.0

C:\visual\studio\path>cd /path/to/OpenCL
C:\path\OpenCL>mkdir lib
C:\path\OpenCL>cd lib
C:\path\OpenCL\lib>dumpbin /exports \windows\system32\OpenCL.dll > OpenCL.def
C:\path\OpenCL\lib>notepad OpenCL.def                     #### edit def file
EXPORTS

clBuildProgram
.....

C:\path\OpenCL\lib>lib /def:OpenCL.def /machine:x64
```

### Download OpenCL Headers

- Download OpenCL Headers form https://github.com/KhronosGroup/OpenCL-Headers
- extract and copy to opencl development directory

```shell
TMP>xcopy OpenCL-Headers-20XX.XX.XX \path\OpenCL\include /S /I
```

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

If you want to build for PHP8.0, you just use Visual Studio Command Prompt.

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
$ configure --enable-rindow_opencl --with-prefix=/path/to/php-installation-path --with-opencl=/path/to/OpenCL-devel-directory
$ nmake clean
$ nmake
$ nmake test
```

### Install from built directory

- Copy the php extension binary(.dll) to the php/ext directory from here/arch/Releases_TS/php_rindow_opencl.dll
- Add the "extension=php_rindow_opencl" entry to php.ini
