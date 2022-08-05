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
#include "Rindow/OpenCL/Program.h"
#include "Rindow/OpenCL/Context.h"
#include "Rindow/OpenCL/DeviceList.h"

#include "php_rindow_opencl.h"

#define PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_SOURCE_CODE      0
#define PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_BINARY           1
#define PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_BUILTIN_KERNEL   2
#define PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_COMPILED_PROGRAM 3

static char ** array_to_strings(
    zval *array_val, cl_uint *num_strings_p, size_t **lengths_p, int *errcode_ret)
{
    cl_uint num_strings;
    char  **strings;
    size_t *lengths;
    zend_ulong idx;
    zend_string *key;
    zval *val;
    cl_uint i=0;

    num_strings = zend_array_count(Z_ARR_P(array_val));
    strings = ecalloc(num_strings,sizeof(char*));
    lengths = ecalloc(num_strings,sizeof(size_t));

    ZEND_HASH_FOREACH_KEY_VAL(Z_ARR_P(array_val), idx, key, val) {
        if(Z_TYPE_P(val) != IS_STRING) {
            efree(strings);
            efree(lengths);
            zend_throw_exception(spl_ce_InvalidArgumentException, "the array must be array of string", CL_INVALID_VALUE);
            *errcode_ret = CL_INVALID_VALUE;
            return NULL;
        }
        if(i<num_strings) {
            strings[i] = Z_STRVAL_P(val);
            lengths[i] = Z_STRLEN_P(val);
            i++;
        }
    } ZEND_HASH_FOREACH_END();
    *num_strings_p = num_strings;
    *lengths_p = lengths;
    *errcode_ret = 0;
    return strings;
}

static cl_program * array_to_programs(
    zval *array_val,
    cl_uint *num_programs_p,
    char ***index_names_p,
    int *errcode_ret)
{
    cl_uint num_programs;
    cl_program  *programs;
    char  **index_names;
    zend_ulong idx;
    zend_string *key;
    zval *val;
    cl_uint i=0;

    num_programs = zend_array_count(Z_ARR_P(array_val));
    programs = ecalloc(num_programs,sizeof(cl_program));
    if(index_names_p) {
        index_names = ecalloc(num_programs,sizeof(char*));
    }

    ZEND_HASH_FOREACH_KEY_VAL(Z_ARR_P(array_val), idx, key, val) {
        if(Z_TYPE_P(val) != IS_OBJECT ||
            !instanceof_function(Z_OBJCE_P(val), php_rindow_opencl_program_ce)
            ) {
            efree(programs);
            if(index_names_p) {
                efree(index_names);
            }
            zend_throw_exception(spl_ce_InvalidArgumentException, "array must be array of Program", CL_INVALID_VALUE);
            *errcode_ret = CL_INVALID_VALUE;
            return NULL;
        }
        if(i<num_programs) {
            programs[i] = Z_RINDOW_OPENCL_PROGRAM_OBJ_P(val)->program;
            if(index_names_p) {
                index_names[i] = ZSTR_VAL(key);
            }
            i++;
        }
    } ZEND_HASH_FOREACH_END();
    *num_programs_p = num_programs;
    if(index_names_p) {
        *index_names_p = index_names;
    }
    *errcode_ret = 0;
    return programs;
}

static zend_object_handlers rindow_opencl_program_object_handlers;

// destractor
static void php_rindow_opencl_program_free_object(zend_object* object)
{
    php_rindow_opencl_program_t* obj = php_rindow_opencl_program_fetch_object(object);
    if(obj->program) {
        clReleaseProgram(obj->program);
    }
    zend_object_std_dtor(&obj->std);
}

// constructor
static zend_object* php_rindow_opencl_program_create_object(zend_class_entry* class_type) /* {{{ */
{
    php_rindow_opencl_program_t* intern = NULL;

    intern = (php_rindow_opencl_program_t*)ecalloc(1, sizeof(php_rindow_opencl_program_t) + zend_object_properties_size(class_type));
    intern->signature = PHP_RINDOW_OPENCL_PROGRAM_SIGNATURE;
    intern->program = NULL;

    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);

    intern->std.handlers = &rindow_opencl_program_object_handlers;

    return &intern->std;
} /* }}} */

/* Method Rindow\OpenCL\Program::__construct(
    Context $context,
    $sources,       // array of source code strings or deivce list object
    int $mode=0,
    DeviceList $devices=null,
    string $options=null
) {{{ */
static PHP_METHOD(Program, __construct)
{
    php_rindow_opencl_program_t* intern;
    php_rindow_opencl_context_t* context_obj;
    php_rindow_opencl_device_list_t* device_list_obj=NULL;
    zval* context_obj_p=NULL;
    zval* source_obj_p=NULL;
    zval* device_list_obj_p=NULL;
    zend_long mode=PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_SOURCE_CODE;
    cl_device_id *devices=NULL;
    cl_uint num_devices=0;
    char *options=NULL;
    size_t options_len=0;
    cl_program program;
    cl_int errcode_ret=0;


    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 5)
        Z_PARAM_OBJECT_OF_CLASS(context_obj_p,php_rindow_opencl_context_ce)
        Z_PARAM_ZVAL(source_obj_p)   // string or list of something
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(mode) // mode  0:source codes, 1:binary, 2:built-in kernel, 3:linker
        Z_PARAM_OBJECT_OF_CLASS_EX(device_list_obj_p,php_rindow_opencl_device_list_ce,1,0)
        Z_PARAM_STRING_EX(options,options_len,1,0)
    ZEND_PARSE_PARAMETERS_END();

    context_obj = Z_RINDOW_OPENCL_CONTEXT_OBJ_P(context_obj_p);

    if(device_list_obj_p!=NULL && Z_TYPE_P(device_list_obj_p)==IS_OBJECT) {
        device_list_obj = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(source_obj_p);
        if(device_list_obj->devices==NULL) {
            zend_throw_exception(spl_ce_InvalidArgumentException, "devices is empty", CL_INVALID_DEVICE);
            return;
        }
        devices = device_list_obj->devices;
        num_devices = device_list_obj->num;
    }

    switch(mode) {
        case PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_SOURCE_CODE: // binary mode
        case PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_BINARY: {// source mode
            cl_uint num_strings;
            char  **strings;
            size_t *lengths;
            if(Z_TYPE_P(source_obj_p)==IS_STRING) {
                num_strings = 1;
                strings = emalloc(sizeof(char**));
                lengths = emalloc(sizeof(size_t));
                strings[0] = Z_STRVAL_P(source_obj_p);
                lengths[0] = Z_STRLEN_P(source_obj_p);
            } else if(Z_TYPE_P(source_obj_p)==IS_ARRAY) {
                strings = array_to_strings(source_obj_p, &num_strings, &lengths, &errcode_ret);
                if(errcode_ret!=CL_SUCCESS) {
                    return;
                }
            } else {
                zend_throw_exception(spl_ce_InvalidArgumentException, "argument 2 must be source string or array of strings in source code mode.", CL_INVALID_VALUE);
                return;
            }
            if(mode==0) {  // source mode
                program = clCreateProgramWithSource(
                    context_obj->context,
                    num_strings,
                    (const char**)strings,
                    lengths,
                    &errcode_ret);
                efree(strings);
                efree(lengths);
                if(errcode_ret!=CL_SUCCESS) {
                    zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clCreateProgramWithSource Error errcode=%d", errcode_ret);
                    return;
                }
            } else {  // binary mode
                program = clCreateProgramWithBinary(
                    context_obj->context,
                    num_devices,
                    devices,
                    lengths,
                    (const unsigned char**)strings,
                    NULL,                 // cl_int * binary_status,
                    &errcode_ret);
                efree(strings);
                efree(lengths);
                if(errcode_ret!=CL_SUCCESS) {
                    zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clCreateProgramWithBinary Error errcode=%d", errcode_ret);
                    return;
                }
            }
            break;
        }
#ifdef CL_VERSION_1_2
        case PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_BUILTIN_KERNEL: { // built-in kernel mode
            if(Z_TYPE_P(source_obj_p)!=IS_STRING) {
                zend_throw_exception(spl_ce_InvalidArgumentException, "built-in kernel mode must be include kernel name string", CL_INVALID_VALUE);
                return;
            }
            size_t names_length;
            char *kernel_names;
            names_length = Z_STRLEN_P(source_obj_p);
            kernel_names = emalloc(names_length+1);
            memcpy(kernel_names,Z_STRVAL_P(source_obj_p),names_length);
            kernel_names[names_length] = 0;
            program = clCreateProgramWithBuiltInKernels(
                context_obj->context,
                num_devices,
                devices,
                kernel_names,
                &errcode_ret);
            efree(kernel_names);
            if(errcode_ret!=CL_SUCCESS) {
                zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clCreateProgramWithBuiltInKernels Error errcode=%d", errcode_ret);
                return;
            }
            break;
        }
        case PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_COMPILED_PROGRAM:{ // linker mode
            cl_program *input_programs;
            cl_uint num_input_programs;

            if(Z_TYPE_P(source_obj_p)!=IS_ARRAY) {
                zend_throw_exception(spl_ce_InvalidArgumentException, "link mode must be include array of programs", CL_INVALID_VALUE);
                return;
            }
            input_programs = array_to_programs(source_obj_p, &num_input_programs, NULL, &errcode_ret);
            if(errcode_ret!=CL_SUCCESS) {
                return;
            }
            program = clLinkProgram(
                context_obj->context,
                num_devices,
                devices,
                options,
                num_input_programs,
                input_programs,
                NULL,        // CL_CALLBACK *  pfn_notify
                NULL,        // void * user_data
                &errcode_ret
            );
            efree(input_programs);
            if(errcode_ret!=CL_SUCCESS) {
                zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clLinkProgram Error errcode=%d", errcode_ret);
                return;
            }
            break;
        }
#endif
        default: {
            zend_throw_exception(spl_ce_InvalidArgumentException, "invalid mode.", CL_INVALID_VALUE);
            return;
        }
    } // end switch
    intern = Z_RINDOW_OPENCL_PROGRAM_OBJ_P(getThis());
    intern->program = program;
}
/* }}} */

/* Method Rindow\OpenCL\Program::build(
    string $options=NULL,
    DeviceList $devices=NULL,
) {{{ */
static PHP_METHOD(Program, build)
{
    php_rindow_opencl_program_t* intern=NULL;
    php_rindow_opencl_device_list_t* device_list_obj=NULL;
    zval* device_list_obj_p=NULL;
    char *options=NULL;
    size_t options_len=0;
    cl_device_id *device_list=NULL;
    cl_uint num_devices=0;
    cl_int errcode_ret = 0;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 2)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING_EX(options,options_len,1,0)
        Z_PARAM_OBJECT_OF_CLASS_EX(device_list_obj_p,php_rindow_opencl_context_ce,1,0)
    ZEND_PARSE_PARAMETERS_END();

    if(device_list_obj_p!=NULL && Z_TYPE_P(device_list_obj_p)==IS_OBJECT) {
        device_list_obj = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(device_list_obj_p);
        device_list = device_list_obj->devices;
        num_devices = device_list_obj->num;
    }
    intern = Z_RINDOW_OPENCL_PROGRAM_OBJ_P(getThis());
    errcode_ret = clBuildProgram(
        intern->program,
        num_devices,
        device_list,
        options,
        NULL,        // CL_CALLBACK *  pfn_notify
        NULL         // void * user_data
    );
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clBuildProgram Error errcode=%d", errcode_ret);
        return;
    }
}
/* }}} */

#ifdef CL_VERSION_1_2
/* Method Rindow\OpenCL\Program::compile(
    array $headers=null,  // ArrayHash<Program> Key:file path Value:program
    string $options=null, // string
    $devices=null,        // DeviceList
) {{{ */
static PHP_METHOD(Program, compile)
{
    php_rindow_opencl_program_t* intern=NULL;
    php_rindow_opencl_device_list_t* device_list_obj=NULL;
    zval* device_list_obj_p=NULL;
    zval* headers_array_p=NULL;
    char *options=NULL;
    size_t options_len=0;
    cl_device_id *device_list=NULL;
    cl_uint num_devices=0;
    cl_int errcode_ret = 0;
    cl_uint num_input_headers=0;
    cl_program *input_headers=NULL;
    char **header_include_names=NULL;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 3)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_EX(headers_array_p,1,0)
        Z_PARAM_STRING_EX(options,options_len,1,0)
        Z_PARAM_OBJECT_OF_CLASS_EX(device_list_obj_p,php_rindow_opencl_context_ce,1,0)
    ZEND_PARSE_PARAMETERS_END();

    if(headers_array_p!=NULL && Z_TYPE_P(headers_array_p)==IS_ARRAY) {
        input_headers = array_to_programs(headers_array_p, &num_input_headers, &header_include_names, &errcode_ret);
        if(errcode_ret!=CL_SUCCESS) {
            return;
        }
    }
    if(device_list_obj_p!=NULL && Z_TYPE_P(device_list_obj_p)==IS_OBJECT) {
        device_list_obj = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(device_list_obj_p);
        device_list = device_list_obj->devices;
        num_devices = device_list_obj->num;
    }
    intern = Z_RINDOW_OPENCL_PROGRAM_OBJ_P(getThis());
    errcode_ret = clCompileProgram(
        intern->program,
        num_devices,
        device_list,
        options,
        num_input_headers,
        input_headers,
        (const char **)header_include_names,
        NULL,        // CL_CALLBACK *  pfn_notify
        NULL         // void * user_data
    );
    if(input_headers) {
        efree(input_headers);
    }
    if(header_include_names) {
        efree(header_include_names);
    }
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clCompileProgram Error errcode=%d", errcode_ret);
        return;
    }
}
/* }}} */
#endif

/* Method Rindow\OpenCL\Program::getInfo(
    int $param_name
) : string {{{ */
static PHP_METHOD(Program, getInfo)
{
    zend_long param_name;
    size_t param_value_size_ret;
    cl_int errcode_ret;
    php_rindow_opencl_program_t* intern;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_LONG(param_name)
    ZEND_PARSE_PARAMETERS_END();

    intern = Z_RINDOW_OPENCL_PROGRAM_OBJ_P(getThis());

    errcode_ret = clGetProgramInfo(intern->program,
                      (cl_program_info)param_name,
                      0, NULL, &param_value_size_ret);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetProgramInfo Error errcode=%d", errcode_ret);
        return;
    }
    switch(param_name) {
        case CL_PROGRAM_REFERENCE_COUNT:
        case CL_PROGRAM_NUM_DEVICES: {
            cl_uint uint_result;
            zend_long result;
            errcode_ret = clGetProgramInfo(intern->program,
                    (cl_program_info)param_name,
                    sizeof(cl_uint), &uint_result, NULL);
            result = (zend_long)uint_result;
            RETURN_LONG(result);
            break;
        }
#ifdef CL_VERSION_1_2
        case CL_PROGRAM_NUM_KERNELS: {
            size_t size_t_result;
            zend_long result;
            errcode_ret = clGetProgramInfo(intern->program,
                    (cl_program_info)param_name,
                    sizeof(size_t), &size_t_result, NULL);
            result = (zend_long)size_t_result;
            RETURN_LONG(result);
            break;
        }
#endif
#ifdef CL_VERSION_2_1
        case CL_PROGRAM_IL:
#endif
#ifdef CL_VERSION_1_2
        case CL_PROGRAM_KERNEL_NAMES:
#endif
        case CL_PROGRAM_SOURCE: {
            char *param_value = emalloc(param_value_size_ret);
            errcode_ret = clGetProgramInfo(intern->program,
                              (cl_program_info)param_name,
                              param_value_size_ret, param_value, NULL);
            zend_string *param_value_val =
                    zend_string_init(param_value, param_value_size_ret-1, 0);
                                            // str, len, persistent
            efree(param_value);
            RETURN_STR(param_value_val);
            break;
        }
        case CL_PROGRAM_DEVICES: {
            cl_device_id* device_ids = emalloc(param_value_size_ret);

            errcode_ret = clGetProgramInfo(intern->program,
                        (cl_program_info)param_name,
                        param_value_size_ret, device_ids, NULL);
            unsigned int items = (unsigned int)(param_value_size_ret/sizeof(cl_device_id));
            // direct set to return_value
            php_rindow_opencl_device_list_new_device_object(
                return_value, device_ids, items);
            efree(device_ids);
            return;
        }
        case CL_PROGRAM_BINARY_SIZES: {
            size_t* sizes = emalloc(param_value_size_ret);
            errcode_ret = clGetProgramInfo(intern->program,
                        (cl_program_info)param_name,
                        param_value_size_ret, sizes, NULL);
            unsigned int items = (unsigned int)(param_value_size_ret/sizeof(size_t));
            // direct set to return_value
            array_init(return_value);
            for(unsigned int i=0;i<items;i++) {
                add_next_index_long(return_value, (zend_long)sizes[i]);
            }
            efree(sizes);
            return;
        }
#ifdef CL_VERSION_2_2
        case CL_PROGRAM_SCOPE_GLOBAL_CTORS_PRESENT:
        case CL_PROGRAM_SCOPE_GLOBAL_DTORS_PRESENT: {
            cl_bool bool_result;
            zend_long result;
            errcode_ret = clGetProgramInfo(intern->program,
                    (cl_program_info)param_name,
                    sizeof(cl_bool), &bool_result, NULL);
            result = (zend_long)bool_result;
            RETURN_LONG(result);
            break;
        }
#endif
        default:
            break;
    }
}
/* }}} */

/* Method Rindow\OpenCL\Program::getBuildInfo(
    int $param_name
    DeviceList $deivce=null,
    int $device_index
) : string {{{ */
static PHP_METHOD(Program, getBuildInfo)
{
    zend_long param_name;
    zval* device_list_obj_p=NULL;
    zend_long device_index=0;
    size_t param_value_size_ret;
    cl_int errcode_ret;
    php_rindow_opencl_program_t* intern;
    php_rindow_opencl_device_list_t* device_list_obj=NULL;
    cl_device_id          device;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_LONG(param_name)
        Z_PARAM_OPTIONAL
        Z_PARAM_OBJECT_OF_CLASS_EX(device_list_obj_p,php_rindow_opencl_context_ce,1,0)
        Z_PARAM_LONG(device_index)
    ZEND_PARSE_PARAMETERS_END();

    intern = Z_RINDOW_OPENCL_PROGRAM_OBJ_P(getThis());
    if(device_list_obj_p!=NULL && Z_TYPE_P(device_list_obj_p)==IS_OBJECT) {
        device_list_obj = Z_RINDOW_OPENCL_DEVICE_LIST_OBJ_P(device_list_obj_p);
        if(device_list_obj->devices==NULL) {
            zend_throw_exception(spl_ce_InvalidArgumentException, "devices is empty", CL_INVALID_DEVICE);
            return;
        }
        if(device_index<0 || device_index >= device_list_obj->num) {
            zend_throw_exception(spl_ce_InvalidArgumentException, "invalid device index.", CL_INVALID_DEVICE);
            return;
        }
        device = device_list_obj->devices[device_index];
    } else { // get first deivce in program
        errcode_ret = clGetProgramInfo(intern->program,
                          (cl_program_info)CL_PROGRAM_DEVICES,
                          0, NULL, &param_value_size_ret);
        if(errcode_ret!=CL_SUCCESS) {
            zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetProgramInfo Error errcode=%d", errcode_ret);
            return;
        }
        cl_device_id* device_ids = emalloc(param_value_size_ret);
        if(device_ids==NULL) {
            zend_throw_exception_ex(spl_ce_RuntimeException, 0, "memory allocate error for device_ids.");
            return;
        }
        errcode_ret = clGetProgramInfo(intern->program,
                    (cl_program_info)CL_PROGRAM_DEVICES,
                    param_value_size_ret, device_ids, NULL);
        device = device_ids[0];
        efree(device_ids);
    }

    errcode_ret = clGetProgramBuildInfo(intern->program,
                          device,
                          (cl_program_build_info)param_name,
                          0, NULL, &param_value_size_ret);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetProgramBuildInfo Error errcode=%d", errcode_ret);
        return;
    }

    switch(param_name) {
        case CL_PROGRAM_BUILD_STATUS: {
            cl_int int_result;
            zend_long result;
            errcode_ret = clGetProgramBuildInfo(intern->program,
                    device,
                    (cl_program_info)param_name,
                    sizeof(cl_int), &int_result, NULL);
            result = (zend_long)int_result;
            RETURN_LONG(result);
            break;
        }
#ifdef CL_VERSION_1_2
        case CL_PROGRAM_BINARY_TYPE: {
            cl_uint uint_result;
            zend_long result;
            errcode_ret = clGetProgramBuildInfo(intern->program,
                    device,
                    (cl_program_info)param_name,
                    sizeof(cl_uint), &uint_result, NULL);
            result = (zend_long)uint_result;
            RETURN_LONG(result);
            break;
        }
#endif
        case CL_PROGRAM_BUILD_OPTIONS:
        case CL_PROGRAM_BUILD_LOG: {
            zend_string *param_value_val = zend_string_safe_alloc(param_value_size_ret, 1, 0, 0);
            char *param_value = ZSTR_VAL(param_value_val);
            errcode_ret = clGetProgramBuildInfo(intern->program,
                            device,
                            (cl_program_info)param_name,
                            param_value_size_ret, param_value, NULL);
            param_value[param_value_size_ret] = 0;
            RETURN_STR(param_value_val);
            break;
        }
#ifdef CL_VERSION_2_0
        case CL_PROGRAM_BUILD_GLOBAL_VARIABLE_TOTAL_SIZE: {
            size_t size_t_result;
            zend_long result;
            errcode_ret = clGetProgramBuildInfo(intern->program,
                    device,
                    (cl_program_info)param_name,
                    sizeof(size_t), &size_t_result, NULL);
            result = (zend_long)size_t_result;
            RETURN_LONG(result);
            break;
        }
#endif
        default:
            break;
    }
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(ai_Program___construct, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, a, Rindow\\OpenCL\\Context, 0)
    ZEND_ARG_INFO(0, sources)
    ZEND_ARG_INFO(0, mode)
    ZEND_ARG_INFO(0, devices)
    ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Program_build, 0, 0, 0)
    ZEND_ARG_INFO(0, options)
    ZEND_ARG_OBJ_INFO(0, devices, Rindow\\OpenCL\\DeviceList, 1)
ZEND_END_ARG_INFO()

#ifdef CL_VERSION_1_2
ZEND_BEGIN_ARG_INFO_EX(ai_Program_compile, 0, 0, 0)
    ZEND_ARG_ARRAY_INFO(0, headers, 1)
    ZEND_ARG_INFO(0, options)
    ZEND_ARG_OBJ_INFO(0, deivces, Rindow\\OpenCL\\DeviceList, 1)
ZEND_END_ARG_INFO()
#endif

ZEND_BEGIN_ARG_INFO_EX(ai_Program_getInfo, 0, 0, 1)
    ZEND_ARG_INFO(0, param_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Program_getBuildInfo, 0, 0, 1)
    ZEND_ARG_INFO(0, param_name)
    ZEND_ARG_OBJ_INFO(0, deivces, Rindow\\OpenCL\\DeviceList, 1)
    ZEND_ARG_INFO(0, device_index)
ZEND_END_ARG_INFO()


/* {{{ Rindow\OpenCL\Program function entries */
static zend_function_entry php_rindow_opencl_program_me[] = {
    /* clang-format off */
    PHP_ME(Program, __construct, ai_Program___construct, ZEND_ACC_PUBLIC)
    PHP_ME(Program, build, ai_Program_build, ZEND_ACC_PUBLIC)
#ifdef CL_VERSION_1_2
    PHP_ME(Program, compile, ai_Program_compile, ZEND_ACC_PUBLIC)
#endif
    PHP_ME(Program, getInfo, ai_Program_getInfo, ZEND_ACC_PUBLIC)
    PHP_ME(Program, getBuildInfo, ai_Program_getBuildInfo, ZEND_ACC_PUBLIC)
    PHP_FE_END
    /* clang-format on */
};
/* }}} */

/* Class Rindow\OpenCL\Program {{{ */
zend_class_entry* php_rindow_opencl_program_ce;

void php_rindow_opencl_program_init_ce(INIT_FUNC_ARGS)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Rindow\\OpenCL", "Program", php_rindow_opencl_program_me);
    php_rindow_opencl_program_ce = zend_register_internal_class(&ce);
    php_rindow_opencl_program_ce->create_object = php_rindow_opencl_program_create_object;
    memcpy(&rindow_opencl_program_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    rindow_opencl_program_object_handlers.offset    = XtOffsetOf(php_rindow_opencl_program_t, std);
    rindow_opencl_program_object_handlers.free_obj  = php_rindow_opencl_program_free_object;
    rindow_opencl_program_object_handlers.clone_obj = NULL;
    zend_declare_class_constant_long(php_rindow_opencl_program_ce, ZEND_STRL("TYPE_SOURCE_CODE"), PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_SOURCE_CODE);
    zend_declare_class_constant_long(php_rindow_opencl_program_ce, ZEND_STRL("TYPE_BINARY"), PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_BINARY);
    zend_declare_class_constant_long(php_rindow_opencl_program_ce, ZEND_STRL("TYPE_BUILTIN_KERNEL"), PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_BUILTIN_KERNEL);
    zend_declare_class_constant_long(php_rindow_opencl_program_ce, ZEND_STRL("TYPE_COMPILED_PROGRAM"), PHP_RINDOW_OPENCL_PROGRAM_CONST_TYPE_COMPILED_PROGRAM);

    //zend_class_implements(php_rindow_opencl_program_ce, 2, spl_ce_ArrayAccess, spl_ce_Countable);
}
/* }}} */
