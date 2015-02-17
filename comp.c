#include "comp.h"
#include <stdlib.h>
#include <stdio.h>

typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef struct key_value_pair pair;
struct key_value_pair {
	u32 key;
	u16 value;
};

u8 get_bits (const u8*, u32*, u8*, u8);
void initialize_pair_array (pair*, int);
int pair_cmp (const void*, const void*);

const int LITERAL_LENGTH_TREE_SIZE = 288;
const int DISTANCE_TREE_SIZE = 30;

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
	pair ll_tree[LITERAL_LENGTH_TREE_SIZE];
	pair d_tree[DISTANCE_TREE_SIZE];

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
			/* Read the first 3 bits to determine the compression coding */
			printf ("NEW_BLOCK, src[index]=%d, ", src[index]);
			last_block_bool = get_bits (src, &index, &bit, 1);
			u8 btype;
			btype = get_bits (src, &index, &bit, 2);
			printf ("last_block_bool=%d, btype=%d\n", last_block_bool, btype);
			/* go to next state based on compression type */
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
			/* reset the code trees with 0 codes */
			initialize_pair_array (ll_tree, LITERAL_LENGTH_TREE_SIZE);
			initialize_pair_array (d_tree, DISTANCE_TREE_SIZE);

			/* Load the fixed code lengths listed on page 11 of the Spec. */
			for (int i=0; i<144; i++)
				ll_tree[i].key = (1 << 8);
			for (int i=144; i<256; i++)
				ll_tree[i].key = (1 << 9);
			for (int i=256; i<280; i++)
				ll_tree[i].key = (1 << 7);
			for (int i=280; i<288; i++)
				ll_tree[i].key = (1 << 8);
			for (int i=0; i<DISTANCE_TREE_SIZE; i++)
				d_tree[i].key = (1 << 5);			

			/* go to next state */
			block_state = BUILD_CODE_TREES;
		}

		else if (block_state == BUILD_CODE_TREES)
		{
			/* sort the code trees by code length, then literal value */
			qsort (ll_tree, LITERAL_LENGTH_TREE_SIZE, sizeof (ll_tree[0]), pair_cmp);
			qsort (d_tree, DISTANCE_TREE_SIZE, sizeof (d_tree[0]), pair_cmp);

			/* assign codes based on code length and order in array */
			u32 new_code, prev_length_bit;
			new_code = 0;
			prev_length_bit = 1;
			for (int i=0; i<LITERAL_LENGTH_TREE_SIZE; i++) {
				/* if the code length changes, bit shift code by delta length */
				if (ll_tree[i].key > prev_length_bit) {
					new_code *= (ll_tree[i].key / prev_length_bit);
					prev_length_bit = ll_tree[i].key;
				}
				ll_tree[i].key |= new_code;
				new_code++;
			}

			new_code = 0;
			prev_length_bit = 1;
			for (int i=0; i<DISTANCE_TREE_SIZE; i++) {
				if (d_tree[i].key > prev_length_bit) {
					new_code *= (ll_tree[i].key / prev_length_bit);
					prev_length_bit = d_tree[i].key;
				}
				d_tree[i].key |= new_code;
				new_code++;
			}

			printf ("Literal/Length Code-Value Pairs:\n");
			for (int i=0; i<LITERAL_LENGTH_TREE_SIZE; i++)
				printf ("\t%d\t%d\n", ll_tree[i].key, ll_tree[i].value);
			printf ("Distance Code-Value Pairs:\n");
			for (int i=0; i<DISTANCE_TREE_SIZE; i++)
				printf ("\t%d\t%d\n", d_tree[i].key, d_tree[i].value);
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

void initialize_pair_array (pair* tree, int size) {
	for (int i=0; i<size; i++) {
		tree[i].key = 0;
		tree[i].value = i;
	}
}

int pair_cmp (const void* p1, const void* p2)
{
	/* make local copies of the pairs */
	u32 key1, key2;
	u16 val1, val2;
	key1 = ((pair*) p1)->key;
	key2 = ((pair*) p2)->key;
	val1 = ((pair*) p1)->value;
	val2 = ((pair*) p2)->value;

	/* keys with no length are not used--sort them to the back of arrays */
	if (key1 == 1 || key1 == 0)
		return 1;
	if (key2 == 1 || key2 == 0)
		return -1;

	/* find the first key-length indicator bit */
	while (key1 > 0 && key2 > 0) {
		key1 >>= 1;
		key2 >>= 1;
	}

	/* compare base on key length, then value numerically */
	if (key1 > key2)
		return 1;
	else if (key1 < key2)
		return -1;
	else if (val1 > val2)
		return 1;
	else if (val1 < val2)
		return -1;
	else
		return 0;
}

