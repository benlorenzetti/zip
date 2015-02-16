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

/* Below are the fundamental state of a zip object. They can occur only
   in increasing order, but some state may be skipped. For example calling
   zip_constructor() followed by zip_append_new_file() causes a skip from
   ZIP_STATE_WITHOUT_FORM to ZIP_STATE_APPEND_FILES.
*/
#define ZIP_STATE_WITHOUT_FORM 0
#define ZIP_STATE_CENTRAL_DIRECTORY_COMPLETE 1

struct zip_Object {
	int state;
	FILE* disks[ZIP_MAXIMUM_NUMBER_OF_DISKS];
	u32 disk_sizes[ZIP_MAXIMUM_NUMBER_OF_DISKS];
	int number_of_disks;	
};

void zip_constructor (struct zip_Object** ptr_ptr) {

	/* allocate the object in memory */
	*ptr_ptr = (struct zip_Object*) malloc (sizeof (struct zip_Object));
	struct zip_Object* obj_ptr = *ptr_ptr;
	if (!obj_ptr)
		printf ("memory allocation failure\n"), exit (EXIT_FAILURE);

	/* initialize variables */
	obj_ptr->state = ZIP_STATE_WITHOUT_FORM;
	obj_ptr->number_of_disks = 0;
}

void zip_destructor (struct zip_Object** ptr_ptr) {

	struct zip_Object* obj_ptr = *ptr_ptr;
	
	/* close all open disks */
	for (int i=0; i< obj_ptr->number_of_disks; i++) {
		fclose (obj_ptr->disks[i]);
	}
	
	/* free the object from memory */
	free (obj_ptr);
}

/* return ZIP_OPEN_SUCCESS, ZIP_OPEN_NEED_ADDITIONAL_DISK, or ZIP_OPEN_FAILURE */
int zip_open_disk (struct zip_Object* obj, const char* fn) {

	/* cannot load any more disks after CD found */
	if (obj->state != ZIP_STATE_WITHOUT_FORM) {
		printf ("zip_open_disk() error: object already has Central Directory\n");
		printf ("(you must open disks in order)\n");
		exit (EXIT_FAILURE);
	}	
	
	/* open file and determine its size */
	FILE* fp;
	char mode[] = "rb";
	fp = fopen (fn, mode);
	if (!fp) {
		return ZIP_OPEN_FAILURE;
	}
	else if (fseek (fp, 0, SEEK_END)) {
		return ZIP_OPEN_FAILURE;
	}
	else {
		obj->disks[obj->number_of_disks] = fp;
		obj->disk_sizes[obj->number_of_disks] = ftell (fp);
		obj->number_of_disks++;
	}

	/* attempt to find the End of Central Directory Record */
	u32 eocdr_pos, cur_pos, cur_val;

	if (fseek (fp, (-1)*ZIP_EOCDR_FIXED_PORTION_SIZE, SEEK_END))
		return ZIP_OPEN_FAILURE;

	eocdr_pos = 0;
	cur_pos = ftell (fp);
	do {
		cur_val = zip_get_field (fp, ZIP_SIGNATURE_FIELD_SIZE);
		cur_pos += ZIP_SIGNATURE_FIELD_SIZE;

		if (cur_val == ZIP_EOCDR_SIGNATURE) {
			/* possible match, verify with comment length field. */
			u32 pos_pos, fcl_offset, fcl;
			pos_pos = cur_pos - ZIP_SIGNATURE_FIELD_SIZE;
			fcl_offset = ZIP_EOCDR_FIXED_PORTION_SIZE - ZIP_SIGNATURE_FIELD_SIZE - ZIP_LENGTH_FIELD_SIZE;

			if (fseek (fp, fcl_offset, SEEK_CUR))
				return ZIP_OPEN_FAILURE;
			cur_pos += fcl_offset;

			fcl = zip_get_field (fp, ZIP_LENGTH_FIELD_SIZE);
			cur_pos += ZIP_LENGTH_FIELD_SIZE;

			if (obj->disk_sizes[obj->number_of_disks-1] == cur_pos) {
				eocdr_pos = pos_pos;
				break;
			}
			else
				cur_pos = pos_pos;
		}
		else {
			/* if not a match, then back up to a new starting position and continue */
			if (fseek (fp, (-1)-ZIP_SIGNATURE_FIELD_SIZE, SEEK_CUR))
				return ZIP_OPEN_FAILURE;
			cur_pos -= (-1) - ZIP_SIGNATURE_FIELD_SIZE;
		}
	} while (cur_pos >= 0);

	if (!eocdr_pos)
		return ZIP_OPEN_NEED_ADDITIONAL_DISK;
	else
		obj->state = ZIP_STATE_CENTRAL_DIRECTORY_COMPLETE;

	printf ("eocdr_pos=%d\n", eocdr_pos);

	/* EOCDR is found, now parse CD records */
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
