#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(sipXportLib, 2.9, sipxtapi@googlegroups.com)
AC_CONFIG_AUX_DIR(config)
AC_PREFIX_DEFAULT([/usr/local/sipx])
AM_INIT_AUTOMAKE(foreign)

# Check minimum for automake version
SFAC_AUTOMAKE_VERSION([1.6])

# Check that we don't have buggy 2.58 autoconf
CHECK_AUTOCONF

AC_CONFIG_SRCDIR([src/os/OsConfigDb.cpp])
AM_CONFIG_HEADER([config.h])
AC_PROG_LIBTOOL

# Checks for compilers and preprocessors
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
# librt is not needed
#CHECK_LIBRT

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h malloc.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

# Check for CPPUnit
CHECK_CPPUNIT

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UID_T
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([clock_gettime getcwd gethostbyname gethostname gettimeofday inet_ntoa localtime_r memchr memset mkdir putenv rmdir select setenv socket strchr strdup strerror strrchr strstr strtol strtoul tzset])

# Check for additional libraries
CHECK_PCRE
CHECK_SSL

# Define additional compiler flags for pingtel code
SFAC_INIT_FLAGS

# Adds option for building TLS version
SFAC_FEATURE_SIP_TLS

# Adds options for doxygen
ENABLE_DOXYGEN
AM_CONDITIONAL(DOC, test x$enable_doc = xyes)
AM_CONDITIONAL(USE_BLDNO, test x$enable_buildnumber = xyes)

# Add options for bakefile
AC_ARG_ENABLE(debug,
              [  --enable-debug          Enable debugging information],
              USE_DEBUG="$enableval", USE_DEBUG="no")

if test $USE_DEBUG = yes ; then 
  DEBUG=1
  CFLAGS="$CFLAGS -g"
else
  DEBUG=0
fi

AC_BAKEFILE([m4_include(autoconf_inc.m4)])

AC_CONFIG_FILES([
  Makefile
  config/sipXcommon.mak
  doc/Makefile
])

AC_OUTPUT
