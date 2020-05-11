############################################################################################
########                        Makefile for  Maman 14	                            ########
############################################################################################

#	constants for field symbol substitution   #
CC = gcc
CFLAGS = -Wall -ansi -pedantic
MATHLIB = -lm
OBJ_MAIN = main.o
OBJ_ASSEMBLER = assembler.o
OBJ_DATA = data_manager.o
OBJ_CODE = code_manager.o
OBJ_SYMBOLS = symbols.o
OBJ = OBJ_MAIN OBJ_ASSEMBLER OBJ_DATA OBJ_CODE OBJ_SYMBOLS
SOURCE_MAIN = main.c
SOURCE_ASSEMBLER = assembler.c
SOURCE_DATA = data_manager.c
SOURCE_CODE = code_manager.c
SOURCE_SYMBOLS = symbols.c

PROG = assembler

# compile commands # 
$(PROG): $(OBJ_MAIN) $(OBJ_ASSEMBLER) $(OBJ_DATA) $(OBJ_CODE) $(OBJ_SYMBOLS) 
	$(CC) -g $(CFLAGS) $(MATHFLIB) $(OBJ_MAIN) $(OBJ_ASSEMBLER) $(OBJ_DATA) $(OBJ_CODE) $(OBJ_SYMBOLS) -o $(PROG)
$(OBJ_MAIN): $(SOURCE_MAIN)
	$(CC) -c $(CFLAGS) $(SOURCE_MAIN) -o $(OBJ_MAIN)	
$(OBJ_ASSEMBLER): $(SOURCE_ASSEMBLER)
	$(CC) -c $(CFLAGS) $(SOURCE_ASSEMBLER) -o $(OBJ_ASSEMBLER)	
$(OBJ_DATA): $(SOURCE_DATA)
	$(CC) -c $(CFLAGS) $(SOURCE_DATA) -o $(OBJ_DATA)	
$(OBJ_CODE): $(SOURCE_CODE)
	$(CC) -c $(CFLAGS) $(SOURCE_CODE) -o $(OBJ_CODE)	
$(OBJ_SYMBOLS): $(SOURCE_SYMBOLS)
	$(CC) -c $(CFLAGS) $(SOURCE_SYMBOLS) -o $(OBJ_SYMBOLS)				
	
clean:
	rm -rf (*~) 
	rm -rf (*.o)
