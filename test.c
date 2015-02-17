#include "zip.h"
#include <stdio.h>
#include <stdlib.h>

int main () {

	zip_object zip1;
	zip_constructor (&zip1);
	printf ("object constructed\n");

	zip_open_disk (zip1, "spreadsheet.ods");
	printf ("file opened.\n");

	char temp[1000];
	printf ("local filename: %s\n",	zip_get_filename (zip1, 3, temp, 1000));

	printf ("index of content.xml: %d\n", zip_search_filename (zip1, "content.xml"));

	printf ("size of content.xml: %d\n", (int) zip_get_file_length (zip1, 3));

	unsigned char* uncompressed = NULL;
	unsigned long size;
	size = zip_get_file (zip1, 3, &uncompressed);
	printf ("zip_get_file() returned size=%d\n", size);
	if (uncompressed)
		free (uncompressed);	

	zip_destructor (&zip1);
}
