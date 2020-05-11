/*************************************************************************************************************************/
/*                                                code_manager.c                                                         */
/*************************************************************************************************************************/
/* The code_manager.c module is responsible for handeling the code image of the memory - processing machine instructions */
/* containing opcodes, parsing and interpreting them to diagnose it's group, arguments and addressing methods inorder to */
/* to be able to code it to raw binary data. upto MAX_CODE_OUTPUT_LINES could be saves in the code imange data base.     */
/*************************************************************************************************************************/

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "assembler.h"
#include "code_manager.h"

/*************************************************************************************************************************/
/*                                             get_instruction_args                                                      */
/*************************************************************************************************************************/
/* This is the main function in charge of processing and parsing machine code instructions. It parses the operands and is*/
/* responsible for determining who the operands are what are their addressing methods, inorder that the assembler.c will */
/* know how to handle them. If the addressig method is immediate or direct register the function already converts it to  */
/* binary representation and passes it back to the assembler inorder to update it in the code image. If it is with symbol*/
/* which require address information, the function saves the symbols name and source line IC in order to be able to come */
/* back after the whole file scan and replace the appropiate line in the code image with relevant address / destination. */
/* The function recevies a character string of a machine instruction who's opcode was allready extracted. It parses the  */
/* operands and saves them and their characteristics on the array, issolating source and target operands and allowing the*/
/* assembler function to know exactly how to operate accordingly to the results of this function.                        */ 
/*************************************************************************************************************************/ 
int get_instruction_args (char *source_input, instrcution_struct target_args[MAX_CODE_ARGS], int *num_of_args, opcode *op_table, assembly_register *reg_table, unsigned line_num, unsigned int *error_flag)
{
  int i; /* index for looping an array that will save the operands */
  int j; /* util index for scanning register and opcode tabels */
  char *char_ptr = NULL; /* char pointer for parsing the instrunction */
  char *char_util_ptr = NULL; /* another utilty char pointer */
  char *current_token = NULL; /* pointer to current single token/word/field/operand being processed */
  char *original_position = NULL; /* char pointer used for back and "remembering" origin point of parser */
  static char temp_token[MAX_LINE_LENGTH]; /* util temp array used to save a token and examining it */
  static char symbol_name[MAX_LINE_LENGTH]; /* util temp array used for saving symbols if found for examinging them */
  static unsigned char is_label = NO; /* flag if a label of a symbol was found in the operands */
  static unsigned int label_length = 0; /* length of label operand if found */
  static unsigned int current_token_length = 0; /* length of current token found */
  unsigned char has_unsatisfied_comma = NO; /* flag to check if there is a missing operand or comma on instruction */ 
  int scan_success = 0; /* scan indicator for scanf success or failure while requesting a specific format scan */ 
  (*num_of_args) = 0; /* amount of operands to determine if it suites the opcode group (0,1 or 2 operands) */
  
  /* addressing methods binary representation for concatinating machine words before image is updated */ 
  const char immediate_bin[] = "00"; 
  const char direct_bin[] = "01";
  const char destination_bin[] = "10";
  const char direct_register_bin[] = "11";
  const char ARE_absolute[] = "00";
  
	
  current_token = source_input; /* point to begining of input instructuion */
  while(isspace(*current_token)) /* skip spaces if necessary */
    current_token++;
  original_position = current_token; /* save original current position on input line for backup */

  /* validate the line is not empty, if it is - return 0 amount of arguments */
  scan_success = sscanf(current_token, "%[^\b\t\n, ]", temp_token);
  if (!scan_success || scan_success == EOF)
    return *num_of_args;

  /* if line is not empty, save current token and calculate it's length */
  current_token = temp_token; /* scanf saved it in temp_token */ 
  current_token_length = 0;
  for(j=0; *(current_token+j) != ',' && *(current_token+j) != '\n' && *(current_token+j) != '\0'; j++)
    current_token_length++;

  /* check if a comma delimiter exists on end of current operand */
  if (current_token[current_token_length] == ',')
    current_token[current_token_length] = '\0'; /* if so - terminate it */
  else /* if not - turn on flag for expected comma if more operands exist */ 
    has_unsatisfied_comma = YES;


  /* loop on the machine instruction from 1 to 2 times depending on source and target operands */
  for(i=0; i < MAX_CODE_ARGS && current_token != NULL; i++)  
  
  { /* skip spaces if necessary */ 
    while(*current_token != '\n' && isspace(*current_token)) 
      current_token++;

    /* check the first character and continue determination accordingly */
    if (*current_token == '#') /* potentially immediate */
    {
      ++current_token; /* advance pointer to operand which follows the # */
      if (check_num(current_token)) /* validate the operand is a legal number */
      { /* if so, save addressing method & the binary representation of the operand */
        target_args[i].addressing_method_decimal = IMMEDIATE;
        strcpy(target_args[i].addressing_method_binary, immediate_bin);
        convert_decimal_to_binary(atoi(current_token), temp_token);

        /* copy the desired length of binary digits from suffix  */
        char_ptr = temp_token + strlen(temp_token) - BITS_IN_ADDITIONAL_WORD; /* go back from the end */
        strncpy(target_args[i].additional_word, char_ptr, BITS_IN_ADDITIONAL_WORD); /* save the desired part */
        strncpy(target_args[i].additional_word+BITS_IN_ADDITIONAL_WORD, ARE_absolute, strlen(ARE_absolute)); /* concatinate ARE bits */
        (*num_of_args)++; /* update argument counter */
        current_token = original_position + current_token_length;
      }
      else /* argument which follows # is not a number */
      {
        fprintf(stderr,"Error: Line %d: Illegal number \"%s\" appeard after \'#\'!\n", line_num, current_token);
        (*error_flag)++;
        return NOT_FOUND;
      }
    }
    else if (isalpha(*current_token)) /* potentially a register or label */
    { /* check if it's a register */
      if(((j = register_lookup(current_token, reg_table)) >= 0))
      { /* if so - save method and register value in binary representation */
        target_args[i].addressing_method_decimal = DIRECT_REGISTER; /* save method in decimal*/
        strcpy(target_args[i].addressing_method_binary, direct_register_bin); /* save method in binary*/
        convert_decimal_to_binary(j, temp_token); /* transalte register name to binary */
        /* copy the desired length of binary digits from suffix  */
        char_ptr = temp_token + strlen(temp_token) - REGISTER_LENGTH; /* go back from the end */

        /* save regitser binary representation for additional word */
        if (i == SOURCE) /* if the reister is a source operand, save 5 left bits */
        {
          strncpy(target_args[i].additional_word, char_ptr, REGISTER_LENGTH); /* save the desired part */
          char_ptr = target_args[i].additional_word + REGISTER_LENGTH; /* advance to next 5 bits */
          for (j=0; j < REGISTER_LENGTH; j++)
            char_ptr[j] = '0';/* fill 5 right bits with zero's */
          strncpy(char_ptr+REGISTER_LENGTH, ARE_absolute, strlen(ARE_absolute)); /*concat ARE bits */
        }
        else /* i == TARGET operand, save 5 right bits */
        { /* save the desired part at 5 right bits (start to copy into index 5 of additional word)*/
          strncpy(target_args[i].additional_word+REGISTER_LENGTH, char_ptr, REGISTER_LENGTH);
          char_ptr = target_args[i].additional_word;
          for (j=0; j < REGISTER_LENGTH; j++)
            char_ptr[j] = '0';/* fill 5 left bits with zero's */
          char_ptr += REGISTER_LENGTH; /* advance ptr to last 2 bits of word */
          strncpy(char_ptr + REGISTER_LENGTH, ARE_absolute, strlen(ARE_absolute)); /* concatinate ARE bits */
        }
        (*num_of_args)++;

        current_token = original_position + current_token_length;
      }
      /* if not a register, maybe it's a label - check if it's name is legal */
      else if((label_length = check_symbol_syntax(current_token, symbol_name, reg_table, op_table, line_num, error_flag, &is_label)))
      { /* if so - save method and label name */
        target_args[i].addressing_method_decimal = DIRECT; /* save method in decimal*/
        strcpy(target_args[i].addressing_method_binary, direct_bin); /* save method in binary*/
        symbol_name[label_length] = '\0'; /* save label name */
        strcpy(target_args[i].operand[0], symbol_name);
        (*num_of_args)++;
        current_token = original_position + label_length;
      }
      else /* alphanumeric prefix and te argument is not a label and not a register */
      {
        fprintf(stderr, "Error: Line %d: Illegal argument!\n", line_num);
        (*error_flag)++;
        return NOT_FOUND;
      }
    }
    else if (*current_token == '~') /* potentially destination addressing */
    { /*advance pointer */
      char_ptr = current_token;
      char_ptr++;
      while(isspace(*char_ptr))
        char_ptr++;

      if ((char_ptr == NULL) || (*(char_ptr-1) == '\n')) /* if missing operands after tilda delimiter */
      {
        fprintf(stderr, "Error: Line %d: Illegal syntax! missing arguments after \'~\' delimiter!\n", line_num);
        (*error_flag)++;
        return NOT_FOUND;
      }

      if (*char_ptr != '(') /* if left closure is missing */ 
      {
        fprintf(stderr, "Error: Line %d: Missing left closure \'(\' delimiter!\n", line_num);
        (*error_flag)++;
        return NOT_FOUND;
      }
      char_ptr++;
      while(isspace(*char_ptr)) /* skip spaces if necessary */
        char_ptr++;

      if ((char_ptr == NULL) || (*(char_ptr-1) == '\n')) /* if missing arguments after left closure */ 
      {
        fprintf(stderr, "Error: Line %d: Illegal syntax! missing arguments after \'(\' delimiter!\n", line_num);
        (*error_flag)++;
        return NOT_FOUND;
      }
		
	  /* validate the variable name after the left closer is a legal name for a label */ 
      if(!(label_length = check_symbol_syntax(char_ptr, symbol_name, reg_table, op_table, line_num, error_flag, &is_label)))
      {
         fprintf(stderr, "Error: Line %d: \"%s\" is not a legal name for a label!\n", line_num, current_token);
         (*error_flag)++;
         return NOT_FOUND;
       }
		
	  /* save label name of first operand */	
      symbol_name[label_length] = '\0'; 
      strcpy(target_args[i].operand[0], symbol_name); 

      char_ptr = source_input + current_token_length;
      while(isspace(*char_ptr))
        char_ptr++;
      if ((char_ptr == NULL) || (*(char_ptr-1) == '\n'))
      {
        fprintf(stderr, "Error: Line %d: Illegal syntax! missing arguments after label %s!\n", line_num, target_args[i].operand[0]);
        (*error_flag)++;
        return NOT_FOUND;
      }
		
	  /* validte there is a comma delimiter between the two symbol operands */ 	
      if (*char_ptr != ',' && has_unsatisfied_comma)
      {
        fprintf(stderr, "Error: Line %d: Illegal syntax! missing comma delitmeter between label \"%s\" and \"%s\"!\n", line_num, target_args[i].operand[0], current_token);
        (*error_flag)++;
        return NOT_FOUND;
      }

      /* skip spaces if necessary */ 
      char_ptr++;
      while(isspace(*char_ptr))
        char_ptr++;
      if (*char_ptr == '\0')
        char_ptr++;

      /* validate there is a second operand after the comma delimiter */ 
      if ((char_ptr == NULL) || (*(char_ptr-1) == '\n'))
      {
        fprintf(stderr, "Error: Line %d: Illegal syntax! label argument missing after the comma delitmeter which follows label \"%s\"!\n", line_num, target_args[i].operand[0]);
        (*error_flag)++;
        return NOT_FOUND;
      }
		
	  /* validate there is a right closer after the second operand */ 
      if ((char_util_ptr = strchr (char_ptr, ')')))
      {
        label_length = char_util_ptr - char_ptr;
        strncpy(temp_token, char_ptr, label_length);
        temp_token[label_length] = '\0';
      }
		
	  /* validate the second operands is a legal name for a label */ 
      if(!(label_length = check_symbol_syntax(temp_token, symbol_name, reg_table, op_table, line_num, error_flag, &is_label)))
      {
         fprintf(stderr, "Error: Line %d: \"%s\" is not a legal name for a label!\n", line_num, current_token);
         (*error_flag)++;
         return NOT_FOUND;
      }
		 
      /* save label name for second operand */		 
      symbol_name[label_length] = '\0'; 
      strcpy(target_args[i].operand[1], symbol_name);
      char_ptr += label_length;
      if (*char_ptr == '\0')
        char_ptr++;
		
	  /* skip spaces if necessary */ 	
      while(isspace(*char_ptr))
        char_ptr++;
        
      /* validate that the next non white character after the second label is a right closer ')' */  
      if ((char_ptr == NULL) || (*(char_ptr-1) == '\n') || ((*char_ptr) != ')'))
      {
        fprintf(stderr, "Error: Line %d: Missing right closure \')\' delimiter!\n", line_num);
        (*error_flag)++;
        return NOT_FOUND;
      }
		
	  /* save current argument (source/target) addressing method information */ 	
      target_args[i].addressing_method_decimal = DESTINATION; /* save method in decimal*/
      strcpy(target_args[i].addressing_method_binary, destination_bin); /* save method in binary*/
      (*num_of_args)++; /* update argument counter */ 
      char_ptr++; /* advance to begining of next token */
      current_token = char_ptr;
    }
	
    else /* error - chacrter after second label is not a right closer  */ 
    {
      fprintf(stderr, "Error: Line %d: Argument \"%s\" is not a legal operand!\n", line_num, current_token);
      (*error_flag)++;
      return NOT_FOUND;
    }

    /* after processing first operand - check if there is any other tokens to fetch other than spaces */
    while(*current_token != '\n' && isspace(*current_token))
      current_token++;
    scan_success = sscanf(current_token, "%[^\b\t\n0]", temp_token);
    original_position = current_token;
    current_token = temp_token;
	
	/* if no other characters, return information gatherd so far back to the assembler */ 
    if (current_token == NULL || scan_success == EOF || !(scan_success) || i > 0)
      return *num_of_args;
    else /* there is at least one more chacrter to parse */ 
    {
     if (has_unsatisfied_comma && *current_token != ',') /* if there is no comma delimiter - report error */ 
     {
       fprintf(stderr, "Error: Line %d: Missing comma delimeter before argument %s!\n", line_num, current_token);
       (*error_flag)++;
       return NOT_FOUND;
     }
		
	 /* if there is a comma delimiter - try to fetch a token that proceeds after it*/ 	
     if (*current_token == ',') 
     {
       current_token++;
       while (*current_token != '\n' && isspace(*current_token))
         current_token++;
       scan_success = sscanf(current_token, "%[^\b\t\n0 ]", temp_token);
       current_token = original_position + 1; /*skip the comma delimeter */
     }
     /* if fetching argument attmemt failed - no further arguments after comma - report error ! */ 
     if (current_token == NULL || scan_success == EOF || !(scan_success))
     {
       fprintf(stderr, "Error: Line %d: Unsatesfied comma delimiter at end of line, possibly missing argument!\n", line_num);
       (*error_flag)++;
       return NOT_FOUND;
     }
    }
  } /* proceed to next iteration inorder to get next target operand */
  return *num_of_args; /* after getting both source and target operands, return information to assembler */ 
}/********************************************************************************************************************/

/*********************************************************************************************************************/
/*                                      code_struct_initialize                                                       */
/*********************************************************************************************************************/
/* This function is called from the assembler parser for each assembly input line scanned. It's job is to clean and  */
/* clear the first machine word structure from garbage values of previous iterations. it initializes it to zero as   */
/* default value, so incase of one argument opcodes and extern labels, the bits unused are already set and ready for */
/* code image updates. for each set of bits in the first machine word, the exact amount of N bits are copied using   */
/* strNcpy, where as each set of bits N = 2, except opcode, which take N = 4 bits.                                   */
/*********************************************************************************************************************/
void code_struct_initialize (code_struct* code_bit_struct)
{ 
  strncpy(code_bit_struct -> group, "00", 2);
  strncpy(code_bit_struct -> opcode, "0000", 4);
  strncpy(code_bit_struct -> source_operand_addressing, "00", 2);
  strncpy(code_bit_struct -> target_operand_addressing, "00", 2);
  strncpy(code_bit_struct -> ERA, "00", 2);
}/*********************************************************************************************************************/

/************************************************************************************************************************/
/*                                             concat_first_word_components                                             */
/************************************************************************************************************************/
/* This function receives a structre with the fields in the form of the first machine word (group, copode, addressing   */
/* methods and ARE, and is responsible for taking all fields and concatinating them to a string vector of exact 12 bits */
/* prepareing it for the code image update as a one unit string  argument, instead of updating each fields separately.  */
/************************************************************************************************************************/
void concat_first_word_components (char binary_vector[BITS_IN_REGISTER], code_struct *components)
{
  char *current_component = components -> group; /* point at first component of the machine word */
  char *util_ptr = binary_vector; /* pointer of the binary 12 chacrcter string vector */
  int current_component_length; /* length of current component */ 

  while (util_ptr < binary_vector + BITS_IN_REGISTER) 
  { /* as long we didn't update all 12 bits yet */ 
    current_component_length = strlen(current_component); /* compute length of current component */ 
    strncpy(util_ptr, current_component, current_component_length); /* copy the component bits to the string vector */
    current_component += current_component_length + 1; /* advance component pointer to point to next one in struct */
    util_ptr += current_component_length; /* advance bit vector pointer to point to next set of bits for update */
  }
}/************************************************************************************************************************/

/*************************************************************************************************************************/
/*                                          set_code                                                                     */
/*************************************************************************************************************************/
/* This function gets a character string vector argument of 12 bits, and the desired address on code image, and updates  */
/* the code image memory with the set of bits given at the string vecotr. it returns next free address of code image if  */
/* update was successful and 0 if error ocured during to lack out of memort space in the code image.                     */
/*************************************************************************************************************************/
int set_code (char code_image[MAX_OUTPUT_CODE_LINES][BITS_IN_REGISTER+1], unsigned int *IC, 
			  char *binary_machine_instruction, unsigned int line_num, unsigned int *error_flag)
{
  if (*IC < MAX_OUTPUT_CODE_LINES) /* assure there is free memory space on code image */
  { /* copy the bits from string vector argument to desired address */
    strncpy(code_image[*IC], binary_machine_instruction, BITS_IN_REGISTER);
    (*IC)++; /* update the instruction counter to point to next freec cell in code image memory */ 
  }
  else /* if no more memory left - report error */
  {  
    fprintf(stderr, "Error: Line %d: Not enough free memory space for saving more data!\n", line_num);
    error_flag++;
    return 0; /* if error */
  }
  return *IC; /* if successful */
}/************************************************************************************************************************/

