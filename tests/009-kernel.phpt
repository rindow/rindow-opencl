--TEST--
Kernel
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
define('NWITEMS',64);
use Interop\Polite\Math\Matrix\NDArray;
use Interop\Polite\Math\Matrix\OpenCL;
$context = new Rindow\OpenCL\Context(OpenCL::CL_DEVICE_TYPE_DEFAULT);
//$context = new Rindow\OpenCL\Context(OpenCL::CL_DEVICE_TYPE_CPU);
$queue = new Rindow\OpenCL\CommandQueue($context);
$sources = [
    "__kernel void saxpy(const global float * x,\n".
    "                    __global float * y,\n".
    "                    const float a)\n".
    "{\n".
    "   uint gid = get_global_id(0);\n".
    "   y[gid] = a* x[gid] + y[gid];\n".
    "}\n"
];
$program = new Rindow\OpenCL\Program($context,$sources);
$program->build();
$kernel = new Rindow\OpenCL\Kernel($program,"saxpy");
echo "SUCCESS Construction\n";
$hostX = new RindowTest\OpenCL\HostBuffer(
    NWITEMS,NDArray::float32);
$hostY = new RindowTest\OpenCL\HostBuffer(
    NWITEMS,NDArray::float32);
for($i=0;$i<NWITEMS;$i++) {
    $hostX[$i] = $i;
    $hostY[$i] = NWITEMS-1-$i;
}
$a = 2.0;
$bufX = new Rindow\OpenCL\Buffer($context,intval(NWITEMS*32/8),
    OpenCL::CL_MEM_READ_ONLY|OpenCL::CL_MEM_COPY_HOST_PTR,
    $hostX);
$bufY = new Rindow\OpenCL\Buffer($context,intval(NWITEMS*32/8),
    OpenCL::CL_MEM_READ_WRITE|OpenCL::CL_MEM_COPY_HOST_PTR,
    $hostY);
$kernel->setArg(0,$bufX);
$kernel->setArg(1,$bufY);
$kernel->setArg(2,$a,NDArray::float32);
echo "SUCCESS setArg\n";
$global_work_size = [NWITEMS];
$local_work_size = [1];
$kernel->enqueueNDRange($queue,$global_work_size,$local_work_size);
echo "SUCCESS enqueueNDRange\n";
$queue->finish();
echo "SUCCESS complete kernel job\n";
$bufY->read($queue,$hostY);
for($i=0;$i<NWITEMS;$i++) {
    echo sprintf("%5.1f",$hostY[$i]).",";
    if((($i+1)%8)==0) {
        echo "\n";
    }
}
echo "SUCCESS read result\n";
$kernel->enqueueNDRange($queue,$global_work_size,
    $local_work_size=null,$global_work_offset=null,$events=null,$wait_events=null);
echo "SUCCESS enqueueNDRange with null arguments\n";
$queue->finish();
echo "SUCCESS complete kernel job with null arguments\n";
$sources =
    "__kernel void reduce_sum(const global float * x,\n".
    "                    __global float * y,\n".
    "                    __local float * work_sum)\n".
    "{\n".
    "    int lid = get_local_id(0);\n".
    "    int group_size = get_local_size(0);\n".
    "    work_sum[lid] = x[get_global_id(0)];\n".
    "    barrier(CLK_LOCAL_MEM_FENCE);\n".
    "    for(int i = group_size/2; i>0; i >>= 1) {\n".
    "        if(lid < i) {\n".
    "            work_sum[lid] += work_sum[lid + i];\n".
    "        }\n".
    "        barrier(CLK_LOCAL_MEM_FENCE);\n".
    "    }\n".
    "    if(lid == 0) {\n".
    "        y[get_group_id(0)] = work_sum[0];\n".
    "    }\n".
    "}\n";
$hostX = new RindowTest\OpenCL\HostBuffer(
    64,NDArray::float32);
$hostY = new RindowTest\OpenCL\HostBuffer(
    8,NDArray::float32);
for($i=0;$i<count($hostX);$i++) {
    $hostX[$i] = $i;
}
$bufX = new Rindow\OpenCL\Buffer($context,intval(count($hostX)*32/8),
    OpenCL::CL_MEM_READ_ONLY|OpenCL::CL_MEM_COPY_HOST_PTR,
    $hostX);
$bufY = new Rindow\OpenCL\Buffer($context,intval(count($hostY)*32/8),
    OpenCL::CL_MEM_READ_WRITE|OpenCL::CL_MEM_COPY_HOST_PTR,
    $hostY);
$bufX->write($queue,$hostX);
$program = new Rindow\OpenCL\Program($context,$sources);
try {
    $program->build();
} catch(\RuntimeException $e) {
    if($e->getCode() == OpenCL::CL_BUILD_PROGRAM_FAILURE) {
        echo "====BUILD_PROGRAM_FAILURE====\n";
        echo $program->getBuildInfo(OpenCL::CL_PROGRAM_BUILD_LOG);
        echo "==============================\n";
    }
    throw $e;
}
$kernel = new Rindow\OpenCL\Kernel($program,"reduce_sum");
$groups = count($hostX)/count($hostY);
$global_work_size = [count($hostX)];
$local_work_size = [intval(count($hostX)/$groups)];
$kernel->setArg(0,$bufX);
$kernel->setArg(1,$bufY);
$kernel->setArg(2,null,intval($bufX->bytes()/$groups));
$kernel->enqueueNDRange($queue,$global_work_size,$local_work_size);
//$bufX->read($queue,$hostX);
$bufY->read($queue,$hostY);
echo "----- localtest X ------\n";
for($i=0;$i<count($hostX);$i++) {
    echo sprintf("%2.0f",$hostX[$i]).",";
    if(($i%8)==7) echo "\n";
}
echo "----- localtest Y ------\n";
for($i=0;$i<count($hostY);$i++) {
    echo sprintf("%2.0f",$hostY[$i]).",";
    if(($i%8)==7) echo "\n";
}
echo "SUCCESS Local buffer\n";

echo "CL_KERNEL_FUNCTION_NAME=".$kernel->getInfo(OpenCL::CL_KERNEL_FUNCTION_NAME)."\n";
echo "CL_KERNEL_NUM_ARGS=".$kernel->getInfo(OpenCL::CL_KERNEL_NUM_ARGS)."\n";
// intel gpu doesn't support attributes
// echo "CL_KERNEL_ATTRIBUTES=".$kernel->getInfo(OpenCL::CL_KERNEL_ATTRIBUTES)."\n";
assert(is_array($kernel->getWorkGroupInfo(OpenCL::CL_KERNEL_COMPILE_WORK_GROUP_SIZE)));
echo "SUCCESS getInfo\n";
/*
echo "CL_KERNEL_WORK_GROUP_SIZE=".$kernel->getWorkGroupInfo(OpenCL::CL_KERNEL_WORK_GROUP_SIZE)."\n";
echo "CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE=".$kernel->getWorkGroupInfo(OpenCL::CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE)."\n";
echo "CL_KERNEL_LOCAL_MEM_SIZE=".$kernel->getWorkGroupInfo(OpenCL::CL_KERNEL_LOCAL_MEM_SIZE)."\n";
echo "CL_KERNEL_PRIVATE_MEM_SIZE=".$kernel->getWorkGroupInfo(OpenCL::CL_KERNEL_PRIVATE_MEM_SIZE)."\n";
echo "CL_KERNEL_COMPILE_WORK_GROUP_SIZE=[".implode(',',$kernel->getWorkGroupInfo(OpenCL::CL_KERNEL_COMPILE_WORK_GROUP_SIZE))."]\n";
try {
    echo "CL_KERNEL_GLOBAL_WORK_SIZE=[".implode(',',$kernel->getWorkGroupInfo(OpenCL::CL_KERNEL_GLOBAL_WORK_SIZE))."]\n";
} catch(RuntimeException $e) {
    echo $e->getMessage()."\n";
}
echo "SUCCESS getWorkGroupInfo\n";
*/
$sources =
    "__kernel void multi_gid(const global float * x,\n".
    "                    __global float * y)\n".
    "{\n".
    "    int gid0 = get_global_id(0);\n".
    "    int gid1 = get_global_id(1);\n".
    "    int gid2 = get_global_id(2);\n".
    "    int gsz0 = get_global_size(0);\n".
    "    int gsz1 = get_global_size(1);\n".
    "    int gsz2 = get_global_size(2);\n".
    "    y[gid0*gsz1*gsz2+gid1*gsz2+gid2] = gid0*100+gid1*10+gid2;\n".
    "}\n";
$hostX = new RindowTest\OpenCL\HostBuffer(
    24,NDArray::float32);
$hostY = new RindowTest\OpenCL\HostBuffer(
    24,NDArray::float32);
for($i=0;$i<count($hostX);$i++) {
    $hostX[$i] = $i;
    $hostY[$i] = 0;
}
$bufX = new Rindow\OpenCL\Buffer($context,intval(count($hostX)*32/8),
    OpenCL::CL_MEM_READ_ONLY|OpenCL::CL_MEM_COPY_HOST_PTR,
    $hostX);
$bufY = new Rindow\OpenCL\Buffer($context,intval(count($hostY)*32/8),
    OpenCL::CL_MEM_READ_WRITE|OpenCL::CL_MEM_COPY_HOST_PTR,
    $hostY);
$bufX->write($queue,$hostX);
$bufY->write($queue,$hostY);
$program = new Rindow\OpenCL\Program($context,$sources);
try {
    $program->build();
} catch(\RuntimeException $e) {
    if($e->getCode() == OpenCL::CL_BUILD_PROGRAM_FAILURE) {
        echo "====BUILD_PROGRAM_FAILURE====\n";
        echo $program->getBuildInfo(OpenCL::CL_PROGRAM_BUILD_LOG);
        echo "==============================\n";
    }
    throw $e;
}
$kernel = new Rindow\OpenCL\Kernel($program,"multi_gid");
$global_work_size = [2,3,4];
$kernel->setArg(0,$bufX);
$kernel->setArg(1,$bufY);
$kernel->enqueueNDRange($queue,$global_work_size);
//$bufX->read($queue,$hostX);
$bufY->read($queue,$hostY);
//echo "----- globaltest X ------\n";
//for($i=0;$i<count($hostX);$i++) {
//    echo sprintf("%3.0f",$hostX[$i]).",";
//    if(($i%8)==7) echo "\n";
//}
echo "----- globaltest Y ------\n";
for($i=0;$i<count($hostY);$i++) {
    echo sprintf("%3.0f",$hostY[$i]).",";
    if(($i%4)==3) echo "\n";
}
echo "SUCCESS Multi gid\n";
?>
--EXPECT--
SUCCESS Construction
SUCCESS setArg
SUCCESS enqueueNDRange
SUCCESS complete kernel job
 63.0, 64.0, 65.0, 66.0, 67.0, 68.0, 69.0, 70.0,
 71.0, 72.0, 73.0, 74.0, 75.0, 76.0, 77.0, 78.0,
 79.0, 80.0, 81.0, 82.0, 83.0, 84.0, 85.0, 86.0,
 87.0, 88.0, 89.0, 90.0, 91.0, 92.0, 93.0, 94.0,
 95.0, 96.0, 97.0, 98.0, 99.0,100.0,101.0,102.0,
103.0,104.0,105.0,106.0,107.0,108.0,109.0,110.0,
111.0,112.0,113.0,114.0,115.0,116.0,117.0,118.0,
119.0,120.0,121.0,122.0,123.0,124.0,125.0,126.0,
SUCCESS read result
SUCCESS enqueueNDRange with null arguments
SUCCESS complete kernel job with null arguments
----- localtest X ------
 0, 1, 2, 3, 4, 5, 6, 7,
 8, 9,10,11,12,13,14,15,
16,17,18,19,20,21,22,23,
24,25,26,27,28,29,30,31,
32,33,34,35,36,37,38,39,
40,41,42,43,44,45,46,47,
48,49,50,51,52,53,54,55,
56,57,58,59,60,61,62,63,
----- localtest Y ------
28,92,156,220,284,348,412,476,
SUCCESS Local buffer
CL_KERNEL_FUNCTION_NAME=reduce_sum
CL_KERNEL_NUM_ARGS=3
SUCCESS getInfo
----- globaltest Y ------
  0,  1,  2,  3,
 10, 11, 12, 13,
 20, 21, 22, 23,
100,101,102,103,
110,111,112,113,
120,121,122,123,
SUCCESS Multi gid
