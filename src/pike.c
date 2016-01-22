/***************************************************************************//**

  @file         pike.c

  @author       Stephen Brennan, based on code by Russ Cox:
                https://swtch.com/~rsc/regexp/regexp2.html

  @date         Created Wednesday, 20 January 2016

  @brief        Regex implementation using virtual machines.

  @copyright    Copyright (c) 2016, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

// Declarations:

enum code {
  Char, Match, Jump, Split, Save
};

typedef struct instr instr;
struct instr {
  enum code code;
  char c;
  size_t s;
  instr *x, *y;
  size_t lastidx;
};

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

void printinstr(instr *i)
{
  switch (i->code) {
  case Char:
    printf("char '%c'\n", i->c);
    break;
  case Match:
    printf("match\n");
    break;
  case Jump:
    printf("jump pc%+d\n", (intptr_t)(i - i->x));
    break;
  case Split:
    printf("split pc%+d, pc%+d\n", (intptr_t)(i - i->x), (intptr_t)(i - i->y));
    break;
  case Save:
    printf("save %u\n", i->s);
  }
}

// Printing, for diagnostics

void printprog(instr *prog, size_t ninstr) {
  for (size_t i = 0; i < ninstr; i++) {
    printf("% 2d: ", i);
    printinstr(prog + i);
  }
  printf("\n");
}

void printthreads(thread_list *tl, instr *prog, size_t nsave) {
  for (size_t i = 0; i < tl->n; i++) {
    printf("T%u@pc=%d{", i, (intptr_t) (tl->t[i].pc - prog));
    for (size_t j = 0; j < nsave; j++) {
      printf("%u,", tl->t[i].saved[j]);
    }
    printf("} ");
  }
  printf("\n");
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

#define nelem(x) (sizeof(x)/sizeof((x)[0]))

int main(int argc, char **argv)
{
  size_t *saved = NULL;
  instr prog[] = {
    {.code=Save, .c='\0', .x=NULL, .y=NULL, .lastidx=0, .s=0},
    {.code=Char, .c='a', .x=NULL, .y=NULL, .lastidx=0},
    {.code=Split, .c='\0', .x=NULL, .y=NULL, .lastidx=0},
    {.code=Save, .c='\0', .x=NULL, .y=NULL, .lastidx=0, .s=1},
    {.code=Char, .c='a', .x=NULL, .y=NULL, .lastidx=0},
    {.code=Split, .c='\0', .x=NULL, .y=NULL, .lastidx=0},
    {.code=Char, .c='b', .x=NULL, .y=NULL, .lastidx=0},
    {.code=Match, .c='\0', .x=NULL, .y=NULL, .lastidx=0}
  };
  prog[2].x = prog + 1;
  prog[2].y = prog + 3;
  prog[5].x = prog + 4;
  prog[5].y = prog + 6;

  printprog(prog, nelem(prog));

  char *test[] = {
    "ab", "aab", "aaab", "aaaaaab", "b", "aaa"
  };

  for (size_t i = 0; i < nelem(test); i++) {
    int res = execute(prog, nelem(prog), test[i], &saved);
    printf("\"%s\": %d", test[i], res);
    if (res != -1) {
      printf(", {%u, %u}\n", saved[0], saved[1]);
    } else {
      printf("\n");
    }
    free(saved);
    saved = NULL;
  }
}
