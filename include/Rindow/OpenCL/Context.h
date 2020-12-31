#ifndef PHP_RINDOW_OPENCL_CONTEXT_H
# define PHP_RINDOW_OPENCL_CONTEXT_H

#define PHP_RINDOW_OPENCL_CONTEXT_CLASSNAME "Rindow\\OpenCL\\Context"
#define PHP_RINDOW_OPENCL_CONTEXT_SIGNATURE 0x78434c4377646e52 // "RndwCLCx"

typedef struct {
    zend_long signature;
    cl_context context;
    cl_int num_devices;
    cl_device_id *devices;
    zend_object std;
} php_rindow_opencl_context_t;
static inline php_rindow_opencl_context_t* php_rindow_opencl_context_fetch_object(zend_object* obj)
{
	return (php_rindow_opencl_context_t*) ((char*) obj - XtOffsetOf(php_rindow_opencl_context_t, std));
}
#define Z_RINDOW_OPENCL_CONTEXT_OBJ_P(zv) (php_rindow_opencl_context_fetch_object(Z_OBJ_P(zv)))

#endif	/* PHP_RINDOW_OPENCL_CONTEXT_H */
