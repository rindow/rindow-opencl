#include <php.h>
#include <Zend/zend_interfaces.h>
#include <Zend/zend_exceptions.h>
#include <ext/spl/spl_iterators.h>
#include <ext/spl/spl_exceptions.h>
#include <stdint.h>
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/opencl.h>
#include "Rindow/OpenCL/PlatformList.h"
#include "Rindow/OpenCL/DeviceList.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_rindow_opencl.h"

int php_rindow_opencl_device_list_new_device_object(
    zval *val, cl_device_id *device_ids, cl_uint num_devices)
{
    int rc;
    php_rindow_opencl_device_list_t* device_list_ret;
    if(rc=object_init_ex(val, php_rindow_opencl_device_list_ce)) {
        return rc;
    }
    device_list_ret = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(val);
    device_list_ret->num = num_devices;
    device_list_ret->devices = ecalloc(num_devices,sizeof(cl_device_id));
    if(device_list_ret->devices==NULL) {
        return -1;
    }
    memcpy(device_list_ret->devices, device_ids, num_devices*sizeof(cl_device_id));
    return 0;
}

static zend_object_handlers rindow_opencl_device_list_object_handlers;

// destractor
static void php_rindow_opencl_device_list_free_object(zend_object* object)
{
    php_rindow_opencl_device_list_t* obj = php_rindow_opencl_device_list_fetch_object(object);
    if(obj->devices) {
        efree(obj->devices);
    }
    zend_object_std_dtor(&obj->std);
}

// constructor
static zend_object* php_rindow_opencl_device_list_create_object(zend_class_entry* class_type) /* {{{ */
{
    php_rindow_opencl_device_list_t* intern = NULL;

    intern = (php_rindow_opencl_device_list_t*)ecalloc(1, sizeof(php_rindow_opencl_device_list_t) + zend_object_properties_size(class_type));
    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);
    intern->signature = PHP_RINDOW_OPENCL_DEVICE_LIST_SIGNATURE;
    intern->devices = NULL;
    intern->num = 0;
    intern->std.handlers = &rindow_opencl_device_list_object_handlers;

    return &intern->std;
} /* }}} */

/* Method Rindow\OpenCL\DeviceList::__construct(
    PlatformList $platforms,
    int $index=null,
    int $device_type=null,
    int $num_entries=null,
) {{{ */
static PHP_METHOD(DeviceList, __construct)
{
    zval* platforms_obj_p=NULL;
    zend_long index=0;
    zend_long device_type=CL_DEVICE_TYPE_ALL;
    zend_long num_entries=0;
    cl_uint num_devices;
    cl_device_id* ids;
    cl_int errcode_ret=0;
    php_rindow_opencl_device_list_t* intern;
    php_rindow_opencl_platform_list_t* platforms;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 3)
        Z_PARAM_OBJECT_OF_CLASS(platforms_obj_p,php_rindow_opencl_platform_list_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(index)
        Z_PARAM_LONG(device_type)
    ZEND_PARSE_PARAMETERS_END();

    platforms = Z_RINDOW_OPENCL_PLATFORM_LIST_OBJ_P(platforms_obj_p);
    if(index<0 || index >= platforms->num) {
        zend_throw_exception_ex(spl_ce_OutOfRangeException, 0, "Invalid index of platforms: %d", index);
        return;
    }
    if(device_type==0) {
        device_type = CL_DEVICE_TYPE_ALL;
    }

    errcode_ret = clGetDeviceIDs(platforms->platforms[index],
                   (cl_device_type)device_type,
                   0,
                   NULL,
                   &num_devices);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetDeviceIDs Error errcode=%d", errcode_ret);
        return;
    }
    if(num_devices<=0) {
        return;
    }
    ids = ecalloc(num_devices,sizeof(cl_device_id));
    if(ids==NULL) {
        zend_throw_exception_ex(spl_ce_RuntimeException, 0, "memory allocation error: %d", num_devices);
        return;
    }
    errcode_ret = clGetDeviceIDs(platforms->platforms[index],
                   (cl_device_type)device_type,
                   (cl_uint)num_devices,
                   ids,
                   NULL);
    if(errcode_ret!=CL_SUCCESS) {
        efree(ids);
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetDeviceIDs Error2 errcode=%d", errcode_ret);
        return;
    }

    intern = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(getThis());
    intern->num = num_devices;
    intern->devices = ids;
}
/* }}} */

/* Method Rindow\OpenCL\DeviceList::count(
) : int {{{ */
static PHP_METHOD(DeviceList, count)
{
    php_rindow_opencl_device_list_t* intern;
    intern = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(getThis());

    RETURN_LONG((zend_long)(intern->num));
}
/* }}} */

/* Method Rindow\OpenCL\DeviceList::getOne(
    int $index
) : void {{{ */
static PHP_METHOD(DeviceList, getOne)
{
    php_rindow_opencl_device_list_t* intern;
    zend_long index;
    cl_int errcode_ret;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_LONG(index)
    ZEND_PARSE_PARAMETERS_END();

    intern = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(getThis());
    if(index<0 || index >=intern->num) {
        zend_throw_exception_ex(spl_ce_OutOfRangeException, 0, "Invalid index of devices: %d", index);
        return;
    }
    // direct set to return_value
    errcode_ret = php_rindow_opencl_device_list_new_device_object(
        return_value, &(intern->devices[index]), 1);
    if(errcode_ret) {
        zend_throw_exception_ex(spl_ce_RuntimeException, 0, "object allocation error");
        return;
    }
    return;
}
/* }}} */

/* Method Rindow\OpenCL\DeviceList::append(
    DeviceList $device
) : void {{{ */
static PHP_METHOD(DeviceList, append)
{
    php_rindow_opencl_device_list_t* intern;
    php_rindow_opencl_device_list_t* devices;
    zval *device_list_obj_p=NULL;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_OBJECT_OF_CLASS(device_list_obj_p,php_rindow_opencl_device_list_ce)
    ZEND_PARSE_PARAMETERS_END();

    devices = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(device_list_obj_p);
    if(devices->devices==NULL) {
        zend_throw_exception(spl_ce_RuntimeException, "DeviceList is not initialized", 0);
        return;
    }

    intern = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(getThis());
    intern->devices = erealloc(intern->devices, (intern->num + devices->num)*sizeof(cl_device_id));
    if(devices->devices==NULL) {
        zend_throw_exception(spl_ce_RuntimeException, "DeviceList reallocate error", 0);
        return;
    }
    memcpy(&(intern->devices[intern->num]),devices->devices,devices->num*sizeof(cl_device_id));
    intern->num += devices->num;
}
/* }}} */

/* Method Rindow\OpenCL\DeviceList::getInfo(
    int $index,
    int $param_name
) : string {{{ */
static PHP_METHOD(DeviceList, getInfo)
{
    zend_long index;
    zend_long param_name;
    size_t param_value_size_ret;
    cl_int errcode_ret;
    php_rindow_opencl_device_list_t* intern;
    zend_long result=0;
    cl_uint uint_result;
    cl_ulong ulong_result;
    cl_bool bool_result;
    cl_bitfield bitfield_result;
    size_t size_t_result;

    intern = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(getThis());

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
        Z_PARAM_LONG(index)
        Z_PARAM_LONG(param_name)
    ZEND_PARSE_PARAMETERS_END();

    if(index<0 || index >= intern->num) {
        zend_throw_exception_ex(spl_ce_OutOfRangeException, 0, "Invalid index of deivces: %d", index);
        return;
    }

    errcode_ret = clGetDeviceInfo(intern->devices[index],
                      (cl_device_info)param_name,
                      0, NULL, &param_value_size_ret);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetDeviceInfo Error errcode=%d", errcode_ret);
        return;
    }
    switch(param_name) {
        case CL_DEVICE_BUILT_IN_KERNELS:
        case CL_DEVICE_NAME:
        case CL_DEVICE_VENDOR:
        case CL_DRIVER_VERSION:
        case CL_DEVICE_PROFILE:
        case CL_DEVICE_VERSION:
        case CL_DEVICE_OPENCL_C_VERSION:
        case CL_DEVICE_EXTENSIONS:
#ifdef CL_VERSION_2_1
        case CL_DEVICE_IL_VERSION:
#endif
        {
            char *param_value = emalloc(param_value_size_ret);
            errcode_ret = clGetDeviceInfo(intern->devices[index],
                            (cl_device_info)param_name,
                            param_value_size_ret, param_value, NULL);
            zend_string *param_value_val =
                    zend_string_init(param_value, param_value_size_ret-1, 0);
                                            // str, len, persistent
            efree(param_value);
            RETURN_STR(param_value_val);
            break;
        }
        case CL_DEVICE_VENDOR_ID:
        case CL_DEVICE_MAX_COMPUTE_UNITS:
        case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
        case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
        case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
        case CL_DEVICE_MAX_CLOCK_FREQUENCY:
        case CL_DEVICE_ADDRESS_BITS:
        case CL_DEVICE_MAX_READ_IMAGE_ARGS:
        case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
        case CL_DEVICE_MAX_SAMPLERS:
        case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
        case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:
        case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
        case CL_DEVICE_MAX_CONSTANT_ARGS:
        case CL_DEVICE_PARTITION_MAX_SUB_DEVICES:
        case CL_DEVICE_REFERENCE_COUNT:
        case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
        case CL_DEVICE_LOCAL_MEM_TYPE:
#ifdef CL_VERSION_2_0
        case CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS:
        case CL_DEVICE_IMAGE_PITCH_ALIGNMENT:
        case CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT:
        case CL_DEVICE_MAX_PIPE_ARGS:
        case CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS:
        case CL_DEVICE_PIPE_MAX_PACKET_SIZE:
        case CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE:
        case CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE:
        case CL_DEVICE_MAX_ON_DEVICE_QUEUES:
        case CL_DEVICE_MAX_ON_DEVICE_EVENTS:
        case CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT:
        case CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT:
        case CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT:
#endif
#ifdef CL_VERSION_2_1
        case CL_DEVICE_MAX_NUM_SUB_GROUPS:
#endif
            errcode_ret = clGetDeviceInfo(intern->devices[index],
                        (cl_device_info)param_name,
                        sizeof(cl_uint), &uint_result, NULL);
            result = (zend_long)uint_result;
            RETURN_LONG(result);
            break;
        case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
        case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
        case CL_DEVICE_GLOBAL_MEM_SIZE:
        case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
        case CL_DEVICE_LOCAL_MEM_SIZE:
            errcode_ret = clGetDeviceInfo(intern->devices[index],
                    (cl_device_info)param_name,
                    sizeof(cl_ulong), &ulong_result, NULL);
            result = (zend_long)ulong_result;
            RETURN_LONG(result);
            break;
        case CL_DEVICE_IMAGE_SUPPORT:
        case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
        case CL_DEVICE_HOST_UNIFIED_MEMORY:
        case CL_DEVICE_ENDIAN_LITTLE:
        case CL_DEVICE_AVAILABLE:
        case CL_DEVICE_COMPILER_AVAILABLE:
        case CL_DEVICE_LINKER_AVAILABLE:
        case CL_DEVICE_PREFERRED_INTEROP_USER_SYNC:
#ifdef CL_VERSION_2_1
        case CL_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS:
#endif
            errcode_ret = clGetDeviceInfo(intern->devices[index],
                (cl_device_info)param_name,
                sizeof(cl_bool), &bool_result, NULL);
            result = (zend_long)bool_result;
            RETURN_LONG(result);
            break;
        case CL_DEVICE_MAX_WORK_GROUP_SIZE:
        case CL_DEVICE_IMAGE2D_MAX_WIDTH:
        case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
        case CL_DEVICE_IMAGE3D_MAX_WIDTH:
        case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
        case CL_DEVICE_IMAGE3D_MAX_DEPTH:
        case CL_DEVICE_IMAGE_MAX_BUFFER_SIZE:
        case CL_DEVICE_IMAGE_MAX_ARRAY_SIZE:
        case CL_DEVICE_MAX_PARAMETER_SIZE:
        case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
        case CL_DEVICE_PRINTF_BUFFER_SIZE:
#ifdef CL_VERSION_2_0
        case CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE:
        case CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE:
#endif
            errcode_ret = clGetDeviceInfo(intern->devices[index],
                (cl_device_info)param_name,
                sizeof(size_t), &size_t_result, NULL);
            result = (zend_long)size_t_result;
            RETURN_LONG(result);
            break;
        case CL_DEVICE_TYPE:
        case CL_DEVICE_SINGLE_FP_CONFIG:
        case CL_DEVICE_DOUBLE_FP_CONFIG:
        case CL_DEVICE_EXECUTION_CAPABILITIES:
        case CL_DEVICE_QUEUE_PROPERTIES:
#ifdef CL_VERSION_2_0
        case CL_DEVICE_QUEUE_ON_HOST_PROPERTIES:
        case CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES:
        case CL_DEVICE_SVM_CAPABILITIES:
#endif
            errcode_ret = clGetDeviceInfo(intern->devices[index],
                        (cl_device_info)param_name,
                        sizeof(cl_bitfield), &bitfield_result, NULL);
            result = (zend_long)bitfield_result;
            RETURN_LONG(result);
            break;
        case CL_DEVICE_PLATFORM: {
            cl_platform_id platform_id;
            errcode_ret = clGetDeviceInfo(intern->devices[index],
                        (cl_device_info)param_name,
                        sizeof(cl_platform_id), &platform_id, NULL);
            // direct set to return_value
            errcode_ret = php_rindow_opencl_platform_list_new_platform_object(
                return_value, &platform_id, 1);
            if(errcode_ret) {
                zend_throw_exception_ex(spl_ce_RuntimeException, 0, "object allocation error");
                return;
            }
            return;
        }
        case CL_DEVICE_PARENT_DEVICE: {
            cl_device_id parent_device_id;
            errcode_ret = clGetDeviceInfo(intern->devices[index],
                        (cl_device_info)param_name,
                        sizeof(cl_device_id), &parent_device_id, NULL);
            if(parent_device_id==0) {
                RETURN_NULL();
            }
            // direct set to return_value
            errcode_ret = php_rindow_opencl_device_list_new_device_object(
                return_value, &parent_device_id, 1);
            if(errcode_ret) {
                zend_throw_exception_ex(spl_ce_RuntimeException, 0, "object allocation error");
                return;
            }
            return;
        }
        case CL_DEVICE_MAX_WORK_ITEM_SIZES: {
            size_t* item_sizes = emalloc(param_value_size_ret);
            errcode_ret = clGetDeviceInfo(intern->devices[index],
                        (cl_device_info)param_name,
                        param_value_size_ret, item_sizes, NULL);
            unsigned int items = (unsigned int)(param_value_size_ret/sizeof(size_t));
            // direct set to return_value
            array_init(return_value);
            for(unsigned int i=0;i<items;i++) {
                add_next_index_long(return_value, (zend_long)item_sizes[i]);
            }
            efree(item_sizes);
            return;
        }
        case CL_DEVICE_PARTITION_PROPERTIES:
        case CL_DEVICE_PARTITION_TYPE: {
            cl_device_partition_property* properties = emalloc(param_value_size_ret);
            errcode_ret = clGetDeviceInfo(intern->devices[index],
                        (cl_device_info)param_name,
                        param_value_size_ret, properties, NULL);
            if(errcode_ret!=CL_SUCCESS) {
                zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetDeviceInfo Error errcode=%d", errcode_ret);
                return;
            }
            unsigned int items = (unsigned int)(param_value_size_ret/sizeof(cl_device_partition_property));
            // direct set to return_value
            array_init(return_value);
            for(unsigned int i=0;i<items;i++) {
                add_next_index_long(return_value, (zend_long)properties[i]);
            }
            efree(properties);
            return;
        }
        default:
            break;
    }
}
/* }}} */


ZEND_BEGIN_ARG_INFO_EX(ai_DeviceList___construct, 0, 0, 1)
    ZEND_ARG_OBJ_INFO(0, platforms, Rindow\\OpenCL\\PlatformList, 0)
    ZEND_ARG_INFO(0, index)
    ZEND_ARG_INFO(0, device_type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_DeviceList_count, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_DeviceList_getOne, 0, 0, 1)
    ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_DeviceList_append, 0, 0, 1)
    ZEND_ARG_OBJ_INFO(0, platforms, Rindow\\OpenCL\\DeviceList, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_DeviceList_getInfo, 0, 0, 2)
    ZEND_ARG_INFO(0, index)
    ZEND_ARG_INFO(0, param_name)
ZEND_END_ARG_INFO()

/* {{{ Rindow\OpenCL\DeviceList function entries */
static zend_function_entry php_rindow_opencl_device_list_me[] = {
    /* clang-format off */
    PHP_ME(DeviceList, __construct, ai_DeviceList___construct, ZEND_ACC_PUBLIC)
    PHP_ME(DeviceList, count,       ai_DeviceList_count,       ZEND_ACC_PUBLIC)
    PHP_ME(DeviceList, getOne,      ai_DeviceList_getOne,      ZEND_ACC_PUBLIC)
    PHP_ME(DeviceList, append,      ai_DeviceList_append,      ZEND_ACC_PUBLIC)
    PHP_ME(DeviceList, getInfo,     ai_DeviceList_getInfo,     ZEND_ACC_PUBLIC)
    PHP_FE_END
    /* clang-format on */
};
/* }}} */

/* Class Rindow\OpenCL\DeviceList {{{ */
zend_class_entry* php_rindow_opencl_device_list_ce;

void php_rindow_opencl_device_list_init_ce(INIT_FUNC_ARGS)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Rindow\\OpenCL", "DeviceList", php_rindow_opencl_device_list_me);
    php_rindow_opencl_device_list_ce = zend_register_internal_class(&ce);
    php_rindow_opencl_device_list_ce->create_object = php_rindow_opencl_device_list_create_object;
    memcpy(&rindow_opencl_device_list_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    rindow_opencl_device_list_object_handlers.offset    = XtOffsetOf(php_rindow_opencl_device_list_t, std);
    rindow_opencl_device_list_object_handlers.free_obj  = php_rindow_opencl_device_list_free_object;
    rindow_opencl_device_list_object_handlers.clone_obj = NULL;

    //zend_class_implements(php_rindow_opencl_device_list_ce, 2, spl_ce_ArrayAccess, spl_ce_Countable);
}
/* }}} */
