#include "comp.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

typedef unsigned long u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef struct key_value_pair pair;
struct key_value_pair {
	u32 key;
	u16 value;
};

int get_data_element (const u8*, u32*, int*, int);
int get_bit (const u8*, u32*, int*);
/* src, byte, bit within byte, # of bits to extract */
void initialize_pair_array (pair*, int);
int pair_cmp (const void*, const void*);
int key_cmp (const void*, const void*);

const int LITERAL_LENGTH_TREE_SIZE = 288;
const int DISTANCE_TREE_SIZE = 30;
#define CODE_LENGTH_TREE_SIZE 19

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
	u16 length_value;
	int bit;
	pair ll_tree[LITERAL_LENGTH_TREE_SIZE];
	pair d_tree[DISTANCE_TREE_SIZE];
	pair cl_tree[CODE_LENGTH_TREE_SIZE];
	u16 hlit, hdist, hclen;

	block_state = NEW_BLOCK;
	last_block_bool = 0;
	index = 0;
	length_value = 0;
	size = 0;
	bit = 0;
	while ((index < src_size && size < dest_size) && 
		!(last_block_bool && (NEW_BLOCK == block_state))) {
		/* (src and dest bounds check)       
		 * && (not end of last block)	*/
		if (block_state == NEW_BLOCK) {
			/* Read the first 3 bits to determine the compression coding */
			printf ("NEW_BLOCK, src[index]=%d, ", src[index]);
			last_block_bool = get_bit (src, &index, &bit);
			u8 btype;
			btype = get_data_element (src, &index, &bit, 2);
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
	
		else if (block_state == LOAD_DEFAULT_CODE_LENGTHS)
		{
			printf ("LOAD_DEFAULT_CODE_LENGTHS\n");
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
			printf ("BUILD_CODE_TREES\n");
			/* sort the code trees by code length, then literal value */
			qsort (ll_tree, LITERAL_LENGTH_TREE_SIZE, sizeof (ll_tree[0]), pair_cmp);
			qsort (d_tree, DISTANCE_TREE_SIZE, sizeof (d_tree[0]), pair_cmp);

			/* assign literal/length codes based on code length and order in array */
			u32 new_code, prev_length_bit;
			new_code = 0;
			prev_length_bit = 1;
			for (int i=0; i<LITERAL_LENGTH_TREE_SIZE; i++) {
				/* if the code length changes, bit shift code by delta length */
				if (ll_tree[i].key > prev_length_bit) {
					new_code *= (ll_tree[i].key / prev_length_bit);
					prev_length_bit = ll_tree[i].key;
				}
				/* if the code length bit is 1 or 0, remaining codes need not be filled */
				if (ll_tree[i].key <= 1)
					break;
				ll_tree[i].key |= new_code;
				new_code++;
			}

			/* assign distance codes */
			new_code = 0;
			prev_length_bit = 1;
			for (int i=0; i<DISTANCE_TREE_SIZE; i++) {
				if (d_tree[i].key > prev_length_bit) {
					new_code *= (d_tree[i].key / prev_length_bit);
					prev_length_bit = d_tree[i].key;
				}
				if (d_tree[i].key <= 1)
					break;
				d_tree[i].key |= new_code;
				new_code++;
			}

			printf ("Literal/Length Code-Value Pairs:\n");
			for (int i=0; i<LITERAL_LENGTH_TREE_SIZE; i++)
				printf ("\t%d\t%d\n", ll_tree[i].key, ll_tree[i].value);
			printf ("Distance Code-Value Pairs:\n");
			for (int i=0; i<DISTANCE_TREE_SIZE; i++)
				printf ("\t%d\t%d\n", d_tree[i].key, d_tree[i].value);
			
			/* go to next state */
			block_state = DECODE_DATA;
		}

		else if (block_state == DECODE_DATA)
		{
			printf ("DECODE_DATA...");
			pair* match;
			pair code;

			/* parse bitwise until a length/literal code is found */
			match = NULL;
			code.key = 1; /* 1 is the length indicator bit */
			while (!match && code.key < ULONG_MAX) {
				code.key <<= 1;
				code.key |= get_bit (src, &index, &bit);
				match = bsearch ((void*) &code, (void*) ll_tree, LITERAL_LENGTH_TREE_SIZE, sizeof (ll_tree[0]), key_cmp);	
			}

			/* if no match was found, then the data must be bad */
			if (!match)
				return size;

			/* take action and choose next state base on the literal/length value */
			if (match->value < 256) {
				printf ("%c\n", (char) match->value);
				dest[size++] = match->value;
				block_state = DECODE_DATA;
			}
			else if (match->value == 256) {
				block_state = NEW_BLOCK;
			}
			else {
				length_value = match->value;
				block_state = DECODE_DISTANCE;				
			}
		}

		else if (block_state == DECODE_DISTANCE)
		{
			printf ("distance!\n");
			break;
		}
		
		else if (block_state == READ_TREE_METADATA)
		{
			/* Read the remainder of the block header for dynamic coding */
			hlit = 257 + get_data_element (src, &index, &bit, 5);
			hdist = 1 + get_data_element (src, &index, &bit, 5);
			hclen = 4 + get_data_element (src, &index, &bit, 4);
			printf ("READ_TREE_METADATA, hlit=%d, hdist=%d, hclen=%d\n", hlit, hdist, hclen);

			block_state = READ_CODE_LENGTH_CODE_LENGTHS;
		}

		else if (block_state == READ_CODE_LENGTH_CODE_LENGTHS)
		{
			/* initialize code length literals in the code length tree */
			const u16 CL_LITERALS[CODE_LENGTH_TREE_SIZE] = {
				16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
			};
			for (int i=0; i<CODE_LENGTH_TREE_SIZE; i++) {
				cl_tree[i].key = 0;
				cl_tree[i].value = CL_LITERALS[i];
			}

			/* read code lengths; each is 3 bits */
			for (int i=0; i<hclen; i++) {
				int code_length;
				code_length = get_data_element (src, &index, &bit, 3);
				cl_tree[i].key = 1 << code_length;
			}

			/* go to next state */
			block_state = BUILD_CODE_LENGTH_CODE_TREE;
		}

		else if (block_state == BUILD_CODE_LENGTH_CODE_TREE)
		{
			printf ("BUILD_CODE_LENGTH_CODE_TREE\n");
			/* sort the code trees by code length, then literal value */
			qsort (cl_tree, CODE_LENGTH_TREE_SIZE, sizeof (cl_tree[0]), pair_cmp);

			/* assign literal/length codes based on code length and order in array */
			u32 new_code, prev_length_bit;
			new_code = 0;
			prev_length_bit = 1;
			for (int i=0; i<CODE_LENGTH_TREE_SIZE; i++) {
				/* if the code length changes, bit shift code by delta length */
				if (cl_tree[i].key > prev_length_bit) {
					new_code *= (cl_tree[i].key / prev_length_bit);
					prev_length_bit = cl_tree[i].key;
				}
				/* if the code length bit is 1 or 0, remaining codes need not be filled */
				if (cl_tree[i].key <= 1)
					break;
				cl_tree[i].key |= new_code;
				new_code++;
			}

			printf ("Code Length Tree, Code-Value Pairs:\n");
			for (int i=0; i<CODE_LENGTH_TREE_SIZE; i++)
				printf ("\t%d\t%d\n", cl_tree[i].key, cl_tree[i].value);
		
			/* go to next state */
			block_state = READ_LENGTH_LITERAL_CODE_LENGTHS;
		}

		else if (block_state == READ_LENGTH_LITERAL_CODE_LENGTHS)
		{
			initialize_pair_array (ll_tree, LITERAL_LENGTH_TREE_SIZE);
			/* need to read hlit number of codes */
			for (int i=0; i<hlit; i++) 
			{
				pair* match;
				pair code;

				/* parse bitwise until a code length code is found */
				match = NULL;
				code.key = 1; /* 1 is the length indicator bit */
				while (!match && code.key < ULONG_MAX) {
					code.key <<= 1;
					code.key |= get_bit (src, &index, &bit);
					match = bsearch ((void*) &code, (void*) cl_tree, CODE_LENGTH_TREE_SIZE, sizeof (cl_tree[0]), key_cmp);	
				}
	
				/* if no match was found, then the data must be bad */
				if (!match)
					return size;
	
				/* take action and choose next state base on the literal/length value */
				if (match->value < 16) {
					ll_tree[i].key = 1 << match->value;
				}
				else if (!i) {
					return 0; /* cannot repeat previous code lengths if i=0 */
				}
				else if (match->value == 16) {
					int repeat_length;
					repeat_length = 3 + get_bit (src, &index, &bit);
					for (int j=0; j<repeat_length; j++) {
						ll_tree[i].key = ll_tree[i-1].key;
						i++;
					}
				}
				else if (match->value == 17) {
					int repeat_length;
					repeat_length = 3 + get_data_element (src, &index, &bit, 3);
					for (int j=0; j<repeat_length; j++) {
						ll_tree[i].key = ll_tree[i-1].key;
						i++;
					}
				}
				else {
					int repeat_length;
					repeat_length = 11 + get_data_element (src, &index, &bit, 3);
					for (int j=0; j<repeat_length; j++) {
						ll_tree[i].key = ll_tree[i-1].key;
						i++;
					}
				}
				
				/* continue to next literal/length code */
			}	
			
			/* go to next state */
			block_state = READ_DISTANCE_CODE_LENGTHS;
		}

		else if (block_state == READ_DISTANCE_CODE_LENGTHS)
		{
			initialize_pair_array (d_tree, DISTANCE_TREE_SIZE);
			/* need to read hlit number of codes */
			for (int i=0; i<hdist; i++) 
			{
				pair* match;
				pair code;

				/* parse bitwise until a code length code is found */
				match = NULL;
				code.key = 1; /* 1 is the length indicator bit */
				while (!match && code.key < ULONG_MAX) {
					code.key <<= 1;
					code.key |= get_bit (src, &index, &bit);
					match = bsearch ((void*) &code, (void*) cl_tree, CODE_LENGTH_TREE_SIZE, sizeof (cl_tree[0]), key_cmp);	
				}
	
				/* if no match was found, then the data must be bad */
				if (!match)
					return size;
	
				/* take action and choose next state base on the literal/length value */
				if (match->value < 16) {
					d_tree[i].key = 1 << match->value;
				}
				else if (!i) {
					return 0; /* cannot repeat previous code lengths if i=0 */
				}
				else if (match->value == 16) {
					int repeat_length;
					repeat_length = 3 + get_data_element (src, &index, &bit, 2);
					for (int j=0; j<repeat_length; j++) {
						d_tree[i].key = d_tree[i-1].key;
						i++;
					}
				}
				else if (match->value == 17) {
					int repeat_length;
					repeat_length = 3 + get_data_element (src, &index, &bit, 3);
					for (int j=0; j<repeat_length; j++) {
						d_tree[i].key = d_tree[i-1].key;
						i++;
					}
				}
				else {
					int repeat_length;
					repeat_length = 11 + get_data_element (src, &index, &bit, 3);
					for (int j=0; j<repeat_length; j++) {
						d_tree[i].key = d_tree[i-1].key;
						i++;
					}
				}
				
				/* continue to next distance code */
			}	
			
			printf ("Distance Tree Code-Value Pairs:\n");
			for (int i=0; i<DISTANCE_TREE_SIZE; i++)
				printf ("\t%d\t%d\n", d_tree[i].key, d_tree[i].value);

			/* go to next state */
			block_state = BUILD_CODE_TREES;
		}
	
	}

	return size;
} 

int get_data_element (const u8* data_stream, u32* current_byte, int* current_bit, int number_of_bits) {

	int bit, byte, data_element;
	data_element = 0;
	for (int i=0; i<number_of_bits; i++) {
		byte = data_stream[*current_byte];
		bit = (byte & (1 << (*current_bit))) >> (*current_bit);
		data_element += bit << i;
		(*current_bit)++;
		(*current_byte) += (*current_bit) / 8;
		(*current_bit) %= 8;
	}
	return data_element;
}

int get_bit (const u8* data_stream, u32* current_byte, int* current_bit) {
	
	u8 bit, byte;
	byte = data_stream[*current_byte];
	bit = (byte & (1 << (*current_bit))) >> (*current_bit);
	printf ("byte=%d, (1 << (*current_bit))=%d, bit=%d\n", byte, (1 << (*current_bit)), bit);
	(*current_bit)++;
	(*current_byte) += (*current_bit) / 8;
	(*current_bit) %= 8;
	return bit;
}

void initialize_pair_array (pair* tree, int size) {
	for (int i=0; i<size; i++) {
		tree[i].key = 0;
		tree[i].value = i;
	}
}

int pair_cmp (const void* p1, const void* p2)
{
	/* compare the keys */
	int keys_cmp;
	keys_cmp = key_cmp (p1, p2);
	if (keys_cmp)
		return keys_cmp;
	else
	{	
		/* keys match, so compare values ASCII-alphabetically */
		u16 val1, val2;
		val1 = ((pair*) p1)->value;
		val2 = ((pair*) p2)->value;
		if (val1 > val2)
			return 1;
		else if (val1 < val2)
			return -1;
		else
			return 0;
	}
}

int key_cmp (const void* p1, const void* p2)
{
	/* make local copies of the keys */
	u32 key1, key2;
	key1 = ((pair*) p1)->key;
	key2 = ((pair*) p2)->key;
	
	/* compare keys */
	if (key1 < key2)
		return -1;
	else if (key1 > key2)
		return 1;
	else
		return 0;
}

