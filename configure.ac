dnl DESCRIPTION: Process this file with autoconf to produce a configure script.
dnl Note:  /usr/local/share/autoconf/ was editied to have X11R6.3 and .4
dnl to support the cygwin default installation directory
AC_REVISION($Revision$)dnl
AC_INIT
AC_CONFIG_SRCDIR([src/dinotrace.c])
AC_CONFIG_HEADERS([src/config.h])

AC_MSG_RESULT([])
AC_MSG_RESULT([You may want to look in bin/ for a precompiled executable])
AC_MSG_RESULT([])

dnl Checks host type
dnl AC_CANONICAL_HOST()
dnl diabled to simplify builds
HOST=$host
AC_SUBST(HOST)
AC_DEFINE_UNQUOTED(HOST,"$host")

dnl Special Substitutions
CONFIG_DATE_STRG=`date`
AC_SUBST(CONFIG_DATE_STRG)
AC_DEFINE_UNQUOTED(CONFIG_DATE_STRG,"$CONFIG_DATE_STRG")

dnl Checks for programs.
dnl CFLAGS generally has a low optimization and -g.  Annoying, and too slow.
dnl This may need to be changed to support other compilers
OPT=${OPT:-"-O3"}
AC_SUBST(OPT)
AC_PROG_CC
AC_PROG_INSTALL
AC_PATH_PROG(PERL,perl)

dnl Checks for libraries.
AC_PATH_XTRA
X_LIBS=$X_LIBS
AC_SUBST(X_LIBS)

dnl Checks for header files.
AC_PATH_X
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h unistd.h sys/file.h sys/time.h sys/un.h math.h)

dnl Checks for typedefs, structures
AC_CHECK_TYPE(size_t,unsigned int)
AC_CHECK_TYPE(uint_t,unsigned int)
AC_CHECK_TYPE(ulong_t,unsigned long)
AC_TYPE_SIZE_T
AC_STRUCT_TM

dnl Checks for compiler characteristics.
AC_C_INLINE
dnl Warning below, don't know how to fix
AC_C_BIGENDIAN

dnl Checks for library functions.
AC_CHECK_FUNCS(gethostname strdup strcasecmp)

dnl Checks for system services

dnl Other install directories
pkgdatadir=${datadir}/dinotrace
AC_DEFINE_UNQUOTED(DINODISK_DEFAULT,"$pkgdatadir")
AC_SUBST(pkgdatadir)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

AC_MSG_RESULT([])
AC_MSG_RESULT([You may want to look in bin/ for a precompiled executable])
AC_MSG_RESULT([else, now type 'make'])
AC_MSG_RESULT([])
