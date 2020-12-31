#ifndef PHP_RINDOW_OPENCL_BUFFER_H
# define PHP_RINDOW_OPENCL_BUFFER_H

#define PHP_RINDOW_OPENCL_BUFFER_CLASSNAME "Rindow\\OpenCL\\Buffer"
#define PHP_RINDOW_OPENCL_BUFFER_SIGNATURE 0x66424c4377646e52 // "RndwCLBf"

typedef struct {
    zend_long signature;
    cl_mem buffer;
    size_t size;
    zend_long dtype;
    zend_long value_size;
    zval *host_buffer_val;
    zend_object std;
} php_rindow_opencl_buffer_t;
static inline php_rindow_opencl_buffer_t* php_rindow_opencl_buffer_fetch_object(zend_object* obj)
{
	return (php_rindow_opencl_buffer_t*) ((char*) obj - XtOffsetOf(php_rindow_opencl_buffer_t, std));
}
#define Z_RINDOW_OPENCL_BUFFER_OBJ_P(zv) (php_rindow_opencl_buffer_fetch_object(Z_OBJ_P(zv)))

#endif	/* PHP_RINDOW_OPENCL_BUFFER_H */
