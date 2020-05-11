/*******************************************************************/
/* data_manager.h - constants defentions and function declarations */
/*******************************************************************/
#ifndef MAMAN14_DATA_MANAGER_H_
#define MAMAN14_DATA_MANAGER_H_

#include "symbols.h"
#include "assembler.h"

#define MAX_AMOUNT_OF_ARGS 40 /* max amount of .data arguments (40 spaces + 40 numbers... */ 
#define MAX_STRING_LENGTH 70 /* max length of .string argument (80 - strlen(".string \"...\")" */

/* function declarations - see full detail API for each function in data_manager.c */
  int get_data (char* line, signed int converted_num_list[], int *amount_of_args, unsigned int line_num, unsigned int *error_flag);
  int set_data (char data_image[MAX_OUTPUT_DATA_LINES][BITS_IN_REGISTER+1], unsigned int *DC, int data_arguments[MAX_AMOUNT_OF_ARGS], int amount_of_args, unsigned int line_num, unsigned int *error_flag);
  int get_string (char* line, char char_arguments[], int *amount_of_args, unsigned int line_num, unsigned int *error_flag);
  int set_string (char data_image[MAX_OUTPUT_DATA_LINES][BITS_IN_REGISTER+1], unsigned int *DC, char char_arguments[MAX_STRING_LENGTH], int amount_of_args, unsigned int line_num, unsigned int *error_flag);

#endif /* MAMAN14_DATA_MANAGER_H_ */

