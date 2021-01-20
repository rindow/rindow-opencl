/* rindow_opencl extension for PHP */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <php.h>
#include <Zend/zend_interfaces.h>
#include <Zend/zend_exceptions.h>
#include <ext/spl/spl_iterators.h>
#include <ext/spl/spl_exceptions.h>
#include "ext/standard/info.h"
#include <Interop/Polite/Math/Matrix.h>
#include "php_rindow_opencl.h"
#include "Rindow/OpenCL/EventList.h"

/* For compatibility with older PHP versions */
#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE() \
    ZEND_PARSE_PARAMETERS_START(0, 0) \
    ZEND_PARSE_PARAMETERS_END()
#endif

size_t php_rindow_opencl_dtype_to_valuesize(zend_long dtype)
{
    switch (dtype) {
        case php_interop_polite_math_matrix_dtype_bool:
        case php_interop_polite_math_matrix_dtype_int8:
        case php_interop_polite_math_matrix_dtype_uint8:
        case php_interop_polite_math_matrix_dtype_float8:
            return 1;
        case php_interop_polite_math_matrix_dtype_int16:
        case php_interop_polite_math_matrix_dtype_uint16:
        case php_interop_polite_math_matrix_dtype_float16:
            return 2;
        case php_interop_polite_math_matrix_dtype_int32:
        case php_interop_polite_math_matrix_dtype_uint32:
        case php_interop_polite_math_matrix_dtype_float32:
            return 4;
        case php_interop_polite_math_matrix_dtype_int64:
        case php_interop_polite_math_matrix_dtype_uint64:
        case php_interop_polite_math_matrix_dtype_float64:
            return 8;
    }
    return 0;
}

int php_rindow_opencl_append_event(zval* event_list_obj_p, cl_event* event)
{
    php_rindow_opencl_event_list_t *events;
    if(event==NULL || event_list_obj_p==NULL || Z_TYPE_P(event_list_obj_p) != IS_OBJECT) {
        return 0;
    }
    events = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(event_list_obj_p);
    events->events = erealloc(events->events, (events->num + 1)*sizeof(cl_event));
    if(events->events==NULL) {
        zend_throw_exception(spl_ce_RuntimeException, "EventList reallocate error", 0);
        return -1;
    }
    memcpy(&(events->events[events->num]),event,sizeof(cl_event));
    events->num++;
    return 0;
}

size_t * php_rindow_opencl_array_to_integers(
    zval* array_obj_p, cl_uint *num_integers_p, int constraint, int *errcode_ret)
{
    cl_uint num_integers;
    size_t *integers;
    zend_long long_integer;
    zend_ulong idx;
    zend_string *key;
    zval *val;
    cl_uint i=0;

    num_integers = zend_array_count(Z_ARR_P(array_obj_p));
    if(*num_integers_p!=0) {
        integers = ecalloc(*num_integers_p,sizeof(size_t));
        memset(integers,0,(*num_integers_p)*sizeof(size_t));
    } else {
        integers = ecalloc(num_integers,sizeof(size_t));
        memset(integers,0,num_integers*sizeof(size_t));
    }

    ZEND_HASH_FOREACH_KEY_VAL(Z_ARR_P(array_obj_p), idx, key, val) {
        if(Z_TYPE_P(val) != IS_LONG) {
            efree(integers);
            zend_throw_exception(spl_ce_InvalidArgumentException, "the array must be array of integer", CL_INVALID_VALUE);
            *errcode_ret = -2;
            return NULL;
        }
        if(i<num_integers) {
            long_integer = Z_LVAL_P(val);
            if(constraint==php_rindow_opencl_array_to_integers_constraint_greater_zero
                && long_integer<1) {
                efree(integers);
                zend_throw_exception(spl_ce_InvalidArgumentException, "values must be greater zero.", CL_INVALID_VALUE);
                *errcode_ret = -3;
                return NULL;
            } else if(constraint==php_rindow_opencl_array_to_integers_constraint_greater_or_equal_zero
                && long_integer<0) {
                efree(integers);
                zend_throw_exception(spl_ce_InvalidArgumentException, "values must be greater or equal zero.", CL_INVALID_VALUE);
                *errcode_ret = -3;
                return NULL;
            }
            integers[i] = (size_t)long_integer;
            i++;
        }
    } ZEND_HASH_FOREACH_END();
    *num_integers_p = num_integers;
    *errcode_ret = 0;
    return integers;
}

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(rindow_opencl)
{
#if defined(ZTS) && defined(COMPILE_DL_RINDOW_OPENCL)
    ZEND_TSRMLS_CACHE_UPDATE();
#endif

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(rindow_opencl)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "rindow_opencl support", "enabled");
    php_info_print_table_end();
}
/* }}} */

/* {{{ rindow_opencl_functions[]
 */
//static const zend_function_entry rindow_opencl_functions[] = {
//};
/* }}} */

PHP_MINIT_FUNCTION(rindow_opencl)
{
    php_rindow_opencl_platform_list_init_ce(INIT_FUNC_ARGS_PASSTHRU);
    php_rindow_opencl_device_list_init_ce(INIT_FUNC_ARGS_PASSTHRU);
    php_rindow_opencl_event_list_init_ce(INIT_FUNC_ARGS_PASSTHRU);
    php_rindow_opencl_context_init_ce(INIT_FUNC_ARGS_PASSTHRU);
    php_rindow_opencl_command_queue_init_ce(INIT_FUNC_ARGS_PASSTHRU);
    php_rindow_opencl_program_init_ce(INIT_FUNC_ARGS_PASSTHRU);
    php_rindow_opencl_buffer_init_ce(INIT_FUNC_ARGS_PASSTHRU);
    php_rindow_opencl_kernel_init_ce(INIT_FUNC_ARGS_PASSTHRU);
    return SUCCESS;
}

/* {{{ php_rindow_opencl_module_entry
 */
zend_module_entry rindow_opencl_module_entry = {
    STANDARD_MODULE_HEADER,
    "rindow_opencl",					/* Extension name */
    NULL,			                    /* zend_function_entry */
    PHP_MINIT(rindow_opencl),			/* PHP_MINIT - Module initialization */
    NULL,							    /* PHP_MSHUTDOWN - Module shutdown */
    PHP_RINIT(rindow_opencl),			/* PHP_RINIT - Request initialization */
    NULL,							    /* PHP_RSHUTDOWN - Request shutdown */
    PHP_MINFO(rindow_opencl),			/* PHP_MINFO - Module info */
    PHP_RINDOW_OPENCL_VERSION,		/* Version */
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_RINDOW_OPENCL
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(rindow_opencl)
#endif
