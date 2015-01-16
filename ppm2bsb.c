/*
 *	ppm2bsb.c - Convert a PPM file into a BSB file, using a template
 *				containing all the geographic information.
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
 *	$Id: ppm2bsb.c,v 1.13 2004/08/25 12:50:58 stuart_hc Exp $
 *
 */


#include <stdio.h>
#include <stdlib.h>		/* for malloc() */
#include <string.h>		/* for strncmp() */

#include <bsb.h>

#define DEBUG_TRACE(x)

DEBUG_TRACE(FILE *debug_fp)

static int		num_colors = 0;
static unsigned	color_map[256];
static int depth_for_colors[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

static int lookup_pixel(unsigned pixel)
{
	int i;

	for (i = 0; i < num_colors; i++)
	{
		if (color_map[i] == pixel)
			return i;
	}
	return -1;
}

/* Convert to little endian if running on big-endian machine */
static int endian_test = 0x11223344;
static void fix_endian(unsigned *pixel)
{
	unsigned char	*p = (unsigned char *)&endian_test;

	if ( *p == 0x11 )		/* big endian? */
	{
		unsigned char   t[4];		/* tmp */
		p = (unsigned char *)pixel;
		t[0] = p[3]; t[1] = p[2]; t[2] = p[1]; t[3] = p[0];
		p[0] = t[0]; p[1] = t[1]; p[2] = t[2]; p[3] = t[3];
	}
}

/* Takes a .kap template and a ppm file */
extern int main (int argc, char *argv[])
{
	BSBImage	image;
	int			i, j, idx, *index, magic, max_sample, start_of_raster;
	int			depth = 0, found_bsb_tag = 0;
	FILE		*tmpl_file, *ppm, *out;
	uint8_t		*buf, *scratch;
	char		line[1024];

	if (argc != 4)
	{
		fprintf(stderr, "Usage:\n\tppm2bsb template.kap input.ppm output.kap\n");
		exit(1);
	}

	/* Read ppm and and store width/height */
	ppm = fopen(argv[2], "rb");
	if (! ppm)
	{
		perror(argv[2]);
		exit(1);
	}
	fscanf(ppm, "P%d\n", &magic);
	fscanf(ppm, "%d %d\n", &image.width, &image.height);
	fscanf(ppm, "%d\n", &max_sample);
	start_of_raster = ftell(ppm);

	/* Count unique colors in the PPM file */
	for (j = 0; j < image.height; j++)
	{
		for (i = 0; i < image.width; i++)
		{
			unsigned pixel = 0;

			if (fread(&pixel, 3, 1, ppm) != 1)
			{
				fprintf(stderr, "Can't read pixel (%d,%d) from PPM file\n",i,j);
				exit(1);
			}
			fix_endian(&pixel);

			/* linear search array of known colors for this pixel */
			for (idx = 0; idx < num_colors; idx++)
			{
				if (color_map[idx] == pixel)
					break;
			}
			if (idx == num_colors)		/* new color found */
			{
				color_map[idx] = pixel;
				num_colors++;
			}
		}
	}
	fseek(ppm, start_of_raster, SEEK_SET);

	/* The BSB format cannot cope with more than 128 colors */
	if (num_colors > 128)
	{
		fprintf(stderr, "Too many colors for BSB format (%d > 128 max.)\n", num_colors);
		fprintf(stderr, "Try reducing the colors.\n\tE.g.\n\tconvert -colors 128 ...\n");
		exit(1);
	}

	/* Given num_colors in input image, compute required "depth"	*/
	/* E.g. num_colors=35, depth=6  (2^6 = 64)						*/
	for (i = 0; i < (int)sizeof(depth_for_colors); i++)
	{
		if (num_colors < depth_for_colors[i])
			break;
		depth = i + 1;
	}
	image.depth = depth;

	buf = (uint8_t *)malloc(image.width);
	if (! buf)
	{
		fprintf(stderr,"Cannot allocate %d bytes for image row\n", image.width);
		exit(1);
	}

	out = fopen(argv[3], "wb");
	if (! out)
	{
		perror(argv[3]);
		exit(1);
	}

	/* Copy in all text lines from template excluding RGB and IFM tags */
	tmpl_file = fopen(argv[1], "rb");
	while (fgets(line, sizeof(line), tmpl_file) != NULL)
	{
		if (strncmp("RGB/", line, 4) == 0)
			continue;
		if (strncmp("IFM/", line, 4) == 0)
			continue;
		if (line[0] == 0x1a)
			break;
		/* Copy all other lines to the output BSB */
		if (strncmp("BSB/", line, 4) == 0)
		{
			/* TODO: read the width,height data and validate */
			found_bsb_tag = 1;
		}
		fputs(line, out);
	}
	fclose(tmpl_file);
	if (! found_bsb_tag)
	{
		fprintf(stderr, "Could not find BSB/ tag in template file\n");
		exit(1);
	}

	/* Write IFM tag */
	fprintf(out, "IFM/%d\r\n", depth);
	/* Write RGB tags for colormap */
	for (i = 0; i < num_colors; i++)
	{
		unsigned pixel = color_map[i];
		fprintf(out, "RGB/%d,%u,%u,%u\r\n",
					i+1,
					pixel & 0xff,
					(pixel >> 8) & 0xff,
					(pixel >> 16) & 0xff
					);
	}
	fputc(0x1a, out);
	fputc('\0', out);
	fputc(depth, out);

	index = (int *)malloc((image.height + 1) * sizeof(int));

	/* encoded line contains row number and encoded pixels */
	scratch = (uint8_t *)malloc(image.width + 8);

	DEBUG_TRACE(debug_fp = fopen("debug.out", "rb"));
	DEBUG_TRACE(image.depth = 6);

	/* Read rows from PPM, write to bsb */
	for (j = 0; j < image.height; j++)
	{
		int len;

		/* Record start-of-row file position for index table */
		index[j] = ftell(out);

		/* Read a row of PPM raster */
		for (i = 0; i < image.width; i++)
		{
			int			cidx;
			unsigned	pixel = 0;

			if (fread(&pixel, 3, 1, ppm) != 1)
			{
				fprintf(stderr, "Can't read pixel (%d,%d) from PPM file\n",i,j);
				exit(1);
			}
			fix_endian(&pixel);

			cidx = lookup_pixel(pixel);
			if (cidx == -1)
			{
				fprintf(stderr, "Error in lookup of colormap index\n");
				exit(1);
			}
			DEBUG_TRACE(fread(&buf[i], 1, 1, debug_fp));
			buf[i] = cidx;
		}

		/* Compress raster and write to BSB file */
		len = bsb_compress_row(&image, j, buf, scratch);
		fwrite(scratch, len, 1, out);
	}
	free(scratch);
	free(buf);
	fclose(ppm);

	/* record start-of-index-table file position in the index table */
	index[image.height] = ftell(out);

	bsb_write_index(out, image.height, index);
	free(index);

	fclose(out);

	return 0;
}
