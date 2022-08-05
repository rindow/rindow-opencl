dnl config.m4 for extension rindow_opencl

PHP_ARG_ENABLE(rindow_opencl, whether to enable rindow_opencl support,
dnl Make sure that the comment is aligned:
[  --enable-rindow_opencl          Enable rindow_opencl support], no)

if test "$PHP_RINDOW_OPENCL" != "no"; then

  dnl # get library OpenCL build options from pkg-config output
  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  AC_MSG_CHECKING(for opencl)
  if test -x "$PKG_CONFIG"; then
    if $PKG_CONFIG --exists OpenCL; then
      if $PKG_CONFIG OpenCL --atleast-version 1.2.0; then
        LIBBLAS_CFLAGS="`$PKG_CONFIG OpenCL --cflags`"
        LIBBLAS_LIBDIR="`$PKG_CONFIG OpenCL --libs`"
        LIBBLAS_VERSON=`$PKG_CONFIG OpenCL --modversion`
        AC_MSG_RESULT(from pkgconfig: version $LIBBLAS_VERSON)
      else
        AC_MSG_ERROR(system OpenCL is too old: version 1.2.0 required)
      fi
    else
      AC_MSG_ERROR(OpenCL not found)
    fi
  else
    AC_MSG_ERROR(pkg-config not found)
  fi
  AC_DEFINE(CL_TARGET_OPENCL_VERSION, 120, [ Target OpenCL version 1.2 ])
  PHP_EVAL_LIBLINE($LIBBLAS_LIBDIR, RINDOW_OPENCL_SHARED_LIBADD)
  PHP_EVAL_INCLINE($LIBBLAS_CFLAGS)

  dnl # PHP_ADD_INCLUDE($RINDOW_OPENCL_DIR/include)
  AC_MSG_CHECKING(for Interop/Polite/Math/Matrix.h)
  if test -f "PHP_EXT_SRCDIR(rindow_opencl)/vendor/interop-phpobjects/polite-math/include/Interop/Polite/Math/Matrix.h" ; then
    AC_MSG_RESULT(ok)
    PHP_ADD_INCLUDE(PHP_EXT_SRCDIR(rindow_opencl)[/vendor/interop-phpobjects/polite-math/include])
  else
    AC_MSG_RESULT(no)
    AC_MSG_ERROR(Interop/Polite/Math/Matrix.h not found. Please type "composer update")
  fi

  PHP_SUBST(RINDOW_OPENCL_SHARED_LIBADD)

  RINDOW_OPENCL_SOURCES="\
     rindow_opencl.c \
     src/Rindow/OpenCL/Buffer.c \
     src/Rindow/OpenCL/CommandQueue.c \
     src/Rindow/OpenCL/Context.c \
     src/Rindow/OpenCL/DeviceList.c \
     src/Rindow/OpenCL/EventList.c \
     src/Rindow/OpenCL/Kernel.c \
     src/Rindow/OpenCL/PlatformList.c \
     src/Rindow/OpenCL/Program.c \
  "

  PHP_NEW_EXTENSION(rindow_opencl, $RINDOW_OPENCL_SOURCES, $ext_shared)
fi
