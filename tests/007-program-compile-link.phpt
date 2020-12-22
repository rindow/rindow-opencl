--TEST--
Programs and compile
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
$context = new Rindow\OpenCL\Context(OpenCL::CL_DEVICE_TYPE_DEFAULT);
$header0 =
    "const int zero = 0;\n";
$sources = [
    "#include \"const_zero.h\"\n".
    "__kernel void saxpy(const global float * x,\n".
    "                    __global float * y,\n".
    "                    const float a)\n".
    "{\n".
    "   uint gid = get_global_id(0);\n".
    "   y[gid] = a* x[gid] + y[gid];\n".
    "}\n"
];
$sources0 = [
    "__kernel void saxpy(const global float * x,\n".
    "                    __global float * y,\n".
    "                    const float a)\n".
    "{\n".
    "   uint gid = get_global_id(0);\n".
    "   y[gid] = a* x[gid] + y[gid];\n".
    "}\n"
];

$programSub = new Rindow\OpenCL\Program($context,$header0);
echo "SUCCESS Construction sub-source\n";
$program = new Rindow\OpenCL\Program($context,$sources);
echo "SUCCESS Construction\n";
$program->compile(['const_zero.h'=>$programSub]);
echo "SUCCESS Compiling\n";
$linkedprogram = new Rindow\OpenCL\Program($context,[$program],
    Rindow\OpenCL\Program::TYPE_COMPILED_PROGRAM);
echo "SUCCESS link program\n";
$program = new Rindow\OpenCL\Program($context,$sources0);
$program->compile($headers=null,$options=null,$devices=null);
echo "SUCCESS Compiling with null arguments\n";
$linkedprogram = new Rindow\OpenCL\Program($context,[$program],
    Rindow\OpenCL\Program::TYPE_COMPILED_PROGRAM,
    $devices=null,$options=null);
echo "SUCCESS linking with null arguments\n";
?>
--EXPECT--
SUCCESS Construction sub-source
SUCCESS Construction
SUCCESS Compiling
SUCCESS link program
SUCCESS Compiling with null arguments
SUCCESS linking with null arguments
