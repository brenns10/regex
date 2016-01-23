/***************************************************************************//**

  @file         instr.c

  @author       Stephen Brennan

  @date         Created Thursday, 21 January 2016

  @brief        Functions for reading and writing virtual machine instructions.

  @copyright    Copyright (c) 2016, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include "regex.h"

#define COMMENT ';'

/*
  Declarations for parsing.
 */

enum linetype {
  Blank, Label, Code
};
typedef enum linetype linetype;

char *Opcodes[] = {
  "char", "match", "jump", "split", "save"
};


/**
   @brief Remove leading and trailing whitespace and comments from a line.
 */
static char *trim(char *line, char *lastchar_out)
{
  ssize_t firstnonwhite = -1;
  ssize_t lastnonwhite = -1;
  size_t i;

  for (i = 0; line[i]; i++) {
    if (isspace(line[i])) {
      continue;
    }

    if (line[i] == COMMENT) {
      line[i] = '\0';
      break;
    }

    if (firstnonwhite == -1) {
      firstnonwhite = i;
    }
    lastnonwhite = i;
  }

  if (firstnonwhite == -1) { // all whitespace
    *lastchar_out = '\0';
    return line + i;
  } else {
    line[lastnonwhite + 1] = '\0';
    *lastchar_out = line[lastnonwhite];
    return line + firstnonwhite;
  }
}

/**
   @brief Return the index of the next whitespace in a string.

   Or the index of the null terminator, whichever comes first.
 */
static size_t nextws(char *s) {
  size_t i = 0;
  while (s[i] && !isspace(s[i])) {
    i++;
  }
  return i;
}

/**
   @brief Return the index of the next non-whitespace in a string.

   Or the index of the null terminator, whichever comes first.
 */
static size_t skipws(char *s) {
  size_t i = 0;
  while (s[i] && isspace(s[i])) {
    i++;
  }
  return i;
}

/**
   @brief Parse and return an instruction from a line.

   Input line should have no preceding or trailing whitespace (see trim()).
   This function will not return on error - it will just fprintf() the error
   message and exit with an error code.  This will change eventually.
   @param line Text to parse.
   @param lineno Line number (just used for error msg).
 */
static instr read_instr(char *line, int lineno)
{
  // First, we're going to tokenize the string into a statically allocated
  // buffer.  We know we don't need more than like TODO
  char *tokens[5];
  size_t i;
  tokens[0] = line;
  for (i = 0; i < nelem(tokens)-1; i++) {
    size_t n = nextws(tokens[i]);
    if (tokens[i][n] == '\0') {
      i++;
      break;
    } else {
      tokens[i][n] = '\0';
      tokens[i+1] = tokens[i] + n + 1;
      tokens[i+1] += skipws(tokens[i+1]);
    }
  }

  if (i == nelem(tokens) - 1) {
    fprintf(stderr, "%d: too many tokens on one line\n", lineno);
    exit(1);
  }

  instr inst = {0};
  if (strcmp(tokens[0], Opcodes[Char]) == 0) {
    if (i != 2) {
      fprintf(stderr, "line %d: require 2 tokens for char\n", lineno);
      exit(1);
    }
    inst.code = Char;
    inst.c = tokens[1][0];
  } else if (strcmp(tokens[0], Opcodes[Match]) == 0) {
    if (i != 1) {
      fprintf(stderr, "line %d: require 1 token for match\n", lineno);
      exit(1);
    }
    inst.code = Match;
  } else if (strcmp(tokens[0], Opcodes[Jump]) == 0) {
    if (i != 2) {
      fprintf(stderr, "line %d: require 2 tokens for jump\n", lineno);
      exit(1);
    }
    inst.code = Jump;
    inst.x = (instr*)tokens[1];
  } else if (strcmp(tokens[0], Opcodes[Split]) == 0) {
    if (i != 3) {
      fprintf(stderr, "line %d: require 3 tokens for split\n", lineno);
      exit(1);
    }
    inst.code = Split;
    inst.x = (instr*)tokens[1];
    inst.y = (instr*)tokens[2];
  } else if (strcmp(tokens[0], Opcodes[Save]) == 0) {
    if (i != 2) {
      fprintf(stderr, "line %d: require 2 tokens for save\n", lineno);
      exit(1);
    }
    inst.code = Save;
    sscanf(tokens[1], "%zu", &inst.s);
  } else {
    fprintf(stderr, "line %d: unknown opcode \"%s\"\n", lineno, tokens[0]);
  }
  return inst;
}

/**
   @brief Return the instruction index corresponding to a label.
   @param labels Array of labels
   @param lablindices Corresponding indices for the labels
   @param nlabels Number of label/index pairs in the arrays
   @param label Label to search for
   @param line Line number (for error messages)
 */
static size_t gettarget(char **labels, size_t *labelindices, size_t nlabels,
                        char *label, size_t line)
{
  for (size_t i = 0; i < nlabels; i++) {
    if (strcmp(labels[i], label) == 0) {
      return labelindices[i];
    }
  }
  fprintf(stderr, "line %zu: label \"%s\" not found\n", line, label);
  exit(1);
}

/**
   @brief Return a block of instructions from some text code.
   @param str Code
   @param[out] ninstr Where to put the number of instructions
 */
instr *read_prog(char *str, size_t *ninstr)
{
  size_t nlines = 1;
  instr *rv = NULL;

  // Count newlines.
  for (size_t i = 0; str[i]; i++) {
    if (str[i] == '\n') {
      nlines++;
    }
  }

  // Create an array of lines.
  char **lines = calloc(nlines, sizeof(char*));
  size_t j = 0;
  lines[j++] = str;
  for (size_t i = 0; str[i]; i++) {
    if (str[i] == '\n') {
      str[i] = '\0';
      lines[j++] = str + i + 1;
    }
  }

  // Trim whitespace of each line, and categorize the lines.
  size_t nlabels = 0;
  size_t ncode = 0;
  linetype *types = calloc(nlines, sizeof(linetype));
  for (size_t i = 0; i < nlines; i++) {
    char last;
    lines[i] = trim(lines[i], &last);
    if (last == '\0') {
      types[i] = Blank;
    } else if (last == ':') {
      types[i] = Label;
      nlabels++;
    } else {
      types[i] = Code;
      ncode++;
    }
  }

  // Associate each label with the next line of code.
  char **labels = calloc(nlabels, sizeof(char *));
  size_t *labelindices = calloc(nlabels, sizeof(size_t));
  ssize_t codeidx = -1;
  size_t labelidx = 0;
  for (size_t i = 0; i < nlines; i++) {
    if (types[i] == Label) {
      size_t len = strlen(lines[i]);
      lines[i][len-1] = '\0'; // remove the colon, we don't want it
      labels[labelidx] = lines[i];
      labelindices[labelidx] = codeidx + 1;
      labelidx++;
    } else if (types[i] == Code) {
      codeidx++;
    }
  }
  // we'll assume that the labels are valid for now

  rv = calloc(ncode, sizeof(instr));
  codeidx = 0;
  for (size_t i = 0; i < nlines; i++) {
    if (types[i] != Code) {
      continue;
    }

    rv[codeidx] = read_instr(lines[i], i+1);

    // lookup labels and point them correctly
    if (rv[codeidx].x) {
      rv[codeidx].x = rv + gettarget(labels, labelindices, nlabels, (char*)rv[codeidx].x, i+1);
    }
    if (rv[codeidx].y) {
      rv[codeidx].y = rv + gettarget(labels, labelindices, nlabels, (char*)rv[codeidx].y, i+1);
    }

    codeidx++;
  }

  free(types);
  free(lines);
  free(labels);
  free(labelindices);
  if (ninstr) {
    *ninstr = codeidx;
  }
  return rv;
}

/**
   @brief Read a program from a file.
 */
instr *fread_prog(FILE *f, size_t *ninstr) {
  size_t alloc = 4096;
  char *buf = malloc(alloc);
  size_t start = 0;
  while (true) {
    start += fread(buf, 1, alloc - start, f);
    if (start == alloc) {
      alloc *= 2;
      buf = realloc(buf, alloc);
    } else {
      break;
    }
  }

  instr *rv = read_prog(buf, ninstr);
  free(buf);
  return rv;
}

/**
   @brief Write a program to a file.
 */
void write_prog(instr *prog, size_t n, FILE *f)
{
  size_t *labels = calloc(n, sizeof(size_t));

  // Find every instruction that needs a label.
  for (size_t i = 0; i < n; i++) {
    if (prog[i].code == Jump || prog[i].code == Split) {
      labels[prog[i].x - prog] = 1;
    }
    if (prog[i].code == Split) {
      labels[prog[i].y - prog] = 1;
    }
  }

  size_t l = 1;
  for (size_t i = 0; i < n; i++) {
    if (labels[i] > 0) {
      labels[i] = l++;
    }
  }

  for (size_t i = 0; i < n; i++) {
    if (labels[i] > 0) {
      fprintf(f, "L%zu:\n", labels[i]);
    }
    switch (prog[i].code) {
    case Char:
      fprintf(f, "    char %c\n", prog[i].c);
      break;
    case Match:
      fprintf(f, "    match\n");
      break;
    case Jump:
      fprintf(f, "    jump L%zu\n", prog[i].x - prog);
      break;
    case Split:
      fprintf(f, "    split L%zu L%zu\n", labels[prog[i].x - prog],
              labels[prog[i].y - prog]);
      break;
    case Save:
      fprintf(f, "    save %zu\n", prog[i].s);
    }
  }
}
