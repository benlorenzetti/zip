#include "comp.h"
#include <stdlib.h>
#include <stdio.h>

typedef unsigned long u32;
typedef unsigned char u8;

typedef struct key_value_pair* pair;
struct key_value_pair {
	u32 key;
	u8 value;
};

int comp_inflate (u8* dest, int dest_size, FILE* src, int src_size) {
	
	return dest_size;
} 
