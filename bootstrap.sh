#!/bin/sh
#
# $Id: bootstrap.sh,v 1.2 2004/07/07 18:19:20 stuart_hc Exp $

# remove generated files (avoids clashes between versions of autotools)
rm -f depcomp missing install-sh mkinstalldirs
rm -f aclocal.m4 configure Makefile.in config.guess ltmain.sh config.sub
rm -rf autom4te.cache

set -x
aclocal
automake --foreign --copy --add-missing
autoconf
