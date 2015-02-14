#include "zip.h"
#include <stdio.h>

typedef unsigned char uchar;
typedef unsigned short u16;
typedef unsigned long u32;

struct zip_Object {
	
};

void zip_constructor (struct zip_Object* obj) {
	puts ("constructor...\n");
}

void zip_destructor (struct zip_Object* obj) {
	puts ("destructor...\n");
}

int zip_open_disk (struct zip_Object* obj, const char* fn) {
	return 0;
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


