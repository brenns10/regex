/***************************************************************************//**

  @file         pike.c

  @author       Stephen Brennan, based on code by Russ Cox:
                https://swtch.com/~rsc/regexp/regexp2.html

  @date         Created Wednesday, 20 January 2016

  @brief        Regex implementation using virtual machines.

  @copyright    Copyright (c) 2016, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

                Portions of this code are based on code by Russ Cox, Copyright
                2007-2009.

*******************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "regex.h"

// Declarations:

typedef struct thread thread;
struct thread {
  instr *pc;
  size_t *saved;
};

typedef struct thread_list thread_list;
struct thread_list {
  thread *t;
  size_t n;
};

// Printing, for diagnostics

void printthreads(thread_list *tl, instr *prog, size_t nsave) {
  for (size_t i = 0; i < tl->n; i++) {
    printf("T%zu@pc=%lu{", i, (intptr_t) (tl->t[i].pc - prog));
    for (size_t j = 0; j < nsave; j++) {
      printf("%lu,", tl->t[i].saved[j]);
    }
    printf("} ");
  }
  printf("\n");
}

// Helper evaluation functions for instructions

bool range(instr in, char test) {
  if (test == '\0') {
    return false;
  }
  bool result = false;
  char *block = (char *) in.x;

  // use in.s for number of ranges, in.x as char* for ranges.
  for (size_t i = 0; i < in.s; i++) {
    if (block[i*2] <= test && test <= block [i*2 + 1]) {
      result = true;
      break; // short circuit yo!
    }
  }

  // negate result for negative ranges
  if (in.code == Range) {
    return result;
  } else {
    return !result;
  }
}

// Pike VM functions:

thread_list newthread_list(size_t n)
{
  thread_list tl;
  tl.t = calloc(n, sizeof(thread));
  tl.n = 0;
  return tl;
}

void addthread(thread_list *threads, instr *pc, size_t *saved, size_t nsave,
               size_t sp)
{
  //printf("addthread(): pc=%d, saved={%u, %u}, sp=%u, lastidx=%u\n", pc - extprog,
  //       saved[0], saved[1], sp, pc->lastidx);
  if (pc->lastidx == sp) {
    // we've executed this instruction on this string index already
    free(saved);
    return;
  }
  pc->lastidx = sp;

  size_t *newsaved;
  switch (pc->code) {
  case Jump:
    addthread(threads, pc->x, saved, nsave, sp);
    break;
  case Split:
    newsaved = calloc(nsave, sizeof(size_t));
    memcpy(newsaved, saved, nsave * sizeof(size_t));
    addthread(threads, pc->x, saved, nsave, sp);
    addthread(threads, pc->y, newsaved, nsave, sp);
    break;
  case Save:
    saved[pc->s] = sp;
    addthread(threads, pc + 1, saved, nsave, sp);
    break;
  default:
    threads->t[threads->n].pc = pc;
    threads->t[threads->n].saved = saved;
    threads->n++;
    break;
  }
}

void stash(size_t *new, size_t **saved)
{
  if (!saved) return;
  if (*saved) {
    free(*saved);
  }
  *saved = new;
}

ssize_t execute(instr *prog, size_t proglen, char *input, size_t **saved)
{
  // Can have at most n threads, where n is the length of the program.  This is
  // because (as it is now) the thread state is simply a program counter.
  thread_list curr = newthread_list(proglen);
  thread_list next = newthread_list(proglen);
  thread_list temp;
  size_t nsave = 0;
  ssize_t match = -1;

  // Need to initialize lastidx to something that will never be used.
  for (size_t i = 0; i < proglen; i++) {
    prog[i].lastidx = (size_t)-1;
    if (prog[i].code == Save) {
      nsave++;
    }
  }

  // Start with a single thread and add more as we need.  Note that addthread()
  // will execute instructions that don't consume input (i.e. epsilon closure).
  addthread(&curr, prog, calloc(nsave, sizeof(size_t)), nsave, 0);

  size_t sp;
  for (sp = 0; curr.n > 0; sp++) {

    //printf("consider input %c\nthreads: ", input[sp]);
    //printthreads(&curr, prog, nsave);

    // Execute each thread (this will only ever reach instructions that are
    // either Char or Match, since addthread() stops with those).
    for (size_t t = 0; t < curr.n; t++) {
      instr *pc = curr.t[t].pc;

      switch (pc->code) {
      case Char:
        if (input[sp] != pc->c) {
          free(curr.t[t].saved);
          break; // fail, don't continue executing this thread
        }
        // add thread containing the next instruction to the next thread list.
        addthread(&next, pc+1, curr.t[t].saved, nsave, sp+1);
        break;
      case Any:
        if (input[sp] == '\0') {
          free(curr.t[t].saved);
          break; // dot can't match end of string!
        }
        // add thread containing the next instruction to the next thread list.
        addthread(&next, pc+1, curr.t[t].saved, nsave, sp+1);
        break;
      case Range:
      case NRange:
        if (!range(*pc, input[sp])) {
          free(curr.t[t].saved);
          break;
        }
        addthread(&next, pc+1, curr.t[t].saved, nsave, sp+1);
        break;
      case Match:
        stash(curr.t[t].saved, saved);
        match = sp;
        goto cont;
      default:
        assert(false);
        break;
      }
    }

  cont:
    // Swap the curr and next lists.
    temp = curr;
    curr = next;
    next = temp;

    // Reset our new next list.
    next.n = 0;
  }

  free(curr.t);
  free(next.t);
  return match;
}

// Driver program

int numsaves(instr *code, size_t ncode)
{
  int ns = 0;
  for (size_t i = 0; i < ncode; i++) {
    if (code[i].code == Save && code[i].s > ns) {
      ns = code[i].s;
    }
  }
  return ns + 1;
}

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
}
