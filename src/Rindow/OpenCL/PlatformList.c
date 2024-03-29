#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <php.h>
#include <Zend/zend_interfaces.h>
#include <Zend/zend_exceptions.h>
#include <ext/spl/spl_iterators.h>
#include <ext/spl/spl_exceptions.h>
#include <stdint.h>
#include <CL/opencl.h>
#include "Rindow/OpenCL/PlatformList.h"

#include "php_rindow_opencl.h"

int php_rindow_opencl_platform_list_new_platform_object(
    zval *val, cl_platform_id *platform_ids, cl_uint num_platforms)
{
    int rc;
    php_rindow_opencl_platform_list_t* platform_list_ret;
    if(rc=object_init_ex(val, php_rindow_opencl_platform_list_ce)) {
        return rc;
    }
    platform_list_ret = Z_RINDOW_OPENCL_PLATFORM_LIST_OBJ_P(val);
    platform_list_ret->num = num_platforms;
    platform_list_ret->platforms = ecalloc(num_platforms,sizeof(cl_platform_id));
    if(platform_list_ret->platforms==NULL) {
        return -1;
    }
    memcpy(platform_list_ret->platforms, platform_ids, num_platforms*sizeof(cl_platform_id));
    return 0;
}

static zend_object_handlers rindow_opencl_platform_list_object_handlers;

// destractor
static void php_rindow_opencl_platform_list_free_object(zend_object* object)
{
    php_rindow_opencl_platform_list_t* obj = php_rindow_opencl_platform_list_fetch_object(object);
    if(obj->platforms) {
        efree(obj->platforms);
    }
    zend_object_std_dtor(&obj->std);
}

// constructor
static zend_object* php_rindow_opencl_platform_list_create_object(zend_class_entry* class_type) /* {{{ */
{
    php_rindow_opencl_platform_list_t* intern = NULL;

    intern = (php_rindow_opencl_platform_list_t*)ecalloc(1, sizeof(php_rindow_opencl_platform_list_t) + zend_object_properties_size(class_type));
    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);
    intern->signature = PHP_RINDOW_OPENCL_PLATFORM_LIST_SIGNATURE;
    intern->platforms = NULL;
    intern->num = 0;
    intern->std.handlers = &rindow_opencl_platform_list_object_handlers;

    return &intern->std;
} /* }}} */

/* Method Rindow\OpenCL\PlatformList::__construct(
    int $index=null,
) {{{ */
static PHP_METHOD(PlatformList, __construct)
{
    php_rindow_opencl_platform_list_t* intern;
    cl_uint numPlatforms;
    cl_platform_id* ids;
    cl_int errcode_ret=0;

    errcode_ret = clGetPlatformIDs(0, NULL, &numPlatforms);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetPlatformIDs Error errcode=%d", errcode_ret);
        return;
    }
    if(numPlatforms<=0) {
        return;
    }
    ids = ecalloc(numPlatforms,sizeof(cl_platform_id));
    if(ids==NULL) {
        zend_throw_exception_ex(spl_ce_RuntimeException, 0, "memory allocation error: %d", numPlatforms);
        return;
    }
    errcode_ret = clGetPlatformIDs(numPlatforms, ids, NULL);
    if(errcode_ret!=CL_SUCCESS) {
        efree(ids);
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetPlatformIDs Error errcode=%d", errcode_ret);
        return;
    }

    intern = Z_RINDOW_OPENCL_PLATFORM_LIST_OBJ_P(getThis());
    intern->num = numPlatforms;
    intern->platforms = ids;
}
/* }}} */

/* Method Rindow\OpenCL\PlatformList::count(
) : int {{{ */
static PHP_METHOD(PlatformList, count)
{
    php_rindow_opencl_platform_list_t* intern;
    intern = Z_RINDOW_OPENCL_PLATFORM_LIST_OBJ_P(getThis());

    RETURN_LONG((zend_long)(intern->num));
}
/* }}} */

/* Method Rindow\OpenCL\PlatformList::getOne(
    int $index
) : void {{{ */
static PHP_METHOD(PlatformList, getOne)
{
    php_rindow_opencl_platform_list_t* intern;
    zend_long index;
    cl_int errcode_ret;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_LONG(index)
    ZEND_PARSE_PARAMETERS_END();

    intern = Z_RINDOW_OPENCL_PLATFORM_LIST_OBJ_P(getThis());
    if(index<0 || index >=intern->num) {
        zend_throw_exception_ex(spl_ce_OutOfRangeException, 0, "Invalid index of platforms: %d", (int)index);
        return;
    }
    // direct set to return_value
    errcode_ret = php_rindow_opencl_platform_list_new_platform_object(
        return_value, &(intern->platforms[index]), 1);
    if(errcode_ret) {
        zend_throw_exception_ex(spl_ce_RuntimeException, 0, "object allocation error");
        return;
    }
    return;
}
/* }}} */

/* Method Rindow\OpenCL\PlatformList::getInfo(
    int $index,
    int $param_name
) : string {{{ */
static PHP_METHOD(PlatformList, getInfo)
{
    zend_long index;
    zend_long param_name;
    size_t param_value_size_ret;
    cl_int errcode_ret;
    php_rindow_opencl_platform_list_t* intern;

    intern = Z_RINDOW_OPENCL_PLATFORM_LIST_OBJ_P(getThis());

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
        Z_PARAM_LONG(index)
        Z_PARAM_LONG(param_name)
    ZEND_PARSE_PARAMETERS_END();

    if(index<0 || index >= intern->num) {
        zend_throw_exception_ex(spl_ce_OutOfRangeException, 0, "Invalid index of platforms: %d", (int)index);
        return;
    }
    errcode_ret = clGetPlatformInfo(intern->platforms[index],
                      (cl_platform_info) param_name,
                      0, NULL, &param_value_size_ret);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetPlatformInfo Error errcode=%d", errcode_ret);
        return;
    }
    switch(param_name) {
        case CL_PLATFORM_PROFILE:
        case CL_PLATFORM_VERSION:
        case CL_PLATFORM_NAME:
        case CL_PLATFORM_VENDOR:
        case CL_PLATFORM_EXTENSIONS: {
            char *param_value = emalloc(param_value_size_ret);
            errcode_ret = clGetPlatformInfo(intern->platforms[index],
                              (cl_platform_info)param_name,
                              param_value_size_ret, param_value, NULL);
            zend_string *param_value_val =
                    zend_string_init(param_value, param_value_size_ret-1, 0);
                                            // str, len, persistent
            efree(param_value);
            RETURN_STR(param_value_val);
            break;
        }
#ifdef CL_VERSION_2_1
        case CL_PLATFORM_HOST_TIMER_RESOLUTION: {
            cl_ulong ulong_result;
            errcode_ret = clGetPlatformInfo(intern->platforms[index],
                    (cl_platform_info)param_name,
                    sizeof(cl_ulong), &ulong_result, NULL);
            zend_long result = (zend_long)ulong_result;
            RETURN_LONG(result);
            break;
        }
#endif
        default:
            zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "Unsupported Parameter Name errcode=%d", errcode_ret);
            break;
    }
}
/* }}} */


ZEND_BEGIN_ARG_INFO_EX(ai_PlatformList___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_PlatformList_count, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_PlatformList_getOne, 0, 0, 1)
    ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_PlatformList_getInfo, 0, 0, 2)
    ZEND_ARG_INFO(0, index)
    ZEND_ARG_INFO(0, param_name)
ZEND_END_ARG_INFO()

/* {{{ Rindow\OpenCL\PlatformList function entries */
static zend_function_entry php_rindow_opencl_platform_list_me[] = {
    /* clang-format off */
    PHP_ME(PlatformList, __construct, ai_PlatformList___construct, ZEND_ACC_PUBLIC)
    PHP_ME(PlatformList, count,       ai_PlatformList_count,       ZEND_ACC_PUBLIC)
    PHP_ME(PlatformList, getOne,      ai_PlatformList_getOne,      ZEND_ACC_PUBLIC)
    PHP_ME(PlatformList, getInfo,     ai_PlatformList_getInfo,     ZEND_ACC_PUBLIC)
    PHP_FE_END
    /* clang-format on */
};
/* }}} */

/* Class Rindow\OpenCL\PlatformList {{{ */
zend_class_entry* php_rindow_opencl_platform_list_ce;

void php_rindow_opencl_platform_list_init_ce(INIT_FUNC_ARGS)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Rindow\\OpenCL", "PlatformList", php_rindow_opencl_platform_list_me);
    php_rindow_opencl_platform_list_ce = zend_register_internal_class(&ce);
    php_rindow_opencl_platform_list_ce->create_object = php_rindow_opencl_platform_list_create_object;
    memcpy(&rindow_opencl_platform_list_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    rindow_opencl_platform_list_object_handlers.offset    = XtOffsetOf(php_rindow_opencl_platform_list_t, std);
    rindow_opencl_platform_list_object_handlers.free_obj  = php_rindow_opencl_platform_list_free_object;
    rindow_opencl_platform_list_object_handlers.clone_obj = NULL;

    //zend_class_implements(php_rindow_opencl_platform_list_ce, 2, spl_ce_ArrayAccess, spl_ce_Countable);
}
/* }}} */
