;; Regex: "[a-ce -]+"

;; TOKENS:
;; nextsym(): {LBracket, '['}
;; nextsym(): {CharSym, 'a'}
;; nextsym(): {Minus, '-'}
;; nextsym(): {CharSym, 'c'}
;; nextsym(): {CharSym, 'e'}
;; nextsym(): {CharSym, '\x20'}
;; nextsym(): {Minus, '-'}
;; nextsym(): {RBracket, ']'}
;; unget(): buffering {Minus, '-'}
;; nextsym(): unbuffering {RBracket, '%'}
;; nextsym(): {Plus, '+'}
;; nextsym(): {Eof, ''}
;; PARSE TREE:
;; REGEX {
;;  SUB {
;;   EXPR {
;;    TERM {
;;     LBracket:'['
;;     CLASS {
;;      CharSym:'a'
;;      CharSym:'c'
;;      CLASS {
;;       CharSym:'e'
;;       CLASS {
;;        CharSym:'\x20'
;;        CLASS {
;;         Minus:'-'
;;        }
;;       }
;;      }
;;     }
;;     RBracket:']'
;;    }
;;    Plus:'+'
;;   }
;;  }
;; }
;; BEGIN GENERATED CODE:
L1:
    range a c e e \x20 \x20 - -
    split L1 L2
L2:
    match
;; BEGIN TEST RUNS:
;; "ab-e": match(4) (0, 0) 
;; "aaabbbcc eee": match(12) (0, 0) 
;; "abcd": match(3) (0, 0) 
