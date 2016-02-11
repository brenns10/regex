/***************************************************************************//**

  @file         main.c

  @author       Stephen Brennan

  @date         Created Wednesday, 10 February 2016

  @brief        Main file for tests.

  @copyright    Copyright (c) 2016, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include "tests.h"

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  lex_test();
  parse_test();
  codegen_test();

  return 0;
}

