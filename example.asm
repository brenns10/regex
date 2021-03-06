;; Regex: "(a+)(b+)"

;; TOKENS:
;; nextsym(): {LParen, '('}
;; nextsym(): {CharSym, 'a'}
;; nextsym(): {Plus, '+'}
;; nextsym(): {RParen, ')'}
;; nextsym(): {LParen, '('}
;; nextsym(): {CharSym, 'b'}
;; nextsym(): {Plus, '+'}
;; nextsym(): {RParen, ')'}
;; nextsym(): {Eof, ''}
;; PARSE TREE:
;; REGEX {
;;  SUB {
;;   EXPR {
;;    TERM {
;;     LParen:'('
;;     REGEX {
;;      SUB {
;;       EXPR {
;;        TERM {
;;         CharSym:'a'
;;        }
;;        Plus:'+'
;;       }
;;      }
;;     }
;;     RParen:')'
;;    }
;;   }
;;   SUB {
;;    EXPR {
;;     TERM {
;;      LParen:'('
;;      REGEX {
;;       SUB {
;;        EXPR {
;;         TERM {
;;          CharSym:'b'
;;         }
;;         Plus:'+'
;;        }
;;       }
;;      }
;;      RParen:')'
;;     }
;;    }
;;   }
;;  }
;; }
;; BEGIN GENERATED CODE:
    save 0
L1:
    char a
    split L1 L2
L2:
    save 1
    save 2
L3:
    char b
    split L3 L4
L4:
    save 3
    match
;; BEGIN TEST RUNS:
;; "aabb": match(4) (0, 2) (2, 4) 
;; "abbbb": match(5) (0, 1) (1, 5) 
;; "aaaab": match(5) (0, 4) (4, 5) 
;; "bb": no match
;; "aa": no match
