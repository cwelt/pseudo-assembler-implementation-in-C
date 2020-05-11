.entry LOOP
.entry LENGTH
.extern L3
.extern W
MAIN:    mov ~(K,END), W
add r2,STR
LOOP: jmp             L3
prn #-5
sub #1, r1
inc r0
mov ~(STR,MAIN), r3
bne L3
END: stop
STR: .string "abcdef" 
LENGTH: .data 6, -9, 15
K: .data 2