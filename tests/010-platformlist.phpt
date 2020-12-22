--TEST--
PlatformList
--SKIPIF--
<?php
if (!extension_loaded('rindow_opencl')) {
	echo 'skip';
}
?>
--FILE--
<?php
$loader = include __DIR__.'/../vendor/autoload.php';
include __DIR__.'/../testPHP/HostBuffer.php';
use Interop\Polite\Math\Matrix\OpenCL;
#
# construct by default
#
$platforms = new Rindow\OpenCL\PlatformList();
#echo "count=".$platforms->count()."\n";
assert($platforms->count()>=0);
echo "SUCCESS construct by default\n";
#
# getone
#
$num = $platforms->count();
$one = $platforms->getOne(0);
assert($platforms->count()==1);
#echo "CL_PLATFORM_NAME=".$one->getInfo(0,OpenCL::CL_PLATFORM_NAME)."\n";
echo "SUCCESS getOne\n";
#
# get info
#
$n = $platforms->count();
assert(null!=$platforms->getInfo(0,OpenCL::CL_PLATFORM_NAME));
echo "SUCCESS info\n";
for($i=0;$i<$n;$i++) {
    assert(null!=$platforms->getInfo($i,OpenCL::CL_PLATFORM_NAME));
    #echo "platform(".$i.")\n";
    #echo "    CL_PLATFORM_NAME=".$platforms->getInfo($i,OpenCL::CL_PLATFORM_NAME)."\n";
    #echo "    CL_PLATFORM_PROFILE=".$platforms->getInfo($i,OpenCL::CL_PLATFORM_PROFILE)."\n";
    #echo "    CL_PLATFORM_VERSION=".$platforms->getInfo($i,OpenCL::CL_PLATFORM_VERSION)."\n";
    #echo "    CL_PLATFORM_VENDOR=".$platforms->getInfo($i,OpenCL::CL_PLATFORM_VENDOR)."\n";
    #echo "    CL_PLATFORM_EXTENSIONS=".$platforms->getInfo($i,OpenCL::CL_PLATFORM_EXTENSIONS)."\n";
}
echo "SUCCESS";
?>
--EXPECT--
SUCCESS construct by default
SUCCESS getOne
SUCCESS info
SUCCESS
