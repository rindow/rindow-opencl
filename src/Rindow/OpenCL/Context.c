#include <php.h>
#include <Zend/zend_interfaces.h>
#include <Zend/zend_exceptions.h>
#include <ext/spl/spl_iterators.h>
#include <ext/spl/spl_exceptions.h>
#include <stdint.h>
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/opencl.h>
#include "Rindow/OpenCL/Context.h"
#include "Rindow/OpenCL/DeviceList.h"
#include "Rindow/OpenCL/Program.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_rindow_opencl.h"

cl_device_id* php_rindow_opencl_context_get_devices(
    cl_context context,
    cl_int *num_of_device,
    cl_int *errcode_ret
)
{
    cl_int errcode;
    cl_int n_device;
    cl_device_id *devices;
    errcode = clGetContextInfo(
        context,
        CL_CONTEXT_NUM_DEVICES,
        sizeof(n_device),
        &n_device,
        NULL);
    if(errcode!=CL_SUCCESS) {
        *errcode_ret = errcode;
        return NULL;
    }
    devices = ecalloc(n_device, sizeof(cl_device_id));
    if(devices==NULL) {
        *errcode_ret = CL_OUT_OF_RESOURCES;
        return NULL;
    }
    errcode = clGetContextInfo(
        context,
        CL_CONTEXT_DEVICES,
        n_device*sizeof(cl_device_id),
        devices,
        NULL);
    if(errcode!=CL_SUCCESS) {
        efree(devices);
        *errcode_ret = errcode;
        return NULL;
    }
    *num_of_device = n_device;
    return devices;
}

cl_device_id php_rindow_opencl_context_get_first_device(
    cl_context context,
    cl_int *errcode_ret
)
{
    cl_device_id device;
    cl_device_id *devices;
    cl_int num_of_device;
    cl_int errcode;
    devices = php_rindow_opencl_context_get_devices(
        context,
        &num_of_device,
        &errcode);
    if(errcode!=CL_SUCCESS) {
        *errcode_ret = errcode;
        return NULL;
    }
    device = devices[0];
    efree(devices);
    return device;
}


static zend_object_handlers rindow_opencl_context_object_handlers;

// destractor
static void php_rindow_opencl_context_free_object(zend_object* object)
{
    php_rindow_opencl_context_t* obj = php_rindow_opencl_context_fetch_object(object);
    if(obj->context) {
        clReleaseContext(obj->context);
    }
    zend_object_std_dtor(&obj->std);
}

// constructor
static zend_object* php_rindow_opencl_context_create_object(zend_class_entry* class_type) /* {{{ */
{
    php_rindow_opencl_context_t* intern = NULL;

    intern = (php_rindow_opencl_context_t*)ecalloc(1, sizeof(php_rindow_opencl_context_t) + zend_object_properties_size(class_type));
    intern->context = NULL;
    intern->devices = NULL;
    intern->num_devices = 0;

    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);

    intern->std.handlers = &rindow_opencl_context_object_handlers;

    return &intern->std;
} /* }}} */



/* Method Rindow\OpenCL\Context::__construct(
    $deivces,
    int $properties    // bitfield
) {{{ */
static PHP_METHOD(Context, __construct)
{
    php_rindow_opencl_context_t* intern;
    php_rindow_opencl_device_list_t* device_list_obj;
    zval* device_list_obj_p=NULL;
    zend_long device_id=0;
    zend_long properties=0;
    cl_context context;
    cl_device_id *devices;
    cl_int num_devices;
    cl_int errcode_ret=0;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_ZVAL(device_list_obj_p)  // ,php_rindow_opencl_context_ce
    ZEND_PARSE_PARAMETERS_END();

    if(Z_TYPE_P(device_list_obj_p) == IS_OBJECT) {
        if(!instanceof_function(Z_OBJCE_P(device_list_obj_p), php_rindow_opencl_device_list_ce)) {
            zend_throw_exception(spl_ce_InvalidArgumentException, "devices must be integer of device_type or the DeviceList", CL_INVALID_DEVICE);
            return;
        }
        device_list_obj = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(device_list_obj_p);
        if(device_list_obj->devices==NULL) {
            zend_throw_exception(spl_ce_InvalidArgumentException, "DeviceList is empty", CL_INVALID_DEVICE);
            return;
        }
        errcode_ret= 0;
        context = clCreateContext(
            NULL,        // const cl_context_properties * properties,
            device_list_obj->num, // cl_uint  num_devices,
            device_list_obj->devices,     // const cl_device_id * devices,
            NULL,        // CL_CALLBACK * pfn_notify,
            NULL,        // void *user_data,
            &errcode_ret // cl_int *errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clCreateContext Error errcode=%d", errcode_ret);
            return;
        }
    } else if(Z_TYPE_P(device_list_obj_p) == IS_LONG) {
        // *** CAUTHON ***
        // When it call clCreateContextFromType without properties,
        // it throw the CL_INVALID_PLATFORM.
        // Probably the clCreateContextFromType needs flatform in cl_context_properties.
        //
        cl_device_type device_type = Z_LVAL_P(device_list_obj_p);
        cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, 0, 0 };
        cl_platform_id platform;
        cl_uint num_platforms=0;

        clGetPlatformIDs( 1,          // cl_uint num_entries
                          &platform,  // cl_platform_id * platforms
                          &num_platforms );     // cl_uint * num_platforms
        properties[1] = (cl_context_properties)platform;
        errcode_ret= 0;
        context = clCreateContextFromType(
            properties,        // const cl_context_properties * properties,
            device_type, // cl_device_type      device_type,
            NULL,        // CL_CALLBACK * pfn_notify
            NULL,        // void *user_data,
            &errcode_ret // cl_int *errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            // static char message[128];
            // sprintf(message,"clCreateContextFromType Error: device_type=%lld, error=%d",device_type,errcode_ret);
            zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clCreateContextFromType Error errcode=%d", errcode_ret);
            return;
        }
    } else {
        zend_throw_exception(spl_ce_InvalidArgumentException, "devices must be integer of device-type or the DeviceList", CL_INVALID_DEVICE);
        return;
    }

    devices = php_rindow_opencl_context_get_devices(
        context,
        &num_devices,
        &errcode_ret
    );
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetContextInfo Error errcode=%d", errcode_ret);
        return;
    }

    intern = Z_RINDOW_OPENCL_CONTEXT_OBJ_P(getThis());
    intern->context = context;
    intern->num_devices = num_devices;
    intern->devices = devices;
}
/* }}} */

/* Method Rindow\OpenCL\Context::getInfo(
    int $param_name
) : string {{{ */
static PHP_METHOD(Context, getInfo)
{
    zend_long param_name;
    size_t param_value_size_ret;
    cl_uint uint_result;
    cl_int errcode_ret;
    php_rindow_opencl_context_t* intern;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_LONG(param_name)
    ZEND_PARSE_PARAMETERS_END();

    intern = Z_RINDOW_OPENCL_CONTEXT_OBJ_P(getThis());

    errcode_ret = clGetContextInfo(intern->context,
                      (cl_context_info)param_name,
                      0, NULL, &param_value_size_ret);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetContextInfo Error errcode=%d", errcode_ret);
        return;
    }
    switch(param_name) {
        case CL_CONTEXT_DEVICES: {
            cl_device_id* device_ids = emalloc(param_value_size_ret);

            errcode_ret = clGetContextInfo(intern->context,
                        (cl_context_info)param_name,
                        param_value_size_ret, device_ids, NULL);
            unsigned int items = (unsigned int)(param_value_size_ret/sizeof(cl_device_id));
            // direct set to return_value
            php_rindow_opencl_device_list_new_device_object(
                return_value, device_ids, items);
            efree(device_ids);
            return;
            break;
        }
        case CL_CONTEXT_PROPERTIES: {
            cl_context_properties* properties = emalloc(param_value_size_ret);
            errcode_ret = clGetContextInfo(intern->context,
                        (cl_context_info)param_name,
                        param_value_size_ret, properties, NULL);
            unsigned int items = (unsigned int)(param_value_size_ret/sizeof(cl_context_properties));
            // direct set to return_value
            array_init(return_value);
            for(unsigned int i=0;i<items;i++) {
                add_next_index_long(return_value, (zend_long)properties[i]);
            }
            efree(properties);
            return;
        }
        case CL_CONTEXT_REFERENCE_COUNT:
        case CL_CONTEXT_NUM_DEVICES: {
            zend_long result;
            errcode_ret = clGetContextInfo(intern->context,
                    (cl_context_info)param_name,
                    sizeof(cl_uint), &uint_result, NULL);
            result = (zend_long)uint_result;
            RETURN_LONG(result);
        }
        break;
#ifdef CL_VERSION_2_1
#endif
        default:
            break;
    }
}
/* }}} */


ZEND_BEGIN_ARG_INFO_EX(ai_Context___construct, 0, 0, 1)
    ZEND_ARG_INFO(0, devices)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Context_getInfo, 0, 0, 1)
    ZEND_ARG_INFO(0, param_name)
ZEND_END_ARG_INFO()

/* {{{ Rindow\OpenCL\Context function entries */
static zend_function_entry php_rindow_opencl_context_me[] = {
    /* clang-format off */
    PHP_ME(Context, __construct, ai_Context___construct, ZEND_ACC_PUBLIC)
    PHP_ME(Context, getInfo, ai_Context_getInfo, ZEND_ACC_PUBLIC)
    PHP_FE_END
    /* clang-format on */
};
/* }}} */

/* Class Rindow\OpenCL\Context {{{ */
zend_class_entry* php_rindow_opencl_context_ce;

void php_rindow_opencl_context_init_ce(INIT_FUNC_ARGS)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Rindow\\OpenCL", "Context", php_rindow_opencl_context_me);
    php_rindow_opencl_context_ce = zend_register_internal_class(&ce);
    php_rindow_opencl_context_ce->create_object = php_rindow_opencl_context_create_object;
    memcpy(&rindow_opencl_context_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    rindow_opencl_context_object_handlers.offset    = XtOffsetOf(php_rindow_opencl_context_t, std);
    rindow_opencl_context_object_handlers.free_obj  = php_rindow_opencl_context_free_object;
    rindow_opencl_context_object_handlers.clone_obj = NULL;

    //zend_class_implements(php_rindow_opencl_context_ce, 2, spl_ce_ArrayAccess, spl_ce_Countable);
}
/* }}} */
