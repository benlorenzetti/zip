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
	printf ("local filename: %s\n",	zip_get_filename (zip1, 17, temp, 1000));

	zip_destructor (&zip1);
}
