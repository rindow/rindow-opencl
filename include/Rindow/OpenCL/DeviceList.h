#ifndef PHP_RINDOW_OPENCL_DEVICE_LIST_H
# define PHP_RINDOW_OPENCL_DEVICE_LIST_H

#define PHP_RINDOW_OPENCL_DEVICE_LIST_CLASSNAME "Rindow\\OpenCL\\DeviceList"

typedef struct {
    cl_uint num;
    cl_device_id *devices;
    zend_object std;
} php_rindow_opencl_device_list_t;
static inline php_rindow_opencl_device_list_t* php_rindow_opencl_device_list_fetch_object(zend_object* obj)
{
	return (php_rindow_opencl_device_list_t*) ((char*) obj - XtOffsetOf(php_rindow_opencl_device_list_t, std));
}
#define Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(zv) (php_rindow_opencl_device_list_fetch_object(Z_OBJ_P(zv)))

#endif	/* PHP_RINDOW_OPENCL_DEVICE_LIST_H */
