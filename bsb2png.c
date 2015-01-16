/*
 *	bsb2png.c - Convert a bsb raster image to a Portable Network Graphics (png).
 *				See http://www.libpng.org for details of the PNG format and how
 *				to use libpng.
 *
 *	Copyright (C) 2004  Stefan Petersen <spetm@users.sourceforge.net>
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
 *	$Id: bsb2png.c,v 1.7 2007/02/05 17:08:18 mikrom Exp $
 *
 */

#include <stdlib.h>
#include <stdio.h>

#include <bsb.h>
#include <png.h>

static void copy_bsb_to_png(BSBImage *image, png_structp png_ptr)
{
	int		row, bp, pp;
	uint8_t	*bsb_row, *png_row;

	bsb_row = (uint8_t *)malloc(image->height * sizeof(uint8_t *));
	png_row = (uint8_t *)malloc(image->height * sizeof(uint8_t *) * image->depth);

	/* Copy row by row */
	for (row = 0; row < image->height; row++)
	{
		bsb_seek_to_row(image, row);
		bsb_read_row(image, bsb_row);
		for (bp = 0, pp = 0; bp < image->width; bp++)
		{
			png_row[pp++] = image->red[bsb_row[bp]];
			png_row[pp++] = image->green[bsb_row[bp]];
			png_row[pp++] = image->blue[bsb_row[bp]];
		}
		png_write_row(png_ptr, png_row);
	}

	free(bsb_row);
	free(png_row);
} /* copy_bsb_to_png */

int main(int argc, char *argv[])
{
	BSBImage image;
	FILE *png_fd;
	png_structp png_ptr;
	png_infop info_ptr;
	png_text text[2];

	if (argc != 3) {
		fprintf(stderr, "Usage:\n\tbsb2png input.kap output.png\n");
		exit(1);
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL) {
		fprintf(stderr, "png_ptr == NULL\n");
		exit(1);
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		fprintf(stderr, "info_ptr == NULL\n");
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		exit(1);
	}

	if ((png_fd = fopen(argv[2], "wb")) == NULL) {
		perror("fopen");
		exit(1);
	}

	png_init_io(png_ptr, png_fd);

	if (! bsb_open_header(argv[1], &image)) {
		exit(1);
	}

	png_set_IHDR(png_ptr, info_ptr, image.width, image.height, 8,
				PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, 
				PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	/* Some text to go with the png image */
	text[0].key = "Title";
	text[0].text = argv[2];
	text[0].compression = PNG_TEXT_COMPRESSION_NONE;
	text[1].key = "Generator";
	text[1].text = "bsb2png";
	text[1].compression = PNG_TEXT_COMPRESSION_NONE;
	png_set_text(png_ptr, info_ptr, text, 2);

	/* Write header data */
	png_write_info(png_ptr, info_ptr);

	/* Copy the image in itself */
	copy_bsb_to_png(&image, png_ptr);

	png_write_end(png_ptr, NULL);
	fclose(png_fd);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	bsb_close(&image);

	return 0;
}
