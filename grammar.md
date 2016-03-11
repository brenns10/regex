# Regular Expression Grammar

## Tokens

Each meta-character (`( ) [ ] + - * ? ^ |`) is its own token type.  Regular
(non-special) characters have a token type `CharSym`.  This includes characters
that would have been escaped (such as `\n` or `\\`).  Next, there is the
"special" token, which corresponds to the special character classes, which look
like escaped characters (eg. `\b`, `\w`, `\s`, or `\d`).  Finally, there is an
`Eof` token, which tells the parser that the input has been exhausted.

## Grammar

There are five non-terminal symbols in the grammar that I have defined for
regular expressions.

- `REGEX`: the top-level symbol representing a full regular expression.
- `SUB`: a sub-regex, which consists of anything between `|` operators.
- `EXPR`: an expression that can be concatenated with adjacent expressions.
  Can be a term followed by any of the repetition operators (`+ * ?`).
- `TERM`: either a character, a special character class, a parenthesized regex,
  or a character class.
- `CLASS`: the internal contents of a character class.

Here is the grammar I created:

```
  REGEX (-)-> SUB
        (-)-> SUB | REGEX

  SUB   (-)-> EXPR
        (-)-> EXPR SUB

  EXPR  (-)-> TERM
        (-)-> TERM +
        (-)-> TERM + ?
        (-)-> TERM *
        (-)-> TERM * ?
        (-)-> TERM ?
        (-)-> TERM ? ?

  TERM  (1)-> char <OR> . <OR> - <OR> ^ <OR> special
        (2)-> ( REGEX )
        (3)-> [ CLASS ]
        (4)-> [ ^ CLASS ]

  CLASS (1)-> CCHAR - CCHAR CLASS
        (2)-> CCHAR - CCHAR
        (3)-> CCHAR CLASS
        (4)-> CCHAR
        (5)-> -

  CCHAR (-)-> char <or> . <OR> ( <OR> ) <OR> + <OR> * <OR> ? <OR> |
```

The numbers label the production number, which is actually recorded in the parse
tree data structure in order to make it simpler to generate code.
