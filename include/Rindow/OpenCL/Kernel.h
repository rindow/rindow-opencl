#ifndef PHP_RINDOW_OPENCL_KERNEL_H
# define PHP_RINDOW_OPENCL_KERNEL_H

#define PHP_RINDOW_OPENCL_KERNEL_CLASSNAME "Rindow\\OpenCL\\Kernel"

typedef struct {
    cl_kernel kernel;
    zend_object std;
} php_rindow_opencl_kernel_t;
static inline php_rindow_opencl_kernel_t* php_rindow_opencl_kernel_fetch_object(zend_object* obj)
{
	return (php_rindow_opencl_kernel_t*) ((char*) obj - XtOffsetOf(php_rindow_opencl_kernel_t, std));
}
#define Z_RINDOW_OPENCL_KERNEL_OBJ_P(zv) (php_rindow_opencl_kernel_fetch_object(Z_OBJ_P(zv)))

#endif	/* PHP_RINDOW_OPENCL_KERNEL_H */
