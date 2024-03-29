--TEST--
Program and build
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
function safestring($string) {
    $out = '';
    $string = str_split($string);
    $len = count($string);
    for($i=0;$i<$len;$i++) {
        $c = ord($string[$i]);
        if($c>=32&&$c<127) {
            $out .= chr($c);
        } elseif($c==10||$c==13) {
            $out .= "\n";
        } else {
            $out .= '($'.dechex($c).')';
        }
    }
    return $out;
}
function compile_error($program,$e) {
    echo $e->getMessage();
    switch($e->getCode()) {
        case OpenCL::CL_BUILD_PROGRAM_FAILURE: {
            echo "CL_PROGRAM_BUILD_STATUS=".$program->getBuildInfo(OpenCL::CL_PROGRAM_BUILD_STATUS)."\n";
            echo "CL_PROGRAM_BUILD_OPTIONS=".safestring($program->getBuildInfo(OpenCL::CL_PROGRAM_BUILD_OPTIONS))."\n";
            echo "CL_PROGRAM_BUILD_LOG=".safestring($program->getBuildInfo(OpenCL::CL_PROGRAM_BUILD_LOG))."\n";
            echo "CL_PROGRAM_BINARY_TYPE=".safestring($program->getBuildInfo(OpenCL::CL_PROGRAM_BINARY_TYPE))."\n";
        }
        case OpenCL::CL_COMPILE_PROGRAM_FAILURE: {
            echo "CL_PROGRAM_BUILD_LOG=".safestring($program->getBuildInfo(OpenCL::CL_PROGRAM_BUILD_LOG))."\n";
        }
    }
    throw $e;
}

try {
    $context = new Rindow\OpenCL\Context(OpenCL::CL_DEVICE_TYPE_GPU);
} catch(RuntimeException $e) {
    $context = new Rindow\OpenCL\Context(OpenCL::CL_DEVICE_TYPE_DEFAULT);
}
$devices = $context->getInfo(OpenCL::CL_CONTEXT_DEVICES);
$dev_version = $devices->getInfo(0,OpenCL::CL_DEVICE_VERSION);
//$dev_version = 'OpenCL 1.1 Mesa';
$isOpenCL110 = strstr($dev_version,'OpenCL 1.1') !== false;

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
echo "SUCCESS Construction\n";
try {
    $program->build();
} catch(\RuntimeException $e) {
    compile_error($program,$e);
}

echo "SUCCESS build\n";
$program = new Rindow\OpenCL\Program($context,$sources,
    $mode=0,$devices=null,$options=null);
echo "SUCCESS constructor with null arguments\n";
try {
    $program->build($options=null,$devices=null);
} catch(\RuntimeException $e) {
    compile_error($program,$e);
}

echo "SUCCESS build with null arguments\n";
if(!$isOpenCL110) {
    assert(null!==$program->getInfo(OpenCL::CL_PROGRAM_KERNEL_NAMES));
}
echo "SUCCESS getInfo\n";
/*
echo "CL_PROGRAM_REFERENCE_COUNT=".$program->getInfo(OpenCL::CL_PROGRAM_REFERENCE_COUNT)."\n";
echo "CL_PROGRAM_CONTEXT=".$program->getInfo(OpenCL::CL_PROGRAM_CONTEXT)."\n";
echo "CL_PROGRAM_NUM_DEVICES=".$program->getInfo(OpenCL::CL_PROGRAM_NUM_DEVICES)."\n";
echo "CL_PROGRAM_DEVICES=\n";
$devices = $program->getInfo(OpenCL::CL_PROGRAM_DEVICES);
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
echo "CL_PROGRAM_SOURCE=\n";
echo $program->getInfo(OpenCL::CL_PROGRAM_SOURCE)."\n";
echo "CL_PROGRAM_BINARY_SIZES=[".implode(',',$program->getInfo(OpenCL::CL_PROGRAM_BINARY_SIZES))."]\n";
#echo "CL_PROGRAM_BINARIES=".$program->getInfo(OpenCL::CL_PROGRAM_BINARIES)."\n";
echo "CL_PROGRAM_NUM_KERNELS=".$program->getInfo(OpenCL::CL_PROGRAM_NUM_KERNELS)."\n";
echo "CL_PROGRAM_KERNEL_NAMES=".$program->getInfo(OpenCL::CL_PROGRAM_KERNEL_NAMES)."\n";
echo "============ build info ============\n";
echo "CL_PROGRAM_BUILD_STATUS=".$program->getBuildInfo(OpenCL::CL_PROGRAM_BUILD_STATUS)."\n";
echo "CL_PROGRAM_BUILD_OPTIONS=".$program->getBuildInfo(OpenCL::CL_PROGRAM_BUILD_OPTIONS)."\n";
echo "CL_PROGRAM_BUILD_LOG=".$program->getBuildInfo(OpenCL::CL_PROGRAM_BUILD_LOG)."\n";
echo "CL_PROGRAM_BINARY_TYPE=".$program->getBuildInfo(OpenCL::CL_PROGRAM_BINARY_TYPE)."\n";
*/
?>
--EXPECT--
SUCCESS Construction
SUCCESS build
SUCCESS constructor with null arguments
SUCCESS build with null arguments
SUCCESS getInfo
