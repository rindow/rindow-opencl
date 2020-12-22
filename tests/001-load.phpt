--TEST--
Check if rindow_opencl is loaded
--SKIPIF--
<?php
if (!extension_loaded('rindow_opencl')) {
	echo 'skip';
}
?>
--FILE--
<?php
echo 'The extension "rindow_opencl" is available';
?>
--EXPECT--
The extension "rindow_opencl" is available
