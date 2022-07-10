/**************************************************************************/
/*              COPYRIGHT Carnegie Mellon University 2021                 */
/* Do not post this file or any derivative on a public site or repository */
/**************************************************************************/
/* Huffman coding
 *
 * 15-122 Principles of Imperative Computation
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "lib/contracts.h"
#include "lib/xalloc.h"
#include "lib/file_io.h"
#include "lib/heaps.h"

#include "freqtable.h"
#include "htree.h"
#include "encode.h"
#include "bitpacking.h"

// Print error message
void error(char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(1);
}

/* in htree.h: */
/* typedef struct htree_node htree; */
struct htree_node {
  symbol_t value;
  unsigned int frequency;
  htree *left;
  htree *right;
};

/**********************************************/
/* Task 1: Checking data structure invariants */
/**********************************************/

/* forward declaration -- DO NOT MODIFY */
bool is_htree(htree *H);

bool is_htree_leaf(htree *H) {
  if (H == NULL || H->frequency == 0 || H->right != NULL || H->left != NULL){
    return false;
  }
  return true;
}

bool is_htree_interior(htree *H) {
  if (H == NULL) return false;
  unsigned int own = H->frequency;
  if (H->left == NULL || H->right == NULL) return false;
  unsigned int left = H->left->frequency;
  unsigned int right = H->right->frequency;
  if (!is_htree(H->left) || !is_htree(H->right) || own != left + right){
    return false;
  }

  return true;
}

bool is_htree(htree *H) {
  if (H == NULL || (!is_htree_interior(H) && !is_htree_leaf(H)) ) return false;
  return true;
}


/********************************************************/
/* Task 2: Building Huffman trees from frequency tables */
/********************************************************/

/* for lib/heaps.c */
bool htree_higher_priority(elem e1, elem e2) {
  htree *first = (htree *)e1;
  htree *second = (htree *)e2;
  if (first->frequency < second->frequency) return true;
  return false;
}

// build a htree from a frequency table
htree* build_htree(freqtable_t table) {
  // case where there are less than two non-zero frequency symbols
  int count = 0;
  for (int b = 0; b < NUM_SYMBOLS; b++){
    if (table[b] != 0) count++;
  }
  // if less than two symbols at end, trigger error
  char *msg = "less than two non-zero frequency symbols!\0";
  if (count < 2) error(msg);

  // initialize the priority queue
  pq_t Q = pq_new(NUM_SYMBOLS, &htree_higher_priority, NULL);
  for (int a = 0; a < NUM_SYMBOLS; a++){
    if (table[a] != 0){
      htree *new = xcalloc(1, sizeof(htree));
      new->frequency = table[a];
      new->value = a;
      new->right = NULL;
      new->left = NULL;
      pq_add(Q, (elem)new);
    }
  }

  htree * first = NULL;
  // get things off of the queue and merge them into one
  while (!pq_empty(Q)){
    // get the first thing off the queue
    first = (htree *)(pq_rem(Q));
    // if queue not empty, get second thing off
    if (!pq_empty(Q)){
      htree * second = (htree *)(pq_rem(Q));
      // build an inner node
      htree *T = xcalloc(1, sizeof(htree));
      T->frequency = first->frequency + second->frequency;
      T->value = 0;
      T->right = second;
      T->left = first;
      // add the node back in queue
      pq_add(Q, (elem)(T));
    }
  }
  // free the queue
  pq_free(Q);
  // return the thing
  return first;
}


/*******************************************/
/*  Task 3: decoding a text                */
/*******************************************/

// Decode code according to H, putting decoded length in src_len
symbol_t* decode_src(htree *H, bit_t *code, size_t *src_len) {
  REQUIRES(is_htree(H));
  //char *msg = "code comes to end but not reaching a leaf!\0";

  // change src_len to actual length
  htree *root = H;
  htree *cur = H;
  int len = strlen(code);
  size_t count = 0;
  
  if (src_len == NULL){
    src_len = xcalloc(1, sizeof(size_t));
  }

  // get the length of the result
  for (int i = 0; i < len; i++){
    if (code[i] == '1') cur = cur->right;
    if (code[i] == '0') cur = cur->left;
    if (is_htree_leaf(cur)){
      count++;
      cur = root;
    }
  }
  *src_len = count;

  // populate the thing
  htree *now = root;
  symbol_t *res = xcalloc((*src_len),sizeof(symbol_t));
  int str_i = 0;
  for (int j = 0; j < len; j++){
    if (code[j] == '1') now = now->right;
    if (code[j] == '0') now = now->left;
    if (is_htree_leaf(now)){
      res[str_i] = now->value;
      str_i++;
      now = root;
    }
  }
  return res;
}



/***************************************************/
/* Task 4: Building code tables from Huffman trees */
/***************************************************/

void helper(htree *H, bit_t* B, codetable_t T, int len){

  // when leaf put in the codetable
  if (is_htree_leaf(H)){
    T[H->value] = B;
    //free(B);
  }
  else{
    // goes left
    bit_t *D = xcalloc(len+2, sizeof(bit_t));
    strcpy(D, B);
    // add zero if goes left
    (bit_t*)strcat(D, "0");
    helper(H->left, D, T, len+2);

    // goes right
    bit_t *E = xcalloc(len+2, sizeof(bit_t));
    strcpy(E, B);
    // add one if goes right
    (bit_t*)strcat(E, "1");
    helper(H->right, E, T, len+2);
    free(B);
  }
}

// Returns code table for characters in H
codetable_t htree_to_codetable(htree *H) {
  REQUIRES(is_htree(H));
  codetable_t T = xcalloc(NUM_SYMBOLS, sizeof(bitstring_t));
  // get a array on the stack
  bit_t *B = xcalloc(1, sizeof(bit_t));
  helper(H, B, T, 0);
  ENSURES(is_htree(H));

  return T;
}


/*******************************************/
/*  Task 5: Encoding a text                */
/*******************************************/

// Encodes source according to codetable
bit_t* encode_src(codetable_t table, symbol_t *src, size_t src_len) {

  // initialize the bit_t array length
  int bit_len = 0;
  for (size_t i = 0; i < src_len; i++){
    bit_len += (int)strlen(table[src[i]]);
  }

  // initialize a bit_t array with suitable size
  bit_t *res = xcalloc(bit_len+1, sizeof(bit_t));

  // offset is the updated array length
  int b_i = 0;
  for (size_t j = 0; j < src_len; j++){
    strcpy(res+b_i, table[src[j]]);
    b_i += strlen(table[src[j]]);
  }
  return res;
}


/**************************************************/
/*  Task 6: Building a frequency table from file  */
/**************************************************/

// Build a frequency table from a source file (or STDIN)
freqtable_t build_freqtable(char *fname) {
  freqtable_t res = xcalloc(NUM_SYMBOLS, sizeof(int));
  // open file
  FILE *a = fopen(fname, "r");
  int temp = fgetc(a);
  // when EOF, reach the end of the file
  while (temp != EOF){
    res[temp] += 1;
    temp = fgetc(a);
  }
  // close file when it's at the end
  fclose(a);
  return res;
}



/************************************************/
/*  Task 7: Packing and unpacking a bitstring   */
/************************************************/

// Pack a string of bits into an array of bytes
uint8_t* pack(bit_t *bits){
  int len = strlen(bits);
  int bit_len = 0;
  if (len%8 == 0) bit_len = len/8;
  if (len%8 != 0) bit_len = len/8+1;

  uint8_t *res = xcalloc(bit_len, sizeof(uint8_t));
  // now is the temporary thing used to put in res
  uint8_t now = 0;

  // loop through the bits 
  for (int i = 0; i < len; i++){
    if (bits[i] == '1'){
      now = now<<1;
      now += 1;
    }
    if (bits[i] == '0'){
      now = now<<1;
    }
    if (i != 0 && i % 8 == 7){
      int a = i/8;
      res[a] = now;
      now = 0;
    }
  }

  // deals with the stuff at the end, add zeros to the right (left shift)
  if (len % 8 != 0){
    int offset = 8 - len % 8;
    for (int j = 0; j < offset; j++){
      now = now << 1;
    }
    res[len/8] = now;
  }
  return res;
}



// Unpack an array of bytes c of length len into a NUL-terminated string of ASCII bits
bit_t* unpack(uint8_t *c, size_t len) {
  bit_t *out = xcalloc(len*8+1, sizeof(bit_t));
  for (size_t i = 0; i < len; i++){
    // get the stuff that is in the unit8_t array
    uint8_t now = c[i];
    // add stuff from the right to the left
    for (int j = 7; j >= 0; j--){
      int a = now % 2;
      if (a == 1) out[i*8+j] = '1';
      if (a == 0) out[i*8+j] = '0';
      now = now >> 1;
    }
  }
  return out;
}
