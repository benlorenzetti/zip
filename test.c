#include "zip.h"
#include <stdio.h>

int main () {
	zip_object zip1;
	zip_constructor (&zip1);
	zip_open_disk (zip1, "spreadsheet.ods");
	zip_destructor (&zip1);
}
