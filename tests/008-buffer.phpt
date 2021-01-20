--TEST--
Buffer
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
use Interop\Polite\Math\Matrix\NDArray;
use Interop\Polite\Math\Matrix\OpenCL;

//
//  Constract buffer
//
$context = new Rindow\OpenCL\Context(OpenCL::CL_DEVICE_TYPE_DEFAULT);
$queue = new Rindow\OpenCL\CommandQueue($context);
$hostBuffer = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
foreach(range(0,15) as $value) {
    $hostBuffer[$value] = $value;
}

$buffer = new Rindow\OpenCL\Buffer($context,intval(16*32/8),
    OpenCL::CL_MEM_READ_WRITE);
assert($buffer->dtype()==0);
assert($buffer->value_size()==0);
$buffer->write($queue,$hostBuffer);
assert($buffer->dtype()==NDArray::float32);
assert($buffer->value_size()==(32/8));
echo "SUCCESS Pure buffer\n";
//
//  Constract buffer with null
//
$buffer = new Rindow\OpenCL\Buffer($context,intval(16*32/8),
    $flags=null,$htbuffer=null,$offset=null);
echo "SUCCESS constructor with null\n";
//
//  Constract with host buffer
//
$buffer = new Rindow\OpenCL\Buffer($context,intval(16*32/8),
    OpenCL::CL_MEM_USE_HOST_PTR,$hostBuffer);
assert($buffer->dtype()==NDArray::float32);
assert($buffer->value_size()==(32/8));
$newHostBuffer = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
$buffer->read($queue,$newHostBuffer);
foreach(range(0,15) as $value) {
    assert($newHostBuffer[$value] == $value);
}
echo "SUCCESS with hostBuffer\n";
//
//  Type constraint
//
$invalidBuffer = new \stdClass();
try {
    $buffer = new Rindow\OpenCL\Buffer($context,intval(16*32/8),
        OpenCL::CL_MEM_USE_HOST_PTR,$invalidBuffer);
} catch(\TypeError $e) {
    echo "Invalid Host Buffer catch: ".get_class($e)."\n";
}
//
// blocking read buffer
//
foreach(range(0,15) as $value) {
    $hostBuffer[$value] = $value;
}
$buffer = new Rindow\OpenCL\Buffer($context,intval(16*32/8),
    OpenCL::CL_MEM_READ_WRITE|OpenCL::CL_MEM_COPY_HOST_PTR,
    $hostBuffer);
assert($buffer->dtype()==NDArray::float32);
assert($buffer->value_size()==(32/8));

$newHostBuffer = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
foreach(range(0,15) as $value) {
    $newHostBuffer[$value]=0;
}
$buffer->read($queue,$newHostBuffer);
foreach(range(0,15) as $value) {
    assert($newHostBuffer[$value] == $value);
}
echo "SUCCESS blocking read\n";
//
// non-blocking read buffer
//
$newHostBuffer = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
$newHostBuffer2 = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
for($i=0;$i<16;$i++) {
    $newHostBuffer2[$i]=$i*2;
}
$buffer2 = new Rindow\OpenCL\Buffer($context,intval(16*32/8),
    OpenCL::CL_MEM_READ_WRITE|OpenCL::CL_MEM_COPY_HOST_PTR,
    $newHostBuffer2);
for($i=0;$i<16;$i++) {
    $newHostBuffer[$i]=0;
    $newHostBuffer2[$i]=0;
}
$events = new Rindow\OpenCL\EventList();
$buffer->read($queue,$newHostBuffer,
        $size=0,$offset=0,$host_offset=0,$blocking_read=false,$events);
$buffer2->read($queue,$newHostBuffer2,
        $size=0,$offset=0,$host_offset=0,$blocking_read=false,$events);
$events->wait();
for($i=0;$i<16;$i++) {
    assert($newHostBuffer[$i] == $i);
    assert($newHostBuffer2[$i] == $i*2);
}
echo "SUCCESS non-blocking read\n";
//
// blocking read with null arguments
//
$newHostBuffer = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
foreach(range(0,15) as $value) {
    $newHostBuffer[$value]=0;
}
$buffer->read($queue,$newHostBuffer,
                $size=null,$offset=null,$host_offset=null,$blocking_read=null,$events=null,$waitEvent=null);
for($i=0;$i<16;$i++) {
    assert($newHostBuffer[$i] == $i);
}
echo "SUCCESS read with null arguments\n";
//
// read with invalid object arguments
//
$invalidBuffer = new \stdClass();
try {
    $buffer->read($queue,$invalidBuffer);
} catch (\Throwable $e) {
    echo "Invalid Host Buffer catch: ".get_class($e)."\n";
}
echo "SUCCESS read with invalid object arguments\n";

//
// blocking write buffer
//
$hostBuffer = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
for($i=0;$i<16;$i++) {
    $hostBuffer[$i] = $i+10;
}
$buffer->write($queue,$hostBuffer);
$buffer->read($queue,$newHostBuffer);
for($i=0;$i<16;$i++) {
    assert($newHostBuffer[$i] == $i+10);
}
echo "SUCCESS blocking write\n";
//
// non-blocking write buffer
//
$hostBuffer = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
$hostBuffer2 = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
for($i=0;$i<16;$i++) {
    $hostBuffer[$i] = $i+20;
    $hostBuffer2[$i] = $i*3;
}
$events = new Rindow\OpenCL\EventList();
$buffer->write($queue,$hostBuffer,
                $size=0,$offset=0,$host_offset=0,$blocking_write=false,$events);
$buffer2->write($queue,$hostBuffer2,
                $size=0,$offset=0,$host_offset=0,$blocking_write=false,$events);
$events->wait();
$buffer->read($queue,$newHostBuffer);
$buffer2->read($queue,$newHostBuffer2);
for($i=0;$i<16;$i++) {
    assert($newHostBuffer[$i] == $i+20);
    assert($newHostBuffer2[$i] == $i*3);
}
echo "SUCCESS non-blocking write\n";
//
// blocking write with null argments
//
$hostBuffer = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
for($i=0;$i<16;$i++) {
    $hostBuffer[$i] = $i+20;
}
$buffer->write($queue,$hostBuffer,
                $size=null,$offset=null,$host_offset=null,$blocking_write=null,$events=null,$waitEvent=null);
$buffer->read($queue,$newHostBuffer);
for($i=0;$i<16;$i++) {
    assert($newHostBuffer[$i] == $i+20);
}
echo "SUCCESS write with null arguments\n";
//
// write with invalid object arguments
//
$invalidBuffer = new \stdClass();
try {
    $buffer->write($queue,$invalidBuffer);
} catch (\Throwable $e) {
    echo "Invalid Host Buffer catch: ".get_class($e)."\n";
}
echo "SUCCESS write with invalid object arguments\n";

//
// read and write buffer with wait event list
//
$hostBuffer = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
foreach(range(0,15) as $value) {
    $hostBuffer[$value] = $value+30;
}
$hostBuffer3 = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
$hostBuffer4 = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
for($i=0;$i<16;$i++) {
    $hostBuffer3[$i] = $i+40;
    $hostBuffer4[$i] = $i*4;
}
$write_events = new Rindow\OpenCL\EventList();
$buffer->write($queue,$hostBuffer,
        $size=0,$offset=0,$host_offset=0,$blocking_write=false,$write_events);
$buffer2->write($queue,$hostBuffer2,
        $size=0,$offset=0,$host_offset=0,$blocking_write=false,$write_events);

$read_events = new Rindow\OpenCL\EventList();
$buffer->read($queue,$newHostBuffer,
        $size=0,$offset=0,$host_offset=0,$blocking_read=false,$read_events,$write_events);
$buffer2->read($queue,$newHostBuffer2,
        $size=0,$offset=0,$host_offset=0,$blocking_read=false,$read_events,$write_events);

$write_events2 = new Rindow\OpenCL\EventList();
$buffer->write($queue,$hostBuffer3,
        $size=0,$offset=0,$host_offset=0,$blocking_write=false,$write_events2,$read_events);
$buffer2->write($queue,$hostBuffer4,
        $size=0,$offset=0,$host_offset=0,$blocking_write=false,$write_events2,$read_events);
$write_events2->wait();
for($i=0;$i<16;$i++) {
    assert($newHostBuffer[$i] == $i+30);
    assert($newHostBuffer2[$i] == $i*3);
}
echo "SUCCESS read and write with wait events\n";
//
// fill
//
$hostBuffer = new RindowTest\OpenCL\HostBuffer(
    2,NDArray::float32);
foreach(range(0,1) as $value) {
    $hostBuffer[$value] = $value+123;
}
$buffer->fill($queue,$hostBuffer);
$queue->finish();
$buffer->read($queue,$newHostBuffer);
foreach(range(0,15) as $value) {
    assert($newHostBuffer[$value] == 123+($value%2));
}
echo "SUCCESS fill\n";
//
// fill with null arguments
//
$hostBuffer = new RindowTest\OpenCL\HostBuffer(
    2,NDArray::float32);
foreach(range(0,1) as $value) {
    $hostBuffer[$value] = $value+123;
}
$buffer->fill($queue,$hostBuffer,
    $size=0,$offset=0,$pattern_size=0,$pattern_offset=0,$events=null,$waitEvent=null);
$queue->finish();
$buffer->read($queue,$newHostBuffer);
foreach(range(0,15) as $value) {
    assert($newHostBuffer[$value] == 123+($value%2));
}
echo "SUCCESS fill with null arguments\n";
//
// fill with invalid object arguments
//
$invalidBuffer = new \stdClass();
try {
    $buffer->fill($queue,$invalidBuffer);
} catch (\Throwable $e) {
    echo "Invalid Host Buffer catch: ".get_class($e)."\n";
}
echo "SUCCESS fill with invalid object arguments\n";
//
// copy
//
$hostBuffer = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
foreach(range(0,15) as $value) {
    $hostBuffer[$value] = 0;
}
$buffer2 = new Rindow\OpenCL\Buffer($context,intval(16*32/8),
    OpenCL::CL_MEM_READ_WRITE|OpenCL::CL_MEM_COPY_HOST_PTR,
    $hostBuffer);
$buffer->copy($queue,$buffer2);
$queue->finish();
$buffer2->read($queue,$hostBuffer);
foreach(range(0,15) as $value) {
    assert($hostBuffer[$value] == 123+($value%2));
}
echo "SUCCESS copy\n";
//
// copy with null arguments
//
$hostBuffer = new RindowTest\OpenCL\HostBuffer(
    16,NDArray::float32);
foreach(range(0,15) as $value) {
    $hostBuffer[$value] = 0;
}
$buffer2 = new Rindow\OpenCL\Buffer($context,intval(16*32/8),
    OpenCL::CL_MEM_READ_WRITE|OpenCL::CL_MEM_COPY_HOST_PTR,
    $hostBuffer);
$buffer->copy($queue,$buffer2,
    $size=0,$offset=0,$src_offset=0,$events=null,$waitEvent=null);
$queue->finish();
$buffer2->read($queue,$hostBuffer);
foreach(range(0,15) as $value) {
    assert($newHostBuffer[$value] == 123+($value%2));
}
echo "SUCCESS copy with null arguments\n";
//
// construct with explicit dtype
//
$buffer = new Rindow\OpenCL\Buffer($context,intval(16*32/8),
    OpenCL::CL_MEM_READ_WRITE,
    null,null,NDArray::float32);
assert($buffer->dtype()==NDArray::float32);
assert($buffer->value_size()==intval(32/8));
echo "SUCCESS construct with explicit dtype\n";
//
// readRect
//
$hostBuffer = new RindowTest\OpenCL\HostBuffer(
    27,NDArray::float32);
$data = [ 99,99,99,  99,99,99,  99,99,99,
          99,99,99,  99, 1, 2,  99, 3, 4,
          99,99,99,  99, 5, 6,  99, 7, 8, ];
foreach($data as $idx => $value) {
    $hostBuffer[$idx] = $value;
}
$buffer = new Rindow\OpenCL\Buffer(
    $context,
    $hostBuffer->value_size()*count($hostBuffer),
    OpenCL::CL_MEM_READ_WRITE|OpenCL::CL_MEM_COPY_HOST_PTR,
    $hostBuffer);
$subHostBuffer = new RindowTest\OpenCL\HostBuffer(
    2*2*2,NDArray::float32);
$value_size = $subHostBuffer->value_size();
$buffer->readRect($queue,$subHostBuffer,[2*$value_size,2,2],
    $hostBufferOffset=0,
    $bufferOffsets=[1*$value_size,1,1],
    null,
    $buffer_row_pitch=$value_size*3,$buffer_slice_pitch=$value_size*3*3,
    $host_row_pitch=$value_size*2,$host_slice_pitch=$value_size*2*2,
    $blocking_read=true);
$trues = [ 1,2, 3,4,
           5,6, 7,8 ];
foreach($trues as $idx => $value) {
    assert($subHostBuffer[$idx] == $value);
}
echo "SUCCESS readRect\n";
$data = [ -1,-2, -3,-4,
          -5,-6, -7,-8 ];
foreach($data as $idx => $value) {
    $subHostBuffer[$idx] = $value;
}
$buffer->writeRect($queue,$subHostBuffer,[2*$value_size,2,2],
    $hostBufferOffset=0,
    $bufferOffsets=[1*$value_size,1,1],
    null,
    $buffer_row_pitch=$value_size*3,$buffer_slice_pitch=$value_size*3*3,
    $host_row_pitch=$value_size*2,$host_slice_pitch=$value_size*2*2,
    $blocking_write=true);
$trues = [ 99,99,99,  99,99,99,  99,99,99,
           99,99,99,  99,-1,-2,  99,-3,-4,
           99,99,99,  99,-5,-6,  99,-7,-8, ];
$buffer->read($queue,$hostBuffer);
foreach($trues as $idx => $value) {
    assert($hostBuffer[$idx] == $value);
}
echo "SUCCESS writeRect\n";
$data = [ 0,0, 0,0,
          0,0, 0,0 ];
foreach($data as $idx => $value) {
    $subHostBuffer[$idx] = $value;
}
$dstBuffer = new Rindow\OpenCL\Buffer(
    $context,
    $subHostBuffer->value_size()*count($subHostBuffer),
    OpenCL::CL_MEM_READ_WRITE|OpenCL::CL_MEM_COPY_HOST_PTR,
    $subHostBuffer);
$buffer->copyRect($queue,$dstBuffer,[2*$value_size,2,2],
    $src_origin=[1*$value_size,1,1],
    $dst_origin=null,
    $src_row_pitch=$value_size*3,$src_slice_pitch=$value_size*3*3,
    $dst_row_pitch=$value_size*2,$dst_slice_pitch=$value_size*2*2,
    );
$queue->finish();
$dstBuffer->read($queue,$subHostBuffer);
$trues = [ -1,-2, -3,-4,
           -5,-6, -7,-8 ];
foreach($trues as $idx => $value) {
    assert($subHostBuffer[$idx] == $value);
}
echo "SUCCESS copyRect\n";
?>
--EXPECT--
SUCCESS Pure buffer
SUCCESS constructor with null
SUCCESS with hostBuffer
Invalid Host Buffer catch: TypeError
SUCCESS blocking read
SUCCESS non-blocking read
SUCCESS read with null arguments
Invalid Host Buffer catch: TypeError
SUCCESS read with invalid object arguments
SUCCESS blocking write
SUCCESS non-blocking write
SUCCESS write with null arguments
Invalid Host Buffer catch: TypeError
SUCCESS write with invalid object arguments
SUCCESS read and write with wait events
SUCCESS fill
SUCCESS fill with null arguments
Invalid Host Buffer catch: TypeError
SUCCESS fill with invalid object arguments
SUCCESS copy
SUCCESS copy with null arguments
SUCCESS construct with explicit dtype
SUCCESS readRect
SUCCESS writeRect
SUCCESS copyRect
