# $Id: Makefile.am,v 1.17 2004/12/22 13:01:19 stuart_hc Exp $
#

EXTRA_DIST = australia4c.kap

SUBDIRS = . tests

.PHONY: valgrind-check
valgrind-check:
	cd tests && $(MAKE) $(AM_MAKEFLAGS) $@

lib_LIBRARIES = libbsb.a
libbsb_a_SOURCES = bsb_io.c
INCLUDES = -I$(top_builddir)
include_HEADERS = bsb.h

bin_PROGRAMS = bsb2ppm ppm2bsb bsbfix
if HAVE_LIBTIFF
bin_PROGRAMS += bsb2tif tif2bsb
endif
if HAVE_LIBPNG
bin_PROGRAMS += bsb2png
endif

LDADD = libbsb.a

bsb2tif_LDADD = libbsb.a -ltiff -lm
tif2bsb_LDADD = libbsb.a -ltiff -lm

# Under MinGW libpng needs -lz (doesn't hurt under other platforms)
bsb2png_LDADD = libbsb.a -lpng -lz -lm

if USE_MSVC
# The '; true' is necessary to throw away the remaining arguments from the
# command line in the Makefile (which is too rigid to permit /out:<arg>)
libbsb_a_AR = link.exe -lib -nologo /out:libbsb.a $(libbsb_a_OBJECTS) ; true
else
libbsb_a_AR = ar crv
endif
