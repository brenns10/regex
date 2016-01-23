        ;; NDFA Virtual Machine program to match the regular expression:
        ;;   (a+)(b+)

        ;; Along the sides are the characters or metacharacters that the
        ;; instructions correspond to.

        ;; Note that, although this is superficially similar to an assembler
        ;; file, the format is not quite so forgiving.  The comment character is
        ;; semicolon.  Comments are allowed anywhere, and last until the end of
        ;; the line.  Instructions must be exactly on one line, and arguments
        ;; are separated from instructions by whitespace ONLY.  No commas!
        ;; Labels are lines that end with a colon, and they should not contain
        ;; whitespace.  Labels MAY NOT occur on the same line as an instruction.

        save 0                  ; (
L1:
        char a                  ; a
        split L1 L2             ; +
L2:
        save 1                  ; )
        save 2                  ; (
L3:
        char b                  ; b
        split L3 L4             ; +
L4:
        save 5                  ; )
        match
