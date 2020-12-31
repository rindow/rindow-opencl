#ifndef PHP_RINDOW_OPENCL_PROGRAM_H
# define PHP_RINDOW_OPENCL_PROGRAM_H

#define PHP_RINDOW_OPENCL_PROGRAM_CLASSNAME "Rindow\\OpenCL\\Program"
#define PHP_RINDOW_OPENCL_PROGRAM_SIGNATURE 0x67504c4377646e52 // "RndwCLPg"

typedef struct {
    zend_long signature;
    cl_program program;
    zend_object std;
} php_rindow_opencl_program_t;
static inline php_rindow_opencl_program_t* php_rindow_opencl_program_fetch_object(zend_object* obj)
{
	return (php_rindow_opencl_program_t*) ((char*) obj - XtOffsetOf(php_rindow_opencl_program_t, std));
}
#define Z_RINDOW_OPENCL_PROGRAM_OBJ_P(zv) (php_rindow_opencl_program_fetch_object(Z_OBJ_P(zv)))

#endif	/* PHP_RINDOW_OPENCL_PROGRAM_H */
