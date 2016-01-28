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

/*
  TOKENS:

  char (any directly matched text, including backslash escaped metachars)
  metachars: (  )  +  -  *  ?  [  ]
  special: \w, \d, \s, etc
  eof
 */

/*

  REGEX -> SUB
        -> SUB REGEX

  SUB   -> EXPR
        -> EXPR EXPR

  EXPR  -> TERM
        -> TERM +
        -> TERM + ?
        -> TERM *
        -> TERM * ?
        -> TERM ?
        -> TERM ? ?

  TERM  -> char
        -> special
        -> ( REGEX )
        -> [ (^) CLASS ]

  CLASS -> char - char CLASS
        -> char CLASS
        -> -
        ->
 */

enum Sym {
  CharSym, Special, Eof, LParen, RParen, LBracket, RBracket, Plus, Minus,
  Star, Question, Caret, Pipe
};
typedef enum Sym Sym;

char *names[] = {
  "CharSym", "Special", "Eof", "LParen", "RParen", "LBracket", "RBracket",
  "Plus", "Minus", "Star", "Question", "Caret"
};

enum NonTerminal {
  TERMnt, EXPRnt, REGEXnt, CLASSnt, SUBnt
};
typedef enum NonTerminal NonTerminal;

char *ntnames[] = {
  "TERM", "EXPR", "REGEX", "CLASS", "SUB"
};

struct parse_tree {
  size_t nchildren; // 0 -> terminal, >0 -> nonterminal

  NonTerminal nt;
  Sym sym;

  struct parse_tree *children[4];
};

typedef struct parse_tree parse_tree;

parse_tree *terminal_tree(Sym sym)
{
  parse_tree *tree = calloc(1, sizeof(parse_tree));
  tree->nchildren = 0;
  tree->sym = sym;
  return tree;
}

parse_tree *nonterminal_tree(NonTerminal nt, size_t nchildren)
{
  parse_tree *tree = calloc(1, sizeof(parse_tree));
  tree->nchildren = nchildren;
  tree->nt = nt;
  return tree;
}

void print_indent(int indent)
{
  while (indent--) {
    printf(" ");
  }
}

void print_tree(parse_tree *tree, int indent)
{
  print_indent(indent);
  if (tree == NULL) {
    printf("NULL\n");
    return;
  }

  if (tree->nchildren == 0) {
    // terminal
    printf("%s\n", names[tree->sym]);
  } else {
    printf("%s {\n", ntnames[tree->nt]);
    for (size_t i = 0; i < tree->nchildren; i++) {
      print_tree(tree->children[i], indent + 1);
    }
    print_indent(indent);
    printf("}\n");
  }
}

Sym input_[] = {
  LParen, CharSym, Plus, RParen, LParen, CharSym, CharSym, RParen, Plus, Eof
};
size_t index_ = 0;

Sym sym(void)
{
  return input_[index_];
}

Sym nextsym(void)
{
  // printf("  (Advance sym to %s)\n", names[input_[index_+1]]);
  return input_[++index_];
}

Sym prevsym(void)
{
  return input_[index_-1];
}

bool accept(Sym s)
{
  if (sym() == s) {
    nextsym();
    return true;
  }
  return false;
}

void expect(Sym s) {
  if (sym() == s) {
    nextsym();
    return;
  }
  fprintf(stderr, "error: expected %s, got %s\n", names[s], names[sym()]);
  exit(1);
}

parse_tree *TERM(void);
parse_tree *EXPR(void);
parse_tree *REGEX(void);
parse_tree *CLASS(void);
parse_tree *SUB(void);

parse_tree *TERM(void)
{
  // printf("TERM\n");
  if (accept(CharSym)) {
    // printf("TERM -> CharSym\n");
    parse_tree *result = nonterminal_tree(TERMnt, 1);
    result->children[0] = terminal_tree(CharSym);
    return result;
  } else if (accept(Special)) {
    // printf("TERM -> Special\n");
    parse_tree *result = nonterminal_tree(TERMnt, 1);
    result->children[0] = terminal_tree(Special);
    return result;
  } else if (accept(LParen)) {
    // printf("TERM -> LParen REGEX RParen\n");
    parse_tree *result = nonterminal_tree(TERMnt, 3);
    result->children[0] = terminal_tree(LParen);
    result->children[1] = REGEX();
    expect(RParen);
    result->children[2] = terminal_tree(RParen);
    return result;
  } else if (accept(LBracket)) {
    // printf("TERM -> LBracket (Caret) CLASS \n");
    parse_tree *result;
    if (accept(Caret)) {
      result = nonterminal_tree(TERMnt, 4);
      result->children[0] = terminal_tree(LBracket);
      result->children[1] = terminal_tree(Caret);
      result->children[2] = CLASS();
      expect(RBracket);
      result->children[3] = terminal_tree(RBracket);
    } else {
      result = nonterminal_tree(TERMnt, 4);
      result->children[0] = terminal_tree(LBracket);
      result->children[1] = CLASS();
      expect(RBracket);
      result->children[2] = terminal_tree(RBracket);
    }
    return result;
  } else {
    fprintf(stderr, "error: TERM: syntax error\n");
    exit(1);
  }
  // printf("END TERM\n");
}

parse_tree *EXPR(void)
{
  // printf("EXPR\n");
  parse_tree *result = nonterminal_tree(EXPRnt, 1);
  result->children[0] = TERM();
  if (accept(Plus) || accept(Star) || accept(Question)) {
    result->nchildren++;
    result->children[1] = terminal_tree(prevsym());
    if (accept(Question)) {
      result->nchildren++;
      result->children[2] = terminal_tree(Question);
    }
  }
  return result;
  // printf("END EXPR\n");
}

parse_tree *SUB(void)
{
  // printf("SUB\n");
  parse_tree *result = nonterminal_tree(SUBnt, 1);
  parse_tree *orig = result, *prev = result;

  while (sym() != Eof && sym() != RParen) { // seems like a bit of a hack
    result->children[0] = EXPR();
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
  // printf("END SUB\n");
}

parse_tree *REGEX(void)
{
  parse_tree *result = nonterminal_tree(REGEXnt, 1);
  result->children[0] = SUB();

  if (accept(Pipe)) {
    result->nchildren = 3;
    result->children[1] = terminal_tree(Pipe);
    result->children[2] = REGEX();
  }
  return result;
}

parse_tree *CLASS(void)
{
  while (true) {
    if (accept(CharSym)) {
      if (accept(Minus)) {
        expect(CharSym);
      }
    } else if (accept(Minus)) {
      // yay!
    } else {
      break;
    }
  }
  return NULL; // not yet implemented
}

void parser(void)
{
  printf("Start (index=%zu).\n", index_);
  parse_tree *tree = REGEX();
  expect(Eof);
  print_tree(tree, 0);
  printf("Done!\n");
}
