#include <php.h>
#include <Zend/zend_interfaces.h>
#include <Zend/zend_exceptions.h>
#include <ext/spl/spl_iterators.h>
#include <ext/spl/spl_exceptions.h>
#include <stdint.h>
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/opencl.h>
#include <Interop/Polite/Math/Matrix.h>
#include "Rindow/OpenCL/Kernel.h"
#include "Rindow/OpenCL/Program.h"
#include "Rindow/OpenCL/Buffer.h"
#include "Rindow/OpenCL/CommandQueue.h"
#include "Rindow/OpenCL/EventList.h"
#include "Rindow/OpenCL/DeviceList.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_rindow_opencl.h"

static zend_object_handlers rindow_opencl_kernel_object_handlers;

// destractor
static void php_rindow_opencl_kernel_free_object(zend_object* object)
{
    php_rindow_opencl_kernel_t* obj = php_rindow_opencl_kernel_fetch_object(object);
    if(obj->kernel) {
        clReleaseKernel(obj->kernel);
    }
    zend_object_std_dtor(&obj->std);
}

// constructor
static zend_object* php_rindow_opencl_kernel_create_object(zend_class_entry* class_type) /* {{{ */
{
    php_rindow_opencl_kernel_t* intern = NULL;

    intern = (php_rindow_opencl_kernel_t*)ecalloc(1, sizeof(php_rindow_opencl_kernel_t) + zend_object_properties_size(class_type));
    intern->signature = PHP_RINDOW_OPENCL_KERNEL_SIGNATURE;
    intern->kernel = NULL;

    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);

    intern->std.handlers = &rindow_opencl_kernel_object_handlers;

    return &intern->std;
} /* }}} */


/* Method Rindow\OpenCL\Kernel::__construct(
    Program $program,
    string $name       // kernel name
) {{{ */
static PHP_METHOD(Kernel, __construct)
{
    php_rindow_opencl_kernel_t* intern;
    php_rindow_opencl_program_t* program_obj;
    zval* program_obj_p=NULL;
    char *kernel_name;
    size_t kernel_name_len;
    cl_kernel kernel;
    cl_int errcode_ret=0;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 2)
        Z_PARAM_OBJECT_OF_CLASS(program_obj_p,php_rindow_opencl_program_ce)
        Z_PARAM_STRING(kernel_name,kernel_name_len)
    ZEND_PARSE_PARAMETERS_END();

    program_obj = Z_RINDOW_OPENCL_PROGRAM_OBJ_P(program_obj_p);

    kernel = clCreateKernel(
        program_obj->program,
        kernel_name,
        &errcode_ret
    );

    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clCreateKernel Error errcode=%d", errcode_ret);
        return;
    }
    intern = Z_RINDOW_OPENCL_KERNEL_OBJ_P(getThis());
    intern->kernel = kernel;
}
/* }}} */

/* Method Rindow\OpenCL\Kernel::setArg(
    int $index,
    $arg_value,
    int $dtype=null
) : void {{{ */
static PHP_METHOD(Kernel, setArg)
{
    php_rindow_opencl_kernel_t* intern;
    zval* arg_obj_p=NULL;
    zend_long arg_index;
    void *arg_value;
    zend_long dtype=0;
    size_t arg_size;
    cl_int errcode_ret=0;
    php_rindow_opencl_buffer_t* buffer_obj;
    php_rindow_opencl_command_queue_t* command_queue_obj;
    cl_char   arg_char;
    cl_uchar  arg_uchar;
    cl_short  arg_short;
    cl_ushort arg_ushort;
    cl_int    arg_int;
    cl_uint   arg_uint;
    cl_long   arg_long;
    cl_ulong  arg_ulong;
    cl_float  arg_float;
    cl_double arg_double;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 3)
        Z_PARAM_LONG(arg_index)
        Z_PARAM_ZVAL(arg_obj_p) // long | double | opencl_buffer_ce | command_queue_ce
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(dtype)
    ZEND_PARSE_PARAMETERS_END();

    if(Z_TYPE_P(arg_obj_p)==IS_OBJECT) {
        if(instanceof_function(Z_OBJCE_P(arg_obj_p), php_rindow_opencl_buffer_ce)) {
            buffer_obj = Z_RINDOW_OPENCL_BUFFER_OBJ_P(arg_obj_p);
            arg_value = &(buffer_obj->buffer);
            arg_size = sizeof(cl_mem);
        } else if(instanceof_function(Z_OBJCE_P(arg_obj_p), php_rindow_opencl_command_queue_ce)) {
            command_queue_obj = Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(arg_obj_p);
            arg_value = &(command_queue_obj->command_queue);
            arg_size = sizeof(cl_command_queue);
        } else {
            zend_throw_exception(spl_ce_InvalidArgumentException, "Unsupported argument type", CL_INVALID_VALUE);
            return;
        }
    } else if(Z_TYPE_P(arg_obj_p)==IS_LONG || Z_TYPE_P(arg_obj_p)==IS_DOUBLE) {
        zend_long arg_zend_long;
        double arg_zend_double;
        if(dtype==0) {
            zend_throw_exception(spl_ce_InvalidArgumentException, "Must be specified data type for integer or float", CL_INVALID_VALUE);
            return;
        }
        if(Z_TYPE_P(arg_obj_p)==IS_LONG) {
            arg_zend_long = Z_LVAL_P(arg_obj_p);
            switch(dtype) {
                case php_interop_polite_math_matrix_dtype_float32:
                case php_interop_polite_math_matrix_dtype_float64:
                    arg_zend_double = (double)arg_zend_long;
                    break;
            }
        } else {
            arg_zend_double = Z_DVAL_P(arg_obj_p);
            switch(dtype) {
                case php_interop_polite_math_matrix_dtype_int8:
                case php_interop_polite_math_matrix_dtype_int16:
                case php_interop_polite_math_matrix_dtype_int32:
                case php_interop_polite_math_matrix_dtype_int64:
                case php_interop_polite_math_matrix_dtype_uint8:
                case php_interop_polite_math_matrix_dtype_uint16:
                case php_interop_polite_math_matrix_dtype_uint32:
                case php_interop_polite_math_matrix_dtype_uint64:
                    arg_zend_long = (zend_long)arg_zend_double;
                    break;
            }
        }
        switch(dtype) {
            case php_interop_polite_math_matrix_dtype_int8:
                arg_char = (cl_char)arg_zend_long;
                arg_value = &arg_char;
                arg_size = sizeof(cl_char);
                break;
            case php_interop_polite_math_matrix_dtype_int16:
                arg_short = (cl_short)arg_zend_long;
                arg_value = &arg_short;
                arg_size = sizeof(cl_short);
                break;
            case php_interop_polite_math_matrix_dtype_int32:
                arg_int = (cl_int)arg_zend_long;
                arg_value = &arg_int;
                arg_size = sizeof(cl_int);
                break;
            case php_interop_polite_math_matrix_dtype_int64:
                arg_long = (cl_long)arg_zend_long;
                arg_value = &arg_long;
                arg_size = sizeof(cl_long);
                break;
            case php_interop_polite_math_matrix_dtype_uint8:
                arg_uchar = (cl_uchar)arg_zend_long;
                arg_value = &arg_uchar;
                arg_size = sizeof(cl_uchar);
                break;
            case php_interop_polite_math_matrix_dtype_uint16:
                arg_ushort = (cl_ushort)arg_zend_long;
                arg_value = &arg_ushort;
                arg_size = sizeof(cl_ushort);
                break;
            case php_interop_polite_math_matrix_dtype_uint32:
                arg_uint = (cl_uint)arg_zend_long;
                arg_value = &arg_uint;
                arg_size = sizeof(cl_uint);
                break;
            case php_interop_polite_math_matrix_dtype_uint64:
                arg_ulong = (cl_ulong)arg_zend_long;
                arg_value = &arg_ulong;
                arg_size = sizeof(cl_ulong);
                break;
            case php_interop_polite_math_matrix_dtype_float32:
                arg_float = (cl_float)arg_zend_double;
                arg_value = &arg_float;
                arg_size = sizeof(cl_float);
                break;
            case php_interop_polite_math_matrix_dtype_float64:
                arg_double = (cl_double)arg_zend_double;
                arg_value = &arg_double;
                arg_size = sizeof(cl_double);
                break;
            case php_interop_polite_math_matrix_dtype_bool:
            case php_interop_polite_math_matrix_dtype_float8:
            case php_interop_polite_math_matrix_dtype_float16:
            default:
                zend_throw_exception(spl_ce_InvalidArgumentException, "Unsuppored binding data type for integer or float", CL_INVALID_VALUE);
                return;
        }
    } else if(Z_TYPE_P(arg_obj_p)==IS_NULL) {
        arg_value = NULL;
        arg_size = (size_t)dtype;
    } else {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Invalid argument type", CL_INVALID_VALUE);
        return;
    }

    intern = Z_RINDOW_OPENCL_KERNEL_OBJ_P(getThis());
    errcode_ret = clSetKernelArg(
        intern->kernel,
        (cl_uint)arg_index,
        arg_size,
        arg_value);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clSetKernelArg Error errcode=%d", errcode_ret);
        return;
    }
}
/* }}} */

/* Method Rindow\OpenCL\Kernel::enqueueNDRange(
    CommandQueue $command_queue,
    array<int> $global_work_size,
    array<int> $global_work_offset=NULL,
    array<int> $local_work_size=NULL,
    EventList $events=null,
    EventList $event_wait_list=null
) : void {{{ */
static PHP_METHOD(Kernel, enqueueNDRange)
{
    zval* command_queue_obj_p=NULL;
    zval* global_work_size_obj_p;
    zval* global_work_offset_obj_p=NULL;
    zval* local_work_size_obj_p=NULL;
    zval* events_obj_p=NULL;
    zval* event_wait_list_obj_p=NULL;
    php_rindow_opencl_kernel_t* intern;
    php_rindow_opencl_command_queue_t* command_queue_obj;
    php_rindow_opencl_event_list_t* event_wait_list_obj;
    cl_uint work_dim;
    size_t *global_work_size;
    size_t *local_work_size=NULL;
    size_t *global_work_offset=NULL;
    cl_uint num_events_in_wait_list=0;
    cl_event *event_wait_list=NULL;
    cl_int errcode_ret=0;
    cl_event event_ret;
    cl_event *event_p=NULL;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 6)
        Z_PARAM_OBJECT_OF_CLASS(command_queue_obj_p,php_rindow_opencl_command_queue_ce)
        Z_PARAM_ARRAY(global_work_size_obj_p)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_EX(local_work_size_obj_p,1,0)
        Z_PARAM_ARRAY_EX(global_work_offset_obj_p,1,0)
        Z_PARAM_OBJECT_OF_CLASS_EX(events_obj_p,php_rindow_opencl_event_list_ce,1,0)
        Z_PARAM_OBJECT_OF_CLASS_EX(event_wait_list_obj_p,php_rindow_opencl_event_list_ce,1,0)
    ZEND_PARSE_PARAMETERS_END();

    command_queue_obj = Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(command_queue_obj_p);

    work_dim = 0;
    global_work_size = php_rindow_opencl_array_to_integers(
        global_work_size_obj_p, &work_dim,
        php_rindow_opencl_array_to_integers_constraint_greater_zero, &errcode_ret
    );
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_InvalidArgumentException, errcode_ret, "Invalid global work size. errcode=%d", errcode_ret);
        return;
    }
    if(global_work_size==NULL) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "global_work_size NULL.", CL_INVALID_VALUE);
        return;
    }
    if(local_work_size_obj_p!=NULL && Z_TYPE_P(local_work_size_obj_p)==IS_ARRAY) {
        cl_uint tmp_dim=0;
        local_work_size = php_rindow_opencl_array_to_integers(
            local_work_size_obj_p, &tmp_dim,
            php_rindow_opencl_array_to_integers_constraint_greater_zero, &errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            efree(global_work_size);
            zend_throw_exception(spl_ce_InvalidArgumentException, "Invalid local work size.", CL_INVALID_VALUE);
            return;
        }
        if(work_dim!=tmp_dim) {
            efree(global_work_size);
            efree(local_work_size);
            zend_throw_exception(spl_ce_InvalidArgumentException, "Unmatch number of dimensions between global work size and local work size.", CL_INVALID_VALUE);
            return;
        }
    }

    if(global_work_offset_obj_p!=NULL && Z_TYPE_P(global_work_offset_obj_p)==IS_ARRAY) {
        cl_uint tmp_dim=0;
        global_work_offset = php_rindow_opencl_array_to_integers(
            global_work_offset_obj_p, &tmp_dim,
            php_rindow_opencl_array_to_integers_constraint_greater_zero, &errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            efree(global_work_size);
            if(local_work_size) {
                efree(local_work_size);
            }
            zend_throw_exception(spl_ce_InvalidArgumentException, "Invalid local work size.", CL_INVALID_VALUE);
            return;
        }
        if(work_dim!=tmp_dim) {
            efree(global_work_size);
            if(local_work_size) {
                efree(local_work_size);
            }
            efree(global_work_offset);
            zend_throw_exception(spl_ce_InvalidArgumentException, "Unmatch number of dimensions between global work size and global work offset.", CL_INVALID_VALUE);
            return;
        }
    }

    if(events_obj_p!=NULL && Z_TYPE_P(events_obj_p)==IS_OBJECT) {
        event_p = &event_ret;
    }
    if(event_wait_list_obj_p!=NULL && Z_TYPE_P(event_wait_list_obj_p)==IS_OBJECT) {
        event_wait_list_obj = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(event_wait_list_obj_p);
        num_events_in_wait_list = event_wait_list_obj->num;
        event_wait_list = event_wait_list_obj->events;
    }

    intern = Z_RINDOW_OPENCL_KERNEL_OBJ_P(getThis());
    errcode_ret = clEnqueueNDRangeKernel(
        command_queue_obj->command_queue,
        intern->kernel,
        work_dim,
        global_work_offset,
        global_work_size,
        local_work_size,
        num_events_in_wait_list,
        event_wait_list,
        event_p
    );
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clEnqueueNDRangeKernel Error errcode=%d", errcode_ret);
        return;
    }

    // append event to events
    if(php_rindow_opencl_append_event(events_obj_p, event_p)) {
        return;
    }
}
/* }}} */

/* Method Rindow\OpenCL\Kernel::getInfo(
    int $param_name
) : string {{{ */
static PHP_METHOD(Kernel, getInfo)
{
    zend_long param_name;
    size_t param_value_size_ret;
    cl_int errcode_ret;
    php_rindow_opencl_kernel_t* intern;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_LONG(param_name)
    ZEND_PARSE_PARAMETERS_END();

    intern = Z_RINDOW_OPENCL_KERNEL_OBJ_P(getThis());

    errcode_ret = clGetKernelInfo(intern->kernel,
                      (cl_kernel_info)param_name,
                      0, NULL, &param_value_size_ret);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetKernelInfo Error errcode=%d", errcode_ret);
        return;
    }

    switch(param_name) {
        case CL_KERNEL_REFERENCE_COUNT:
        case CL_KERNEL_NUM_ARGS: {
            cl_uint uint_result;
            zend_long result;
            errcode_ret = clGetKernelInfo(intern->kernel,
                    (cl_kernel_info)param_name,
                    sizeof(cl_uint), &uint_result, NULL);
            result = (zend_long)uint_result;
            RETURN_LONG(result);
            break;
        }
        case CL_KERNEL_ATTRIBUTES:
        case CL_KERNEL_FUNCTION_NAME: {
            char *param_value = emalloc(param_value_size_ret);
            errcode_ret = clGetKernelInfo(intern->kernel,
                              (cl_kernel_info)param_name,
                              param_value_size_ret, param_value, NULL);
            zend_string *param_value_val =
                    zend_string_init(param_value, param_value_size_ret-1, 0);
                                            // str, len, persistent
            efree(param_value);
            RETURN_STR(param_value_val);
            break;
        }
        case CL_KERNEL_CONTEXT: {
            cl_context context;
            errcode_ret = clGetKernelInfo(intern->kernel,
                        (cl_kernel_info)param_name,
                        sizeof(cl_context), &context, NULL);
            // return just context id
            zend_long result = (zend_long)context;
            RETURN_LONG(result);
            break;
        }
        case CL_KERNEL_PROGRAM: {
            cl_program program;
            errcode_ret = clGetKernelInfo(intern->kernel,
                        (cl_kernel_info)param_name,
                        sizeof(cl_program), &program, NULL);
            // return just program id
            zend_long result = (zend_long)program;
            RETURN_LONG(result);
            break;
        }
        default:
            break;
    }
}
/* }}} */

/* Method Rindow\OpenCL\Kernel::getWorkGroupInfo(
    int $param_name
    DeivceList $devices=null
) : string {{{ */
static PHP_METHOD(Kernel, getWorkGroupInfo)
{
    zend_long param_name;
    zval *device_list_obj_p=NULL;
    php_rindow_opencl_device_list_t* device_list_obj;
    size_t param_value_size_ret;
    cl_int errcode_ret;
    cl_device_id device_id;
    php_rindow_opencl_kernel_t* intern;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 2)
        Z_PARAM_LONG(param_name)
        Z_PARAM_OPTIONAL
        Z_PARAM_OBJECT_OF_CLASS_EX(device_list_obj_p,php_rindow_opencl_device_list_ce,1,0)
    ZEND_PARSE_PARAMETERS_END();

    intern = Z_RINDOW_OPENCL_KERNEL_OBJ_P(getThis());

    if(device_list_obj_p!=NULL && Z_TYPE_P(device_list_obj_p)==IS_OBJECT) {
        device_list_obj = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(device_list_obj_p);
        if(device_list_obj->num<1) {
            zend_throw_exception(spl_ce_InvalidArgumentException, "device list is empty", CL_INVALID_VALUE);
            return;
        }
        device_id = device_list_obj->devices[0];
    } else {
        cl_program program;
        errcode_ret = clGetKernelInfo(intern->kernel,
                    (cl_kernel_info)CL_KERNEL_PROGRAM,
                    sizeof(cl_program), &program, NULL);
        if(errcode_ret!=CL_SUCCESS) {
            zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetKernelInfo(CL_KERNEL_PROGRAM) Error errcode=%d", errcode_ret);
            return;
        }
        errcode_ret = clGetProgramInfo(program,
                          (cl_program_info)CL_PROGRAM_DEVICES,
                          0, NULL, &param_value_size_ret);
        if(errcode_ret!=CL_SUCCESS) {
            zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetProgramInfo(CL_PROGRAM_DEVICES) Error errcode=%d", errcode_ret);
            return;
        }
        unsigned int items = (unsigned int)(param_value_size_ret/sizeof(cl_device_id));
        if(items<1) {
            zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "Num of device is zero.");
            return;
        }
        cl_device_id* device_ids = emalloc(param_value_size_ret);
        errcode_ret = clGetProgramInfo(program,
                    (cl_program_info)CL_PROGRAM_DEVICES,
                    param_value_size_ret, device_ids, NULL);
        device_id = device_ids[0];
        efree(device_ids);
    }

    errcode_ret = clGetKernelWorkGroupInfo(intern->kernel,
                      device_id,
                      (cl_kernel_work_group_info)param_name,
                      0, NULL, &param_value_size_ret);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetKernelWorkGroupInfo Error errcode=%d", errcode_ret);
        return;
    }

    switch(param_name) {
        case CL_KERNEL_WORK_GROUP_SIZE:
        case CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE: {
            size_t size_t_result;
            zend_long result;
            errcode_ret = clGetKernelWorkGroupInfo(intern->kernel,
                    device_id,
                    (cl_kernel_work_group_info)param_name,
                    sizeof(size_t), &size_t_result, NULL);
            result = (zend_long)size_t_result;
            RETURN_LONG(result);
            break;
        }
        case CL_KERNEL_LOCAL_MEM_SIZE:
        case CL_KERNEL_PRIVATE_MEM_SIZE: {
            cl_ulong ulong_result;
            zend_long result;
            errcode_ret = clGetKernelWorkGroupInfo(intern->kernel,
                    device_id,
                    (cl_kernel_work_group_info)param_name,
                    sizeof(cl_ulong), &ulong_result, NULL);
            result = (zend_long)ulong_result;
            RETURN_LONG(result);
            break;
        }
        case CL_KERNEL_COMPILE_WORK_GROUP_SIZE:
        case CL_KERNEL_GLOBAL_WORK_SIZE: {
            size_t* item_sizes = emalloc(param_value_size_ret);
            errcode_ret = clGetKernelWorkGroupInfo(intern->kernel,
                        device_id,
                        (cl_kernel_work_group_info)param_name,
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
        default:
            break;
    }
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(ai_Kernel___construct, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, program, Rindow\\OpenCL\\Program, 0)
    ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Kernel_setArg, 0, 0, 2)
    ZEND_ARG_INFO(0, arg_index)
    ZEND_ARG_INFO(0, arg_value)
    ZEND_ARG_INFO(0, dtype)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Kernel_enqueueNDRange, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, command_queue, Rindow\\OpenCL\\CommandQueue, 0)
    ZEND_ARG_ARRAY_INFO(0, global_work_size, 0)
    ZEND_ARG_ARRAY_INFO(0, local_work_size, 1)
    ZEND_ARG_ARRAY_INFO(0, global_work_offset, 1)
    ZEND_ARG_OBJ_INFO(0, events, Rindow\\OpenCL\\EventList, 1)
    ZEND_ARG_OBJ_INFO(0, event_wait_list, Rindow\\OpenCL\\EventList, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Kernel_getInfo, 0, 0, 1)
    ZEND_ARG_INFO(0, param_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Kernel_getWorkGroupInfo, 0, 0, 1)
    ZEND_ARG_INFO(0, param_name)
    ZEND_ARG_OBJ_INFO(0, events, Rindow\\OpenCL\\DeviceList, 1)
ZEND_END_ARG_INFO()

/* {{{ Rindow\OpenCL\Kernel function entries */
static zend_function_entry php_rindow_opencl_kernel_me[] = {
    /* clang-format off */
    PHP_ME(Kernel, __construct,      ai_Kernel___construct,     ZEND_ACC_PUBLIC)
    PHP_ME(Kernel, setArg,           ai_Kernel_setArg,          ZEND_ACC_PUBLIC)
    PHP_ME(Kernel, enqueueNDRange,   ai_Kernel_enqueueNDRange,  ZEND_ACC_PUBLIC)
    PHP_ME(Kernel, getInfo,          ai_Kernel_getInfo,         ZEND_ACC_PUBLIC)
    PHP_ME(Kernel, getWorkGroupInfo, ai_Kernel_getWorkGroupInfo,ZEND_ACC_PUBLIC)
    PHP_FE_END
    /* clang-format on */
};
/* }}} */

/* Class Rindow\OpenCL\Kernel {{{ */
zend_class_entry* php_rindow_opencl_kernel_ce;

void php_rindow_opencl_kernel_init_ce(INIT_FUNC_ARGS)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Rindow\\OpenCL", "Kernel", php_rindow_opencl_kernel_me);
    php_rindow_opencl_kernel_ce = zend_register_internal_class(&ce);
    php_rindow_opencl_kernel_ce->create_object = php_rindow_opencl_kernel_create_object;
    memcpy(&rindow_opencl_kernel_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    rindow_opencl_kernel_object_handlers.offset    = XtOffsetOf(php_rindow_opencl_kernel_t, std);
    rindow_opencl_kernel_object_handlers.free_obj  = php_rindow_opencl_kernel_free_object;
    rindow_opencl_kernel_object_handlers.clone_obj = NULL;

    //zend_class_implements(php_rindow_opencl_kernel_ce, 2, spl_ce_ArrayAccess, spl_ce_Countable);
}
/* }}} */
