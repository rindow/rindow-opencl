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
#include "Rindow/OpenCL/Buffer.h"
#include "Rindow/OpenCL/Context.h"
#include "Rindow/OpenCL/EventList.h"
#include "Rindow/OpenCL/CommandQueue.h"
#include <Interop/Polite/Math/Matrix.h>

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
        Z_PARAM_OBJECT(host_buffer_obj_p)  // Interop\Polite\Math\Matrix\LinearBuffer
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

/* Method Rindow\OpenCL\Buffer::readRect(
    CommandQueue $command_queue,
    Interop\Polite\Math\LinearBuffer $hostBuffer  // host buffer assigned
    array<int> $region, // [width,height,depth],
    int        $host_buffer_offset=0,
    array<int> $buffer_origin=[0,0,0],
    array<int> $host_origin=[0,0,0],
    bool     $blocking_read=true,
    EventList $events=null,
    EventList $event_wait_list=null
) : void {{{ */
static PHP_METHOD(Buffer, readRect)
{
    zval* command_queue_obj_p=NULL;
    zval* host_buffer_obj_p=NULL;
    zval* region_obj_p=NULL;
    zend_long host_buffer_offset=0;
    zval* buffer_offset_obj_p=NULL;
    zval* host_offset_obj_p=NULL;
    zend_long buffer_row_pitch=0;
    zend_long buffer_slice_pitch=0;
    zend_long host_row_pitch=0;
    zend_long host_slice_pitch=0;
    zend_bool blocking_read=TRUE;
    zend_bool blocking_read_is_null=TRUE;
    zval* events_obj_p=NULL;
    zval* event_wait_list_obj_p=NULL;
    php_rindow_opencl_buffer_t* intern;
    php_rindow_opencl_command_queue_t* command_queue_obj;
    php_interop_polite_math_matrix_linear_buffer_t* host_buffer_obj;
    size_t* region;
    size_t* host_offsets;
    size_t* buffer_offsets;
    php_rindow_opencl_event_list_t* event_wait_list_obj;
    char *host_ptr=NULL;
    cl_event *event_wait_list=NULL;
    cl_uint num_events_in_wait_list=0;
    cl_event event_ret;
    cl_event *event_p=NULL;
    cl_int errcode_ret=0;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 13)
        Z_PARAM_OBJECT_OF_CLASS(command_queue_obj_p,php_rindow_opencl_command_queue_ce)
        Z_PARAM_OBJECT(host_buffer_obj_p)  // Interop\Polite\Math\Matrix\LinearBuffer
        Z_PARAM_ARRAY(region_obj_p)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(host_buffer_offset)
        Z_PARAM_ARRAY_EX(buffer_offset_obj_p,1,0)
        Z_PARAM_ARRAY_EX(host_offset_obj_p,1,0)
        Z_PARAM_LONG(buffer_row_pitch)
        Z_PARAM_LONG(buffer_slice_pitch)
        Z_PARAM_LONG(host_row_pitch)
        Z_PARAM_LONG(host_slice_pitch)
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

    {
        cl_uint tmp_dim = 3;
        region = php_rindow_opencl_array_to_integers(
            region_obj_p, &tmp_dim,
            php_rindow_opencl_array_to_integers_constraint_greater_zero,
            &errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            zend_throw_exception_ex(spl_ce_InvalidArgumentException, errcode_ret, "Invalid region size. errcode=%d", errcode_ret);
            return;
        }
        for(int i=0;i<3;i++) {
            if(region[i]==0) {
                region[i] = 1;
            }
        }
    }

    if(buffer_offset_obj_p!=NULL && Z_TYPE_P(buffer_offset_obj_p)==IS_ARRAY) {
        cl_uint tmp_dim = 3;
        buffer_offsets = php_rindow_opencl_array_to_integers(
            buffer_offset_obj_p, &tmp_dim,
            php_rindow_opencl_array_to_integers_constraint_greater_or_equal_zero,
            &errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            efree(region);
            zend_throw_exception_ex(spl_ce_InvalidArgumentException, errcode_ret, "Invalid buffer_offsets. errcode=%d", errcode_ret);
            return;
        }
    } else {
        buffer_offsets = ecalloc(3,sizeof(size_t));
        memset(buffer_offsets,0,3*sizeof(size_t));
    }

    if(host_offset_obj_p!=NULL && Z_TYPE_P(host_offset_obj_p)==IS_ARRAY) {
        cl_uint tmp_dim = 3;
        host_offsets = php_rindow_opencl_array_to_integers(
            host_offset_obj_p, &tmp_dim,
            php_rindow_opencl_array_to_integers_constraint_greater_or_equal_zero,
            &errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            efree(region);
            efree(buffer_offsets);
            zend_throw_exception_ex(spl_ce_InvalidArgumentException, errcode_ret, "Invalid host_offsets. errcode=%d", errcode_ret);
            return;
        }
    } else {
        host_offsets = ecalloc(3,sizeof(size_t));
        memset(host_offsets,0,3*sizeof(size_t));
    }

    if(buffer_row_pitch<0) {
        efree(region);
        efree(buffer_offsets);
        efree(host_offsets);
        zend_throw_exception(spl_ce_InvalidArgumentException, "buffer_row_pitch must be greater then or equal zero.", CL_INVALID_VALUE);
        return;
    } else if(buffer_row_pitch==0) {
        buffer_row_pitch = region[0];
    }

    if(buffer_slice_pitch<0) {
        efree(region);
        efree(buffer_offsets);
        efree(host_offsets);
        zend_throw_exception(spl_ce_InvalidArgumentException, "buffer_slice_pitch must be greater then or equal zero.", CL_INVALID_VALUE);
        return;
    } else if(buffer_slice_pitch==0) {
        buffer_slice_pitch = region[1]*buffer_row_pitch;
    }

    if(host_row_pitch<0) {
        efree(region);
        efree(buffer_offsets);
        efree(host_offsets);
        zend_throw_exception(spl_ce_InvalidArgumentException, "host_row_pitch must be greater then or equal zero.", CL_INVALID_VALUE);
        return;
    } else if(host_row_pitch==0) {
        host_row_pitch = region[0];
    }

    if(host_slice_pitch<0) {
        efree(region);
        efree(buffer_offsets);
        efree(host_offsets);
        zend_throw_exception(spl_ce_InvalidArgumentException, "host_slice_pitch must be greater then or equal zero.", CL_INVALID_VALUE);
        return;
    } else if(host_slice_pitch==0) {
        host_slice_pitch = region[1]*host_row_pitch;
    }

    {
        zend_long pos_max
            = (host_offsets[2]+region[2]-1)*host_slice_pitch
            + (host_offsets[1]+region[1]-1)*host_row_pitch
            + (host_offsets[0]+region[0]-1);
        if(pos_max >= (((zend_long)(host_buffer_obj->size) - host_buffer_offset) * host_buffer_obj->value_size)) {
            efree(region);
            efree(buffer_offsets);
            efree(host_offsets);
            zend_throw_exception(spl_ce_InvalidArgumentException, "Host buffer is too small.", CL_INVALID_VALUE);
            return;
        }
    }
    host_ptr = host_buffer_obj->data;
    host_ptr += host_buffer_offset * host_buffer_obj->value_size;

    intern = Z_RINDOW_OPENCL_BUFFER_OBJ_P(getThis());
    {
        zend_long pos_max
            = (buffer_offsets[2]+region[2]-1)*buffer_slice_pitch
            + (buffer_offsets[1]+region[1]-1)*buffer_row_pitch
            + (buffer_offsets[0]+region[0]-1);
        if(pos_max >= (zend_long)(intern->size)) {
            efree(region);
            efree(buffer_offsets);
            efree(host_offsets);
            zend_throw_exception(spl_ce_InvalidArgumentException, "buffer is too small.", CL_INVALID_VALUE);
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

    errcode_ret = clEnqueueReadBufferRect(
        command_queue_obj->command_queue,
        intern->buffer,
        (cl_bool)blocking_read,
        buffer_offsets,
        host_offsets,
        region,
        (size_t)buffer_row_pitch,
        (size_t)buffer_slice_pitch,
        (size_t)host_row_pitch,
        (size_t)host_slice_pitch,
        host_ptr,
        num_events_in_wait_list,
        event_wait_list,
        event_p);

    if(errcode_ret!=CL_SUCCESS) {
        efree(region);
        efree(buffer_offsets);
        efree(host_offsets);
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clEnqueueReadBufferRect Error errcode=%d", errcode_ret);
        return;
    }

    // append event to events
    if(php_rindow_opencl_append_event(events_obj_p, event_p)) {
        efree(region);
        efree(buffer_offsets);
        efree(host_offsets);
        return;
    }

    efree(region);
    efree(buffer_offsets);
    efree(host_offsets);
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
        Z_PARAM_OBJECT(host_buffer_obj_p)  // Interop\Polite\Math\Matrix\LinearBuffer
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

/* Method Rindow\OpenCL\Buffer::writeRect(
    CommandQueue $command_queue,
    Interop\Polite\Math\LinearBuffer $hostBuffer  // host buffer assigned
    array<int> $region, // [width,height,depth],
    int        $host_buffer_offset=0,
    array<int> $buffer_origin=[0,0,0],
    array<int> $host_origin=[0,0,0],
    bool     $blocking_write=true,
    EventList $events=null,
    EventList $event_wait_list=null
) : void {{{ */
static PHP_METHOD(Buffer, writeRect)
{
    zval* command_queue_obj_p=NULL;
    zval* host_buffer_obj_p=NULL;
    zval* region_obj_p=NULL;
    zend_long host_buffer_offset=0;
    zval* buffer_offset_obj_p=NULL;
    zval* host_offset_obj_p=NULL;
    zend_long buffer_row_pitch=0;
    zend_long buffer_slice_pitch=0;
    zend_long host_row_pitch=0;
    zend_long host_slice_pitch=0;
    zend_bool blocking_write=TRUE;
    zend_bool blocking_write_is_null=TRUE;
    zval* events_obj_p=NULL;
    zval* event_wait_list_obj_p=NULL;
    php_rindow_opencl_buffer_t* intern;
    php_rindow_opencl_command_queue_t* command_queue_obj;
    php_interop_polite_math_matrix_linear_buffer_t* host_buffer_obj;
    size_t* region;
    size_t* host_offsets;
    size_t* buffer_offsets;
    php_rindow_opencl_event_list_t* event_wait_list_obj;
    char *host_ptr=NULL;
    cl_event *event_wait_list=NULL;
    cl_uint num_events_in_wait_list=0;
    cl_event event_ret;
    cl_event *event_p=NULL;
    cl_int errcode_ret=0;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 13)
        Z_PARAM_OBJECT_OF_CLASS(command_queue_obj_p,php_rindow_opencl_command_queue_ce)
        Z_PARAM_OBJECT(host_buffer_obj_p)  // Interop\Polite\Math\Matrix\LinearBuffer
        Z_PARAM_ARRAY(region_obj_p)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(host_buffer_offset)
        Z_PARAM_ARRAY_EX(buffer_offset_obj_p,1,0)
        Z_PARAM_ARRAY_EX(host_offset_obj_p,1,0)
        Z_PARAM_LONG(buffer_row_pitch)
        Z_PARAM_LONG(buffer_slice_pitch)
        Z_PARAM_LONG(host_row_pitch)
        Z_PARAM_LONG(host_slice_pitch)
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

    {
        cl_uint tmp_dim = 3;
        region = php_rindow_opencl_array_to_integers(
            region_obj_p, &tmp_dim,
            php_rindow_opencl_array_to_integers_constraint_greater_zero,
            &errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            zend_throw_exception_ex(spl_ce_InvalidArgumentException, errcode_ret, "Invalid region size. errcode=%d", errcode_ret);
            return;
        }
        for(int i=0;i<3;i++) {
            if(region[i]==0) {
                region[i] = 1;
            }
        }
    }

    if(buffer_offset_obj_p!=NULL && Z_TYPE_P(buffer_offset_obj_p)==IS_ARRAY) {
        cl_uint tmp_dim = 3;
        buffer_offsets = php_rindow_opencl_array_to_integers(
            buffer_offset_obj_p, &tmp_dim,
            php_rindow_opencl_array_to_integers_constraint_greater_or_equal_zero,
            &errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            efree(region);
            zend_throw_exception_ex(spl_ce_InvalidArgumentException, errcode_ret, "Invalid buffer_offsets. errcode=%d", errcode_ret);
            return;
        }
    } else {
        buffer_offsets = ecalloc(3,sizeof(size_t));
        memset(buffer_offsets,0,3*sizeof(size_t));
    }

    if(host_offset_obj_p!=NULL && Z_TYPE_P(host_offset_obj_p)==IS_ARRAY) {
        cl_uint tmp_dim = 3;
        host_offsets = php_rindow_opencl_array_to_integers(
            host_offset_obj_p, &tmp_dim,
            php_rindow_opencl_array_to_integers_constraint_greater_or_equal_zero,
            &errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            efree(region);
            efree(buffer_offsets);
            zend_throw_exception_ex(spl_ce_InvalidArgumentException, errcode_ret, "Invalid host_offsets. errcode=%d", errcode_ret);
            return;
        }
    } else {
        host_offsets = ecalloc(3,sizeof(size_t));
        memset(host_offsets,0,3*sizeof(size_t));
    }

    if(buffer_row_pitch<0) {
        efree(region);
        efree(buffer_offsets);
        efree(host_offsets);
        zend_throw_exception(spl_ce_InvalidArgumentException, "buffer_row_pitch must be greater then or equal zero.", CL_INVALID_VALUE);
        return;
    } else if(buffer_row_pitch==0) {
        buffer_row_pitch = region[0];
    }

    if(buffer_slice_pitch<0) {
        efree(region);
        efree(buffer_offsets);
        efree(host_offsets);
        zend_throw_exception(spl_ce_InvalidArgumentException, "buffer_slice_pitch must be greater then or equal zero.", CL_INVALID_VALUE);
        return;
    } else if(buffer_slice_pitch==0) {
        buffer_slice_pitch = region[1]*buffer_row_pitch;
    }

    if(host_row_pitch<0) {
        efree(region);
        efree(buffer_offsets);
        efree(host_offsets);
        zend_throw_exception(spl_ce_InvalidArgumentException, "host_row_pitch must be greater then or equal zero.", CL_INVALID_VALUE);
        return;
    } else if(host_row_pitch==0) {
        host_row_pitch = region[0];
    }

    if(host_slice_pitch<0) {
        efree(region);
        efree(buffer_offsets);
        efree(host_offsets);
        zend_throw_exception(spl_ce_InvalidArgumentException, "host_slice_pitch must be greater then or equal zero.", CL_INVALID_VALUE);
        return;
    } else if(host_slice_pitch==0) {
        host_slice_pitch = region[1]*host_row_pitch;
    }

    {
        zend_long pos_max
            = (host_offsets[2]+region[2]-1)*host_slice_pitch
            + (host_offsets[1]+region[1]-1)*host_row_pitch
            + (host_offsets[0]+region[0]-1);
        if(pos_max >= (((zend_long)(host_buffer_obj->size) - host_buffer_offset) * host_buffer_obj->value_size)) {
            efree(region);
            efree(buffer_offsets);
            efree(host_offsets);
            zend_throw_exception(spl_ce_InvalidArgumentException, "Host buffer is too small.", CL_INVALID_VALUE);
            return;
        }
    }
    host_ptr = host_buffer_obj->data;
    host_ptr += host_buffer_offset * host_buffer_obj->value_size;

    intern = Z_RINDOW_OPENCL_BUFFER_OBJ_P(getThis());
    {
        zend_long pos_max
            = (buffer_offsets[2]+region[2]-1)*buffer_slice_pitch
            + (buffer_offsets[1]+region[1]-1)*buffer_row_pitch
            + (buffer_offsets[0]+region[0]-1);
        if(pos_max >= (zend_long)(intern->size)) {
            efree(region);
            efree(buffer_offsets);
            efree(host_offsets);
            zend_throw_exception(spl_ce_InvalidArgumentException, "buffer is too small.", CL_INVALID_VALUE);
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

    errcode_ret = clEnqueueWriteBufferRect(
        command_queue_obj->command_queue,
        intern->buffer,
        (cl_bool)blocking_write,
        buffer_offsets,
        host_offsets,
        region,
        (size_t)buffer_row_pitch,
        (size_t)buffer_slice_pitch,
        (size_t)host_row_pitch,
        (size_t)host_slice_pitch,
        host_ptr,
        num_events_in_wait_list,
        event_wait_list,
        event_p);

    if(errcode_ret!=CL_SUCCESS) {
        efree(region);
        efree(buffer_offsets);
        efree(host_offsets);
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clEnqueueReadBufferRect Error errcode=%d", errcode_ret);
        return;
    }

    // append event to events
    if(php_rindow_opencl_append_event(events_obj_p, event_p)) {
        efree(region);
        efree(buffer_offsets);
        efree(host_offsets);
        return;
    }

    efree(region);
    efree(buffer_offsets);
    efree(host_offsets);
}
/* }}} */

#ifdef CL_VERSION_1_2
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
        Z_PARAM_OBJECT(pattern_buffer_obj_p)  // Interop\Polite\Math\Matrix\LinearBuffer
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

    //if(1) {
    //    zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, 
    //        "debug=%d,offset=%d,size=%d,pattern_size=%d,pattern_value_size=%d",
    //        3,(int)offset,(int)size,(int)pattern_size,(int)(pattern_buffer_obj->value_size));
    //    return;
    //}
    //php_printf("debug=%d,offset=%d,size=%d,pattern_size=%d,pattern_value_size=%d,sizeof(cl_float)=%d\n",
    //        3,(int)offset,(int)size,(int)pattern_size,(int)(pattern_buffer_obj->value_size),(int)sizeof(cl_float));
    //if(num_events_in_wait_list!=0 || event_wait_list!=NULL) {
    //    zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, 
    //    "event_wait_list is not null");
    //    return;
    //}
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
#endif

/* Method Rindow\OpenCL\Buffer::copy(
    CommandQueue $command_queue,
    Rindow\OpenCL\Buffer $src_buffer  // source buffer
    int      $size=0,
    int      $src_offset=0,
    int      $dst_offset=0,
    EventList $events=null,
    EventList $event_wait_list=null
) : void {{{ */
static PHP_METHOD(Buffer, copy)
{
    zval* command_queue_obj_p=NULL;
    zval* src_buffer_obj_p=NULL;
    zend_long size=0;
    zend_long src_offset=0;
    zend_long dst_offset=0;
    zval* events_obj_p=NULL;
    zval* event_wait_list_obj_p=NULL;
    php_rindow_opencl_command_queue_t* command_queue_obj;
    php_rindow_opencl_buffer_t* src_buffer_obj;
    php_rindow_opencl_buffer_t* dst_buffer_obj;
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
        Z_PARAM_LONG(src_offset)
        Z_PARAM_LONG(dst_offset)
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

/* Method Rindow\OpenCL\Buffer::copyRect(
    CommandQueue $command_queue,
    Rindow\OpenCL\Buffer $src_buffer  // source buffer
    array<int> $region, // [width,height,depth],
    int        $host_buffer_offset=0,
    array<int> $buffer_origin=[0,0,0],
    array<int> $host_origin=[0,0,0],
    bool     $blocking_write=true,
    EventList $events=null,
    EventList $event_wait_list=null
) : void {{{ */
static PHP_METHOD(Buffer, copyRect)
{
    zval* command_queue_obj_p=NULL;
    zval* src_buffer_obj_p=NULL;
    zval* region_obj_p=NULL;
    zval* src_origin_obj_p=NULL;
    zval* dst_origin_obj_p=NULL;
    zend_long src_row_pitch=0;
    zend_long src_slice_pitch=0;
    zend_long dst_row_pitch=0;
    zend_long dst_slice_pitch=0;
    zval* events_obj_p=NULL;
    zval* event_wait_list_obj_p=NULL;
    php_rindow_opencl_command_queue_t* command_queue_obj;
    php_rindow_opencl_buffer_t* src_buffer_obj;
    php_rindow_opencl_buffer_t* dst_buffer_obj;
    size_t* region;
    size_t* src_origins;
    size_t* dst_origins;
    php_rindow_opencl_event_list_t* event_wait_list_obj;
    cl_event *event_wait_list=NULL;
    cl_uint num_events_in_wait_list=0;
    cl_event event_ret;
    cl_event *event_p=NULL;
    cl_int errcode_ret=0;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 3, 11)
        Z_PARAM_OBJECT_OF_CLASS(command_queue_obj_p,php_rindow_opencl_command_queue_ce)
        Z_PARAM_OBJECT_OF_CLASS(src_buffer_obj_p,php_rindow_opencl_buffer_ce)
        Z_PARAM_ARRAY(region_obj_p)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_EX(src_origin_obj_p,1,0)
        Z_PARAM_ARRAY_EX(dst_origin_obj_p,1,0)
        Z_PARAM_LONG(src_row_pitch)
        Z_PARAM_LONG(src_slice_pitch)
        Z_PARAM_LONG(dst_row_pitch)
        Z_PARAM_LONG(dst_slice_pitch)
        Z_PARAM_OBJECT_OF_CLASS_EX(events_obj_p,php_rindow_opencl_event_list_ce,1,0)
        Z_PARAM_OBJECT_OF_CLASS_EX(event_wait_list_obj_p,php_rindow_opencl_event_list_ce,1,0)
    ZEND_PARSE_PARAMETERS_END();

    command_queue_obj = Z_RINDOW_OPENCL_COMMAND_QUEUE_OBJ_P(command_queue_obj_p);

    src_buffer_obj = Z_RINDOW_OPENCL_BUFFER_OBJ_P(src_buffer_obj_p);
    dst_buffer_obj = Z_RINDOW_OPENCL_BUFFER_OBJ_P(getThis());
    if(src_buffer_obj->buffer==NULL) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Source buffer is not initialized.", CL_INVALID_VALUE);
        return;
    }
    if(dst_buffer_obj->buffer==NULL) {
        zend_throw_exception(spl_ce_InvalidArgumentException, "Destination buffer is not initialized.", CL_INVALID_VALUE);
        return;
    }

    {
        cl_uint tmp_dim = 3;
        region = php_rindow_opencl_array_to_integers(
            region_obj_p, &tmp_dim,
            php_rindow_opencl_array_to_integers_constraint_greater_zero,
            &errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            zend_throw_exception_ex(spl_ce_InvalidArgumentException, errcode_ret, "Invalid region size. errcode=%d", errcode_ret);
            return;
        }
        for(int i=0;i<3;i++) {
            if(region[i]==0) {
                region[i] = 1;
            }
        }
    }

    if(src_origin_obj_p!=NULL && Z_TYPE_P(src_origin_obj_p)==IS_ARRAY) {
        cl_uint tmp_dim = 3;
        src_origins = php_rindow_opencl_array_to_integers(
            src_origin_obj_p, &tmp_dim,
            php_rindow_opencl_array_to_integers_constraint_greater_or_equal_zero,
            &errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            efree(region);
            zend_throw_exception_ex(spl_ce_InvalidArgumentException, errcode_ret, "Invalid source origin. errcode=%d", errcode_ret);
            return;
        }
    } else {
        src_origins = ecalloc(3,sizeof(size_t));
        memset(src_origins,0,3*sizeof(size_t));
    }

    if(dst_origin_obj_p!=NULL && Z_TYPE_P(dst_origin_obj_p)==IS_ARRAY) {
        cl_uint tmp_dim = 3;
        dst_origins = php_rindow_opencl_array_to_integers(
            dst_origin_obj_p, &tmp_dim,
            php_rindow_opencl_array_to_integers_constraint_greater_or_equal_zero,
            &errcode_ret
        );
        if(errcode_ret!=CL_SUCCESS) {
            efree(region);
            efree(src_origins);
            zend_throw_exception_ex(spl_ce_InvalidArgumentException, errcode_ret, "Invalid destination origin. errcode=%d", errcode_ret);
            return;
        }
    } else {
        dst_origins = ecalloc(3,sizeof(size_t));
        memset(dst_origins,0,3*sizeof(size_t));
    }

    if(src_row_pitch<0) {
        efree(region);
        efree(src_origins);
        efree(dst_origins);
        zend_throw_exception(spl_ce_InvalidArgumentException, "src_row_pitch must be greater then or equal zero.", CL_INVALID_VALUE);
        return;
    } else if(src_row_pitch==0) {
        src_row_pitch = region[0];
    }

    if(src_slice_pitch<0) {
        efree(region);
        efree(src_origins);
        efree(dst_origins);
        zend_throw_exception(spl_ce_InvalidArgumentException, "src_slice_pitch must be greater then or equal zero.", CL_INVALID_VALUE);
        return;
    } else if(src_slice_pitch==0) {
        src_slice_pitch = region[1]*src_row_pitch;
    }

    if(dst_row_pitch<0) {
        efree(region);
        efree(src_origins);
        efree(dst_origins);
        zend_throw_exception(spl_ce_InvalidArgumentException, "dst_row_pitch must be greater then or equal zero.", CL_INVALID_VALUE);
        return;
    } else if(dst_row_pitch==0) {
        dst_row_pitch = region[0];
    }

    if(dst_slice_pitch<0) {
        efree(region);
        efree(src_origins);
        efree(dst_origins);
        zend_throw_exception(spl_ce_InvalidArgumentException, "dst_slice_pitch must be greater then or equal zero.", CL_INVALID_VALUE);
        return;
    } else if(dst_slice_pitch==0) {
        dst_slice_pitch = region[1]*dst_row_pitch;
    }

    {
        zend_long pos_max
            = (src_origins[2]+region[2]-1)*src_slice_pitch
            + (src_origins[1]+region[1]-1)*src_row_pitch
            + (src_origins[0]+region[0]-1);
        if(pos_max >= (zend_long)(src_buffer_obj->size)) {
            efree(region);
            efree(src_origins);
            efree(dst_origins);
            zend_throw_exception(spl_ce_InvalidArgumentException, "Source buffer is too small.", CL_INVALID_VALUE);
            return;
        }
    }

    {
        zend_long pos_max
            = (dst_origins[2]+region[2]-1)*dst_slice_pitch
            + (dst_origins[1]+region[1]-1)*dst_row_pitch
            + (dst_origins[0]+region[0]-1);
        if(pos_max >= (zend_long)(dst_buffer_obj->size)) {
            efree(region);
            efree(src_origins);
            efree(dst_origins);
            zend_throw_exception(spl_ce_InvalidArgumentException, "destination buffer is too small.", CL_INVALID_VALUE);
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

    errcode_ret = clEnqueueCopyBufferRect(
        command_queue_obj->command_queue,
        src_buffer_obj->buffer,
        dst_buffer_obj->buffer,
        src_origins,
        dst_origins,
        region,
        (size_t)src_row_pitch,
        (size_t)src_slice_pitch,
        (size_t)dst_row_pitch,
        (size_t)dst_slice_pitch,
        num_events_in_wait_list,
        event_wait_list,
        event_p);

    if(errcode_ret!=CL_SUCCESS) {
        efree(region);
        efree(src_origins);
        efree(dst_origins);
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clEnqueueCopyBufferRect Error errcode=%d", errcode_ret);
        return;
    }

    // append event to events
    if(php_rindow_opencl_append_event(events_obj_p, event_p)) {
        efree(region);
        efree(src_origins);
        efree(dst_origins);
        return;
    }

    efree(region);
    efree(src_origins);
    efree(dst_origins);
}
/* }}} */

/* Method Rindow\OpenCL\Buffer::getInfo(
    int $param_name
) : string {{{ */
static PHP_METHOD(Buffer, getInfo)
{
    zend_long param_name;
    size_t param_value_size_ret;
    cl_int errcode_ret;
    php_rindow_opencl_buffer_t* intern;
    zend_long result=0;
    cl_uint uint_result;
    cl_ulong ulong_result;
    cl_bool bool_result;
    cl_bitfield bitfield_result;
    size_t size_t_result;

    intern = Z_RINDOW_OPENCL_BUFFER_OBJ_P(getThis());

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_LONG(param_name)
    ZEND_PARSE_PARAMETERS_END();

    errcode_ret = clGetMemObjectInfo(intern->buffer,
                      (cl_mem_info)param_name,
                      0, NULL, &param_value_size_ret);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clGetMemObjectInfo Error errcode=%d", errcode_ret);
        return;
    }
    switch(param_name) {
        case CL_MEM_TYPE:
        case CL_MEM_MAP_COUNT:
        case CL_MEM_REFERENCE_COUNT:
            errcode_ret = clGetMemObjectInfo(intern->buffer,
                        (cl_mem_info)param_name,
                        sizeof(cl_uint), &uint_result, NULL);
            result = (zend_long)uint_result;
            RETURN_LONG(result);
            break;
#ifdef CL_VERSION_2_0
        case CL_MEM_USES_SVM_POINTER:
            errcode_ret = clGetMemObjectInfo(intern->buffer,
                (cl_mem_info)param_name,
                sizeof(cl_bool), &bool_result, NULL);
            result = (zend_long)bool_result;
            RETURN_LONG(result);
            break;
#endif
        case CL_MEM_SIZE:
        case CL_MEM_OFFSET:
            errcode_ret = clGetMemObjectInfo(intern->buffer,
                (cl_mem_info)param_name,
                sizeof(size_t), &size_t_result, NULL);
            result = (zend_long)size_t_result;
            RETURN_LONG(result);
            break;
        case CL_MEM_FLAGS:
            errcode_ret = clGetMemObjectInfo(intern->buffer,
                        (cl_mem_info)param_name,
                        sizeof(cl_bitfield), &bitfield_result, NULL);
            result = (zend_long)bitfield_result;
            RETURN_LONG(result);
            break;
        //case CL_MEM_CONTEXT: {
        //    cl_context context_result;
        //    errcode_ret = clGetMemObjectInfo(intern->buffer,
        //                (cl_mem_info)param_name,
        //                sizeof(cl_context), &context_result, NULL);
        //    // return just context id
        //    zend_long result = (zend_long)context_result;
        //    RETURN_LONG(result);
        //    break;
        //}
        //case CL_MEM_ASSOCIATED_MEMOBJECT: {
        //    cl_mem mem_result;
        //    errcode_ret = clGetMemObjectInfo(intern->buffer,
        //                (cl_mem_info)param_name,
        //                sizeof(cl_mem), &mem_result, NULL);
        //    // return just context id
        //    zend_long result = (zend_long)mem_result;
        //    RETURN_LONG(result);
        //    break;
        //}
        default:
            zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "Unsupported Parameter Name errcode=%d", errcode_ret);
            break;
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


ZEND_BEGIN_ARG_INFO_EX(ai_Buffer_readRect, 0, 0, 3)
    ZEND_ARG_OBJ_INFO(0, queue, Rindow\\OpenCL\\CommandQueue, 0)
    ZEND_ARG_OBJ_INFO(0, host_buffer, Interop\\Polite\\Math\\Matrix\\LinearBuffer, 0)
    ZEND_ARG_INFO(0, region)
    ZEND_ARG_INFO(0, host_buffer_offset)
    ZEND_ARG_INFO(0, buffer_offsets)
    ZEND_ARG_INFO(0, host_offsets)
    ZEND_ARG_INFO(0, buffer_row_pitch)
    ZEND_ARG_INFO(0, buffer_slice_pitch)
    ZEND_ARG_INFO(0, host_row_pitch)
    ZEND_ARG_INFO(0, host_slice_pitch)
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

ZEND_BEGIN_ARG_INFO_EX(ai_Buffer_writeRect, 0, 0, 3)
    ZEND_ARG_OBJ_INFO(0, queue, Rindow\\OpenCL\\CommandQueue, 0)
    ZEND_ARG_OBJ_INFO(0, host_buffer, Interop\\Polite\\Math\\Matrix\\LinearBuffer, 0)
    ZEND_ARG_INFO(0, region)
    ZEND_ARG_INFO(0, host_buffer_offset)
    ZEND_ARG_INFO(0, buffer_offsets)
    ZEND_ARG_INFO(0, host_offsets)
    ZEND_ARG_INFO(0, buffer_row_pitch)
    ZEND_ARG_INFO(0, buffer_slice_pitch)
    ZEND_ARG_INFO(0, host_row_pitch)
    ZEND_ARG_INFO(0, host_slice_pitch)
    ZEND_ARG_INFO(0, blocking_write)
    ZEND_ARG_OBJ_INFO(0, events, Rindow\\OpenCL\\EventList, 1)
    ZEND_ARG_OBJ_INFO(0, event_wait_list, Rindow\\OpenCL\\EventList, 1)
ZEND_END_ARG_INFO()

#ifdef CL_VERSION_1_2
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
#endif

ZEND_BEGIN_ARG_INFO_EX(ai_Buffer_copy, 0, 0, 2)
    ZEND_ARG_OBJ_INFO(0, queue, Rindow\\OpenCL\\CommandQueue, 0)
    ZEND_ARG_OBJ_INFO(0, src_buffer, Rindow\\OpenCL\\Buffer, 0)
    ZEND_ARG_INFO(0, size)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, src_offset)
    ZEND_ARG_OBJ_INFO(0, events, Rindow\\OpenCL\\EventList, 1)
    ZEND_ARG_OBJ_INFO(0, event_wait_list, Rindow\\OpenCL\\EventList, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Buffer_copyRect, 0, 0, 3)
    ZEND_ARG_OBJ_INFO(0, queue, Rindow\\OpenCL\\CommandQueue, 0)
    ZEND_ARG_OBJ_INFO(0, src_buffer, Rindow\\OpenCL\\Buffer, 0)
    ZEND_ARG_INFO(0, region)
    ZEND_ARG_INFO(0, src_origins)
    ZEND_ARG_INFO(0, dst_origins)
    ZEND_ARG_INFO(0, src_row_pitch)
    ZEND_ARG_INFO(0, src_slice_pitch)
    ZEND_ARG_INFO(0, dst_row_pitch)
    ZEND_ARG_INFO(0, dst_slice_pitch)
    ZEND_ARG_OBJ_INFO(0, events, Rindow\\OpenCL\\EventList, 1)
    ZEND_ARG_OBJ_INFO(0, event_wait_list, Rindow\\OpenCL\\EventList, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Buffer_getInfo, 0, 0, 1)
    ZEND_ARG_INFO(0, param_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_Buffer_void, 0, 0, 0)
ZEND_END_ARG_INFO()

/* {{{ Rindow\OpenCL\Buffer function entries */
static zend_function_entry php_rindow_opencl_buffer_me[] = {
    /* clang-format off */
    PHP_ME(Buffer, __construct, ai_Buffer___construct, ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, dtype,      ai_Buffer_void,      ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, value_size, ai_Buffer_void,      ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, bytes,      ai_Buffer_void,      ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, read,       ai_Buffer_read,      ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, readRect,   ai_Buffer_readRect,  ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, write,      ai_Buffer_write,     ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, writeRect,  ai_Buffer_writeRect, ZEND_ACC_PUBLIC)
#ifdef CL_VERSION_1_2
    PHP_ME(Buffer, fill,       ai_Buffer_fill,      ZEND_ACC_PUBLIC)
#endif
    PHP_ME(Buffer, copy,       ai_Buffer_copy,      ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, copyRect,   ai_Buffer_copyRect,  ZEND_ACC_PUBLIC)
    PHP_ME(Buffer, getInfo,    ai_Buffer_getInfo,   ZEND_ACC_PUBLIC)
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
