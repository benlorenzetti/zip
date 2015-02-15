#include "zip.h"
#include <stdio.h>
#include <stdlib.h>

typedef unsigned char uchar;
typedef unsigned short u16;
typedef unsigned long u32;

#define ZIP_MAXIMUM_NUMBER_OF_DISKS 10
#define ZIP_MINIMUM_EOCDR_SIZE 22
#define ZIP_RECORD_SIGNATURE_SIZE 4

#define ZIP_STATE_CONSTRUCTED 0
#define ZIP_STATE_ALL_DISKS_PARSED 1

struct zip_Object {
	int state;
	FILE* disk_array[ZIP_MAXIMUM_NUMBER_OF_DISKS];
	int disk_array_size;	
};

void zip_constructor (struct zip_Object** ptr_ptr) {
	puts ("constructor...");

	*ptr_ptr = (struct zip_Object*) malloc (sizeof (struct zip_Object));
	struct zip_Object* obj_ptr = *ptr_ptr;
	if (!obj_ptr)
		printf ("memory allocation failure\n"), exit (EXIT_FAILURE);

	obj_ptr->state = ZIP_STATE_CONSTRUCTED;
	obj_ptr->disk_array_size = 0;
}

void zip_destructor (struct zip_Object** ptr_ptr) {
	puts ("destructor...");
	struct zip_Object* obj_ptr = *ptr_ptr;
	
	/* close all open disks */
	for (int i=0; i< obj_ptr->disk_array_size; i++) {
		puts ("attemping iteration...");
		fclose (obj_ptr->disk_array[i]);
	}
	
	/* free the object from memory */
	puts ("free object...");
	free (obj_ptr);
	puts ("leaving destructor...");
}

/* return ZIP_OPEN_SUCCESS, ZIP_OPEN_NEED_ADDITIONAL_DISK, or ZIP_OPEN_FAILURE */
int zip_open_disk (struct zip_Object* obj, const char* fn) {
	
	FILE* fp;
	char mode[] = "r+b";
	fp = fopen (fn, mode);
	if (!fp)
		return ZIP_OPEN_FAILURE;
	else
		obj->disk_array[obj->disk_array_size++] = fp;

	/* attempt to find the End of Central Directory Record */
	u32 file_size, eocd_pos, cd_pos, cur_pos, cur_val;
	if (fseek (fp, 0, SEEK_END))
		return ZIP_OPEN_FAILURE;
	file_size = ftell (fp);
	printf ("file_size=%d\n", file_size);
	if (fseek (fp, (-1)*ZIP_MINIMUM_EOCDR_SIZE, SEEK_END)) /* 22 is min size of end of CD record */
		return ZIP_OPEN_FAILURE;
	cur_pos = file_size - ZIP_MINIMUM_EOCDR_SIZE;
	printf ("cur_pos=%d\n", cur_pos);
	for (cur_pos; cur_pos >= 0; cur_pos--) {
		if (1 != fread ((void*)&cur_val, ZIP_RECORD_SIGNATURE_SIZE, 1, fp))
			return ZIP_OPEN_FAILURE;
		printf ("cur_pos=%d, cur_val=%d\n", cur_pos, cur_val);
		if (fseek (fp, (-1)-ZIP_RECORD_SIGNATURE_SIZE, SEEK_CUR))
			return ZIP_OPEN_FAILURE;
		

	}
	return -999;	
}

char* zip_get_filename (struct zip_Object* obj, int n, char* dest) {
	return dest;
}

int zip_search_filename (struct zip_Object* obj, const char* fn) {
	return 0;
}

int zip_get_file_length (struct zip_Object* obj, int n) {
	return 0;
}

int zip_get_file (struct zip_Object* obj, int n, uchar* dest) {
	return 0;
}

int zip_error_code (struct zip_Object* obj) {
	return 0;
}

char* zip_error_name (int error_code, char* dest) {
	return dest;
}
