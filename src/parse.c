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
  "Plus", "Minus", "Star", "Question", "Caret", "Pipe", "Dot"
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

void free_tree(parse_tree *tree)
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
    printf("%s:'%s'\n", names[tree->tok.sym], char_to_string(tree->tok.c));
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

bool accept(Sym s, Lexer *l)
{
  if (l->tok.sym == s) {
    nextsym(l);
    return true;
  }
  return false;
}

void expect(Sym s, Lexer *l)
{
  if (l->tok.sym == s) {
    nextsym(l);
    return;
  }
  fprintf(stderr, "error: expected %s, got %s\n", names[s], names[l->tok.sym]);
  exit(1);
}

parse_tree *TERM(Lexer *l)
{
  if (accept(CharSym, l) || accept(Dot, l) || accept(Special, l)) {
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
      result->children[0] = terminal_tree((Token){LBracket, '['});
      result->children[1] = terminal_tree(l->prev);
      result->children[2] = CLASS(l);
      expect(RBracket, l);
      result->children[3] = terminal_tree(l->prev);
    } else {
      result = nonterminal_tree(TERMnt, 3);
      result->children[0] = terminal_tree((Token){LBracket, '['});
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

parse_tree *EXPR(Lexer *l)
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

parse_tree *SUB(Lexer *l)
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

parse_tree *REGEX(Lexer *l)
{
  parse_tree *result = nonterminal_tree(REGEXnt, 1);
  result->children[0] = SUB(l);

  if (accept(Pipe, l)) {
    result->nchildren = 3;
    result->children[1] = terminal_tree(l->prev);
    result->children[2] = REGEX(l);
  }
  return result;
}

parse_tree *CLASS(Lexer *l)
{
  // this guy isn't really implemented yet
  parse_tree *result = nonterminal_tree(CLASSnt, 0), *curr, *prev;
  Token t1, t2, t3;
  curr = result;

  while (true) {
    if (accept(CharSym, l)) {
      prev = curr;
      t1 = l->prev;
      if (accept(Minus, l)) {
        t2 = l->prev;
        if (accept(CharSym, l)) {
          t3 = l->prev;
          // We have ourselves a range!  Parse it.
          curr->children[0] = terminal_tree(t1);
          curr->children[1] = terminal_tree(t3);
          curr->children[2] = nonterminal_tree(CLASSnt, 0);
          curr->nchildren = 3;
          curr = curr->children[2];
        } else {
          // character followed by minus, but not range.
          unget(t2, l);
          curr->children[0] = terminal_tree(t1);
          curr->children[1] = nonterminal_tree(CLASSnt, 0);
          curr->nchildren = 2;
          curr = curr->children[1];
        }
      } else {
        // just a character
        curr->children[0] = terminal_tree(t1);
        curr->children[1] = nonterminal_tree(CLASSnt, 0);
        curr->nchildren = 2;
        curr = curr->children[1];
      }
    } else if (accept(Minus, l)) {
      // just a minus
      prev = curr;
      curr->children[0] = terminal_tree(l->prev);
      curr->children[1] = nonterminal_tree(CLASSnt, 0);
      curr->nchildren = 2;
      curr = curr->children[1];
    } else {
      free(curr);
      prev->nchildren--;
      break;
    }
  }
  return result; // not yet implemented
}

parse_tree *reparse(char *regex)
{
  Lexer l;

  // Initialize the lexer.
  l.input = regex;
  l.index = 0;
  l.nbuf = 0;
  l.tok = (Token){0};

  // Create a parse tree!
  //printf(";; TOKENS:\n");
  nextsym(&l);
  parse_tree *tree = REGEX(&l);
  expect(Eof, &l);

  return tree;
}

instr *recomp(char *regex, size_t *n)
{
  // Diagnostics - print parse tree.
  parse_tree *tree = reparse(regex);
  printf(";; PARSE TREE:\n");
  print_tree(tree, 0);

  // Generate code from parse tree.
  instr *code = codegen(tree, n);

  // Free tree.
  free_tree(tree);

  // Return code.
  return code;
}
