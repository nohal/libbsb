/*
 *  bsb2ppm.c - Convert a bsb raster image to a Portable Pixel Map (PPM) file.
 *				See http://netpbm.sourceforge.net/doc/ppm.html or the ppm(5)
 *				manpage for details of the PPM format.
 *
 *  Copyright (C) 2000  Stuart Cunningham <stuart_hc@users.sourceforge.net>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id: bsb2ppm.c,v 1.10 2004/08/13 18:37:46 stuart_hc Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>		/* for malloc() */
#include <bsb.h>

#ifdef DEBUG
#define DEBUG_TRACE(x) x
#else
#define DEBUG_TRACE(x)
#endif

extern int main (int argc, char *argv[])
{
	DEBUG_TRACE(FILE *debug_fp;)
	FILE*			ppm;
	BSBImage		image;
	int				x, y;
	uint8_t			*buf;

	if (argc != 3)
	{
		fprintf(stderr, "Usage:\n\tbsb2ppm input.kap output.ppm\n");
		exit(1);
	}

	if (! bsb_open_header(argv[1], &image))
		exit(1);

	buf = (uint8_t *)malloc(image.width);
	if (! buf)
		exit(1);

	/* Open PPM file for output */
	ppm = fopen(argv[2], "wb");
	if (! ppm)
	{
		perror(argv[2]);
		exit(1);
	}
	DEBUG_TRACE(debug_fp = fopen("debug.out", "wb"));

	/* Write PPM header (for "raw" format) */
	fprintf(ppm, "P6\n%d %d\n255\n", image.width, image.height);

	/* Read rows from bsb file and write rows to PPM */
	for (y = 0; y < image.height; y++)
	{
		bsb_seek_to_row(&image, y);
		bsb_read_row(&image, buf);

		/* Each pixel is a triplet of Red,Green,Blue samples */
		for (x = 0; x < image.width; x++)
		{
			fputc(image.red[(int)buf[x]], ppm);
			fputc(image.green[(int)buf[x]], ppm);
			fputc(image.blue[(int)buf[x]], ppm);
			DEBUG_TRACE(fputc(buf[x], debug_fp));
		}
	}
	fclose(ppm);
	bsb_close(&image);

	free(buf);

	return 0;
}
