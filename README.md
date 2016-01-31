regex
=====

This is a regular expression engine built on the theory of finite automata,
combined with the power of a "virtual machine" abstraction.  It is destined to
replace the engine in [libstephen](https://github.com/brenns10/libstephen).

The current libstephen regex engine is somewhat effective, but the code is
pretty difficult to understand, and it's not terribly efficient.  So, this new
regular expression implementation is destined to replace that one in my
library.

This engine is based on a "bytecode" approach to regular expressions.  While
this approach is still based on finite automata, it is a bit easier to think
about and extend with new features, such as capturing groups and special
operators.  I learned about this approach in a [series of articles][re] about
implementing regular expressions by [Russ Cox][rsc].  The code for the virtual
machine is very similar to his, to the point where I might classify my VM as a
derivative work.

How It Works
------------

### Compiling

Regular expressions are a (very) simple programming language, and thus I use a
common compiler architecture.

1. Lexical analysis is very simple in this case.  Keeping this phase separate
   makes dealing with things like escaped characters pretty simple.  See
   [grammar.md][] for a description of the tokens.  The (small) lexical analysis
   code is in [src/lex.c][].  The output of this phase is a sequence of "tokens"
   and their corresponding character.
2. I defined a formal grammar for the regular expression language.  Then, I
   implemented a recursive descent parser to parse this language.  The grammar
   is defined in [grammar.md][] and the parser is in [src/parse.c][].  The
   output of this phase is a parse tree.
3. From the parse tree, we then generate bytecode for the Pike VM (described
   below).  During code generation, bytecode is represented as a linked list of
   "fragments", each one containing an instruction and an associated "ID" that
   can be used as the target of jumps.  Once code generation is complete,
   instructions are copied into a single array, and jump targets are resolved.
   The code for this is in [src/codegen.c][].

All of these steps are performed by the `recomp()` function, defined in
[src/parse.c][].

Compiled bytecode can be inspected using the `write_prog()` function, which will
write a textual (assembly-like) representation of the bytecode to a file.
Additionally, there is a `read_prog()` function that can read this same
assembly-like language into VM bytecode.

### Executing

Once the bytecode is created, it may be passed to the `execute()` function, in
[src/pike.c][].  This function is based on Russ Cox's description of the Pike
VM, and you can read more about it in his second article of the aforementioned
series.

[re]: https://swtch.com/~rsc/regexp/
[rsc]: https://swtch.com/~rsc/
