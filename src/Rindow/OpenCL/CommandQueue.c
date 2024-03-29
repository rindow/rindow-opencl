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
#include <Rindow/OpenCL/CommandQueue.h>
#include "Rindow/OpenCL/Context.h"

#include "php_rindow_opencl.h"


static zend_object_handlers rindow_opencl_command_queue_object_handlers;

// destractor
static void php_rindow_opencl_command_queue_free_object(zend_object* object)
{
    php_rindow_opencl_command_queue_t* obj = php_rindow_opencl_command_queue_fetch_object(object);
    if(obj->command_queue) {
        clReleaseCommandQueue(obj->command_queue);
    }
    zend_object_std_dtor(&obj->std);
}

// constructor
static zend_object* php_rindow_opencl_command_queue_create_object(zend_class_entry* class_type) /* {{{ */
{
    php_rindow_opencl_command_queue_t* intern = NULL;

    intern = (php_rindow_opencl_command_queue_t*)ecalloc(1, sizeof(php_rindow_opencl_command_queue_t) + zend_object_properties_size(class_type));
    intern->signature = PHP_RINDOW_OPENCL_COMMAND_QUEUE_SIGNATURE;
    intern->command_queue = 0;

    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);

    intern->std.handlers = &rindow_opencl_command_queue_object_handlers;

    return &intern->std;
} /* }}} */

/* Method Rindow\OpenCL\CommandQueue::__construct(
    Context $context,
    int $device,       // cl_device_id
    int $properties    // bitfield
) {{{ */
static PHP_METHOD(CommandQueue, __construct)
{
    php_rindow_opencl_command_queue_t* intern;
    php_rindow_opencl_context_t* context_obj;
    zval* context_obj_p=NULL;
    cl_device_id device=0;
    zend_long device_id=0;
    zend_long properties=0;
    cl_command_queue command_queue;
    cl_int errcode_ret=0;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 3)
        Z_PARAM_OBJECT_OF_CLASS(context_obj_p,php_rindow_opencl_context_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(device_id)
        Z_PARAM_LONG(properties)
    ZEND_PARSE_PARAMETERS_END();

    context_obj = Z_RINDOW_OPENCL_CONTEXT_OBJ_P(context_obj_p);
    if(device_id==0) {
        if(context_obj->devices==NULL) {
            zend_throw_exception(spl_ce_RuntimeException, "Context is not initialized", CL_INVALID_CONTEXT);
            return;
        }
        device = context_obj->devices[0];
    } else {
        device = (cl_device_id)device_id;
    }

    command_queue = clCreateCommandQueue(
        context_obj->context,
        device,
        properties,
        &errcode_ret);

    intern = Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(getThis());
    if(errcode_ret!=0) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "CreateCommandQueue Error errcode=%d", errcode_ret);
        return;
    }

    intern->command_queue = command_queue;
    zend_update_property(php_rindow_opencl_command_queue_ce, &(intern->std),
         "context", sizeof("context")-1, context_obj_p);
}
/* }}} */

/* Method Rindow\OpenCL\CommandQueue::getContext(
) : void {{{ */
static PHP_METHOD(CommandQueue, getContext)
{
    php_rindow_opencl_command_queue_t* intern;
    zval* context_obj_p=NULL;

    intern = Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(getThis());
    context_obj_p = zend_read_property(php_rindow_opencl_command_queue_ce, &(intern->std),
        "context", sizeof("context")-1, 1, NULL);

    RETURN_ZVAL(context_obj_p, 1, 0);
}
/* }}} */

/* Method Rindow\OpenCL\CommandQueue::flush(
) : void {{{ */
static PHP_METHOD(CommandQueue, flush)
{
    php_rindow_opencl_command_queue_t* intern;
    cl_int errcode_ret;

    intern = Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(getThis());
    errcode_ret = clFlush(intern->command_queue);
    if(errcode_ret!=0) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clFlush Error=%d",errcode_ret);
        return;
    }
}
/* }}} */

/* Method Rindow\OpenCL\CommandQueue::finish(
) : void {{{ */
static PHP_METHOD(CommandQueue, finish)
{
    php_rindow_opencl_command_queue_t* intern;
    cl_int errcode_ret;

    intern = Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(getThis());
    errcode_ret = clFinish(intern->command_queue);
    if(errcode_ret!=0) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clFinish Error=%d",errcode_ret);
        return;
    }
}
/* }}} */

/* Method Rindow\OpenCL\CommandQueue::getInfo(
    int $param_name
) : string {{{ */
static PHP_METHOD(CommandQueue, getInfo)
{
    zend_long param_name;
    size_t param_value_size_ret;
    cl_uint uint_result;
    cl_int errcode_ret;
    php_rindow_opencl_command_queue_t* intern;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_LONG(param_name)
    ZEND_PARSE_PARAMETERS_END();

    intern = Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(getThis());

    errcode_ret = clGetCommandQueueInfo(intern->command_queue,
                      (cl_command_queue_info)param_name,
                      0, NULL, &param_value_size_ret);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetCommandQueueInfo Error errcode=%d", errcode_ret);
        return;
    }
    switch(param_name) {
        case CL_QUEUE_DEVICE: {
            cl_device_id* device_ids = emalloc(param_value_size_ret);

            errcode_ret = clGetCommandQueueInfo(intern->command_queue,
                        (cl_command_queue_info)param_name,
                        param_value_size_ret, device_ids, NULL);
            unsigned int items = (unsigned int)(param_value_size_ret/sizeof(cl_device_id));
            // direct set to return_value
            php_rindow_opencl_device_list_new_device_object(
                return_value, device_ids, items);
            efree(device_ids);
            return;
            break;
        }
        case CL_QUEUE_PROPERTIES: {
            cl_command_queue_properties* properties = emalloc(param_value_size_ret);
            errcode_ret = clGetCommandQueueInfo(intern->command_queue,
                        (cl_command_queue_info)param_name,
                        param_value_size_ret, properties, NULL);
            unsigned int items = (unsigned int)(param_value_size_ret/sizeof(cl_command_queue_properties));
            // direct set to return_value
            array_init(return_value);
            for(unsigned int i=0;i<items;i++) {
                add_next_index_long(return_value, (zend_long)properties[i]);
            }
            efree(properties);
            return;
        }

#ifdef CL_VERSION_2_1
        case CL_QUEUE_SIZE:
#endif
        case CL_QUEUE_REFERENCE_COUNT: {
            zend_long result;
            errcode_ret = clGetCommandQueueInfo(intern->command_queue,
                    (cl_command_queue_info)param_name,
                    sizeof(cl_uint), &uint_result, NULL);
            result = (zend_long)uint_result;
            RETURN_LONG(result);
        }
        break;
        default:
            zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "Unsupported Parameter Name errcode=%d", errcode_ret);
            break;
    }
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(ai_CommandQueue___construct, 0, 0, 3)
    ZEND_ARG_OBJ_INFO(0, context, Rindow\\OpenCL\\Context, 0)
    ZEND_ARG_INFO(0, device)
    ZEND_ARG_INFO(0, properties)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_CommandQueue_getInfo, 0, 0, 2)
    ZEND_ARG_INFO(0, index)
    ZEND_ARG_INFO(0, param_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_CommandQueue_void, 0, 0, 0)
ZEND_END_ARG_INFO()

/* {{{ Rindow\OpenCL\CommandQueue function entries */
static zend_function_entry php_rindow_opencl_command_queue_me[] = {
    /* clang-format off */
    PHP_ME(CommandQueue, __construct, ai_CommandQueue___construct, ZEND_ACC_PUBLIC)
    PHP_ME(CommandQueue, getContext,  ai_CommandQueue_void,        ZEND_ACC_PUBLIC)
    PHP_ME(CommandQueue, flush,       ai_CommandQueue_void,        ZEND_ACC_PUBLIC)
    PHP_ME(CommandQueue, finish,      ai_CommandQueue_void,        ZEND_ACC_PUBLIC)
    PHP_ME(CommandQueue, getInfo,     ai_CommandQueue_getInfo,     ZEND_ACC_PUBLIC)
    PHP_FE_END
    /* clang-format on */
};
/* }}} */

/* Class Rindow\OpenCL\CommandQueue {{{ */
zend_class_entry* php_rindow_opencl_command_queue_ce;

void php_rindow_opencl_command_queue_init_ce(INIT_FUNC_ARGS)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Rindow\\OpenCL", "CommandQueue", php_rindow_opencl_command_queue_me);
    php_rindow_opencl_command_queue_ce = zend_register_internal_class(&ce);
    php_rindow_opencl_command_queue_ce->create_object = php_rindow_opencl_command_queue_create_object;
    memcpy(&rindow_opencl_command_queue_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    rindow_opencl_command_queue_object_handlers.offset    = XtOffsetOf(php_rindow_opencl_command_queue_t, std);
    rindow_opencl_command_queue_object_handlers.free_obj  = php_rindow_opencl_command_queue_free_object;
    rindow_opencl_command_queue_object_handlers.clone_obj = NULL;

    //zend_class_implements(php_rindow_opencl_command_queue_ce, 2, spl_ce_ArrayAccess, spl_ce_Countable);
    zend_declare_property_null(php_rindow_opencl_command_queue_ce, "context", sizeof("context")-1, ZEND_ACC_PROTECTED);
}
/* }}} */
