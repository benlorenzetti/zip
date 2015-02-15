#include "zip.h"
#include <stdio.h>
#include <stdlib.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#define ZIP_MAXIMUM_NUMBER_OF_DISKS 10
#define ZIP_EOCDR_FIXED_PORTION_SIZE 22
#define ZIP_SIGNATURE_FIELD_SIZE 4
#define ZIP_LENGTH_FIELD_SIZE 2
#define ZIP_EOCDR_SIGNATURE 0x06054b50

static u32 zip_get_field (FILE*, int); /* file stream, field size (bytes) */

#define ZIP_STATE_CONSTRUCTED 0
#define ZIP_STATE_ALL_DISKS_PARSED 1

struct zip_Object {
	int state;
	FILE* disk_array[ZIP_MAXIMUM_NUMBER_OF_DISKS];
	int disk_array_size;	
};

void zip_constructor (struct zip_Object** ptr_ptr) {
	puts ("constructor...");

	/* allocate the object in memory */
	*ptr_ptr = (struct zip_Object*) malloc (sizeof (struct zip_Object));
	struct zip_Object* obj_ptr = *ptr_ptr;
	if (!obj_ptr)
		printf ("memory allocation failure\n"), exit (EXIT_FAILURE);

	/* initialize variables */
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
	
	/* open file */
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

	if (fseek (fp, (-1)*ZIP_EOCDR_FIXED_PORTION_SIZE, SEEK_END))
		return ZIP_OPEN_FAILURE;
	cur_pos = file_size - ZIP_EOCDR_FIXED_PORTION_SIZE;

	while (cur_pos >= 0) {
		cur_val = zip_get_field (fp, ZIP_SIGNATURE_FIELD_SIZE);
		if (cur_val == ZIP_EOCDR_SIGNATURE) {
			/* possible match, verify with comment length field. */
			u32 offset, file_comment_length;

			offset = ZIP_EOCDR_FIXED_PORTION_SIZE - ZIP_SIGNATURE_FIELD_SIZE - ZIP_LENGTH_FIELD_SIZE;
			if (fseek (fp, offset, SEEK_CUR))
				return ZIP_OPEN_FAILURE;
			file_comment_length = zip_get_field (fp, ZIP_LENGTH_FIELD_SIZE);

			if (file_size == (cur_pos + file_comment_length + ZIP_EOCDR_FIXED_PORTION_SIZE))
				break;
		}
		else {
			/* if not a match, then back up to a new starting position and continue */
			cur_pos--;
			if (fseek (fp, (-1)-ZIP_SIGNATURE_FIELD_SIZE, SEEK_CUR))
				return ZIP_OPEN_FAILURE;
		}
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

int zip_get_file (struct zip_Object* obj, int n, u8* dest) {
	return 0;
}

int zip_error_code (struct zip_Object* obj) {
	return 0;
}

char* zip_error_name (int error_code, char* dest) {
	return dest;
}

static u32 zip_get_field (FILE* fp, int field_size) {
	u8 field[8];
	u32 concat;
	if (field_size > 8) {
		printf ("zip_get_field() error: parameter field_size > 8.\n");
		exit (EXIT_FAILURE);
	}
	if (field_size != fread (field, 1, field_size, fp)) {
		return 9999999999;
	}
	concat = 0;
	for (int i=0; i<field_size; i++) {
		concat |= (field[i] << 8*i);
	}
	return concat;
}
