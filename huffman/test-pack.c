/**************************************************************************/
/*              COPYRIGHT Carnegie Mellon University 2021                 */
/* Do not post this file or any derivative on a public site or repository */
/**************************************************************************/
/* Huffman coding
 *
 * Main file for testing pack and unpack
 * 15-122 Principles of Imperative Computation
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "compress.h"
#include "bitpacking.h"


// You are welcome to define helper functions for your tests

int main () {
  // Create a few NUL-terminated string of ASCII bits and corresponding arrays of bytes
  char *a = "111";
  //char *b = "000";
  printf("%X",pack(a)[0]);
  // Using them, test pack and unpack



  printf("All tests passed!\n");
  return 0;
}
