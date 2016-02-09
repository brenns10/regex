/***************************************************************************//**

  @file         codegen.c

  @author       Stephen Brennan

  @date         Created Saturday, 30 January 2016

  @brief        Generating code from parse trees.

  @copyright    Copyright (c) 2016, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

  Notes on how code generation works:

  The difficult part of code generation is that you need to generate code with
  "jump" and "split" instructions that point to other instructions.  But since
  the jump and split targets are just (instr*) pointers, and these frequently
  get moved around or freed during code generation, a naive implementation will
  end up with a bunch of dangling pointers.

  My solution to this issue is to use "Fragments."  Fragments are stored in a
  singly linked list, and they contain an instruction along with an ID.  As code
  is generated, jump and split targets are stored using this ID rather than a
  raw pointer.  Once you have a final Fragment list that contains all of your
  code, it will be turned into an array, and all the IDs will be resolved
  efficiently to locations in the final array using a table.

*******************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "regex.h"
#include "regparse.h"

typedef struct Fragment Fragment;
struct Fragment {
  instr in;
  intptr_t id;
  Fragment *next;
};

typedef struct State State;
struct State {
  intptr_t id; // "global" id counter
  size_t capture; // capture parentheses counter
};

static Fragment *last(Fragment *f)
{
  while (f->next) {
    f = f->next;
  }
  return f;
}

/**
   @brief "Join" a fragment to the given ID.

   This replaces each "match" instruction with a "jump" instruction to the given
   ID.  Since it's assumed that the instruction with that ID will be placed
   after this one, if the last instruction of this fragment is a "match", it
   will just be deleted.
 */
static void join(Fragment *a, Fragment *b)
{
  Fragment *prev = NULL;
  Fragment *l = last(a);
  instr *lastid;

  // lastid will be set in case the last instruction is a Match, which will be
  // deleted later.
  if (l->in.code == Match) {
    lastid = (instr*) l->id;
  } else {
    lastid = (instr*) -1;
  }

  while (a->next != NULL) {
    // Matches should be replaced with jumps to the next fragment.
    if (a->in.code == Match) {
      a->in.code = Jump;
      a->in.x = (instr*)b->id;
    }
    // Jumps to the final match instruction should instead be targeted to the
    // next fragment.
    if ((a->in.code == Jump || a->in.code == Split) && a->in.x == lastid) {
      a->in.x = (instr*) b->id;
    }
    if (a->in.code == Split && a->in.y == lastid) {
      a->in.y = (instr*) b->id;
    }
    prev = a;
    a = a->next;
  }

  // If the last instruction is a Match, delete it.
  if (prev != NULL && a->in.code == Match) {
    free(a);
    prev->next = b;
  }
}

static size_t fraglen(Fragment *f)
{
  size_t len = 0;
  while (f) {
    len++;
    f = f->next;
  }
  return len;
}

static Fragment *newfrag(enum code code, State *s)
{
  Fragment *new = calloc(1, sizeof(Fragment));
  new->in.code = code;
  new->id = s->id++;
  return new;
}

static void freefraglist(Fragment *f)
{
  Fragment *next;
  while (f) {
    next = f->next;
    free(f);
    f = next;
  }
}

static Fragment *regex(parse_tree *t, State *s);
static Fragment *term(parse_tree *t, State *s);
static Fragment *expr(parse_tree *t, State *s);
//static Fragment *class(parse_tree *t, State *s);
static Fragment *sub(parse_tree *t, State *s);

static Fragment *term(parse_tree *t, State *s)
{
  Fragment *f = NULL;

  assert(t->nt == TERMnt);

  if (t->nchildren == 1) {
    // Either character or special
    if (t->children[0]->tok.sym == CharSym) {
      // Character
      f = newfrag(Char, s);
      f->in.c = t->children[0]->tok.c;
      f->next = newfrag(Match, s);
    } else if (t->children[0]->tok.sym == Dot) {
      // Dot
      f = newfrag(Any, s);
      f->next = newfrag(Match, s);
    } else if (t->children[0]->tok.sym == Special) {
      // Special
      fprintf(stderr, "not implemented: term special\n");
    }
  } else if (t->nchildren == 3 && t->children[0]->tok.sym == RParen) {
    // Parenthesized expression
    f = newfrag(Save, s);
    f->in.s = s->capture++;
    f->next = regex(t->children[1], s);
    Fragment *n = newfrag(Save, s);
    n->in.s = s->capture++;
    n->next = newfrag(Match, s);
    join(f, n);
  } else {
    // Character class
    fprintf(stderr, "not implemented: term character class\n");
    exit(1);
  }
  return f;
}

static Fragment *expr(parse_tree *t, State *s)
{
  Fragment *f = NULL, *a = NULL, *b = NULL, *c = NULL;

  assert(t->nt == EXPRnt);

  f = term(t->children[0], s);
  if (t->nchildren == 1) {
    return f;
  } else {
    if (t->children[1]->tok.sym == Star) {
      /*
        L1:
            split L2 L3   ;; this is "a"  [ non-greedy: split L3 L2 ]
        L2:
            BLOCK from f
            jump L1       ;; this is "b"
        L3:
            match         ;; this is "c"
       */
      a = newfrag(Split, s);
      b = newfrag(Jump, s);
      c = newfrag(Match, s);
      if (t->nchildren == 3) {
        // Non-greedy
        a->in.x = (instr*) c->id;
        a->in.y = (instr*) f->id;
      } else {
        // Greedy
        a->in.x = (instr*) f->id;
        a->in.y = (instr*) c->id;
      }
      b->in.x = (instr*) a->id;
      a->next = f;
      b->next = c;
      join(a, b);
      return a;
    } else if (t->children[1]->tok.sym == Plus) {
      /*
        L1:
            BLOCK from f
            split L1 L2   ;; this is "a"  [ non-greedy: split L2 L1 ]
        L2:
            match         ;; this is "b"
       */
      a = newfrag(Split, s);
      b = newfrag(Match, s);
      if (t->nchildren == 3) {
        // Non-greedy
        a->in.x = (instr*) b->id;
        a->in.y = (instr*) f->id;
      } else {
        // Greedy
        a->in.x = (instr*) f->id;
        a->in.y = (instr*) b->id;
      }
      join(f, a);
      a->next = b;
      return f;
    } else if (t->children[1]->tok.sym == Question) {
      /*
            split L1 L2   ;; this is "a"  [ non-greedy: split L2 L1 ]
        L1:
            BLOCK from f
        L2:
            match         ;; this is "b"
       */
      a = newfrag(Split, s);
      b = newfrag(Match, s);
      if (t->nchildren == 3) {
        // Non-greedy
        a->in.x = (instr*) b->id;
        a->in.y = (instr*) f->id;
      } else {
        // Greedy
        a->in.x = (instr*) f->id;
        a->in.y = (instr*) b->id;
      }
      a->next = f;
      join(f, b);
      return a;
    }
  }
  return NULL;
}

static Fragment *sub(parse_tree *tree, State *state)
{
  assert(tree->nt == SUBnt);
  Fragment *e = expr(tree->children[0], state);
  if (tree->nchildren == 2) {
    /*
      BLOCK from e
      BLOCK from s
     */
    Fragment *s = sub(tree->children[1], state);
    join(e, s);
  }
  return e;
}

static Fragment *regex(parse_tree *tree, State *state)
{
  assert(tree->nt == REGEXnt);
  Fragment *s = sub(tree->children[0], state);
  if (tree->nchildren == 3) {
    /*
          split L1 L2     ;; this is "pre"
      L1:
          BLOCK from s
          jump L3         ;; this is "j"
      L2:
          BLOCK from r
      L3:
          match           ;; this is "m"
     */

    Fragment *r = regex(tree->children[2], state);

    Fragment *pre = newfrag(Split, state);
    pre->in.x = (instr*) s->id;
    pre->in.y = (instr*) r->id;
    pre->next = s;

    Fragment *m = newfrag(Match, state);
    Fragment *j = newfrag(Jump, state);
    j->in.x = (instr*) m->id;
    j->next = r;
    join(j, m);
    join(pre, j);

    return pre;
  }
  return s;
}

instr *codegen(parse_tree *tree, size_t *n)
{
  // Generate code.
  State s = {0, 0};
  Fragment *f = regex(tree, &s);

  // Get the length of the code
  *n = fraglen(f);

  // Allocate buffers for the code, and for a lookup table of targets for jumps.
  instr *code = calloc(*n, sizeof(instr));
  size_t *targets = calloc(s.id, sizeof(size_t));

  // Fill up the lookup table.
  size_t i = 0;
  Fragment *curr;
  for (curr = f; curr; curr = curr->next, i++) {
    targets[curr->id] = i;
  }

  // Now, copy in the instructions, replacing the jump targets from the table.
  for (curr = f, i = 0; curr; curr = curr->next, i++) {
    code[i] = curr->in;
    if (code[i].code == Jump || code[i].code == Split) {
      code[i].x = code + targets[(intptr_t)code[i].x];
    }
    if (code[i].code == Split) {
      code[i].y = code + targets[(intptr_t)code[i].y];
    }
  }

  free(targets);
  freefraglist(f);
  return code;
}
