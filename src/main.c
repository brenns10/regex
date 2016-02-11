/***************************************************************************//**

  @file         main.c

  @author       Stephen Brennan

  @date         Created Thursday, 11 February 2016

  @brief        Main program for regex implementation.

  @copyright    Copyright (c) 2016, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "regex.h"


int main(int argc, char **argv)
{
  if (argc < 3) {
    fprintf(stderr, "too few arguments\n");
    fprintf(stderr, "usage: %s REGEXP string1 [string2 [...]]\n", argv[0]);
    exit(1);
  }
  size_t n;
  instr *code;
  FILE *in = fopen(argv[1], "r");

  if (in == NULL) {
    printf(";; Regex: \"%s\"\n\n", argv[1]);
    code = recomp(argv[1], &n);
    printf(";; BEGIN GENERATED CODE:\n");
    write_prog(code, n, stdout);
  } else {
    code = fread_prog(in, &n);
    printf(";; BEGIN READ CODE:\n");
    write_prog(code, n, stdout);
  }

  int ns = numsaves(code, n);
  printf(";; BEGIN TEST RUNS:\n");

  for (int i = 2; i < argc; i++) {
    size_t *saves = NULL;
    ssize_t match = execute(code, n, argv[i], &saves);
    if (match != -1) {
      printf(";; \"%s\": match(%zd) ", argv[i], match);
      for (size_t j = 0; j < ns; j += 2) {
        printf("(%zd, %zd) ", saves[j], saves[j+1]);
      }
      printf("\n");
    } else {
      printf(";; \"%s\": no match\n", argv[i]);
    }
  }

  free_prog(code, n);
}
