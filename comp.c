#include "comp.h"
#include <stdlib.h>
#include <stdio.h>

typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef struct key_value_pair* pair;
struct key_value_pair {
	u32 key;
	u16 value;
};

u8 get_bits (const u8*, u32*, u8*, u8);

const int NEW_BLOCK = 0;
const int GET_BLOCK_LENGTH = 1;						/* jump here if no compression */
const int COPY_BLOCK_TO_DEST = 2;
const int LOAD_DEFAULT_CODE_LENGTHS= 3;				/* jump here if fixed Huffman coding */
const int BUILD_CODE_TREES = 4;		
const int DECODE_DATA = 5;
const int DECODE_DISTANCE = 6;
const int READ_TREE_METADATA = 7;					/* jump here if dynamic Huffman coding */
const int READ_CODE_LENGTH_CODE_LENGTHS = 9;
const int BUILD_CODE_LENGTH_CODE_TREE = 10;
const int READ_LENGTH_LITERAL_CODE_LENGTHS = 11;
const int READ_DISTANCE_CODE_LENGTHS = 12;

int comp_inflate (u8* dest, int dest_size, const u8* src, int src_size) {

	int block_state, last_block_bool;
	u32 index, size; /* index into source and size written to dest */
	u8 bit;

	block_state = NEW_BLOCK;
	last_block_bool = 0;
	index = 0;
	size = 0;
	bit = 0;
	while ((index < src_size && size < dest_size) && 
		!(last_block_bool && (NEW_BLOCK == block_state))) {
		/* (src and dest bounds check)       
		 * && (not end of last block)	*/
		if (block_state == NEW_BLOCK) {
			printf ("NEW_BLOCK, src[index]=%d, ", src[index]);
			last_block_bool = get_bits (src, &index, &bit, 1);
			u8 btype;
			btype = get_bits (src, &index, &bit, 2);
			printf ("last_block_bool=%d, btype=%d\n", last_block_bool, btype);
			if (btype == 0)
				block_state = GET_BLOCK_LENGTH;
			else if (btype == 1)
				block_state = LOAD_DEFAULT_CODE_LENGTHS;
			else if (btype == 2)
				block_state = READ_TREE_METADATA;
			else {
				fprintf (stderr, "Error: deflated file has bad BTYPE.\n");
				return -1;
			}
		}
		else if (block_state == LOAD_DEFAULT_CODE_LENGTHS) {
			
			break;
		}
	}

	return dest_size;
} 

u8 get_bits (const u8* data_stream, u32* current_byte, u8* current_bit, u8 number_of_bits) {
	u8 value;
	value = 0;
	for (int i=0; i<number_of_bits; i++) {
		value << 1;
		value |= (data_stream[*current_byte] & (1 << (*current_bit & 8))) >> (*current_bit & 8);
		(*current_bit)++;
		(*current_byte) += !((*current_bit) & 8) ? 1 : 0;
	}
	return value;
}
