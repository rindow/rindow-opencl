--TEST--
Context from DeviceType
--SKIPIF--
<?php
if (!extension_loaded('rindow_opencl')) {
	echo 'skip';
}
?>
--FILE--
<?php
$loader = include __DIR__.'/../vendor/autoload.php';
use Interop\Polite\Math\Matrix\OpenCL;
$platforms = new Rindow\OpenCL\PlatformList();
$devices = new Rindow\OpenCL\DeviceList($platforms);
$total_dev = $devices->count();
assert($total_dev>=0);

#
#  construct by default
#
$context = new Rindow\OpenCL\Context(OpenCL::CL_DEVICE_TYPE_DEFAULT);
echo "SUCCESS construct by default\n";
#
#  construct context from type
#
$count = 0;
foreach([OpenCL::CL_DEVICE_TYPE_GPU,OpenCL::CL_DEVICE_TYPE_CPU] as $type) {
    try {
        $context = new Rindow\OpenCL\Context($type);
        $con_type = $context->getInfo(OpenCL::CL_CONTEXT_DEVICES)->getInfo(0,OpenCL::CL_DEVICE_TYPE);
        assert(true==($con_type&$type));
        $count++;
    } catch(\RuntimeException $e) {
        ;
    }
}
assert($total_dev==$count);
echo "SUCCESS construct from device type\n";
#
#  construct context from device_id
#
$platform = new Rindow\OpenCL\PlatformList();
$count = 0;
foreach([OpenCL::CL_DEVICE_TYPE_GPU,OpenCL::CL_DEVICE_TYPE_CPU] as $type) {
    try {
        $devices = new Rindow\OpenCL\DeviceList($platform,0,$type);
        $context = new Rindow\OpenCL\Context($devices);
        #echo $context->getInfo(OpenCL::CL_CONTEXT_DEVICES)->getInfo(0,OpenCL::CL_DEVICE_NAME)."\n";
        $con_type = $context->getInfo(OpenCL::CL_CONTEXT_DEVICES)->getInfo(0,OpenCL::CL_DEVICE_TYPE);
        assert(true==($con_type&$type));
        $count++;
    } catch(\RuntimeException $e) {
        ;
    }
}
assert($total_dev==$count);
echo "SUCCESS construct from device_id\n";
#
#  get information
#

$context = new Rindow\OpenCL\Context(OpenCL::CL_DEVICE_TYPE_DEFAULT);
assert(1==$context->getInfo(OpenCL::CL_CONTEXT_REFERENCE_COUNT));
$devices = $context->getInfo(OpenCL::CL_CONTEXT_DEVICES);
assert($devices instanceof Rindow\OpenCL\DeviceList);
$properties = $context->getInfo(OpenCL::CL_CONTEXT_PROPERTIES);
assert(is_array($properties));
echo "SUCCESS get array info\n";
/*
echo $context->getInfo(OpenCL::CL_CONTEXT_REFERENCE_COUNT)."\n";
echo "CL_CONTEXT_REFERENCE_COUNT=".$context->getInfo(OpenCL::CL_CONTEXT_REFERENCE_COUNT)."\n";
echo "CL_CONTEXT_NUM_DEVICES=".$context->getInfo(OpenCL::CL_CONTEXT_NUM_DEVICES)."\n";
echo "CL_CONTEXT_DEVICES=";
$devices = $context->getInfo(OpenCL::CL_CONTEXT_DEVICES);
echo "deivces(".$devices->count().")\n";
for($i=0;$i<$devices->count();$i++) {
    echo "    CL_DEVICE_NAME=".$devices->getInfo($i,OpenCL::CL_DEVICE_NAME)."\n";
    echo "    CL_DEVICE_VENDOR=".$devices->getInfo($i,OpenCL::CL_DEVICE_VENDOR)."\n";
    echo "    CL_DEVICE_TYPE=(";
    $device_type = $devices->getInfo($i,OpenCL::CL_DEVICE_TYPE);
    if($device_type&OpenCL::CL_DEVICE_TYPE_CPU) { echo "CPU,"; }
    if($device_type&OpenCL::CL_DEVICE_TYPE_GPU) { echo "GPU,"; }
    if($device_type&OpenCL::CL_DEVICE_TYPE_ACCELERATOR) { echo "ACCEL,"; }
    if($device_type&OpenCL::CL_DEVICE_TYPE_CUSTOM) { echo "CUSTOM,"; }
    echo ")\n";
    echo "    CL_DRIVER_VERSION=".$devices->getInfo($i,OpenCL::CL_DRIVER_VERSION)."\n";
    echo "    CL_DEVICE_VERSION=".$devices->getInfo($i,OpenCL::CL_DEVICE_VERSION)."\n";
}
echo "CL_CONTEXT_PROPERTIES=(".implode(',',array_map(function($x){ return "0x".dechex($x);},
    $context->getInfo(OpenCL::CL_CONTEXT_PROPERTIES))).")\n";
*/
echo 'SUCCESS';
?>
--EXPECT--
SUCCESS construct by default
SUCCESS construct from device type
SUCCESS construct from device_id
SUCCESS get array info
SUCCESS
