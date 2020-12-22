--TEST--
EventList
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

$events1 = new Rindow\OpenCL\EventList();
assert(count($events1)==0);
echo "SUCCESS construct empty\n";
$context = new Rindow\OpenCL\Context(OpenCL::CL_DEVICE_TYPE_DEFAULT);
$events2 = new Rindow\OpenCL\EventList($context);
assert(count($events2)==1);
echo "SUCCESS construct user event\n";
$events3 = new Rindow\OpenCL\EventList($context);
assert(count($events3)==1);
#  move ev2 to ev1
$events1->move($events2);
#  copy ev3 to ev1
$events1->copy($events3);
assert(count($events1)==2);
assert(count($events2)==0);
assert(count($events3)==1);
echo "SUCCESS move events\n";
$events3->setStatus(OpenCL::CL_COMPLETE);
echo "SUCCESS setStatus\n";
unset($events1);
unset($events2);
unset($events3);
echo "SUCCESS destruct events\n";
$events = new Rindow\OpenCL\EventList(null);
assert(count($events)==0);
echo "SUCCESS construct events with null arguments\n";
?>
--EXPECT--
SUCCESS construct empty
SUCCESS construct user event
SUCCESS move events
SUCCESS setStatus
SUCCESS destruct events
SUCCESS construct events with null arguments
