--TEST--
CommandQueue
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
use Interop\Polite\Math\Matrix\NDArray;
try {
    $context = new Rindow\OpenCL\Context(OpenCL::CL_DEVICE_TYPE_GPU);
} catch(RuntimeException $e) {
    $context = new Rindow\OpenCL\Context(OpenCL::CL_DEVICE_TYPE_DEFAULT);
}
$queue = new Rindow\OpenCL\CommandQueue($context);
echo "SUCCESS\n";

$hostBuffer = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
foreach(range(0,15) as $value) {
    $hostBuffer[$value] = $value;
}
$buffer = new Rindow\OpenCL\Buffer($context,intval(16*32/8),
    OpenCL::CL_MEM_READ_WRITE);
$buffer->write($queue,$hostBuffer,0,0,0,false);
$queue->flush();
$queue->finish();
echo "SUCCESS finish\n";

$retContext = $queue->getContext();
assert(Rindow\OpenCL\Context::class == get_class($retContext));
echo "SUCCESS getContext\n";

//assert(1==$queue->getInfo(OpenCL::CL_QUEUE_REFERENCE_COUNT));
/*
{
    $queue = new Rindow\OpenCL\CommandQueue($context);

    echo "========\n";
    $devices = $queue->getInfo(OpenCL::CL_QUEUE_DEVICE);
    echo "deivces(".$devices->count().")\n";
    for($i=0;$i<$devices->count();$i++) {
        echo "    CL_DEVICE_NAME=".$devices->getInfo($i,OpenCL::CL_DEVICE_NAME)."\n";
        echo "    CL_DEVICE_VENDOR=".$devices->getInfo($i,OpenCL::CL_DEVICE_VENDOR)."\n";
    }
    echo "CL_QUEUE_REFERENCE_COUNT=".$queue->getInfo(OpenCL::CL_QUEUE_REFERENCE_COUNT)."\n";
    echo "CL_QUEUE_PROPERTIES=(".implode(',',array_map(function($x){ return "0x".dechex($x);},
        $queue->getInfo(OpenCL::CL_QUEUE_PROPERTIES))).")\n";
}
*/
?>
--EXPECT--
SUCCESS
SUCCESS finish
SUCCESS getContext