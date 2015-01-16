/*
 *  bsbfix.c - Rewrite the index table in the BSB file.  Useful after the
 *				ASCII header has been hand edited to give a valid BSB file.
 *
 *  Copyright (C) 2004  Stuart Cunningham <stuart_hc@users.sourceforge.net>
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
 *  $Id: bsbfix.c,v 1.12 2004/11/03 13:29:39 stuart_hc Exp $
 *
 */

#include <stdio.h>
#include <stdlib.h>		/* for malloc() */

#ifdef _WIN32			/* provide a WIN32 replacement for truncate() */

#include <windows.h>		/* for CreateFile() et al */
/* truncate() returns 0 on success */
static int truncate(const char *path, long length)
{
	HANDLE	fh;
	int		result;

	if ((fh = CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE)) <= NULL)
	{
		perror(path);
		return 1;
	}

	SetFilePointer( fh, length, NULL, FILE_BEGIN );
	if (! SetEndOfFile(fh))
	{
		LPTSTR msg;
		DWORD err = GetLastError();
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &msg, 0, NULL);
		fprintf(stderr, "SetEndOfFile error (%08X) %s\n", (int)err, msg);
		result = 1;
	}
	else
		result = 0;
	CloseHandle(fh);
	return result;
}
#else
#include <unistd.h>		/* for truncate() */
#endif

#include <bsb.h>

extern int main (int argc, char *argv[])
{
	BSBImage	image;
	int			i, arg_idx, *index, delete_only = 0;
	uint8_t		*buf;

	arg_idx = 1;		// points to the first non-option arg
	if (argc > 1 && (argv[1][0] == '-' && argv[1][1] == 'd'))
	{
		delete_only = 1;
		arg_idx++;
	}

	if ((argc - arg_idx) != 1)
	{
		fprintf(stderr, "Usage:\n\tbsbfix [-d] file.kap\n");
		fprintf(stderr, "\n\tRewrite the index table for the specified BSB file, editing it in place\n");
		fprintf(stderr, "\n\t-d   Delete the index table without restoring a correct index table\n");
		exit(1);
	}

	if (! bsb_open_header(argv[arg_idx], &image))
		exit(1);

	buf = (uint8_t *)malloc(image.width);
	if (! buf)
		exit(1);

	index = (int *)malloc((image.height + 1) * sizeof(int));


	/* Read rows from bsb file */
	for (i = 0; i < image.height; i++)
	{
		index[i] = ftell(image.pFile);

		bsb_read_row(&image, buf);

	}
	free(buf);

	/* record start-of-index-table file position in the index table */
	index[image.height] = ftell(image.pFile);
	bsb_close(&image);

	/* delete index table by truncating file */
	if (truncate(argv[arg_idx], index[image.height]) != 0)
	{
		perror(argv[arg_idx]);
		exit(1);
	}

	/* If specified, do not rewrite a correct index table */
	if (delete_only)
	{
		free(index);
		return 0;
	}

	/* Open the file to write new index table */
	if ((image.pFile = fopen(argv[arg_idx], "ab")) == NULL)
	{
		perror(argv[arg_idx]);
		exit(1);
	}

	bsb_write_index(image.pFile, image.height, index);
	free(index);

	return 0;
}
