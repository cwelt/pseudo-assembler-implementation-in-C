/*********************************************************************************************************************/
/* assembler.h - Instruction Set Architechture constants, functions and data structures defentions and declaretinons */
/*********************************************************************************************************************/

#ifndef MAMAN14_ASSEMBLER_H_
#define MAMAN14_ASSEMBLER_H_

#include <stdio.h>
#include "symbols.h"

#define NUM_OF_OPCODES 16 /* total amount of opcode machine instructions in this assembly ISA */ 
#define NUM_OF_REGISTERS 8 /* total amount of register on this simulated machine */ 
#define NUM_OF_ADDRESSING_METHODS 4 /* total amout on addresing method types */ 
#define BITS_IN_BYTE 8 /* amount of bits in a standard single byte */ 
#define MAX_LINE_LENGTH 80 /* max characters allowed for an assembly source line */
#define MAX_INPUT_LINES 100 /* max input lines alloed for an assembly source file */ 
#define MAX_OUTPUT_CODE_LINES 500 /* max memory cells reseverd for code image */
#define MAX_OUTPUT_DATA_LINES 500 /* max memory cells reseverd for data image */
#define BITS_IN_ADDITIONAL_WORD 10 /* num of bits in additional word, excluding the ARE bits */ 
#define REGISTER_LENGTH 5 /* register binary representation length in code image */
#define MAX_CODE_ARGS 2 /* max amount of args for machine opcode instructions */
#define MAX_LEGAL_METHODS 4 /* amount of legal addressing method kinds */ 

/* constants for opcode argument gropus */ 
#define ZERO_ARGUMENT_GROUP 0 
#define ONE_ARGUMENT_GROUP 1
#define TWO_ARGUMENT_GROUP 2

/* constants for addressing methods groups */ 
#define IMMEDIATE 0
#define DIRECT 1
#define DESTINATION 2
#define DIRECT_REGISTER 3

/* constants for marking and distinguishing source operand from target operand */ 
#define SOURCE 0 
#define TARGET 1

/* decimal base start offset of memory address used for offseting the initial addresses in object , entry and extern files */ 
#define BASE_START 100

/* constant basic digital bases used */ 
#define BINARY_BASE 2
#define OBJECT_BASE 16

#ifndef BITS_IN_REGISTER 
#define BITS_IN_REGISTER 12 /* amount of bits in a register on this ISA */ 
#endif

#define YES 1 /* flag activation indicator (equivalent for boolean - true )*/
#define NO 0 /* flag de-activation indicator (equivalent for boolean - false )*/
#define NOT_FOUND -1 /* indicator for error reporting in search function that are expection a non negetive index */ 


/* data structure for raw data of an object file */ 
typedef struct object_file_structure
{
  FILE *file_ptr; /* actual pointer to source file */
  char name[FILENAME_MAX]; /* name of file */
  unsigned int IC; /* instruction counter - total amount of machine words in code image */ 
  unsigned int DC; /* data counter - total amount of machine words in data image */ 
  char code_image[MAX_OUTPUT_CODE_LINES][BITS_IN_REGISTER+1]; /* code image two dimensional array of char bits */
  char data_image[MAX_OUTPUT_CODE_LINES][BITS_IN_REGISTER+1]; /* data image two dimensional array of char bits */
  unsigned int errors; /* error amount counter and flag ( if zero - no errors) */
} obj_file_struct; 

/* opcode info record defention */
typedef struct opcode_node
{
    char name[5]; /* name of operation */
    char bin_value[5]; /* binary value of the opcode name */
    char instruction_group_binary[3]; /* group type of the instruction for determining num of args and format*/
    unsigned char instruction_group_decimal; /* group in decimal representation */ 
    char source_operand_legal_methods[MAX_LEGAL_METHODS+1]; /* string of allowed addressing methods for source operand */
    char target_operand_legal_methods[MAX_LEGAL_METHODS+1]; /* string of allowed addressing methods for source operand */
} opcode;

/* register info record defention */
typedef struct register_node
{
    char name[3]; /* name of regsiter  */
    char bin_value[6]; /* binary value of the register name */
} assembly_register; /*register name and value */

/* functions called once for each program execution from main for setting constant tables of opcodes and registers */
void set_register_table (assembly_register table[NUM_OF_REGISTERS]);
void set_opcode_table (opcode table[NUM_OF_OPCODES]);

/* util function declaration - see detailed API for each function in assembler.c */
int opcode_lookup (char *token, opcode opcode_table[NUM_OF_OPCODES]);
int register_lookup (char *token, assembly_register register_table[NUM_OF_REGISTERS]);
int check_symbol_syntax (char* input_line, char* symbol_name, assembly_register *reg_table, opcode *op_table, unsigned int line_num, unsigned int *error_flag, unsigned char *symbol_flag);
char *convert_decimal_to_binary(signed int decimal_num, char * binary_num);
unsigned int convert_binary_to_decimal(const char * binary_num);
int check_legal_addressing_method (int op_index, int actual_method, char operand_role, opcode *op_table);
int power_num (int base, int power);

#endif /* MAMAN14_ASSEMBLER_H_ */
