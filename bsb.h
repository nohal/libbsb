/*
 *  bsb.h	- libbsb types and functions
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
 *  $Id: bsb.h,v 1.12 2004/08/13 18:37:46 stuart_hc Exp $
 *
 */

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#else
// not ISO C99 - use best guess instead
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#endif

typedef struct BSBImage
{
	FILE		*pFile;
	char		depth;
	char		num_colors;
	float		version;
	int			width;
	int			height;
	double		xresolution;
	double		yresolution;
	uint8_t		red[256];
	uint8_t		green[256];
	uint8_t		blue[256];
} BSBImage;

/* See comments in bsb_io.c for documentation on these functions */
extern int bsb_get_header_size(FILE *fp);
extern int bsb_open_header(char *filename, BSBImage *p);
extern int bsb_seek_to_row(BSBImage *p, int row);
extern int bsb_read_row(BSBImage *p, uint8_t *buf);
extern int bsb_compress_row(BSBImage *p, int row, const uint8_t *pixel, uint8_t *buf);
extern int bsb_write_index(FILE *fp, int height, int index[]);
extern int bsb_close(BSBImage *p);
