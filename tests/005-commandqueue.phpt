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
$context = new Rindow\OpenCL\Context(OpenCL::CL_DEVICE_TYPE_DEFAULT);
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
?>
--EXPECT--
SUCCESS
SUCCESS finish
