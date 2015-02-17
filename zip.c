#include "zip.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#define ZIP_MAXIMUM_NUMBER_OF_DISKS 10
#define ZIP_EOCDR_FIXED_PORTION_SIZE 22
#define ZIP_SIGNATURE_FIELD_SIZE 4
#define ZIP_LENGTH_FIELD_SIZE 2
#define ZIP_EOCDR_SIGNATURE 0x06054b50
#define ZIP_CDFH_FIXED_SIZE 46
#define ZIP_LFH_FIXED_SIZE

static u32 zip_get_field (FILE*, int); /* file stream, field size (bytes) */

/* Below are the fundamental states of the zip object. They can occur only
   in increasing order, but some state may be skipped. For example calling
   zip_constructor() followed by zip_append_new_file() causes a skip from
   ZIP_STATE_WITHOUT_FORM to ZIP_STATE_APPEND_FILES.
*/
#define ZIP_STATE_WITHOUT_FORM 0
#define ZIP_STATE_CENTRAL_DIRECTORY_COMPLETE 1

typedef struct zip_central_directory_file_header* cdfh;
struct zip_central_directory_file_header {
	u16 version;
	u16 bit_flag;
	u16 comp_method;
	u32 crc_32;
	u32 comp_size;
	u32 uncomp_size;
	u16 fnl, efl, fcl; /* file name length, extra field len, file comment len */
	u16 disk;
	u16 int_attr;
	u32 ext_attr;
	u32 offset;
	char* file_name;
	u8* extra_field;
	char* file_comment;
	cdfh next_cdfh, prev_cdfh;
};

struct zip_Object {
	int state;
	FILE* disks[ZIP_MAXIMUM_NUMBER_OF_DISKS];
	u32 disk_sizes[ZIP_MAXIMUM_NUMBER_OF_DISKS];
	int number_of_disks;
	cdfh central_dir;
	u16 total_cd_entries;
	char* zip_file_comment;
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
	obj_ptr->central_dir = NULL;
	obj_ptr->total_cd_entries = 0;
	obj_ptr->zip_file_comment = NULL;
}

void zip_destructor (struct zip_Object** ptr_ptr) {

	struct zip_Object* obj_ptr = *ptr_ptr;
	
	/* deallocate the central_dir linked list */
	if (obj_ptr->central_dir) {
		cdfh current, next;
		current = obj_ptr->central_dir;
		while (current) {
			next = current->next_cdfh;
			free ((current->file_name));
			free ((current->extra_field));
			free ((current->file_comment));
			free (current);
			current = next;
		}
	}

	/* deallocate the zip file comment */
	free (obj_ptr->zip_file_comment);

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

	/* check that the EOCDR was found */
	if (!eocdr_pos)
		return ZIP_OPEN_NEED_ADDITIONAL_DISK;
	else
		obj->state = ZIP_STATE_CENTRAL_DIRECTORY_COMPLETE;

	/* EOCDR is found, parse it */
	u32 signature;
	u16 this_disk;
	u16 start_disk;
	u16 this_disk_entries;
	u16 tot_entries;
	u32 cd_size;
	u32 cd_offset;
	u16 fc_length;
	
	if (fseek (fp, eocdr_pos, SEEK_SET))
		return ZIP_OPEN_FAILURE;

	signature = zip_get_field (fp, ZIP_SIGNATURE_FIELD_SIZE);
	this_disk = zip_get_field (fp, 2);
	start_disk = zip_get_field (fp, 2);
	this_disk_entries = zip_get_field (fp, 2);
	tot_entries = zip_get_field (fp, 2);
	cd_size = zip_get_field (fp, 4);
	cd_offset = zip_get_field (fp, 4);
	fc_length = zip_get_field (fp, 2);

	obj->zip_file_comment = malloc (fc_length);
	if (!obj->zip_file_comment)
		return ZIP_OPEN_FAILURE;
	if (fc_length != fread ((void*) obj->zip_file_comment, 1, fc_length, fp))
		return ZIP_OPEN_FAILURE;
	obj->zip_file_comment[fc_length] = 0;

	/* Go to the start of CD Central Directory */
	if (start_disk >= obj->number_of_disks)
		return ZIP_OPEN_FAILURE;
	if (cd_offset >= obj->disk_sizes[start_disk] - ZIP_CDFH_FIXED_SIZE)
		return ZIP_OPEN_FAILURE;
	if (fseek (obj->disks[start_disk], cd_offset, SEEK_SET))
		return ZIP_OPEN_FAILURE;

	/* Parse the Central Directory File Headers */
	u32 disk;
	cdfh temp, prev_temp;
	disk = start_disk;
	temp = NULL;
	for (u16 i=0; i<tot_entries; i++) {

		/* save the prior header for building a linked list */
		prev_temp = temp;

		/* determine if there is enough information on current disk */
		if (disk >= obj->number_of_disks)
			return ZIP_OPEN_FAILURE;
		if (ftell (obj->disks[disk]) >= obj->disk_sizes[disk] - ZIP_CDFH_FIXED_SIZE)
			return ZIP_OPEN_FAILURE;

		/* if a CDFH Signature is found, allocate a new CDFH structure */
		signature = zip_get_field (obj->disks[disk], ZIP_SIGNATURE_FIELD_SIZE);
		if (signature != 0x02014b50)
			return ZIP_OPEN_FAILURE;
		temp = malloc (sizeof (struct zip_central_directory_file_header));
		if (!temp) {
			printf ("memory allocation error.\n");
			exit (EXIT_FAILURE);
		}

		/* read the fields from disk into the CDFH structure */
		u16 dummy;
		dummy = zip_get_field (obj->disks[disk], 2);
		temp->version = zip_get_field (obj->disks[disk], 2);
		temp->bit_flag = zip_get_field (obj->disks[disk], 2);
		temp->comp_method = zip_get_field (obj->disks[disk], 2);
		dummy = zip_get_field (obj->disks[disk], 2);
		dummy = zip_get_field (obj->disks[disk], 2);
		temp->crc_32 = zip_get_field (obj->disks[disk], 4);
		temp->comp_size = zip_get_field (obj->disks[disk], 4);
		temp->uncomp_size = zip_get_field (obj->disks[disk], 4);
		temp->fnl = zip_get_field (obj->disks[disk], 2);
		temp->efl = zip_get_field (obj->disks[disk], 2);
		temp->fcl = zip_get_field (obj->disks[disk], 2);
		temp->disk = zip_get_field (obj->disks[disk], 2);
		temp->int_attr = zip_get_field (obj->disks[disk], 2);
		temp->ext_attr = zip_get_field (obj->disks[disk], 4);
		temp->offset = zip_get_field (obj->disks[disk], 4);
		if (!(temp->file_name = malloc (temp->fnl + 1)))
			printf ("memory allocation error.\n"), exit (EXIT_FAILURE);
		if (temp->fnl != fread (temp->file_name, 1, temp->fnl, obj->disks[disk]))
			return ZIP_OPEN_FAILURE;
		temp->file_name[temp->fnl] = 0;
		if (!(temp->extra_field = malloc (temp->efl + 1)))
			printf ("memory allocation error.\n"), exit (EXIT_FAILURE);
		if (temp->efl != fread (temp->extra_field, 1, temp->efl, obj->disks[disk]))
			return ZIP_OPEN_FAILURE;
		temp->extra_field[temp->efl] = 0;
		if (!(temp->file_comment = malloc (temp->fcl + 1)))
			printf ("memory allocation error.\n"), exit (EXIT_FAILURE);
		if (temp->fcl != fread (temp->file_comment, 1, temp->fcl, obj->disks[disk]))
			return ZIP_OPEN_FAILURE;
		temp->file_comment[temp->fcl] = 0;

		/* if there is no more information on this disk, increment disks */
		if (ftell (obj->disks[disk]) >= obj->disk_sizes[disk] - ZIP_CDFH_FIXED_SIZE)
			disk++;


		/* append the temporary CDFH to the central_dir linked list */
		if (!obj->central_dir) {
			temp->prev_cdfh = NULL;
			obj->central_dir = temp;
			temp->next_cdfh = NULL;
			obj->total_cd_entries++;
		}
		else {
			temp->prev_cdfh = prev_temp;
			prev_temp->next_cdfh = temp;
			temp->next_cdfh = NULL;
			obj->total_cd_entries++;
		}
		
		/* continue to the next Central Directory File Header */
	}

	return ZIP_OPEN_SUCCESS;
}

char* zip_get_filename (struct zip_Object* obj, int n, char* dest, int size) {

	if (n >=  obj->total_cd_entries) {
		return NULL;
	}
	else {
		cdfh header_n;
		header_n = obj->central_dir;
		for (int i=0; i<n; i++)
			header_n = header_n->next_cdfh;

		strncpy (dest, header_n->file_name, size);
		dest[size-1] = 0;		
		return dest;
	}
}

int zip_search_filename (struct zip_Object* obj, const char* fn) {
	
	cdfh header_i;
	header_i = obj->central_dir;
	for (int i=0; i< obj->total_cd_entries; i++) {
		if (!strcmp (fn, header_i->file_name))
			return i;
		else
			header_i = header_i->next_cdfh;
	}
	return -1;
}

unsigned long zip_get_file_length (struct zip_Object* obj, int n) {
	
	if (n >= obj->total_cd_entries) {
		return 0;
	}
	else {
		cdfh header_n;
		header_n = obj->central_dir;
		for (int i=0; i<n; i++)
			header_n = header_n->next_cdfh;
		return header_n->uncomp_size;
	}
}

u32 zip_get_file (struct zip_Object* obj, int n, u8** dest_ptr) {
	
	if (*dest_ptr) {
		fprintf (stderr, "ERROR: zip_get_file() allocates memory for the destination; ");
		fprintf (stderr, "the destination parameter MUST be NULL.\n");
		exit (EXIT_FAILURE);	
	}
	
	/* check that n is in bounds and state is Ok */
	if (obj->state != ZIP_STATE_CENTRAL_DIRECTORY_COMPLETE) {
		printf ("zip_get_file() error: central directory is not complete;");
		printf (" it cannot be parsed.\n");
		exit (EXIT_FAILURE);
	}
	if (n >= obj->total_cd_entries)
		return 0;

	/* make local copies of relevent variables */
	cdfh cdfh_n;
	FILE* fstream;
	u32 uncomp_size, comp_size, offset;
	u16 disk, comp_method, fnl, efl;

	cdfh_n = obj->central_dir;
	for (int i=0; i<n; i++)
		cdfh_n = cdfh_n->next_cdfh;
	fstream = obj->disks[cdfh_n->disk];
	uncomp_size = cdfh_n->uncomp_size;
	comp_size = cdfh_n->comp_size;
	offset = cdfh_n->offset;
	disk = cdfh_n->disk;
	comp_method = cdfh_n->comp_method;
	printf ("uncomp_size=%d, comp_size=%d, offset=%d, comp_method=%d\n", uncomp_size, comp_size, offset, comp_method);
	if (!uncomp_size || !comp_size)
		return 0;

	/* parse the local file header to check for consistency */
	fnl = cdfh_n->fnl;
	efl = cdfh_n->efl;	

	/* position the file pointer to start of compressed data */
	if (disk >= obj->number_of_disks)
		return 0;
	if (obj->disk_sizes[disk] <= (offset + ZIP_LFH_FIXED_SIZE + fnl + efl)) {
		printf ("failing here.\n");
		return 0;
	}
	if (fseek (fstream, (offset + ZIP_LFH_FIXED_SIZE + fnl + efl), SEEK_SET))
		return 0;

	/* allocate destination buffer */
	*dest_ptr = malloc (uncomp_size);
	if (!(*dest_ptr)) {
		printf ("ERROR: memory allocation failed in zip_get_file().\n");
		exit (EXIT_FAILURE);
	}

	/* decompress the data */
	if (comp_method == ZIP_APPEND_NO_COMPRESSION) {

	}
	else if (comp_method == ZIP_APPEND_DEFLATE_COMPRESSION) {

	}
	else {
		free (*dest_ptr);
		*dest_ptr = NULL;
		return 0;
	}
	

	return uncomp_size;
}

void zip_remove_file (struct zip_Object* obj, int n) {

}

void zip_append_file (
	struct zip_Object* obj,
	const unsigned char* raw,
	int raw_size,
	int comp_method
) {

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
