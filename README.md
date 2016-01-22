regex
=====

Once upon a time, I implemented a regular expression engine in C.  It was based
on my C data structures library and the theory of nondeterministic finite
automata.  Its design wasn't very good, but since it had a solid theoretical
foundation, it succeeded at doing the basic things that a regular expression
engine should be able to do.  The problem was that it was so needlessly complex
that it couldn't be easily understood or extended.  When the time came to add
capturing to the regular expression engine, I had trouble even hacking on a
partially effective solution.

Then I learned about a [series of articles][re] about implementing regular
expressions by a wickedly smart person named [Russ Cox][rsc].  All of the code
in these articles is simple and well-designed, and there is even an article all
about a virtual-machine based approach, which is very extendable.  So, I decided
to implement a new engine based on Russ's bytecode and the Pike virtual
machine.  These will eventually be merged into libstephen as a replacement for
my old regex implementation.  As of now, the code is remarkably similar to
Russ's own code.  I did write my own using his article and some of his code as a
reference, so I would probably classify this code as a derivative of his own,
which is released under a BSD style license.

[re]: https://swtch.com/~rsc/regexp/
[rsc]: https://swtch.com/~rsc/
