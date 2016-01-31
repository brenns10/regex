/***************************************************************************//**

  @file         parse.c

  @author       Stephen Brennan

  @date         Created Saturday, 23 January 2016

  @brief        Recursive descent regex parser!  (modeled after Wikipedia)

  @copyright    Copyright (c) 2016, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "regex.h"
#include "regparse.h"

char *names[] = {
  "CharSym", "Special", "Eof", "LParen", "RParen", "LBracket", "RBracket",
  "Plus", "Minus", "Star", "Question", "Caret", "Pipe"
};

char *ntnames[] = {
  "TERM", "EXPR", "REGEX", "CLASS", "SUB"
};

/*
  Convenience functions for parse trees.
 */

static parse_tree *terminal_tree(Token tok)
{
  parse_tree *tree = calloc(1, sizeof(parse_tree));
  tree->nchildren = 0;
  tree->tok = tok;
  return tree;
}

static parse_tree *nonterminal_tree(NonTerminal nt, size_t nchildren)
{
  parse_tree *tree = calloc(1, sizeof(parse_tree));
  tree->nchildren = nchildren;
  tree->nt = nt;
  return tree;
}

static void free_tree(parse_tree *tree)
{
  for (size_t i = 0; i < tree->nchildren; i++) {
    free_tree(tree->children[i]);
  }
  free(tree);
}

/*
  Convenience printing functions.
 */

static void print_indent(int indent)
{
  printf(";; ");
  while (indent--) {
    printf(" ");
  }
}

static void print_tree(parse_tree *tree, int indent)
{
  print_indent(indent);
  if (tree == NULL) {
    printf("NULL\n");
    return;
  }

  if (tree->nchildren == 0) {
    // terminal
    printf("%s:'%c'\n", names[tree->tok.sym], tree->tok.c);
  } else {
    printf("%s {\n", ntnames[tree->nt]);
    for (size_t i = 0; i < tree->nchildren; i++) {
      print_tree(tree->children[i], indent + 1);
    }
    print_indent(indent);
    printf("}\n");
  }
}

/*
  Simple convenience functions for a parser.
 */

static bool accept(Sym s, Lexer *l)
{
  if (l->tok.sym == s) {
    nextsym(l);
    return true;
  }
  return false;
}

static void expect(Sym s, Lexer *l) {
  if (l->tok.sym == s) {
    nextsym(l);
    return;
  }
  fprintf(stderr, "error: expected %s, got %s\n", names[s], names[l->tok.sym]);
  exit(1);
}

/*
  Recursive descent parser (main functions)
 */

static parse_tree *TERM(Lexer *l);
static parse_tree *EXPR(Lexer *l);
static parse_tree *REGEX(Lexer *l);
static parse_tree *CLASS(Lexer *l);
static parse_tree *SUB(Lexer *l);

static parse_tree *TERM(Lexer *l)
{
  if (accept(CharSym, l)) {
    parse_tree *result = nonterminal_tree(TERMnt, 1);
    result->children[0] = terminal_tree(l->prev);
    return result;
  } else if (accept(Special, l)) {
    parse_tree *result = nonterminal_tree(TERMnt, 1);
    result->children[0] = terminal_tree(l->prev);
    return result;
  } else if (accept(LParen, l)) {
    parse_tree *result = nonterminal_tree(TERMnt, 3);
    result->children[0] = terminal_tree(l->prev);
    result->children[1] = REGEX(l);
    expect(RParen, l);
    result->children[2] = terminal_tree(l->prev);
    return result;
  } else if (accept(LBracket, l)) {
    parse_tree *result;
    if (accept(Caret, l)) {
      result = nonterminal_tree(TERMnt, 4);
      result->children[0] = terminal_tree((Token){LBracket, ']'});
      result->children[1] = terminal_tree(l->prev);
      result->children[2] = CLASS(l);
      expect(RBracket, l);
      result->children[3] = terminal_tree(l->prev);
    } else {
      result = nonterminal_tree(TERMnt, 4);
      result->children[0] = terminal_tree((Token){LBracket, ']'});
      result->children[1] = CLASS(l);
      expect(RBracket, l);
      result->children[2] = terminal_tree(l->prev);
    }
    return result;
  } else {
    fprintf(stderr, "error: TERM: syntax error\n");
    exit(1);
  }
}

static parse_tree *EXPR(Lexer *l)
{
  parse_tree *result = nonterminal_tree(EXPRnt, 1);
  result->children[0] = TERM(l);
  if (accept(Plus, l) || accept(Star, l) || accept(Question, l)) {
    result->nchildren++;
    result->children[1] = terminal_tree(l->prev);
    if (accept(Question, l)) {
      result->nchildren++;
      result->children[2] = terminal_tree((Token){Question, '?'});
    }
  }
  return result;
}

static parse_tree *SUB(Lexer *l)
{
  parse_tree *result = nonterminal_tree(SUBnt, 1);
  parse_tree *orig = result, *prev = result;

  while (l->tok.sym != Eof && l->tok.sym != RParen && l->tok.sym != Pipe) { // seems like a bit of a hack
    result->children[0] = EXPR(l);
    result->children[1] = nonterminal_tree(SUBnt, 0);
    result->nchildren = 2;
    prev = result;
    result = result->children[1];
  }

  // This prevents SUB nonterminals with no children in the final parse tree.
  if (prev != result) {
    prev->nchildren = 1;
    free(result);
  }
  return orig;
}

static parse_tree *REGEX(Lexer *l)
{
  parse_tree *result = nonterminal_tree(REGEXnt, 1);
  result->children[0] = SUB(l);

  if (accept(Pipe, l)) {
    result->nchildren = 3;
    result->children[1] = terminal_tree(l->tok);
    result->children[2] = REGEX(l);
  }
  return result;
}

static parse_tree *CLASS(Lexer *l)
{
  // this guy isn't really implemented yet
  while (true) {
    if (accept(CharSym, l)) {
      if (accept(Minus, l)) {
        expect(CharSym, l);
      }
    } else if (accept(Minus, l)) {
      // yay!
    } else {
      break;
    }
  }
  return NULL; // not yet implemented
}

instr *recomp(char *regex, size_t *n)
{
  Lexer l;

  // Initialize the lexer.
  l.input = regex;
  l.index = 0;

  // Create a parse tree!
  printf(";; TOKENS:\n");
  nextsym(&l);
  parse_tree *tree = REGEX(&l);
  expect(Eof, &l);

  // Diagnostics - print parse tree.
  printf(";; PARSE TREE:\n");
  print_tree(tree, 0);

  // Generate code from parse tree.
  instr *code = codegen(tree, n);

  // Free tree.
  free_tree(tree);

  // Return code.
  return code;
}
