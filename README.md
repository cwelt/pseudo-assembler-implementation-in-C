# pseudo-assembler-implementation-in-C

## Description 
This application simulates an assembler: 
it receives input files written in a pseudo predefined assembly language, 
parses them, separating data section from code section, 
managing symbols table, internal and external refereneces, etc,
converts it to binary code and then finally produces as output the object file in hexa-decimal base code, 
error report file (if any), entry file, and external reference file. 

## Further Details 
See [Project Description](Project%20Decription.pdf) for a complete decsription of the application requirements specification. 

## Assembly Input File Example 
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
    
    
## Output Object File Example 
    	Base 16 Address 	 Base 16 machine code

                                 14    B
	      64 					 824
	      65 					 078
	      66 					 001
	      67 					 8B4
	      68 					 100
	      69 					 1E2
	      6A 					 644
	      6B 					 001
	      6C 					 700
	      6D 					 FEC
	      6E 					 8FC
	      6F 					 090
	      70 					 5CC
	      71 					 000
	      72 					 82C
	      73 					 050
	      74 					 00C
	      75 					 684
	      76 					 001
	      77 					 3C0
	      78 					 061
	      79 					 062
	      7A 					 063
	      7B 					 064
	      7C 					 065
	      7D 					 066
	      7E 					 000
	      7F 					 006
	      80 					 FF7
	      81 					 00F
	      82 					 002
