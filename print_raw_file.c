#include "zip.h"
#include <stdio.h>
#include <stdlib.h>
#define ASCII_CHAR_ZERO_TO_INT_ZERO -48

int main (int argc, char* argv[]) {

	/* get command line parameters */
	int local_file, index;
	if (argc != 2) {
		printf ("usage: %s <integer>\n", argv[0]);
		exit (EXIT_FAILURE);
	}
	else {
		index = 0;
		local_file = 0;
		while (*(argv[1] + index)) { 
			local_file *= 10;
			local_file += *(argv[1] + index) + ASCII_CHAR_ZERO_TO_INT_ZERO;
			index++;
		}
	}

	zip_object zip1;
	zip_constructor (&zip1);
	printf ("object constructed\n");

	zip_open_disk (zip1, "spreadsheet.ods");
	printf ("file opened.\n");

	char temp[1000];
	printf ("local filename: %s\n",	zip_get_filename (zip1, local_file, temp, 1000));

	printf ("local file index: %d\n", zip_search_filename (zip1, temp));

	printf ("local file size: %d\n", (int) zip_get_file_length (zip1, local_file));

	unsigned char* raw = NULL;
	unsigned long size;
	size = zip_get_file_raw (zip1, local_file, &raw);
	printf ("zip_get_file_raw() returned size=%d\n", size);

	for (int i=0; i<size; i++) {
		printf ("%d\t%d\t", i, raw[i]);
		for (int j=7; j>=0; j--)
			printf ("%d", ((raw[i] & (1 << j)) >> j));
		printf ("\n");
	}

	if (raw)
		free (raw);	

	zip_destructor (&zip1);
}
