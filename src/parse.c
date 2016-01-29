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
        -> EXPR SUB

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

typedef struct Token Token;
struct Token {
  Sym sym;
  char c;
};

struct parse_tree {
  size_t nchildren; // 0 -> terminal, >0 -> nonterminal

  NonTerminal nt;
  Token tok;

  struct parse_tree *children[4];
};

typedef struct parse_tree parse_tree;

parse_tree *terminal_tree(Token tok)
{
  parse_tree *tree = calloc(1, sizeof(parse_tree));
  tree->nchildren = 0;
  tree->tok = tok;
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

char *input_ = "(a+)(bc)+";
size_t index_ = 0;
Token tok, prev;

Token token(void)
{
  return tok;
}

Token prevtoken(void)
{
  return prev;
}

void escape(void)
{
  switch (input_[index_]) {
  case '(':
    tok = (Token){CharSym, '('};
    break;
  case ')':
    tok = (Token){CharSym, ')'};
    break;
  case '[':
    tok = (Token){CharSym, '['};
    break;
  case ']':
    tok = (Token){CharSym, ']'};
    break;
  case '+':
    tok = (Token){CharSym, '+'};
    break;
  case '-':
    tok = (Token){CharSym, '-'};
    break;
  case '*':
    tok = (Token){CharSym, '*'};
    break;
  case '?':
    tok = (Token){CharSym, '?'};
    break;
  case '^':
    tok = (Token){CharSym, '^'};
    break;
  case 'n':
    tok = (Token){CharSym, '\n'};
    break;
  default:
    tok = (Token){Special, input_[index_]};
    break;
  }
}

Token nextsym(void)
{
  prev = tok;
  switch (input_[index_]) {
  case '(':
    tok = (Token){LParen, '('};
    break;
  case ')':
    tok = (Token){RParen, ')'};
    break;
  case '[':
    tok = (Token){LBracket, '['};
    break;
  case ']':
    tok = (Token){RBracket, ']'};
    break;
  case '+':
    tok = (Token){Plus, '+'};
    break;
  case '-':
    tok = (Token){Minus, '-'};
    break;
  case '*':
    tok = (Token){Star, '*'};
    break;
  case '?':
    tok = (Token){Question, '?'};
    break;
  case '^':
    tok = (Token){Caret, '^'};
    break;
  case '\\':
    index_++;
    escape();
    break;
  case '\0':
    tok = (Token){Eof, '\0'};
    break;
  default:
    tok = (Token){CharSym, input_[index_]};
    break;
  }
  index_++;
  printf("nextsym(): {%s, '%c'}\n", names[tok.sym], tok.c);
  return tok;
}

bool accept(Sym s)
{
  if (token().sym == s) {
    nextsym();
    return true;
  }
  return false;
}

void expect(Sym s) {
  if (token().sym == s) {
    nextsym();
    return;
  }
  fprintf(stderr, "error: expected %s, got %s\n", names[s], names[token().sym]);
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
    result->children[0] = terminal_tree(prevtoken());
    return result;
  } else if (accept(Special)) {
    // printf("TERM -> Special\n");
    parse_tree *result = nonterminal_tree(TERMnt, 1);
    result->children[0] = terminal_tree(prevtoken());
    return result;
  } else if (accept(LParen)) {
    // printf("TERM -> LParen REGEX RParen\n");
    parse_tree *result = nonterminal_tree(TERMnt, 3);
    result->children[0] = terminal_tree(prevtoken());
    result->children[1] = REGEX();
    expect(RParen);
    result->children[2] = terminal_tree(prevtoken());
    return result;
  } else if (accept(LBracket)) {
    // printf("TERM -> LBracket (Caret) CLASS \n");
    parse_tree *result;
    if (accept(Caret)) {
      result = nonterminal_tree(TERMnt, 4);
      result->children[0] = terminal_tree((Token){LBracket, ']'});
      result->children[1] = terminal_tree(prevtoken());
      result->children[2] = CLASS();
      expect(RBracket);
      result->children[3] = terminal_tree(prevtoken());
    } else {
      result = nonterminal_tree(TERMnt, 4);
      result->children[0] = terminal_tree((Token){LBracket, ']'});
      result->children[1] = CLASS();
      expect(RBracket);
      result->children[2] = terminal_tree(prevtoken());
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
    result->children[1] = terminal_tree(prevtoken());
    if (accept(Question)) {
      result->nchildren++;
      result->children[2] = terminal_tree((Token){Question, '?'});
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

  while (token().sym != Eof && token().sym != RParen && token().sym != Pipe) { // seems like a bit of a hack
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
    result->children[1] = terminal_tree(token());
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
  nextsym();
  parse_tree *tree = REGEX();
  expect(Eof);
  print_tree(tree, 0);
  printf("Done!\n");
}
