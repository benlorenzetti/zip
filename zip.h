#ifndef ZIP_H
#define ZIP_H
/******************************************************************************
 *                                                                            *
 ******************************************************************************/

typedef struct zip_Object* zip_object;

void zip_constructor (zip_object*);
void zip_destructor (zip_object*);

#define ZIP_OPEN_SUCCESS 1
#define ZIP_OPEN_NEED_ADDITIONAL_DISK -1
#define ZIP_OPEN_FAILURE 0
int zip_open_disk (zip_object, const char*);

#define ZIP_MAX_FILENAME_LENGTH 1000
char* zip_get_filename (zip_object, int, char*, int);                         /*
      @param: n, the local file number
      @param: destination (which is also returned)
      @param: size of destination                                             */

int zip_search_filename (zip_object, const char*);                            /*
      return: local file number n, or -1 if filename not found                */

unsigned long zip_get_file_length (zip_object, int);                          /*
      @param: n, the local file number.                                       */

unsigned long zip_get_file (zip_object, int, unsigned char**);                /*
      @param: n, the local file number
      @param: destination ptr - destination will be malloc()ated
      return: size allocated                                                  */

unsigned long zip_get_file_raw (zip_object, int, unsigned char**);            /*
      @param: n, the local file number
      @param: dest ptr; dest will be allocated but caller must free
      return: size allocated                                                  */

void zip_remove_file (zip_object, int);                                       /*
      @param: n, the local file number                                        */

#define ZIP_APPEND_NO_COMPRESSION 0
#define ZIP_APPEND_DEFLATE_COMPRESSION 8
void zip_append_file (zip_object, const unsigned char*, int, int);            /*
      @param: raw data to be appended
      @param: size of raw data
      @param: compression method                                              */

int   zip_error_code (zip_object);
char* zip_error_name (int, char*);


#endif
