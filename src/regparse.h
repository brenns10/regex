/***************************************************************************//**

  @file         regparse.h

  @author       Stephen Brennan

  @date         Created Friday, 29 January 2016

  @brief        Private lex/parse/codegen declarations.

  @copyright    Copyright (c) 2016, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#ifndef SMB_REGEX_REGPARSE_H
#define SMB_REGEX_REGPARSE_H

#include <stdlib.h>
#include "regex.h"

/**
   @brief Types of terminal symbols!
 */
enum Sym {
  CharSym, Special, Eof, LParen, RParen, LBracket, RBracket, Plus, Minus,
  Star, Question, Caret, Pipe
};
typedef enum Sym Sym;

// Lookup the name of a terminal symbol.
extern char *names[];

/**
   @brief Types of non-terminal symbols!
 */
enum NonTerminal {
  TERMnt, EXPRnt, REGEXnt, CLASSnt, SUBnt
};
typedef enum NonTerminal NonTerminal;

// Lookup the name of a non-terminal symbol.
extern char *ntnames[];

/**
   @brief Tokens are simply a combination of a symbol type and a character.

   This means that you can associate the CharSym with the particular character,
   and you can tell the difference between different Special's (like \b, \w, \s,
   etc).  The character field may be useless for symbols like LParen.  A token
   goes into a parse tree, and should contain all the information necessary for
   code generation from a parse tree.
 */
typedef struct Token Token;
struct Token {
  Sym sym;
  char c;
};

/**
   @brief Tree data structure to store information parsed out of a regex.
 */
typedef struct parse_tree parse_tree;
struct parse_tree {
  size_t nchildren; // 0 -> terminal, >0 -> nonterminal

  NonTerminal nt;
  Token tok;

  struct parse_tree *children[4];
};

/**
   @brief Data structure containing lexer information.
 */
typedef struct Lexer Lexer;
struct Lexer {
  char *input;
  size_t index;
  Token tok, prev;
};

void escape(Lexer *l);
Token nextsym(Lexer *l);
instr *codegen(parse_tree *tree, size_t *n);

#endif // SMB_REGEX_REGPARSE_H
