dnl $Id: configure.ac,v 1.25 2007/02/18 07:35:04 mikrom Exp $
dnl
dnl Process this file with autoconf to produce a configure script.
AC_INIT(bsb2tif.c)
AM_INIT_AUTOMAKE(libbsb, 0.0.8)

dnl AC_PROG_CC finds CC and defaults CFLAGS (usually -g -O2)
orig_cflags=$CFLAGS
AC_PROG_CC
AC_PROG_CXX

dnl Default CFLAGS for developer's convenience and avoid ranlib with MSVC
use_msvc=no
case "$CC" in
	cl*|CL*)
		# -Od turn off optimisation, -Zi turn on debugging info
		CFLAGS="-nologo -Od -W3 -Zi"
		# Use a no-op command for ranlib
		RANLIB="true"
		use_msvc=yes
		;;
	*)
		# override AC_PROG_CC default CFLAGS except when user specified CFLAGS
		if test x"$orig_cflags" != x"$CFLAGS" ; then
			CFLAGS="-g -O2 -W -Wall -Werror"
		fi
		AC_PROG_RANLIB
		;;
esac
AM_CONDITIONAL(USE_MSVC, test x"$use_msvc" = "xyes")


dnl If libtiff found, build TIFF programs: tif2bsb, bsb2tif
have_libtiff=no
AC_CHECK_HEADER([tiff.h],
	[AC_CHECK_LIB(tiff, TIFFOpen, [ have_libtiff=yes ],
		[], [-lm])]
	)
AM_CONDITIONAL(HAVE_LIBTIFF, test x"$have_libtiff" = "xyes")
if test x"$have_libtiff" = "xno" ; then
	AC_MSG_WARN([*** TIFF conversion tools will not be built ***])
fi

dnl If libpng found, build PNG programs: tif2png
have_libpng=no
AC_CHECK_HEADER([png.h],
	[AC_CHECK_LIB(png, png_init_io, [ have_libpng=yes ],
		[], [-lz -lm])]
	)
AM_CONDITIONAL(HAVE_LIBPNG, test x"$have_libpng" = "xyes")
if test x"$have_libpng" = "xno" ; then
	AC_MSG_WARN([*** PNG conversion tools will not be built ***])
fi

have_qt=no
AC_PATH_X
qtdir=${QTDIR}
if test x"$qtdir" = "x" ; then
        dnl Try to locate Qt4 in well-known places
        AC_CHECK_FILE([/usr/share/qt4],
                      [qtdir=/usr/share/qt4],
                      [qtdir=/usr/lib/qt4])
        export QTDIR=$qtdir
	AC_MSG_WARN([QTDIR not set, assuming $qtdir but build will fail if QTDIR env var not set])
fi
qt_includes=$qtdir/include
export CPPFLAGS="$CPPFLAGS -I$qt_includes"
AC_CHECK_HEADER([QtCore/qglobal.h], [ have_libqt=yes ] )
AM_CONDITIONAL(HAVE_LIBQT, test x"$have_libqt" = "xyes")
if test x"$have_libqt" = "xyes" ; then
   echo "Qt4 seems OK. Including bsbview in the build!"
else
   AC_MSG_WARN([*** bsbview will not be built (no QT or QTDIR wrong) ***])
fi


dnl Test for valgrind and test which args are needed for memcheck.
dnl Using -q --leak-check on valgrind pre-2.2 gives stderr output when
dnl no leaks are found so only use --leak-check with recent valgrind.
AC_CHECK_PROGS(VALGRIND, valgrind)
if test x"$VALGRIND" != "x" ; then
	if $VALGRIND -q --tool=memcheck true 2>/dev/null; then
		VALGRIND="valgrind --tool=memcheck --leak-check=yes"
	fi
fi

dnl Initialize the test suite
dnl (the use of '.' is needed to get top-level in the PATH under FreeBSD)
AC_CONFIG_TESTDIR([tests], [tests:.])
AC_CONFIG_FILES([tests/Makefile])
AC_CONFIG_FILES([tests/at_wrap], [chmod +x tests/at_wrap])
AM_MISSING_PROG([AUTOM4TE], [autom4te])

AC_OUTPUT(Makefile)
