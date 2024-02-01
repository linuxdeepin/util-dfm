#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT([udfclient], [0.8.12], [reinoud@NetBSD.org])
#AC_CONFIG_SRCDIR([udf.c])
AC_CANONICAL_HOST
#AC_CONFIG_HEADER([config.h])


#
# Checks for programs.
#
AC_PROG_CC
AC_PROG_INSTALL


#
# Check for OS dependent flags 
#
case $host_os in
netbsd*)
	POSTOBJ="-lprop -lutil"
	;;
linux*)
	CPPFLAGS="$CPPFLAGS -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=500 -D__USE_BSD -D_FILE_OFFSET_BITS=64"
	;;
darwin*)
	# CPPFLAGS="-D_POSIX_C_SOURCE"
	COPTS="$COPTS -fnested-functions"
esac

#
# Checks for libraries.
#
# Rudimentary check for pthread library: if libpthread is found, then
# link it in, otherwise pray it'll be in libc like FreeBSD's.
#
THREADLIB="$with_thread_libs"
if test "$THREADLIB" = ""; then
	AC_CHECK_LIB(pthread, pthread_create, [THREADLIB="-lpthread"])
fi
if test "$THREADLIB" = ""; then
	AC_CHECK_LIB(c_r, pthread_create, [THREADLIB="-lc_r"])
fi
if test "$THREADLIB" = ""; then
	THREADLIB=
fi
AC_SUBST(THREADLIB)
CPPFLAGS="$CPPFLAGS $with_pthread_cflags"


#
# Check where to find clock_gettime. In Linux it is found in librt where
# in BSD its in libc.
#
TIMELIB="$with_time_lib"
if test "$TIMELIB" = ""; then
	AC_CHECK_LIB(c, clock_gettime, [TIMELIB="-lc"])
fi
if test "$TIMELIB" = ""; then
	AC_CHECK_LIB(rt, clock_gettime, [TIMELIB="-lrt"])
fi
if test "$TIMELIB" = ""; then
	TIMELIB=
fi
AC_SUBST(TIMELIB)
CPPFLAGS="$CPPFLAGS $with_time_cflags"


#
# Checks for various header files.
#
#AC_HEADER_STDC
AC_CHECK_HEADERS([endian.h sys/endian.h machine/endian.h])
AC_CHECK_HEADERS([machine/bswap.h sys/bswap.h])
AC_CHECK_HEADERS([machine/int_fmtio.h], [:], [
	CPPFLAGS="$CPPFLAGS -DNO_INT_FMTIO"
])


#
# Check if we have the strlcpy function allready or have to emulate it
#
AC_CHECK_FUNC([strlcpy], [:], [
	CPPFLAGS="$CPPFLAGS -DNO_STRLCPY"
])


#
# Check struct stat
#
stat_cppflags=""
AC_CHECK_MEMBERS([struct stat.st_atimespec], [:], [
	# Non Posix struct stat, assume Linux
	AC_CHECK_MEMBERS([struct stat.st_atim], [
		stat_cppflags="-Dst_atimespec=st_atim -Dst_ctimespec=st_ctim -Dst_mtimespec=st_mtim"
	], [
		echo "Failed to determine struct stat's timespec names"
		exit
	])
])
AC_CHECK_MEMBERS([struct stat.st_birthtimespec], [:], [
	stat_cppflags="$stat_cppflags -DNO_STAT_BIRTHTIME"
])
CPPFLAGS="$CPPFLAGS $stat_cppflags"


#
# Check struct dirent
#
dirent_cppflags=""
AC_CHECK_MEMBERS([struct dirent.d_namlen], [:], [
	dirent_cppflags="$dirent_cppflags -DNO_DIRENT_NAMLEN"
])
CPPFLAGS="$CPPFLAGS $dirent_cppflags"


#
# Check for SCSI implementations
#

HAVE_SCSI="yes"
SCSI_CFLAGS="$with_scsi_cflags"
SCSI_LIB="$with_uscsi_lib"
if test "$SCSI_CFLAGS" = ""; then
	HAVE_SCSI=no
fi

# NetBSD SCSI stack
if test "$HAVE_SCSI" = "no"; then
	AC_CHECK_HEADERS([dev/scsipi/scsipi_all.h], [
		SCSI_CFLAGS="-DUSCSI_SCSIPI"
		HAVE_SCSI=yes
	])
fi

# Linux SCSI stack
if test "$HAVE_SCSI" = "no"; then
	AC_CHECK_HEADERS([scsi/sg.h], [
		SCSI_CFLAGS="-DUSCSI_LINUX_SCSI"
		HAVE_SCSI=yes
	])
fi

# FreeBSD SCSI stack
if test "$HAVE_SCSI" = "no"; then
	AC_CHECK_HEADERS([camlib.h], [
		SCSI_CFLAGS="-DUSCSI_FREEBSD_CAM"
		HAVE_SCSI=yes
		SCSI_LIB="-lcam"
	])
fi

# OpenBSD SCSI stack
if test "$HAVE_SCSI" = "no"; then
	AC_CHECK_HEADERS([scsi/scsi_all.h], [
		CPPFLAGS="$CPPFLAGS -DUSCSI_SCSIPI"
		HAVE_SCSI=yes
	])
fi


# process SCSI stack presence results
if test "$HAVE_SCSI" = "yes"; then
	CPPFLAGS="$CPPFLAGS -DSCSI $SCSI_CFLAGS"
	BUILD_APPS="\$(APPS) \$(SCSI_APPS)"
else
	SCSI_LIB=
	BUILD_APPS="\$(APPS)"
fi
AC_SUBST(BUILD_APPS)
AC_SUBST(SCSI_LIB)
AC_SUBST(CPPFLAGS)
AC_SUBST(POSTOBJ)


#
# Generate output files
#
AC_CONFIG_FILES([Makefile])
# AC_CONFIG_FILES([config.h])
AC_OUTPUT

echo ""
echo "Compile the project with Posix compliant make; possibly installed as bmake or pmake"

