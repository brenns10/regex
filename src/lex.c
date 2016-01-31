/***************************************************************************//**

  @file         lex.c

  @author       Stephen Brennan

  @date         Created Friday, 29 January 2016

  @brief        All lexer-related functions.

  @copyright    Copyright (c) 2016, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include <stdio.h>

#include "regparse.h"

void escape(Lexer *l)
{
  switch (l->input[l->index]) {
  case '(':
    l->tok = (Token){CharSym, '('};
    break;
  case ')':
    l->tok = (Token){CharSym, ')'};
    break;
  case '[':
    l->tok = (Token){CharSym, '['};
    break;
  case ']':
    l->tok = (Token){CharSym, ']'};
    break;
  case '+':
    l->tok = (Token){CharSym, '+'};
    break;
  case '-':
    l->tok = (Token){CharSym, '-'};
    break;
  case '*':
    l->tok = (Token){CharSym, '*'};
    break;
  case '?':
    l->tok = (Token){CharSym, '?'};
    break;
  case '^':
    l->tok = (Token){CharSym, '^'};
    break;
  case 'n':
    l->tok = (Token){CharSym, '\n'};
    break;
  default:
    l->tok = (Token){Special, l->input[l->index]};
    break;
  }
}

Token nextsym(Lexer *l)
{
  if (l->tok.sym == Eof) {
    return l->tok; // eof never ceases to be eof!
  }

  l->prev = l->tok;
  switch (l->input[l->index]) {
  case '(':
    l->tok = (Token){LParen, '('};
    break;
  case ')':
    l->tok = (Token){RParen, ')'};
    break;
  case '[':
    l->tok = (Token){LBracket, '['};
    break;
  case ']':
    l->tok = (Token){RBracket, ']'};
    break;
  case '+':
    l->tok = (Token){Plus, '+'};
    break;
  case '-':
    l->tok = (Token){Minus, '-'};
    break;
  case '*':
    l->tok = (Token){Star, '*'};
    break;
  case '?':
    l->tok = (Token){Question, '?'};
    break;
  case '^':
    l->tok = (Token){Caret, '^'};
    break;
  case '|':
    l->tok = (Token){Pipe, '|'};
    break;
  case '\\':
    l->index++;
    escape(l);
    break;
  case '\0':
    l->tok = (Token){Eof, '\0'};
    break;
  default:
    l->tok = (Token){CharSym, l->input[l->index]};
    break;
  }
  l->index++;
  printf(";; nextsym(): {%s, '%c'}\n", names[l->tok.sym], l->tok.c);
  return l->tok;
}
