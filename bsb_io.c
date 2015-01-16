/*
 *  bsb_io.c	- implementation of libbsb reading and writing
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
 *  $Id: bsb_io.c,v 1.20 2004/12/22 13:04:53 stuart_hc Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bsb.h>

#ifdef _WIN32
#define DIR_SEPARATOR '\\'
#else
#define DIR_SEPARATOR '/'
#endif

/* MSVC doesn't supply a strcasecmp(), so use the MSVC workalike */
#ifdef _MSC_VER
#define strcasecmp(s1, s2) stricmp(s1, s2)
#endif

/* bsb_ntohl - portable ntohl */
static uint32_t bsb_ntohl(uint32_t netlong)
{
	static const uint32_t testvalue = 0x12345678;
	uint8_t *p = (uint8_t *)&testvalue;

	if (p[0] == 0x12 && p[3] == 0x78)		// big endian?
		return netlong;

	// for little endian swap bytes
	return	( (netlong & 0xff000000) >> 24
			| (netlong & 0x00ff0000) >> 8
			| (netlong & 0x0000ff00) << 8
			| (netlong & 0x000000ff) << 24 );
}

/* Copies the next newline-delimited line from *pp into line as a NUL		*/
/* terminated string and increments	the pp pointer to point to the start	*/
/* of the next line.														*/
/* Removes \r characters from the line (if any).							*/
static int next_line(char **pp, int len, char *line)
{
	char *p = *pp;
	char *q = line;

	while (*p != '\0')
	{
		/* Don't overflow destination buffer */
		if (q - line > len)
			return 0;		/* line didn't fit in buffer */

		if (*p == '\r')
		{
			p++;
			continue;
		}
		if (*p == '\n')
		{
			*q = '\0';
			p++;
			*pp = p;
			return 1;
		}
		*q++ = *p++;
	}
	return 0;				/* didn't find \n line terminator */
}

extern int bsb_get_header_size(FILE *fp)
{
	int		text_size = 0, c;

	/* scan for end-of-text marker and record size of text section */
	while ( (c = fgetc(fp)) != -1 )
	{
		if (c == 0x1a)		/* Control-Z */
			break;
		text_size++;
	}
	return text_size;
}

/* return 0 on failure */
extern int bsb_open_header(char *filename, BSBImage *p)
{
	int			text_size = 0, c, depth;
	char		*p_ext, *pt, *text_buf, line[1024];

	p->pFile = NULL;

	/* Look for an extension (if any) to test for '.NO1' files */
	/* which must be treated specially since they are obfuscated */
	if (	(p_ext = strrchr(filename, '.')) != NULL &&
			(p_ext > strrchr(filename, DIR_SEPARATOR)) &&
			(strcasecmp(p_ext, ".NO1") == 0) )
	{
		FILE *inputFile;

		if (! (inputFile = fopen(filename, "rb")))
		{
			perror(filename);
			return 0;
		}

		/* Open temporary file to store unobfuscated file */
		if (! (p->pFile = tmpfile()))
		{
			perror("tmpfile()");
			return 0;
		}

		/* .NO1 files are obfuscated using ROT-9 */
		while ((c = fgetc(inputFile)) != EOF)
		{
			int r = (c - 9) & 0xFF;
			fputc(r, p->pFile);
		}
		fflush(p->pFile);
		fseek(p->pFile, 0, SEEK_SET);
	}
	else
	{
		/* Normal unobfuscated BSB/NOS files can be opened straight away */
		if (! (p->pFile = fopen(filename, "rb")))
		{
			perror(filename);
			return 0;
		}
	}

	if ((text_size = bsb_get_header_size(p->pFile)) == 0)
		return 0;

	/* allocate space & read in the entire text header */
	text_buf = (char *)malloc(text_size + 1);
	if (text_buf == NULL)
	{
		fprintf(stderr,
				"malloc(%d) failed for text header - BSB file possibly corrupt",
				text_size + 1);
		return 0;
	}

	fseek(p->pFile, 0, SEEK_SET);
	if (fread(text_buf, text_size, 1, p->pFile) != 1)
		return 0;
	text_buf[text_size] = '\0';

	pt = text_buf;
	p->num_colors = 0;
	p->version = -1.0;
	p->width = -1;
	p->height = -1;
	while ( next_line(&pt, sizeof(line), line) )
	{
		char		*s;
		int			index, r, g, b, ifm_depth;
		float		ver;

		if (sscanf(line, "RGB/%d,%d,%d,%d", &index, &r, &g, &b) == 4)
		{
			if ((unsigned)index < sizeof(p->red))
			{
				if (index > 0)
				{
					p->red[index-1] = r;
					p->green[index-1] = g;
					p->blue[index-1] = b;
					p->num_colors++;
				}
			}
		}
		if ( (s = strstr(line, "RA=")) )
		{
			int x0, y0;

			/* Attempt to read old-style NOS (4 parameter) version of RA= */
			/* then fall back to newer 2-argument version */
			if ((sscanf(s,"RA=%d,%d,%d,%d",&x0,&y0,&p->width,&p->height)!=4) &&
				(sscanf(s,"RA=%d,%d", &p->width, &p->height) != 2))
			{
				fprintf(stderr, "failed to read width,height from RA=\n");
				return 0;
			}
		}
		if ( (s = strstr(line, "DX=")) )
		{
			if ( sscanf(s, "DX=%lf", &p->xresolution) != 1 )
			{
				fprintf(stderr, "failed to read xresolution\n");
				return 0;
			}
		}
		if ( (s = strstr(line, "DY=")) )
		{
			if ( sscanf(s, "DY=%lf", &p->yresolution) != 1 )
			{
				fprintf(stderr, "failed to read xresolution\n");
				return 0;
			}
		}
		if (sscanf(line, "IFM/%d", &ifm_depth) == 1)
		{
			p->depth = ifm_depth;
		}
		if (sscanf(line, "VER/%f", &ver) == 1)
		{
			p->version = ver;
		}
	}
	if (p->width == -1 || p->height == -1)
	{
		fprintf(stderr, "Error: Could not read RA=<width>,<height>\n");
		return 0;
	}

	/* Attempt to read depth from binary section */
	c = fgetc(p->pFile);		/* discard <Control-Z> */
	c = fgetc(p->pFile);		/* discard NUL */

	/* Test depth from bitstream */
	depth = fgetc(p->pFile);
	if (depth != p->depth)
		fprintf(stderr, "Warning: depth from IFM tag (%d) != depth from bitstream (%d)\n", p->depth, depth);

	free(text_buf);
	return 1;
}

/* bsb_seek_to_row(BSBImage *p, int row)
 *
 * p	- pointer to a BSBImage
 * row	- row to seek to starting from row 0 (BSB row 1)
 *
 * Returns 1 on success and 0 on error
 *
 * Uses the index table at the end of the BSB file to quickly jump to a row.
 */
extern int bsb_seek_to_row(BSBImage *p, int row)
{
	int st, start_of_index, start_of_rows;

	/* Read start-of-index offset */
	if (fseek(p->pFile, -4, SEEK_END) == -1)
		return 0;
	if (fread(&st, 4, 1, p->pFile) != 1)
		return 0;
	start_of_index = bsb_ntohl(st);

	/* Read start-of-rows offset */
	if (fseek(p->pFile, row*4 + start_of_index, SEEK_SET) == -1)
		return 0;
	if (fread(&st, 4, 1, p->pFile) != 1)
		return 0;
	start_of_rows = bsb_ntohl(st);

	/* seek to row offset */
	if (fseek(p->pFile, start_of_rows, SEEK_SET) == -1)
		return 0;
	return 1;
}

/* Table used for computing multiplier in bsb_read_row() */
static char mul_mask[8] = { 0, 63, 31, 15, 7, 3, 1, 0 };

/* bsb_read_row(BSBImage *p, int row, uint8_t *buf)
 *
 * p	- pointer to a BSBImage containing file pointer at the start of a row
 *			this occurs after bsb_open_header() or bsb_seek_to_row()
 * buf	- output buffer for uncompressed pixel data
 *
 * Returns 1 on success and 0 on error
 */
extern int bsb_read_row(BSBImage *p, uint8_t *buf)
{
	int		c, i, multiplier, row_num = 0;
	int		pixel = 1, written = 0;

	/* The row number is stored in the low 7 bits of each byte.				*/
	/* The 8th bit indicates if row number is continued in the next byte.	*/
	do {
		c = fgetc(p->pFile);
		row_num = ((row_num & 0x7f) << 7) + c;
	} while (c >= 0x80);

	/* Rows are terminated by '\0'.  Note that rows can contain a '\0'	*/
	/* as part of the run-length data, so '\0' does not delimit rows.	*/
	/* (This occurs when multiplier is a multiple of 128 - 1)			*/
	while ((c = fgetc(p->pFile)) != '\0')
	{
		if (c == EOF)
		{
			fprintf(stderr, "Warning: EOF reading row %d\n", row_num);
			return 0;
		}

		pixel = (c & 0x7f) >> (7 - p->depth);
		multiplier = c & mul_mask[(int)p->depth];

		while (c >= 0x80)
		{
			c = fgetc(p->pFile);
			multiplier = (multiplier << 7) + (c & 0x7f);
		}
		multiplier++;

		if (multiplier > p->width)		/* limit impact of corrupt BSB data */
			multiplier = p->width;

		for (i = 0; i < multiplier; i++)
		{
			/* For the lower depths, the "grain" of the multiplyer is		*/
			/* course, so don't write past the width of the buffer.			*/
			if (written < p->width)
				buf[written++] = pixel - 1;		/* BSB color idx starts at 1 */
		}
	}

	if (written < p->width)
	{
		int short_fall = p->width - written;

		/* It seems valid BSB rows sometimes don't include pixel data for	*/
		/* the very last pixel or two.  Perhaps the decoder is supposed		*/
		/* to merely repeat the last pixel until the width is reached.		*/
		/* A value of 8 was chosen as a guess since the intended behaviour	*/
		/* of a BSB reader in this situation is not known.					*/
		if (short_fall < 8)
		{
			/* Repeat the last pixel value for small short falls */
			while (written < p->width)
				buf[written++] = pixel - 1;
		}
		else
		{
			fprintf(stderr, "Warning: Short row for row %d written=%d width=%d\n", row_num, written, p->width);
			return 0;
		}
	}
	return 1;
}

extern int bsb_write_index(FILE *fp, int height, int index[])
{
	int j;

	/* Write index table */
	for (j = 0; j < height + 1; j++)
	{
		/* Indices must be written as big-endian */
		if ((fputc(index[j] >> 24, fp)) == EOF) return 0;
		if ((fputc((index[j] & 0x00ff0000) >> 16, fp)) == EOF) return 0;
		if ((fputc((index[j] & 0x0000ff00) >> 8, fp)) == EOF) return 0;
		if ((fputc(index[j] & 0x000000ff, fp)) == EOF) return 0;
	}
	return 1;
}

/* Convert an integer into the variable length encoded form used for row
 * numbers in the BSB bitstream.
 */
static int bsb_store_integer(int row, uint8_t *p)
{
	int		req_bytes, i, c;

	/* Using 32 bit integers, row numbers are stored in one to four bytes.	*/
	/* This limits row numbers to 2^28 - 1 (= 268,435,455).					*/
	/* Calculate required bytes to store integer using a table instead of	*/
	/* expensive calls to get log base 2.									*/

	req_bytes = 1;
	if (row > 2097151)						/* 2^(7*3) - 1 */
		req_bytes = 4;
		else if (row > 16383)				/* 2^(7*2) - 1 */
			req_bytes = 3;
			else if (row > 127)				/* 2^(7*1) - 1 */
				req_bytes = 2;

	for (i = req_bytes - 1; i >= 0; i--)
	{
		c = (row >> i * 7) & 0x7f;

		if (i > 0)
			c |= 0x80;						/* set sentinel high bit */

		*p++ = c;
	}
	return req_bytes;
}

/* bsb_compress_row(BSBImage *p, int row, uint8_t *aPixel, uint8_t *buf)
 *
 * p		- pointer to a BSBImage for the width & depth values
 * row		- row number stating at 0 (row 0 will be stored as BSB row 1)
 * aPixel	- array of uncompressed pixels
 * buf		- output buffer for compressed bitstream
 *
 * Returns length of encoded bitstream
 *
 * Use a simple run length encoding, storing the run length in the
 * sentinel bits where possible.  Lower depth images have more room
 * to store this information.
 */
extern int bsb_compress_row(BSBImage *p, int row, const uint8_t *aPixel, uint8_t *buf)
{
	int		width = p->width, depth = p->depth;
	int		run_length, sentinel_bits, max_sentinel_rep, sentinel_mask;
	int		ibuf, ipixel;
	uint8_t	last_pix, shifted_pix;

	/* Write the row number (add 1 since BSB rows number from 1 not 0) */
	ibuf = 0;
	ibuf = bsb_store_integer(row+1, buf);


	sentinel_bits = 7 - depth;		/* number of sentinel bits available */
	sentinel_mask = ((1 << depth) - 1) << (7 - depth);
	max_sentinel_rep = 1 << sentinel_bits;	/* max run length that fits	*/
											/* in sentinel bits			*/

	ipixel = 0;
	while ( ipixel < width )
	{
		last_pix = aPixel[ipixel];
		ipixel++;

		/* Count length of pixel 'run' - run length cannot be greater than width */
		run_length = 0;
		while ( (ipixel < width) && (aPixel[ipixel] == last_pix) )
		{
			ipixel++;
			run_length++;
		}
		ipixel--;

		/* BSB colormap never uses index 0, so add 1 to pixel index before use */
		shifted_pix = (last_pix + 1) << sentinel_bits;

		if ( run_length < max_sentinel_rep )
		{
			if ( run_length == 0 )
			{
				if ( (shifted_pix & 0xff) == 0 )
					shifted_pix = sentinel_mask;
			}
			buf[ibuf++] = shifted_pix | run_length;
		}
		else
		{
			int		i, rc_bytes, rc_bits;
			int		rc = run_length / 2;

			rc_bits = 1;
			while (rc > 0)
			{
				rc = rc / 2;
				rc_bits++;
			}

			rc_bytes = (int)(rc_bits / 7);
			if ( rc_bits - rc_bytes * 7 > sentinel_bits )
				rc_bytes++;

			for ( i = rc_bytes; i > 0; i-- )
			{
				buf[ibuf + i] = run_length & 0x7f;
				run_length = run_length >> 7;
			}

			buf[ibuf] = shifted_pix | run_length;

			for ( i = 0; i < rc_bytes; i++ )
				buf[ibuf + i] |= 0x80;

			ibuf++;
			ibuf = ibuf + rc_bytes;
		}
		ipixel++;
	}
	buf[ibuf] = 0;						/* terminate row with zero */
	return ibuf + 1;
}

extern int bsb_close(BSBImage *p)
{
	fclose(p->pFile);

	return 1;
}
