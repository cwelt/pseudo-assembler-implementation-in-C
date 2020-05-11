/********************************************************************************************************************/
/* code_manager.h - data structures and type defentions and function decalrations for proccessing code instructions */  
/********************************************************************************************************************/
#ifndef CODE_MANAGER_H
#define CODE_MANAGER_H

#include "assembler.h"

/* struct for saving arguments of opcode machine instrunction, used for get_code_arguments function */ 
typedef struct machine_instruction_arguments
{
  char operand[MAX_CODE_ARGS][SYMBOL_MAX_LENGTH+1]; /* array to hold up to two symbols of a single operand */
  unsigned int addressing_method_decimal; /* addressing method code in decimal */
  char addressing_method_binary[3]; /* method in binarty representation */ 
  char additional_word[BITS_IN_REGISTER]; /*if immediate or direct register, this will be used for saving it's binary value*/
} instrcution_struct;

/* strucutre of first word of each machine instruction word - separated to fields - all content here is saved in binary */
typedef struct machine_instruction_first_word_structure
{
  char group[3]; /* group of num of operands - 0, 1 or 2 */
  char opcode[5]; /* opcode number (in binary) */
  char source_operand_addressing[3]; /*source operand addressing method */
  char target_operand_addressing[3]; /* target ""	""	"" */
  char ERA[3]; /* External, Relocatable , Absolute last 2 bits of word */
} code_struct;

/* enumuration of 4 possible addressing methods */ 
enum addressing_method { immediate, direct, destination, direct_register, illegal};

/* addressing method record struct */
typedef struct addressing_method_info_record
{
  enum addressing_method method;
  char operands [MAX_CODE_ARGS+1][SYMBOL_MAX_LENGTH+1];
} address_method_record;

/* util functions declarations - see code_manager.c for full API on each function */
void code_struct_initialize (code_struct* code_bit_struct);
void concat_first_word_components (char binary_vector[BITS_IN_REGISTER], code_struct *components);
int set_code (char code_image[MAX_OUTPUT_CODE_LINES][BITS_IN_REGISTER+1], unsigned int *IC, char *binary_machine_instruction, unsigned int line_num, unsigned int *error_flag);
int get_instruction_args (char *source_input, instrcution_struct target_args[MAX_CODE_ARGS], int *num_of_args, 
						opcode *op_table, assembly_register *reg_table, unsigned line_num, unsigned int *error_flag);
#endif

