#ifndef PHP_RINDOW_OPENCL_COMMAND_QUEUE_H
# define PHP_RINDOW_OPENCL_COMMAND_QUEUE_H

#define PHP_RINDOW_OPENCL_COMMAND_QUEUE_CLASSNAME "Rindow\\OpenCL\\CommandQueue"
#define PHP_RINDOW_OPENCL_COMMAND_QUEUE_SIGNATURE 0x75514c4377646e52 // "RndwCLQu"

typedef struct {
    zend_long signature;
    cl_command_queue command_queue;
    zend_object std;
} php_rindow_opencl_command_queue_t;
static inline php_rindow_opencl_command_queue_t* php_rindow_opencl_command_queue_fetch_object(zend_object* obj)
{
	return (php_rindow_opencl_command_queue_t*) ((char*) obj - XtOffsetOf(php_rindow_opencl_command_queue_t, std));
}
#define Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(zv) (php_rindow_opencl_command_queue_fetch_object(Z_OBJ_P(zv)))

#endif	/* PHP_RINDOW_OPENCL_COMMAND_QUEUE_H */
