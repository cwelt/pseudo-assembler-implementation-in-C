
; example of assembly input file with errors


; illegal addressing method example - 
mov #1, #-2

; incompatible amount of arguments 
clr r1, r4

;illegal label name
 Some_Illegal_name_for_a_symbol
 
; declaring a label as entry which is not defined! 
.entry START

;illegal opcode 
movie Deerhunter

;missing double quotes 
.string "this is a string with missing quotes...

;illegal numbers
.data -3, 16.34, +23CF3

;defineing a symbol twice
SHALOM: rts
SHALOM: stop

;illegal register
inc r9

