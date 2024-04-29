#!/bin/sh

PHP_CONFIG=`which php-config$1`
if [ -z "$1" ]
then
	echo "Argument 1 must be the php version"
	exit
fi
if [ ! -x "$PHP_CONFIG" ]
then
	echo "Invalid version $1";
	exit
fi
PHP_VERSION=$1
PKG_WORK=pkgwork
EXTENSION_DIR=`$PHP_CONFIG --extension-dir`
if [ `echo $PHP_VERSION | awk -F. '{printf "%2d%02d%02d", $1,$2,$3}'` -lt "070400" ]
then
        INI_DIR=/etc/php/$PHP_VERSION/cli/conf.d
else
        INI_DIR=`$PHP_CONFIG --ini-dir`
fi
RINDOW_OPENCL_VERSION=`fgrep "# define PHP_RINDOW_OPENCL_VERSION" php_rindow_opencl.h | cut -d " " -f 4 | cut -d "\"" -f 2`
#. /etc/os-release
#OS_VERSION=$ID$VERSION_ID
echo EXTENSION_DIR=$EXTENSION_DIR
echo INI_DIR=$INI_DIR
echo RINDOW_OPENCL_VERSION=$RINDOW_OPENCL_VERSION
#echo OS_VERSION=$OS_VERSION
rm -rf $PKG_WORK
mkdir -p $PKG_WORK$EXTENSION_DIR
mkdir -p $PKG_WORK$INI_DIR
mkdir -p $PKG_WORK/DEBIAN
cp modules/rindow_opencl.so $PKG_WORK$EXTENSION_DIR/.
chmod 744 $PKG_WORK$EXTENSION_DIR/rindow_opencl.so
cp conf/20-rindow_opencl.ini $PKG_WORK$INI_DIR/.
chmod 744 $PKG_WORK$INI_DIR/20-rindow_opencl.ini
sed -e s/%PHP_RINDOW_OPENCL_VERSION%/$RINDOW_OPENCL_VERSION/g debian/control | \
#sed -e s/%OS_VERSION%/$OS_VERSION/g | \
sed -e s/%PHP_VERSION%/$PHP_VERSION/g > $PKG_WORK/DEBIAN/control
#sed -e s@%EXTENSION_DIR%@$EXTENSION_DIR@g debian/rules | \
#sed -e s@%INI_DIR%@$INI_DIR@g debian/rules | \
#	> $PKG_WORK/DEBIAN/rules
#cp debian/changelog $PKG_WORK/DEBIAN/.
#cp debian/copyright $PKG_WORK/DEBIAN/.

rm -f rindow-opencl*.deb
fakeroot dpkg-deb --build pkgwork .
