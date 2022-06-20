/* rindow_opencl extension for PHP */

#ifndef PHP_RINDOW_OPENCL_H
# define PHP_RINDOW_OPENCL_H

# define phpext_rindow_opencl_ptr &rindow_opencl_module_entry

# define PHP_RINDOW_OPENCL_VERSION "0.1.4"

# if defined(ZTS) && defined(COMPILE_DL_RINDOW_OPENCL)
ZEND_TSRMLS_CACHE_EXTERN()
# endif
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/opencl.h>

enum php_rindow_opencl_array_to_integers_constraint {
    php_rindow_opencl_array_to_integers_constraint_none = 0,
    php_rindow_opencl_array_to_integers_constraint_greater_or_equal_zero = 1,
    php_rindow_opencl_array_to_integers_constraint_greater_zero = 2,
};

extern void php_rindow_opencl_platform_list_init_ce(INIT_FUNC_ARGS);
extern void php_rindow_opencl_device_list_init_ce(INIT_FUNC_ARGS);
extern void php_rindow_opencl_event_list_init_ce(INIT_FUNC_ARGS);
extern void php_rindow_opencl_context_init_ce(INIT_FUNC_ARGS);
extern void php_rindow_opencl_command_queue_init_ce(INIT_FUNC_ARGS);
extern void php_rindow_opencl_program_init_ce(INIT_FUNC_ARGS);
extern void php_rindow_opencl_buffer_init_ce(INIT_FUNC_ARGS);
extern void php_rindow_opencl_kernel_init_ce(INIT_FUNC_ARGS);

extern zend_class_entry* php_rindow_opencl_platform_list_ce;
extern zend_class_entry* php_rindow_opencl_device_list_ce;
extern zend_class_entry* php_rindow_opencl_event_list_ce;
extern zend_class_entry* php_rindow_opencl_context_ce;
extern zend_class_entry* php_rindow_opencl_buffer_ce;
extern zend_class_entry* php_rindow_opencl_command_queue_ce;
extern zend_class_entry* php_rindow_opencl_program_ce;

extern zend_module_entry rindow_opencl_module_entry;

extern int php_rindow_opencl_platform_list_new_platform_object(
    zval *val, cl_platform_id *platform_ids, cl_uint num_platforms);
extern int php_rindow_opencl_device_list_new_device_object(
    zval *val, cl_device_id *device_id, cl_uint num_devices);
extern int php_rindow_opencl_event_list_new_event_object(
    zval *val, cl_event *event, cl_uint num_events);
extern size_t php_rindow_opencl_dtype_to_valuesize(zend_long dtype);
extern int php_rindow_opencl_append_event(zval* event_list_obj_p, cl_event* event);
extern size_t * php_rindow_opencl_array_to_integers(
    zval* array_obj_p, cl_uint *num_integers_p,
    int constraint, int *errcode_ret);

#endif	/* PHP_RINDOW_OPENCL_H */
