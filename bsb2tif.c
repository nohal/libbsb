/*
 *	bsb2tif.c - Convert a bsb raster image to a TIFF image file.
 *
 *	Copyright (C) 2000  Stuart Cunningham <stuart_hc@users.sourceforge.net>
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation; either
 *	version 2.1 of the License, or (at your option) any later version.
 *
 *	This library is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	Lesser General Public License for more details.
 *
 *	You should have received a copy of the GNU Lesser General Public
 *	License along with this library; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	$Id: bsb2tif.c,v 1.13 2004/11/08 17:10:18 stuart_hc Exp $
 *
 */


#include <stdio.h>
#include <stdlib.h>		/* for malloc() */
#include <string.h>		/* for strcpy() etc */

#include <tiffio.h>		/* libtiff - TIFF file I/O */
#include <bsb.h>


extern int main (int argc, char *argv[])
{
	BSBImage	image;
	int			i;
	uint16_t	red[256], green[256], blue[256];
	TIFF*		tif;
	uint8_t		*buf;

	if (argc != 3)
	{
		fprintf(stderr, "Usage:\n\tbsb2tif input.kap output.tif\n");
		exit(1);
	}
	bsb_open_header(argv[1], &image);

	/* Initialise colormap entries */
	memset(red, 0, sizeof(red));
	memset(green, 0, sizeof(green));
	memset(blue, 0, sizeof(blue));

	/* convert BSB colormap to libtiff style colormap */
	for (i = 0; i < image.num_colors; i++)
	{
		red[i] = image.red[i] * 65536 / 256;
		green[i] = image.green[i] * 65536 / 256;
		blue[i] = image.blue[i] * 65536 / 256;
	}

	buf = (uint8_t *)malloc(image.width);
	if (! buf)
		exit(1);

	/* Open tif file for output (truncates any existing file) */
	tif = TIFFOpen(argv[2], "w");
	if (! tif)
	{
		perror(argv[2]);
		exit(1);
	}

	/* From TIFF spec, these are required fields for palette-color images */
	TIFFSetField(tif,TIFFTAG_IMAGEWIDTH, image.width);
	TIFFSetField(tif,TIFFTAG_IMAGELENGTH, image.height);
	/* Can't rely on tiff library having LZW support so use PACKBITS */
	TIFFSetField(tif,TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);
	/* BSB is strictly a colormap only format */
	TIFFSetField(tif,TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
	TIFFSetField(tif,TIFFTAG_ROWSPERSTRIP, 1);
	TIFFSetField(tif,TIFFTAG_BITSPERSAMPLE, 8);
	/* CONTIG means R G B grouped together in each STRIP */
	TIFFSetField(tif,TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	TIFFSetField(tif,TIFFTAG_COLORMAP, red, green, blue);

	/* Read rows from bsb file, write to tif file */
	for (i = 0; i < image.height; i++)
	{
		bsb_seek_to_row(&image, i);
		bsb_read_row(&image, buf);
		TIFFWriteScanline(tif, buf, i, 0);
	}

	TIFFClose(tif);
	free(buf);

	bsb_close(&image);

	return 0;
}
