/******************************************************************************
 *                                                                            *
 ******************************************************************************/

struct zip_Object;
typedef zip_Object* zip_object;

void zip_constructor (zip_object);
void zip_destructor (zip_object);

int zip_open_disk (zip_object, const char*);                                  /*
      return: 0 upon error, -1 if more disks are needed, or 1 on success.     */

#define ZIP_MAX_FILENAME_LENGTH 1000
char* zip_get_filename (zip_object, int, char*);                              /*
      @param: n, the local file number
      @param: destination (which is also returned)                            */

int zip_search_filename (zip_object, const char*);                            /*
      return: local file number n, or 0 if filename not found.                */

int zip_get_file_length (zip_object, int);                                    /*
      @param: n, the local file number.                                       */

int  zip_get_file (zip_object, int, unsigned char*);                          /*
      @param: n, the local file number
      @param: destination
      return: size                                                            */

int   zip_error_code (zip_object);
char* zip_error_name (int, char*);

