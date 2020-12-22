#include <php.h>
#include <Zend/zend_interfaces.h>
#include <Zend/zend_exceptions.h>
#include <ext/spl/spl_iterators.h>
#include <ext/spl/spl_exceptions.h>
#include <stdint.h>
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/opencl.h>
#include "Rindow/OpenCL/EventList.h"
#include "Rindow/OpenCL/Context.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_rindow_opencl.h"

int php_rindow_opencl_event_list_new_event_object(
    zval *val, cl_event *event, cl_uint num_events)
{
    int rc;
    php_rindow_opencl_event_list_t* event_list_ret;
    if(rc=object_init_ex(val, php_rindow_opencl_event_list_ce)) {
        return rc;
    }
    event_list_ret = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(val);
    event_list_ret->num = num_events;
    event_list_ret->events = ecalloc(num_events,sizeof(cl_event));
    if(event_list_ret->events==NULL) {
        return -1;
    }
    memcpy(event_list_ret->events, event, num_events*sizeof(cl_event));
    return 0;
}

static zend_object_handlers rindow_opencl_event_list_object_handlers;

// destractor
static void php_rindow_opencl_event_list_free_object(zend_object* object)
{
    php_rindow_opencl_event_list_t* obj = php_rindow_opencl_event_list_fetch_object(object);
    if(obj->events) {
        for(unsigned int i=0;i<obj->num;i++) {
            cl_int errcode_ret = clReleaseEvent(obj->events[i]);
            if(errcode_ret) {
                php_printf("WARNING: clReleaseEvent error=%d\n",errcode_ret);
            }
        }
        efree(obj->events);
    }
    zend_object_std_dtor(&obj->std);
}

// constructor
static zend_object* php_rindow_opencl_event_list_create_object(zend_class_entry* class_type) /* {{{ */
{
    php_rindow_opencl_event_list_t* intern = NULL;

    intern = (php_rindow_opencl_event_list_t*)ecalloc(1, sizeof(php_rindow_opencl_event_list_t) + zend_object_properties_size(class_type));
    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);
    intern->events = NULL;
    intern->num = 0;
    intern->std.handlers = &rindow_opencl_event_list_object_handlers;

    return &intern->std;
} /* }}} */

/* Method Rindow\OpenCL\EventList::__construct(
    Context $context=null,
) {{{ */
static PHP_METHOD(EventList, __construct)
{
    php_rindow_opencl_event_list_t* intern;
    php_rindow_opencl_context_t* context_obj;
    zval* context_obj_p=NULL;
    cl_event event;
    cl_int errcode_ret=0;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_OBJECT_OF_CLASS_EX(context_obj_p,php_rindow_opencl_context_ce,1,0)
    ZEND_PARSE_PARAMETERS_END();

    if(context_obj_p==NULL||Z_TYPE_P(context_obj_p)!=IS_OBJECT) {
        return;
    }
    context_obj = Z_RINDOW_OPENCL_CONTEXT_OBJ_P(context_obj_p);

    event = clCreateUserEvent(
        context_obj->context,
        &errcode_ret);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clCreateUserEvent Error errcode=%d", errcode_ret);
        return;
    }
    intern = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(getThis());
    intern->num = 1;
    intern->events = ecalloc(1,sizeof(cl_event));
    intern->events[0] = event;
}
/* }}} */

/* Method Rindow\OpenCL\EventList::count(
) : int {{{ */
static PHP_METHOD(EventList, count)
{
    php_rindow_opencl_event_list_t* intern;
    intern = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(getThis());

    RETURN_LONG((zend_long)(intern->num));
}
/* }}} */

/* Method Rindow\OpenCL\EventList::wait(
) : void {{{ */
static PHP_METHOD(EventList, wait)
{
    php_rindow_opencl_event_list_t* intern;
    cl_int errcode_ret=0;

    intern = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(getThis());
    if(intern->events==NULL) {
        zend_throw_exception(spl_ce_RuntimeException, "EventList is not initialized", 0);
        return;
    }
    errcode_ret = clWaitForEvents(intern->num,intern->events);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clWaitForEvents Error errcode=%d", errcode_ret);
        return;
    }
}
/* }}} */

/* Method Rindow\OpenCL\EventList::move(
    EventList $events
) : void {{{ */
static PHP_METHOD(EventList, move)
{
    php_rindow_opencl_event_list_t* intern;
    php_rindow_opencl_event_list_t* events;
    zval *event_list_obj_p=NULL;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_OBJECT_OF_CLASS(event_list_obj_p,php_rindow_opencl_event_list_ce)
    ZEND_PARSE_PARAMETERS_END();

    events = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(event_list_obj_p);
    if(events->events==NULL) {
        zend_throw_exception(spl_ce_RuntimeException, "EventList is not initialized", 0);
        return;
    }

    intern = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(getThis());
    intern->events = erealloc(intern->events, (intern->num + events->num)*sizeof(cl_event));
    if(events->events==NULL) {
        zend_throw_exception(spl_ce_RuntimeException, "EventList reallocate error", 0);
        return;
    }
    memcpy(&(intern->events[intern->num]),events->events,events->num*sizeof(cl_event));
    intern->num += events->num;
    events->num = 0;
    efree(events->events);
    events->events = NULL;
}
/* }}} */

/* Method Rindow\OpenCL\EventList::copy(
    EventList $events
) : void {{{ */
static PHP_METHOD(EventList, copy)
{
    php_rindow_opencl_event_list_t* intern;
    php_rindow_opencl_event_list_t* events;
    zval *event_list_obj_p=NULL;
    cl_int errcode_ret;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 1)
        Z_PARAM_OBJECT_OF_CLASS(event_list_obj_p,php_rindow_opencl_event_list_ce)
    ZEND_PARSE_PARAMETERS_END();

    events = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(event_list_obj_p);
    if(events->events==NULL) {
        zend_throw_exception(spl_ce_RuntimeException, "EventList is not initialized", 0);
        return;
    }

    intern = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(getThis());
    intern->events = erealloc(intern->events, (intern->num + events->num)*sizeof(cl_event));
    if(events->events==NULL) {
        zend_throw_exception(spl_ce_RuntimeException, "EventList reallocate error", 0);
        return;
    }
    memcpy(&(intern->events[intern->num]),events->events,events->num*sizeof(cl_event));
    intern->num += events->num;
    for(unsigned int i=0; i<events->num; i++) {
        errcode_ret = clRetainEvent(events->events[i]);
        if(errcode_ret!=CL_SUCCESS) {
            zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clRetainEvent Error errcode=%d", errcode_ret);
            return;
        }
    }
}
/* }}} */

/* Method Rindow\OpenCL\EventList::setStatus(
    int $status,
    int $index=0
) {{{ */
static PHP_METHOD(EventList, setStatus)
{
    php_rindow_opencl_event_list_t* intern;
    zend_long execution_status;
    zend_long index=0;
    cl_int errcode_ret=0;

    ZEND_PARSE_PARAMETERS_START_EX(ZEND_PARSE_PARAMS_THROW, 1, 2)
        Z_PARAM_LONG(execution_status)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(index)
    ZEND_PARSE_PARAMETERS_END();

    intern = Z_RINDOW_OPENCL_EVENT_LIST_OBJ_P(getThis());
    if(index<0 || index>=intern->num) {
        zend_throw_exception(spl_ce_OutOfRangeException, "event index is out of range", 0);
        return;
    }

    errcode_ret = clSetUserEventStatus(
        intern->events[index],
        (cl_int)execution_status);
    if(errcode_ret!=CL_SUCCESS) {
        zend_throw_exception_ex(spl_ce_RuntimeException, errcode_ret, "clSetUserEventStatus Error errcode=%d", errcode_ret);
        return;
    }
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(ai_EventList___construct, 0, 0, 0)
    ZEND_ARG_OBJ_INFO(0, context, Rindow\\OpenCL\\Context, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_EventList_move, 0, 0, 1)
    ZEND_ARG_OBJ_INFO(0, context, Rindow\\OpenCL\\EventList, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_EventList_copy, 0, 0, 1)
    ZEND_ARG_OBJ_INFO(0, context, Rindow\\OpenCL\\EventList, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_EventList_setStatus, 0, 0, 1)
    ZEND_ARG_INFO(0, execution_status)
    ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(ai_EventList_void, 0, 0, 0)
ZEND_END_ARG_INFO()

/* {{{ Rindow\OpenCL\EventList function entries */
static zend_function_entry php_rindow_opencl_event_list_me[] = {
    /* clang-format off */
    PHP_ME(EventList, __construct, ai_EventList___construct, ZEND_ACC_PUBLIC)
    PHP_ME(EventList, count,       ai_EventList_void,        ZEND_ACC_PUBLIC)
    PHP_ME(EventList, wait,        ai_EventList_void,        ZEND_ACC_PUBLIC)
    PHP_ME(EventList, move,        ai_EventList_move,        ZEND_ACC_PUBLIC)
    PHP_ME(EventList, copy,        ai_EventList_copy,        ZEND_ACC_PUBLIC)
    PHP_ME(EventList, setStatus,   ai_EventList_setStatus,   ZEND_ACC_PUBLIC)
    PHP_FE_END
    /* clang-format on */
};
/* }}} */

/* Class Rindow\OpenCL\EventList {{{ */
zend_class_entry* php_rindow_opencl_event_list_ce;

void php_rindow_opencl_event_list_init_ce(INIT_FUNC_ARGS)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Rindow\\OpenCL", "EventList", php_rindow_opencl_event_list_me);
    php_rindow_opencl_event_list_ce = zend_register_internal_class(&ce);
    php_rindow_opencl_event_list_ce->create_object = php_rindow_opencl_event_list_create_object;
    memcpy(&rindow_opencl_event_list_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    rindow_opencl_event_list_object_handlers.offset    = XtOffsetOf(php_rindow_opencl_event_list_t, std);
    rindow_opencl_event_list_object_handlers.free_obj  = php_rindow_opencl_event_list_free_object;
    rindow_opencl_event_list_object_handlers.clone_obj = NULL;

    //zend_class_implements(php_rindow_opencl_event_list_ce, 2, spl_ce_ArrayAccess, spl_ce_Countable);
    zend_class_implements(php_rindow_opencl_event_list_ce, 1, spl_ce_Countable);
}
/* }}} */
