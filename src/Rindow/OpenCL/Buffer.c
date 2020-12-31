#include <php.h>
#include <Zend/zend_interfaces.h>
#include <Zend/zend_exceptions.h>
#include <ext/spl/spl_iterators.h>
#include <ext/spl/spl_exceptions.h>
#include <stdint.h>
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/opencl.h>
#include "Rindow/OpenCL/Buffer.h"
#include "Rindow/OpenCL/Context.h"
#include "Rindow/OpenCL/EventList.h"
#include "Rindow/OpenCL/CommandQueue.h"
#include <Interop/Polite/Math/Matrix.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_rindow_opencl.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 1
#endif

static zend_object_handlers rindow_opencl_buffer_object_handlers;

int php_rindow_opencl_assert_host_buffer_type(
    php_interop_polite_math_matrix_linear_buffer_t *buffer,
    char* name)
{
    if(!php_interop_polite_math_matrix_is_linear_buffer(buffer)) {
        zend_throw_exception_ex(zend_ce_type_error, 0, "%s must implement interface %s",
            name,PHP_INTEROP_POLITE_MATH_MATRIX_LINEAR_BUFFER_CLASSNAME);
        return 1;
    }
    return 0;
}


// destractor
static void php_rindow_opencl_buffer_free_object(zend_object* object)
{
    php_rindow_opencl_buffer_t* obj = php_rindow_opencl_buffer_fetch_object(object);
    if(obj->buffer) {
        clReleaseMemObject(obj->buffer);
    }
    if(obj->host_buffer_val) {
        Z_DELREF_P(obj->host_buffer_val);
    }
    zend_object_std_dtor(&obj->std);
}

// constructor
static zend_object* php_rindow_opencl_buffer_create_object(zend_class_entry* class_type) /* {{{ */
{
    php_rindow_opencl_buffer_t* intern = NULL;

    intern = (php_rindow_opencl_buffer_t*)ecalloc(1, sizeof(php_rindow_opencl_buffer_t) + zend_object_properties_size(class_type));
    intern->signature = PHP_RINDOW_OPENCL_BUFFER_SIGNATURE;
    intern->buffer = NULL;
    intern->size = 0;
    intern->dtype = 0;
    intern->value_size = 0;
    intern->host_buffer_val = NULL;

    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);

    intern->std.handlers = &rindow_opencl_buffer_object_handlers;

    return &intern->std;
} /* }}} */


/* Method Rindow\OpenCL\Buffer::__construct(
    Context $context,
    int     $size,                               // buffer memory size
    int     $flags=0,                            // flags bit
    Interop\Polite\Math\Matrix\LinearBuffer $hostBuffer=null  // host buffer assigned
    int     $host_offset=0,
    int     $dtype=0
) {{{ */
static PHP_METHOD(Buffer, __construct)
{
    zval* context_obj_p;
    zend_long size;
    zend_long flags=0;
    zval* host_buffer_obj_p=NULL;
    zend_long host_offset=0;
    zend_long dtype=0;
    php_rindow_opencl_buffer_t* intern;
    php_rindow_opencl_context_t* context_obj;
    php_interop_polite_math_matrix_linear_buffer_t* host_buffer_obj;
    char *host_ptr=NULL;
    cl_int errcode_ret=0;
    cl_mem buffer;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 6)
        Z_PARAM_OBJECT_OF_CLASS(context_obj_p,php_rindow_opencl_context_ce)
        Z_PARAM_LONG(size)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(flags)
        Z_PARAM_OBJECT_EX(host_buffer_obj_p,1,0)  // Interop\Polite\Math\Matrix\LinearBuffer
        Z_PARAM_LONG(host_offset)
        Z_PARAM_LONG(dtype)
    ZEND_PARSE_PARAMETERS_END();

    context_obj = Z_RINDOW_OPENCL_CONTEXT_OBJ_P(context_obj_p);

    if(host_buffer_obj_p!=NULL && Z_TYPE_P(host_buffer_obj_p)==IS_OBJECT) {
        host_buffer_obj = Z_INTEROP_POLITE_MATH_MATRIX_LINEAR_BUFFER_OBJ_P(host_buffer_obj_p);
        if(php_rindow_opencl_assert_host_buffer_type(host_buffer_obj, "hostBuffer")) {
            return;
        }
        if(host_buffer_obj->data==NULL) {
            zend_throw_exception(spl_ce_InvalidArgumentException, "Host buffer is not initialized.", CL_INVALID_VALUE);
            return;
        }
        if(((host_buffer_obj->size - host_offset) * host_buffer_obj->value_size)<size) {
            zend_throw_exception(spl_ce_InvalidArgumentException, "Host buffer is too small.", CL_INVALID_VALUE);
            return;
        }
        host_ptr = host_buffer_obj->data;
        host_ptr += host_offset * host_buffer_obj->value_size;
    }

    buffer = clCreateBuffer(
        context_obj->context,
        (cl_mem_flags)flags,
        (size_t)size,
        host_ptr,
        &errcode_ret);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clCreateBuffer Error errcode=%d", errcode_ret);
        return;
    }
    intern = Z_RINDOW_OPENCL_BUFFER_OBJ_P(getThis());
    intern->buffer = buffer;
    intern->size = size;
    if(flags&(CL_MEM_COPY_HOST_PTR|CL_MEM_USE_HOST_PTR)) {
        intern->dtype = host_buffer_obj->dtype;
        intern->value_size = host_buffer_obj->value_size;
    }
    if(dtype) {
        intern->dtype = dtype;
        intern->value_size = php_rindow_opencl_dtype_to_valuesize(dtype);
    }
    if((flags&CL_MEM_USE_HOST_PTR) && host_buffer_obj_p!=NULL) {
        Z_ADDREF_P(host_buffer_obj_p);
        intern->host_buffer_val = host_buffer_obj_p;
    }
}
/* }}} */

/* Method Rindow\OpenCL\Buffer::dtype(
) : int {{{ */
static PHP_METHOD(Buffer, dtype)
{
    php_rindow_opencl_buffer_t* intern;

    intern = Z_RINDOW_OPENCL_BUFFER_OBJ_P(getThis());

    RETURN_LONG(intern->dtype);
}
/* }}} */

/* Method Rindow\OpenCL\Buffer::value_size(
) : int {{{ */
static PHP_METHOD(Buffer, value_size)
{
    php_rindow_opencl_buffer_t* intern;

    intern = Z_RINDOW_OPENCL_BUFFER_OBJ_P(getThis());

    RETURN_LONG(intern->value_size);
}
/* }}} */

/* Method Rindow\OpenCL\Buffer::bytes(
) : int {{{ */
static PHP_METHOD(Buffer, bytes)
{
    php_rindow_opencl_buffer_t* intern;

    intern = Z_RINDOW_OPENCL_BUFFER_OBJ_P(getThis());

    RETURN_LONG(intern->size);
}
/* }}} */


/* Method Rindow\OpenCL\Buffer::read(
    CommandQueue $command_queue,
    Interop\Polite\Math\LinearBuffer $hostBuffer  // host buffer assigned
    int      $size=0,
    int      $offset=0,
    int      $host_offset=0,
    bool     $blocking_read=true,
    EventList $events=null,
    EventList $event_wait_list=null
) : void {{{ */
static PHP_METHOD(Buffer, read)
{
    zval* command_queue_obj_p=NULL;
    zval* host_buffer_obj_p=NULL;
    zend_long size=0;
    zend_long offset=0;
    zend_long host_offset=0;
    zend_bool blocking_read=TRUE;
    zend_bool blocking_read_is_null=TRUE;
    zval* events_obj_p=NULL;
    zval* event_wait_list_obj_p=NULL;
    php_rindow_opencl_buffer_t* intern;
    php_rindow_opencl_command_queue_t* command_queue_obj;
    php_interop_polite_math_matrix_linear_buffer_t* host_buffer_obj;
    php_rindow_opencl_event_list_t* event_wait_list_obj;
    char *host_ptr=NULL;
    cl_event *event_wait_list=NULL;
    cl_uint num_events_in_wait_list=0;
    cl_event event_ret;
    cl_event *event_p=NULL;
    cl_int errcode_ret=0;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 8)
        Z_PARAM_OBJECT_OF_CLASS(command_queue_obj_p,php_rindow_opencl_command_queue_ce)
        Z_PARAM_OBJECT_EX(host_buffer_obj_p,1,0)  // Interop\Polite\Math\Matrix\LinearBuffer
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(size)
        Z_PARAM_LONG(offset)
        Z_PARAM_LONG(host_offset)
        Z_PARAM_BOOL_EX(blocking_read, blocking_read_is_null, 1, 0)
        Z_PARAM_OBJECT_OF_CLASS_EX(events_obj_p,php_rindow_opencl_event_list_ce,1,0)
        Z_PARAM_OBJECT_OF_CLASS_EX(event_wait_list_obj_p,php_rindow_opencl_event_list_ce,1,0)
    ZEND_PARSE_PARAMETERS_END();

    if(blocking_read_is_null) {
        blocking_read = TRUE;
    }

    command_queue_obj = Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(command_queue_obj_p);
    if(host_buffer_obj_p==NULL) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Host buffer must be LinearBuffer object.", CL_INVALID_VALUE);
        return;
    }

    host_buffer_obj = Z_INTEROP_POLITE_MATH_MATRIX_LINEAR_BUFFER_OBJ_P(host_buffer_obj_p);
    if(php_rindow_opencl_assert_host_buffer_type(host_buffer_obj, "hostBuffer")) {
        return;
    }
    if(host_buffer_obj->data==NULL) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Host buffer is not initialized.", CL_INVALID_VALUE);
        return;
    }
    if(((host_buffer_obj->size - host_offset) * host_buffer_obj->value_size)<size-offset) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Host buffer is too small.", CL_INVALID_VALUE);
        return;
    }
    host_ptr = host_buffer_obj->data;
    host_ptr += host_offset * host_buffer_obj->value_size;

    intern = Z_RINDOW_OPENCL_BUFFER_OBJ_P(getThis());

    if(size==0) {
        size = intern->size;
    }

    if(events_obj_p!=NULL && Z_TYPE_P(events_obj_p)==IS_OBJECT) {
        event_p = &event_ret;
    }

    if(event_wait_list_obj_p!=NULL && Z_TYPE_P(event_wait_list_obj_p)==IS_OBJECT) {
        event_wait_list_obj = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(event_wait_list_obj_p);
        num_events_in_wait_list = event_wait_list_obj->num;
        event_wait_list = event_wait_list_obj->events;
    }

    errcode_ret = clEnqueueReadBuffer(
        command_queue_obj->command_queue,
        intern->buffer,
        (cl_bool)blocking_read,
        (size_t)offset,
        (size_t)size,
        host_ptr,
        num_events_in_wait_list,
        event_wait_list,
        event_p);

    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clEnqueueReadBuffer Error errcode=%d", errcode_ret);
        return;
    }

    // append event to events
    if(php_rindow_opencl_append_event(events_obj_p, event_p)) {
        return;
    }
}
/* }}} */

/* Method Rindow\OpenCL\Buffer::write(
    CommandQueue $command_queue,
    Interop\Polite\Math\LinearBuffer $hostBuffer  // host buffer assigned
    int      $size=0,          // bytes
    int      $offset=0,        // bytes
    int      $host_offset=0,   // items
    bool     $blocking_write=true,
    EventList $events=null,
    EventList $event_wait_list=null
) : void {{{ */
static PHP_METHOD(Buffer, write)
{
    zval* command_queue_obj_p=NULL;
    zval* host_buffer_obj_p=NULL;
    zend_long size=0;
    zend_long offset=0;
    zend_long host_offset=0;
    zend_bool blocking_write=TRUE;
    zend_bool blocking_write_is_null=TRUE;
    zval* events_obj_p=NULL;
    zval* event_wait_list_obj_p=NULL;
    php_rindow_opencl_buffer_t* intern;
    php_rindow_opencl_command_queue_t* command_queue_obj;
    php_interop_polite_math_matrix_linear_buffer_t* host_buffer_obj;
    php_rindow_opencl_event_list_t* event_wait_list_obj;
    char *host_ptr=NULL;
    cl_int errcode_ret=0;
    cl_event *event_wait_list=NULL;
    cl_uint num_events_in_wait_list=0;
    cl_event event_ret;
    cl_event *event_p=NULL;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 8)
        Z_PARAM_OBJECT_OF_CLASS(command_queue_obj_p,php_rindow_opencl_command_queue_ce)
        Z_PARAM_OBJECT_EX(host_buffer_obj_p,1,0)  // Interop\Polite\Math\Matrix\LinearBuffer
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(size)
        Z_PARAM_LONG(offset)
        Z_PARAM_LONG(host_offset)
        Z_PARAM_BOOL_EX(blocking_write, blocking_write_is_null, 1, 0)
        Z_PARAM_OBJECT_OF_CLASS_EX(events_obj_p,php_rindow_opencl_event_list_ce,1,0)
        Z_PARAM_OBJECT_OF_CLASS_EX(event_wait_list_obj_p,php_rindow_opencl_event_list_ce,1,0)
    ZEND_PARSE_PARAMETERS_END();

    if(blocking_write_is_null) {
        blocking_write = TRUE;
    }

    command_queue_obj = Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(command_queue_obj_p);
    if(host_buffer_obj_p==NULL) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Host buffer must be LinearBuffer object.", CL_INVALID_VALUE);
        return;
    }

    host_buffer_obj = Z_INTEROP_POLITE_MATH_MATRIX_LINEAR_BUFFER_OBJ_P(host_buffer_obj_p);
    if(php_rindow_opencl_assert_host_buffer_type(host_buffer_obj, "hostBuffer")) {
        return;
    }
    if(host_buffer_obj->data==NULL) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Host buffer is not initialized.", CL_INVALID_VALUE);
        return;
    }
    if(((host_buffer_obj->size - host_offset) * host_buffer_obj->value_size)<size-offset) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Host buffer is too small.", CL_INVALID_VALUE);
        return;
    }
    host_ptr = host_buffer_obj->data;
    host_ptr += host_offset * host_buffer_obj->value_size;

    intern = Z_RINDOW_OPENCL_BUFFER_OBJ_P(getThis());

    if(size==0) {
        size = intern->size;
    }

    if(events_obj_p!=NULL && Z_TYPE_P(events_obj_p)==IS_OBJECT) {
        event_p = &event_ret;
    }

    if(event_wait_list_obj_p!=NULL && Z_TYPE_P(event_wait_list_obj_p)==IS_OBJECT) {
        event_wait_list_obj = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(event_wait_list_obj_p);
        num_events_in_wait_list = event_wait_list_obj->num;
        event_wait_list = event_wait_list_obj->events;
    }

    errcode_ret = clEnqueueWriteBuffer(
        command_queue_obj->command_queue,
        intern->buffer,
        (cl_bool)blocking_write,
        (size_t)offset,
        (size_t)size,
        host_ptr,
        num_events_in_wait_list,
        event_wait_list,
        event_p);

    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clEnqueueWriteBuffer Error errcode=%d", errcode_ret);
        return;
    }
    intern->dtype = host_buffer_obj->dtype;
    intern->value_size = host_buffer_obj->value_size;

    // append event to events
    if(php_rindow_opencl_append_event(events_obj_p, event_p)) {
        return;
    }
}
/* }}} */

/* Method Rindow\OpenCL\Buffer::fill(
    CommandQueue $command_queue,
    Interop\Polite\Math\LinearBuffer $patternBuffer  // host buffer assigned
    int      $size=0,
    int      $offset=0,
    int      $pattern_size=0,
    int      $pattern_offset=0,
    EventList $events=null,
    EventList $event_wait_list=null
) : void {{{ */
static PHP_METHOD(Buffer, fill)
{
    zval* command_queue_obj_p=NULL;
    zval* pattern_buffer_obj_p=NULL;
    zend_long size=0;
    zend_long offset=0;
    zend_long pattern_size=0;
    zend_long pattern_offset=0;
    zval* events_obj_p=NULL;
    zval* event_wait_list_obj_p=NULL;
    php_rindow_opencl_buffer_t* intern;
    php_rindow_opencl_command_queue_t* command_queue_obj;
    php_interop_polite_math_matrix_linear_buffer_t* pattern_buffer_obj;
    php_rindow_opencl_event_list_t* event_wait_list_obj;
    char *pattern_ptr=NULL;
    cl_int errcode_ret=0;
    cl_event *event_wait_list=NULL;
    cl_uint num_events_in_wait_list=0;
    cl_event event_ret;
    cl_event *event_p=NULL;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 8)
        Z_PARAM_OBJECT_OF_CLASS(command_queue_obj_p,php_rindow_opencl_command_queue_ce)
        Z_PARAM_OBJECT_EX(pattern_buffer_obj_p,1,0)  // Interop\Polite\Math\Matrix\LinearBuffer
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(size)
        Z_PARAM_LONG(offset)
        Z_PARAM_LONG(pattern_size)
        Z_PARAM_LONG(pattern_offset)
        Z_PARAM_OBJECT_OF_CLASS_EX(events_obj_p,php_rindow_opencl_event_list_ce,1,0)
        Z_PARAM_OBJECT_OF_CLASS_EX(event_wait_list_obj_p,php_rindow_opencl_event_list_ce,1,0)
    ZEND_PARSE_PARAMETERS_END();

    command_queue_obj = Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(command_queue_obj_p);
    pattern_buffer_obj = Z_INTEROP_POLITE_MATH_MATRIX_LINEAR_BUFFER_OBJ_P(pattern_buffer_obj_p);
    if(php_rindow_opencl_assert_host_buffer_type(pattern_buffer_obj, "patternBuffer")) {
        return;
    }
    if(pattern_buffer_obj->data==NULL) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "pattern buffer is not initialized.", CL_INVALID_VALUE);
        return;
    }
    if(pattern_size==0) {
        pattern_size = pattern_buffer_obj->size;
    }
    if(pattern_buffer_obj->size - pattern_offset < pattern_size) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Host buffer is too small.", CL_INVALID_VALUE);
        return;
    }
    pattern_ptr = pattern_buffer_obj->data;
    pattern_ptr += pattern_offset * pattern_buffer_obj->value_size;

    intern = Z_RINDOW_OPENCL_BUFFER_OBJ_P(getThis());

    if(size==0) {
        size = intern->size;
    }

    if(events_obj_p!=NULL && Z_TYPE_P(events_obj_p)==IS_OBJECT) {
        event_p = &event_ret;
    }
    if(event_wait_list_obj_p!=NULL && Z_TYPE_P(event_wait_list_obj_p)==IS_OBJECT) {
        event_wait_list_obj = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(event_wait_list_obj_p);
        num_events_in_wait_list = event_wait_list_obj->num;
        event_wait_list = event_wait_list_obj->events;
    }

    errcode_ret = clEnqueueFillBuffer(
        command_queue_obj->command_queue,
        intern->buffer,
        pattern_ptr,
        (pattern_size*(pattern_buffer_obj->value_size)),
        (size_t)offset,
        (size_t)size,
        num_events_in_wait_list,
        event_wait_list,
        event_p);

    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clEnqueueFillBuffer Error errcode=%d", errcode_ret);
        return;
    }
    intern->dtype = pattern_buffer_obj->dtype;
    intern->value_size = pattern_buffer_obj->value_size;

    // append event to events
    if(php_rindow_opencl_append_event(events_obj_p, event_p)) {
        return;
    }
}
/* }}} */

/* Method Rindow\OpenCL\Buffer::copy(
    CommandQueue $command_queue,
    Rindow\OpenCL\Buffer $src_buffer  // source buffer
    int      $size=0,
    int      $offset=0,
    int      $src_offset=0,
    EventList $events=null,
    EventList $event_wait_list=null
) : void {{{ */
static PHP_METHOD(Buffer, copy)
{
    zval* command_queue_obj_p=NULL;
    zval* src_buffer_obj_p=NULL;
    zend_long size=0;
    zend_long dst_offset=0;
    zend_long src_offset=0;
    zval* events_obj_p=NULL;
    zval* event_wait_list_obj_p=NULL;
    php_rindow_opencl_command_queue_t* command_queue_obj;
    php_rindow_opencl_buffer_t* dst_buffer_obj;
    php_rindow_opencl_buffer_t* src_buffer_obj;
    php_rindow_opencl_event_list_t* event_wait_list_obj;
    cl_int errcode_ret=0;
    cl_event *event_wait_list=NULL;
    cl_uint num_events_in_wait_list=0;
    cl_event event_ret;
    cl_event *event_p=NULL;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 2, 7)
        Z_PARAM_OBJECT_OF_CLASS(command_queue_obj_p,php_rindow_opencl_command_queue_ce)
        Z_PARAM_OBJECT_OF_CLASS(src_buffer_obj_p,php_rindow_opencl_buffer_ce)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(size)
        Z_PARAM_LONG(dst_offset)
        Z_PARAM_LONG(src_offset)
        Z_PARAM_OBJECT_OF_CLASS_EX(events_obj_p,php_rindow_opencl_event_list_ce,1,0)
        Z_PARAM_OBJECT_OF_CLASS_EX(event_wait_list_obj_p,php_rindow_opencl_event_list_ce,1,0)
    ZEND_PARSE_PARAMETERS_END();

    command_queue_obj = Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(command_queue_obj_p);
    src_buffer_obj = Z_RINDOW_OPENCL_BUFFER_OBJ_P(src_buffer_obj_p);
    dst_buffer_obj = Z_RINDOW_OPENCL_BUFFER_OBJ_P(getThis());
    if(size==0) {
        size = dst_buffer_obj->size;
    }

    if(events_obj_p!=NULL && Z_TYPE_P(events_obj_p)==IS_OBJECT) {
        event_p = &event_ret;
    }
    if(event_wait_list_obj_p!=NULL && Z_TYPE_P(event_wait_list_obj_p)==IS_OBJECT) {
        event_wait_list_obj = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(event_wait_list_obj_p);
        num_events_in_wait_list = event_wait_list_obj->num;
        event_wait_list = event_wait_list_obj->events;
    }

    errcode_ret = clEnqueueCopyBuffer(
        command_queue_obj->command_queue,
        src_buffer_obj->buffer,
        dst_buffer_obj->buffer,
        (size_t)src_offset,
        (size_t)dst_offset,
        (size_t)size,
        num_events_in_wait_list,
        event_wait_list,
        event_p);

    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clEnqueueWriteBuffer Error errcode=%d", errcode_ret);
        return;
    }
    if(dst_buffer_obj->dtype==0) {
        dst_buffer_obj->dtype = src_buffer_obj->dtype;
        dst_buffer_obj->value_size = src_buffer_obj->value_size;
    }

    // append event to events
    if(php_rindow_opencl_append_event(events_obj_p, event_p)) {
        return;
    }
}
/* }}} */


ZEND_BEGIN_ARG_INFO_EX(ai_Buffer___construct, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, context, Rindow\\OpenCL\\Context, 0)
    ZEND_ARG_INFO(0, size)
    ZEND_ARG_INFO(0, flags)
    ZEND_ARG_OBJ_INFO(0, host_buffer, Interop\\Polite\\Math\\Matrix\\LinearBuffer, 1)
    ZEND_ARG_INFO(0, host_offset)
    ZEND_ARG_INFO(0, dtype)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Buffer_read, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, queue, Rindow\\OpenCL\\CommandQueue, 0)
    ZEND_ARG_OBJ_INFO(0, host_buffer, Interop\\Polite\\Math\\Matrix\\LinearBuffer, 0)
    ZEND_ARG_INFO(0, size)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, host_offset)
    ZEND_ARG_INFO(0, blocking_read)
    ZEND_ARG_OBJ_INFO(0, events, Rindow\\OpenCL\\EventList, 1)
    ZEND_ARG_OBJ_INFO(0, event_wait_list, Rindow\\OpenCL\\EventList, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Buffer_write, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, queue, Rindow\\OpenCL\\CommandQueue, 0)
    ZEND_ARG_OBJ_INFO(0, host_buffer, Interop\\Polite\\Math\\Matrix\\LinearBuffer, 0)
    ZEND_ARG_INFO(0, size)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, host_offset)
    ZEND_ARG_INFO(0, blocking_write)
    ZEND_ARG_OBJ_INFO(0, events, Rindow\\OpenCL\\EventList, 1)
    ZEND_ARG_OBJ_INFO(0, event_wait_list, Rindow\\OpenCL\\EventList, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Buffer_fill, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, queue, Rindow\\OpenCL\\CommandQueue, 0)
    ZEND_ARG_OBJ_INFO(0, pattern_buffer, Interop\\Polite\\Math\\Matrix\\LinearBuffer, 0)
    ZEND_ARG_INFO(0, size)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, pattern_size)
    ZEND_ARG_INFO(0, pattern_offset)
    ZEND_ARG_OBJ_INFO(0, events, Rindow\\OpenCL\\EventList, 1)
    ZEND_ARG_OBJ_INFO(0, event_wait_list, Rindow\\OpenCL\\EventList, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Buffer_copy, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, queue, Rindow\\OpenCL\\CommandQueue, 0)
    ZEND_ARG_OBJ_INFO(0, src_buffer, Rindow\\OpenCL\\Buffer, 0)
    ZEND_ARG_INFO(0, size)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, src_offset)
    ZEND_ARG_OBJ_INFO(0, events, Rindow\\OpenCL\\EventList, 1)
    ZEND_ARG_OBJ_INFO(0, event_wait_list, Rindow\\OpenCL\\EventList, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Buffer_void, 0, 0, 0)
ZEND_END_ARG_INFO()

/* {{{ Rindow\OpenCL\Buffer function entries */
static zend_function_entry php_rindow_opencl_buffer_me[] = {
    /* clang-format off */
    PHP_ME(Buffer, __construct, ai_Buffer___construct, ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, dtype,      ai_Buffer_void,  ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, value_size, ai_Buffer_void,  ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, bytes,      ai_Buffer_void,  ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, read,       ai_Buffer_read,  ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, write,      ai_Buffer_write, ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, fill,       ai_Buffer_fill,  ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, copy,       ai_Buffer_copy,  ZEND_ACC_PUBLIC)
    PHP_FE_END
    /* clang-format on */
};
/* }}} */



/* Class Rindow\OpenCL\Buffer {{{ */
zend_class_entry* php_rindow_opencl_buffer_ce;

void php_rindow_opencl_buffer_init_ce(INIT_FUNC_ARGS)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Rindow\\OpenCL", "Buffer", php_rindow_opencl_buffer_me);
    php_rindow_opencl_buffer_ce = zend_register_internal_class(&ce);
    php_rindow_opencl_buffer_ce->create_object = php_rindow_opencl_buffer_create_object;
    memcpy(&rindow_opencl_buffer_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    rindow_opencl_buffer_object_handlers.offset    = XtOffsetOf(php_rindow_opencl_buffer_t, std);
    rindow_opencl_buffer_object_handlers.free_obj  = php_rindow_opencl_buffer_free_object;
    rindow_opencl_buffer_object_handlers.clone_obj = NULL;

    //zend_class_implements(php_rindow_opencl_buffer_ce, 2, spl_ce_ArrayAccess, spl_ce_Countable);
}
/* }}} */
