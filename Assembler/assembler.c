/*************************************************************************************************************************/
/*                                                  assembler.c                                                          */
/*************************************************************************************************************************/
/* The assembler.c module is responsible for processing an individual source file whice is wrriten in assembly language. */
/* It is responsible for main processing of a single file, includeing pipelineing data between the various modules and   */
/* intergrating, keeping track on all data proccessed so far, next inout line left on buffer,memory management and more  */
/* It receives a file struct record and after processing the assembly source file it transfers the record with raw data  */
/* of binary coding, entry and extern entities, error if found and more.                                                 */
/* This method uses only one scan, by saving reqried symbol data along the way in special tables, allowing it to just    */
/* make a few adjustments and adress assigment and then there is no need for a whole second scan.                        */ 
/*************************************************************************************************************************/ 
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "assembler.h"
#include "code_manager.h"
#include "data_manager.h"
#include "symbols.h"

int assemble_file (obj_file_struct *file_record, entry **entry_table, external_symbol **extern_table, opcode opcode_table[NUM_OF_OPCODES], assembly_register register_table[NUM_OF_REGISTERS])
{
/*************************************************************************************************************************/
/*                     primary data initialization, defentions, declarations and assignments                             */
/*************************************************************************************************************************/

  symbol *symbol_ptr; /* utility pointer for manipulating symbols */
  symbol *symbol_table; /*head of symbol table */
  entry *entry_ptr; /* utility pointer for manipulating entry marked symbols */
  direct_address_symbol *direct_address_symbol_ptr; /*utility pointer for manipulating symbols used as direct addressing*/
  direct_address_symbol *symbols_for_replacement_table; /* head of table for saving those symbols */
  distance_addressing_symbol *distance_addressing_symbol_ptr;/*util pointer for manipulating symbols used for distance */
  distance_addressing_symbol *symbols_for_distance_check_table; /* head of table for saving those symbols */
  char symbol_name[SYMBOL_MAX_LENGTH+1]; /*field for saving current input line's symbol name */
  static unsigned int label_length; /* current input line's symbol's name length */
  
  static char current_input_line[MAX_LINE_LENGTH]; /* current input line read from assembly file */
  static char current_input_token[MAX_LINE_LENGTH]; /* current token preocessed from input line */
  char *util_ptr; /* char utility pointer for manipulating input */ 
  static int data_arguments[MAX_AMOUNT_OF_ARGS]; /* array to save numbers as data arguments for ".data" instructions */
  static char string_arguments[MAX_STRING_LENGTH]; /* array to save a string as data arguments for ".string" instructions */
  instrcution_struct code_arguments[MAX_CODE_ARGS]; /* special struct to hold the args for all various code instructions */
  static code_struct code_bit_structure; /*struct shaped as first instruction word (group,opcode,source,..) */
  static char binary_machine_instruction [BITS_IN_REGISTER];/*bit struct shaped vector used for updating code image*/
  static char instruction_group; /* group indicator flag of opcode (0 args, 1 arg, 2 arg) */
  static int amount_of_args; /* generic argument counter for the various instructuions */ 
  unsigned int line_counter = 0; /* assembly language source line counter */
  unsigned char has_symbol = NO; /*flag if a symbol exits for current soruce line or not */
  unsigned char input_too_long = NO; /* flag if current input line length overflows from maximum allowed */
  unsigned char successful_update = NO; /* flag if current operation/update at database was successful */
  unsigned int has_error = NO;  /* generic error flag for file*/
  int i,k; /* generic indexes */
  
  /* pointer initialization from garbage values */   
  util_ptr = NULL;
  symbol_table = NULL;
  symbol_ptr = NULL;
  *entry_table = NULL;
  *extern_table = NULL;
  entry_ptr = NULL;
  symbols_for_replacement_table = NULL;
  direct_address_symbol_ptr = NULL;
  symbols_for_distance_check_table = NULL;
  distance_addressing_symbol_ptr = NULL;

  i = k = 0; /* initialize indexs and counters */; 
  file_record -> IC = 0; /* Instruction Counter for code image */
  file_record -> DC = 0; /* Data Counter for data image */ 


  /**********************************************************************************************************************/
  /*                                            First Main Scan                                                         */
  /**********************************************************************************************************************/
  /* (1) scan each line at a time from the assembly source file */ 
  while(fgets(current_input_line, (MAX_LINE_LENGTH+1), file_record -> file_ptr))
  {
    line_counter++; /* update line counter */
    if (strlen(current_input_line) == MAX_LINE_LENGTH)  
    { /* validate line isn't too big to save */
      fprintf(stderr,"Warning: Line %d: input line too long. Only first %d characters would be processed!\n", 
      			line_counter, MAX_LINE_LENGTH);
      input_too_long = YES;
    }
    else
      input_too_long = NO;

    current_input_line[MAX_LINE_LENGTH-1] = '\n'; /* return the line terminator that fgets overwrited with '\0' */
	
	/* intializatin of line variable flags and fields from garbage values */ 
    has_symbol = label_length = amount_of_args = instruction_group = 0; /* intialize variables for new line */
    code_struct_initialize(&code_bit_structure);

    /* (2) check if current line is a comment */ 
    if (current_input_line[0] == ';')
      continue;

    /* (3) check if there is a potential label by searching a terminal ':' */
    if ((util_ptr = strchr(current_input_line, ':'))) /* if so, check syntax of label and save flag if syntax is valid */
      if(!(label_length = check_symbol_syntax(current_input_line, symbol_name, register_table, opcode_table, 
      											line_counter, &has_error, &has_symbol)))
        continue; /* if func return zero, it means the label was syntacticaly invalid - continue to next input line */

    /* (4) parse the first token (other than a label) from input line  */
    strcpy (current_input_token, current_input_line);
    util_ptr = current_input_token + label_length;

    /* (5) check if it's a empty line (spaces & tabs)*/
    if ((util_ptr = strtok(util_ptr, ":\n\t\b ")) == NULL)
    { /* if the rest of the line is empty */
      if (has_symbol) /* and if a label was defined at the begining of line  - */
      { /* raise error flag for missing conetxt */ 
        fprintf(stderr, "Error: Line %d: Label definition failure: missing context!\n", line_counter);
        has_error++;
      }
      continue; /* continue to next assembly source line */ 
    }

    /*****************************************************************************************************/
    /*                                           .data or .string                                        */
    /*****************************************************************************************************/
    /* (6) check if this is a pseudo guidance line for storing data (number, characters...) */
    if ((strcmp(util_ptr, ".data") == 0) || (strcmp(util_ptr, ".string") == 0))
    { /* (6.1) if so check a symbol was defined in top of file */ 
      if (has_symbol)
      { /* (6.1.1) validate that it does not already exist at the symbol table */
        if ((symbol_ptr = check_symbol_existence(symbol_table, symbol_name)))
        {/*(6.1.2) if it does exit report error */ 
          fprintf(stderr, "Error: Line %d: Symbol \"%s\" already defined on line %d!\n", line_counter, 
          														symbol_name, (*symbol_ptr).source_line);
          has_error++;
        }
        else /* symbol is new, does not exist yet */ 
        { /* (6.1.3) add new symbol to symbol table */ 
          symbol_ptr = add_new_symbol(symbol_table, symbol_name, (*file_record).DC, relocatable, data, line_counter);
          if (symbol_ptr != NULL) /* success update in symbol table */ 
            symbol_table = symbol_ptr;
          else /* failure updateing table */ 
          {
            fprintf(stderr, "Error: Line %d: System Failure: no free memory for saving symbol %s.\n", 
            																			   line_counter, symbol_name);
            has_error++; /* update error flag */ 
            continue; /* continue to next source line */ 
          }
        }
      }
	
	  /* (6.2) check if it's a .data instruction */
      if ((strcmp(util_ptr, ".data") == 0))
      { /* (6.2.1) advance parser accordingly */  
        util_ptr = util_ptr + strlen(".data") + 1; 
        /* (6.2.2) get data arguments */ 
        get_data (util_ptr, data_arguments, &amount_of_args, line_counter, &has_error);
        /*(6.2.3) set data in database of data image */
        if (amount_of_args > 0) 
          successful_update = set_data (file_record->data_image, &file_record->DC, data_arguments, 
          													  		amount_of_args, line_counter, &has_error);
      }
	
	  /* (6.3) check if it's .string instructuin */ 
      if ((strcmp(util_ptr, ".string") == 0))
      { /* (6.3.1) advance parser accordingly */ 
        util_ptr = util_ptr + strlen(".string") + 1;
        /* (6.3.2) get string arguments */ 
        get_string (util_ptr, string_arguments, &amount_of_args, line_counter, &has_error);
        /*(6.3.3) set string data in database of data image */
        successful_update = set_string (file_record->data_image, &file_record->DC, string_arguments, 
        															amount_of_args, line_counter, &has_error);
      }
		
	  /* (6.4) validate the data / string was successfully updated in the data image */	
      if (!successful_update)
      { /* if not - report error */ 
        fprintf(stderr,"Error: Line %d: System failure in attempt to update data in the database!\n", line_counter);
        has_error++;
      }
		
	  /* (6.5) clean rest of line if necessary */ 
      if (input_too_long) /* if input line LENGTH was more than MAX_LINE_LENGTH */
        for(i=0; fgetc(file_record->file_ptr) != '\n'; i++)
            ;   /* clear ifp input buffer from the remaining characters of current line */
      
      continue; /*resume to next input line */
    }

    /*****************************************************************************************************/
    /*                                        .extern or .entry                                          */
    /*****************************************************************************************************/
    /* (7) check if this is a pseudo guidance line for marking symbols (.entry, .extern) */
    else if ((strcmp(util_ptr, ".extern") == 0) || (strcmp(util_ptr, ".entry") == 0))
    { /*(7.1) advance parsing pointer accordingly and save the following token */
      strcpy(current_input_token, util_ptr);
      util_ptr = util_ptr + strlen(current_input_token) + 1;
      while (isspace(*util_ptr)) /* skip spaces */
        util_ptr++;
      
      /* (7.2) validate the label name following the guidance is a valid name */   
      if((label_length = check_symbol_syntax(util_ptr, symbol_name, register_table, opcode_table, line_counter, 
      																						&has_error, &has_symbol)))
      { 
        util_ptr = util_ptr + label_length;
        /* (7.3) validate the rest of the line is empty (spaces & tabs)*/
        if ((util_ptr = strtok(util_ptr, "\n\t\b ")) == NULL)
        { /* (7.4) if it's .extern we need to add it's following label to symbol table */
          if (((strcmp(current_input_token, ".extern") == 0)))
          { /* (7.4.1) validate the label declared is not already defined for some symbol on the symbol table */
            if (!(symbol_ptr = check_symbol_existence(symbol_table, symbol_name)))
            { /*(7.4.2) if it's not exist yet - add it to the symbol table with no address */
              symbol_ptr = add_new_symbol(symbol_table, symbol_name, external, external, external_context, line_counter);
              if (symbol_ptr != NULL) 
              { /* (7.4.2.1) if update at table was successful, update head */ 
                symbol_table = symbol_ptr; 
                continue; /* continue to next line */
              }
              else /* (7.4.2.2) in case of failure updating the table, report error */ 
                fprintf(stderr,"Error: Line %d: System Failure: not enough free memory on disk for saving symbol \"%s\"!\n", line_counter, symbol_name);
            }
            else /* (7.4.3)symbol declared as extern is already defined - report error */
              fprintf(stderr, "Error: Line %d: Symbol \"%s\" already defined on line %d!\n",
              														 line_counter, symbol_name,(*symbol_ptr).source_line);
          }
          else /* (7.5) it's a '.entry' statement */
          { /* (7.5.1) add it to entry table */ 
            entry_ptr = add_new_entry(*entry_table, symbol_name, line_counter);
            if (entry_ptr != NULL) 
            { /* (7.5.1.1) - if update successful - update head */
              *entry_table = entry_ptr;
              continue;
            }
            else /* (7.5.1.2) - if update failed - report error */
              fprintf(stderr, "Error: Line %d: System Failure: not enough memory for saving declaration of entry symbol	\"%s\"!\n", line_counter, symbol_name);
          }
        }
        else /* (7.6) too much arguments - report error */ 
          fprintf(stderr, "Error: Line %d: Extern & Entry Label declarations expect one argument exactly!\n", line_counter);
      }
      else /* (7.7) invalid name for a label - report error */ 
        fprintf(stderr, "Error: Line %d: Symbol name which was declared is illegal!\n", line_counter);

      /* if we have not reached the continue statement, it's defently a error - updte flag and continue to next line */
      has_error++;
      continue;
    }
  
    /*****************************************************************************************************/
    /*                                    code machine instructions                                      */
    /*****************************************************************************************************/
    /* (8) validate existance of command (opcode) */
    else if ((i = opcode_lookup(util_ptr, opcode_table)) == NOT_FOUND)
    {
      fprintf(stderr, "Error: Line %d: Command \"%s\" does not exist!\n", line_counter, util_ptr);
      has_error++;
      continue;
    }
    else
    { /* (8.1) if valid, check if symbol exists at this line */
      if (has_symbol)
      { /* (8.1.1) check if it already exists at the symbol table */
        if ((symbol_ptr = check_symbol_existence(symbol_table, symbol_name)))
        { /* (8.1.2) if so - report error */ 
          fprintf(stderr, "Error: Line %d: Symbol \"%s\" already defined on line %d!\n", line_counter,
          																 symbol_name, (*symbol_ptr).source_line);
          has_error++;
          continue;
        }
        else /* (8.1.3) if not - add it to table with code context */ 
        { /* (8.1.3.1) validate the updated succedded */ 
          symbol_ptr = add_new_symbol(symbol_table, symbol_name, file_record->IC, relocatable, code, line_counter);
          if (symbol_ptr != NULL)
            symbol_table = symbol_ptr;
          else 
          { /* (8.1.3.2) if not - report error */
            fprintf(stderr, "Error: Line %d: System Failure: no free memory for saving symbol %s.\n", 
            																			line_counter, symbol_name);
            has_error++;
            continue; 
          }
        }    
       } 

		
	  /* (9) prepare bit coding structure of first word (opcode) */	
      strncpy(code_bit_structure.group, opcode_table[i].instruction_group_binary, 
      												strlen(opcode_table[i].instruction_group_binary));
      strncpy(code_bit_structure.opcode, opcode_table[i].bin_value, strlen(opcode_table[i].bin_value));
      strcpy(current_input_token, util_ptr);
      util_ptr += strlen(opcode_table[i].name) + 1;
		
	  /* (9.1) find group of opcode */ 
      instruction_group = opcode_table[i].instruction_group_decimal;
      
      /* (9.2) get all arguments of current machine instruction given on current input line */
      get_instruction_args(util_ptr, code_arguments, &amount_of_args, opcode_table, register_table, 
      																				line_counter, &has_error);
      /* (9.3) check the arguments were valid by amount and syntax (if not-the amount is minus 1 '-1' ) */
      if ((amount_of_args != instruction_group))
      {
        fprintf(stderr, "Error: Line %d: Illegal syntax for the arguments. Instruction \"%s\" accepts exactly %d arguments and only syntactically correct ones!\n", line_counter, current_input_token, instruction_group);
        has_error++;
        continue;
      }

      /* (9.4) if arguments are legal, continue to proccess according to the instruction group (amount of args) */
      switch ((instruction_group))
      { /* (9.4.1) - group of opcodes with no arguments */ 
        case  (ZERO_ARGUMENT_GROUP):
        { /* (9.4.1.1) - only one machine word - gather all params and update code image  */
          concat_first_word_components(binary_machine_instruction, &code_bit_structure);
          successful_update = set_code(file_record->code_image, &file_record->IC, binary_machine_instruction,
          																					 line_counter, &has_error);
          if (!successful_update) /* report error in case of failure */ 
          {
            fprintf(stderr, "Error: Line %d: System failure in attempt to update data in the database!\n", line_counter);
            has_error++;
          }
          continue;
        }
  		
  		/* (9.4.2) - group of opcodes with only one argument */ 
        case (ONE_ARGUMENT_GROUP):
        { /* (9.4.2.1) check if the addressing method of the target operand is legal for this opcode */
          if (!check_legal_addressing_method(i, code_arguments[0].addressing_method_decimal, TARGET, opcode_table))
          { /* if not legal - report error */ 
            fprintf(stderr, "Error: Line %d: Illegal addressing method for target operand of instrcution %s!\n",
            																		 line_counter, opcode_table[i].name);
            has_error++;
            continue;
          }
          /* (9.4.2.2) if legal - update first word in code image */
          strcpy(code_bit_structure.target_operand_addressing, code_arguments[0].addressing_method_binary);
          concat_first_word_components(binary_machine_instruction, &code_bit_structure);
          successful_update = set_code(file_record->code_image, &file_record->IC, binary_machine_instruction, 
          																				line_counter, &has_error);
          if (!successful_update) /* report error in case of failure */
          {
            fprintf(stderr, "Error: Line %d: System failure in attempt to update data in the database!\n", line_counter);
            has_error++;
            continue;
          }
			
		  /* (9.4.2.3) - prepare bit coding for the additional word according to address method */ 
          switch ((code_arguments[0].addressing_method_decimal))
          {
            case  (IMMEDIATE):
            case (DIRECT_REGISTER):
            { /* 9.4.2.4 - check if it is a register addressing method */ 
              if ((code_arguments[0].addressing_method_decimal) == DIRECT_REGISTER)
              { /* if register method, this is target operand - move bits to 5 right */
               strncpy(code_arguments[0].additional_word+REGISTER_LENGTH, code_arguments[0].additional_word,
               																				 REGISTER_LENGTH);
               /* and initialize the left 5 to zeros */
               for (k=0; k < REGISTER_LENGTH; k++)
                 code_arguments[0].additional_word[k] = '0';
              }
              /* (9.4.2.5) set additional word if it is immediate or register methods */ 
              successful_update = set_code(file_record->code_image, &file_record->IC, code_arguments[0].additional_word,
              																					 line_counter, &has_error);
              if (!successful_update) /* report error in case of failure */
              {
                fprintf(stderr, "Error: Line %d: System failure in attempt to update data in the database!\n", line_counter);
                has_error++;
              }
              continue;
            }
            case (DIRECT): /* (9.4.2.6) if direct method - save label details and IC for address replacement */ 
            {
              direct_address_symbol_ptr = add_direct_address_symbol(symbols_for_replacement_table,
              									 code_arguments[0].operand[0], file_record->IC, line_counter);
              if (direct_address_symbol_ptr != NULL)
                symbols_for_replacement_table = direct_address_symbol_ptr;
              else
              { /* report error if failed saving symbol record */ 
                fprintf(stderr, "Error: Line %d: System Failure: not enough free memory on disk for saving symbol \"%s\" for direct addressing!\n", line_counter, symbol_name);
                has_error++;
              }
              file_record -> IC++; /* reserve place on code image for the symbol address */ 
              continue;
            }
            case (DESTINATION): /* (9.4.2.7)  if destination addressing method, save symbols names */ 
            {
              distance_addressing_symbol_ptr = add_distance_address_symbols(symbols_for_distance_check_table,code_arguments	
              			[0].operand[0], code_arguments[0].operand[1], file_record->IC-1, file_record->IC, line_counter);
              if (distance_addressing_symbol_ptr != NULL)
                symbols_for_distance_check_table = distance_addressing_symbol_ptr;
              else
              {
                fprintf(stderr, "Error: Line %d: System Failure: not enough free memory on disk for saving symbols for distance addressing!\n", line_counter);
                has_error++;
              }
              file_record -> IC++; /* reserve place for the distance on code image */
              continue;
            }
          }
        }
        continue;
		
		
		/* (9.4.3) opcode of group with 2 arguments */ 
        case (TWO_ARGUMENT_GROUP): 
        {
          /* check if the addressing method of the SOURCE operand is legal for this opcode */
          if(!(check_legal_addressing_method(i, code_arguments[SOURCE].addressing_method_decimal, SOURCE, opcode_table)))
          {
            fprintf(stderr, "Error: Line %d: Illegal addressing method for source operand of instrcution %s!\n", line_counter, opcode_table[i].name);
            has_error++;
            continue;
          }
          /* check if the addressing method of the TARGET operand is legal for this opcode */
          if(!(check_legal_addressing_method(i, code_arguments[TARGET].addressing_method_decimal, TARGET, opcode_table)))
          {
            fprintf(stderr, "Error: Line %d: Illegal addressing method for target operand of instrcution %s!\n", line_counter, opcode_table[i].name);
            has_error++;
            continue;
          }

		  /* update first machine word in code image according to all params */
          strcpy(code_bit_structure.source_operand_addressing, code_arguments[SOURCE].addressing_method_binary);
          strcpy(code_bit_structure.target_operand_addressing, code_arguments[TARGET].addressing_method_binary);
          concat_first_word_components(binary_machine_instruction, &code_bit_structure);
          successful_update = set_code(file_record->code_image, &file_record->IC, binary_machine_instruction, 
          																					line_counter, &has_error);
          if (!successful_update)
          {
            fprintf(stderr, "Error: Line %d: System failure in attempt to update data in the database!\n", line_counter);
            has_error++;
            continue;
          }

          /* check if both operands need to share the same additional word */
          if (code_arguments[SOURCE].addressing_method_decimal == DIRECT_REGISTER)
            if(code_arguments[TARGET].addressing_method_decimal == DIRECT_REGISTER)
            { /* if both operands are registers, there is only one additional word - update it in code imange*/
              strncpy(code_arguments[TARGET].additional_word, code_arguments[SOURCE].additional_word, REGISTER_LENGTH);
              successful_update = set_code(file_record->code_image, &file_record->IC, code_arguments[TARGET].additional_word, line_counter, &has_error);
              if (!successful_update)
              {
                fprintf(stderr, "Error: Line %d: System failure in attempt to update data in the database!\n", line_counter);
                has_error++;
              }
              continue;
            }

          /* if operands do not share the same additional word, set each word individuadly: for each operand    */
          /* loop at both operands (source & tareget) and set code section accordingly: */
          for (k = SOURCE; k <= TARGET; k++)
          {
            switch ((code_arguments[k].addressing_method_decimal))
            {
              case  (IMMEDIATE):
              case (DIRECT_REGISTER):
              { /* in case of immediate and register we could immediately update code image */
                successful_update = set_code(file_record->code_image, &file_record->IC, code_arguments[k].additional_word, line_counter, &has_error);
                if (!successful_update)
                {
                  fprintf(stderr, "Error: Line %d: System failure in attempt to update data in the database!\n", line_counter);
                  has_error++;
                }
                continue;
              }
              case (DIRECT): /* save symbol and IC index for later address replacement */ 
              {
                direct_address_symbol_ptr = add_direct_address_symbol(symbols_for_replacement_table, code_arguments[k].operand[0], file_record->IC, line_counter);
                if (direct_address_symbol_ptr != NULL)
                  symbols_for_replacement_table = direct_address_symbol_ptr;
                else
                {
                  fprintf(stderr, "Error: Line %d: System Failure: not enough free memory on disk for saving symbol \"%s\" for direct addressing!\n", line_counter, code_arguments[SOURCE].operand[0]);
                  has_error++;
                }
                file_record->IC++;
                continue;
              }
              case (DESTINATION): /* save symbols and current IC address for later address replacement */
              {
                distance_addressing_symbol_ptr = add_distance_address_symbols(symbols_for_distance_check_table, code_arguments[k].operand[0], code_arguments[k].operand[1], file_record->IC-1-k, file_record->IC, line_counter);
                if (distance_addressing_symbol_ptr != NULL)
                  symbols_for_distance_check_table = distance_addressing_symbol_ptr;
                else
                {
                  fprintf(stderr, "Error: Line %d: System Failure: not enough free memory on disk for saving symbols for distance addressing!\n", line_counter);
                  has_error++;
                }
                file_record->IC++;
                continue;
              }
            } /* end of switch of addressing method for current operand */
          } continue; /* end of for loop for both operands */
        } /* end of two argument group instructions */
      } /* end of instruction group determination */
    } /* end of opcode instruction line parsing */
  } /* end of assembly source code file */


   /*****************************************************************************************************/
   /*                          after whole scan - set missing addresses from symbol tables              */
   /*****************************************************************************************************/
  
  /* add IC counter to all DC addresses for the delta and code and data isolation */ 
  relocate_data_symbols_address(symbol_table, file_record->IC);
  
  /* replace immediate addressing methods missing addresses from symbol table */
  set_missing_addresses(symbols_for_replacement_table, symbol_table, extern_table, file_record->code_image, &has_error);
  
  /* calculate distances of relvenat symbols and update max distance in appropiate entity */
  set_missing_distances(symbols_for_distance_check_table, symbol_table, file_record->code_image, &has_error);
  
  /* update for each entry it's source address */ 
  set_entry_addresses(*entry_table, symbol_table, &has_error);
  
  /* update error counter(flag) on main.c's objet file struct */
  file_record -> errors = has_error;

  /* Clean Up - Free dynamicly allocated memory */ 
  free_symbol_table(symbol_table);
  free_direct_address_symbol(symbols_for_replacement_table);
  free_distance_addressing_symbol(symbols_for_distance_check_table);

  return EXIT_SUCCESS;
}/*********************************************************************************************************************/



/**********************************************************************************************************************/
/*                                           set_opcode_table                                                         */
/**********************************************************************************************************************/
/* This function is called once for each program run from main.c, in order to create an opcode hash table for all the */
/* assembly source files being compiled in a single run. Because the table has constant values for all compilations,it*/
/* is generated from the main, and passed to each file parser during the call to the assembly.c .                     */
/**********************************************************************************************************************/
void set_opcode_table (opcode table[NUM_OF_OPCODES])
{
  int i; /* index */
  /* constant values to be update in the table. each column of three rows form a single line in the table */
  const char names[NUM_OF_OPCODES][5] = {"mov", "cmp", "add", "sub", "not", "clr", "lea", "inc", "dec", "jmp", "bne",   											"red", "prn", "jsr", "rts", "stop"};
  const char values[NUM_OF_OPCODES][5] = {"0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", 											  "1010", "1011", "1100", "1101", "1110", "1111"};
  const char instruction_group_binary[NUM_OF_OPCODES][3] = {"10", "10", "10", "10", "10", "01", "01", "01", "01", "01", 															"01", "01", "01", "01", "00", "00"};
  unsigned char instruction_group_decimal[NUM_OF_OPCODES] = {2, 2, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 0, 0};
  
  
  for (i=0; i < NUM_OF_OPCODES; i++)
  { /* copy values from temporary const string to array table */ 
    strcpy(table[i].name, *(names+i));
    strcpy(table[i].bin_value, *(values+i));
    strcpy(table[i].instruction_group_binary, *(instruction_group_binary+i));
    table[i].instruction_group_decimal = instruction_group_decimal[i];
  }

  /* set legal addressing methods table for each opcode (source & destination) */
  strcpy(table[0].source_operand_legal_methods, "0123");
  strcpy(table[0].target_operand_legal_methods, "-1-3");
  strcpy(table[1].source_operand_legal_methods, "0123");
  strcpy(table[1].target_operand_legal_methods, "0123");
  strcpy(table[2].source_operand_legal_methods, "0123");
  strcpy(table[2].target_operand_legal_methods, "-1-3");
  strcpy(table[3].source_operand_legal_methods, "0123");
  strcpy(table[3].target_operand_legal_methods, "-1-3");
  strcpy(table[4].source_operand_legal_methods, "----");
  strcpy(table[4].target_operand_legal_methods, "-1-3");
  strcpy(table[5].source_operand_legal_methods, "----");
  strcpy(table[5].target_operand_legal_methods, "-1-3");
  strcpy(table[6].source_operand_legal_methods, "-1--");
  strcpy(table[6].target_operand_legal_methods, "-1-3");
  strcpy(table[7].source_operand_legal_methods, "----");
  strcpy(table[7].target_operand_legal_methods, "-1-3");
  strcpy(table[8].source_operand_legal_methods, "----");
  strcpy(table[8].target_operand_legal_methods, "-1-3");
  strcpy(table[9].source_operand_legal_methods, "----");
  strcpy(table[9].target_operand_legal_methods, "-123");
  strcpy(table[10].source_operand_legal_methods, "----");
  strcpy(table[10].target_operand_legal_methods, "-123");
  strcpy(table[11].source_operand_legal_methods, "----");
  strcpy(table[11].target_operand_legal_methods, "-123");
  strcpy(table[12].source_operand_legal_methods, "----");
  strcpy(table[12].target_operand_legal_methods, "0123");
  strcpy(table[13].source_operand_legal_methods, "----");
  strcpy(table[13].target_operand_legal_methods, "-1--");
  strcpy(table[14].source_operand_legal_methods, "----");
  strcpy(table[14].target_operand_legal_methods, "----");
  strcpy(table[15].source_operand_legal_methods, "----");
  strcpy(table[15].target_operand_legal_methods, "----");
}/********************************************************************************************************************/

/**********************************************************************************************************************/
/*                                           set_register_table                                                       */
/**********************************************************************************************************************/
/* This function is called once for each program run from main.c, in order to create an register  table for all the   */
/* assembly source files being compiled in a single run. Because the table has constant values for all compilations,it*/
/* is generated from the main, and passed to each file parser during the call to the assembly.c .                     */
/**********************************************************************************************************************/
void set_register_table (assembly_register table[NUM_OF_OPCODES])
{
  int i; /* index */
  const char reg_ids [NUM_OF_OPCODES][3] = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7"};
  const char reg_values[NUM_OF_REGISTERS][6] = {"00000", "00001", "00010", "00011", "00100", "00101", "00110", "00111"};

  for (i=0; i < NUM_OF_REGISTERS; i++)
  { /* copy temp values from const vector strings to register array table */
    strcpy(table[i].name, *(reg_ids+i));
    strcpy(table[i].bin_value, *(reg_values+i));
  }
}/*********************************************************************************************************************/


/**********************************************************************************************************************/
/*                                               opcode_lookup                                                        */
/**********************************************************************************************************************/
/* This function gets a token name and check if it exists in the opcode table. If it does, it return it's index in the*/
/* the table. If it does not exist it return the constant NOT FOUND which is equal to minus 1 (-1).                   */
/**********************************************************************************************************************/
int opcode_lookup (char *token, opcode opcode_table[NUM_OF_OPCODES])
{
  int i; /*index for search */

  for (i=0; i < NUM_OF_OPCODES; i++)
    if (strcmp (opcode_table[i].name, token) == 0)
      return i; /* if found - return index of the opcode in the table */
  return NOT_FOUND; /* in case not found */
}/*********************************************************************************************************************/


/**********************************************************************************************************************/
/*                                               register_lookup                                                      */
/**********************************************************************************************************************/
/* This function gets a token name and check if it exists in the register If it does, it return it's index in the     */
/* the table. If it does not exist it returns the constant NOT FOUND which is equal to minus 1 (-1).                  */
/**********************************************************************************************************************/
int register_lookup (char *token, assembly_register register_table[NUM_OF_REGISTERS])
{
  int i; /*index for search */
  int reg_name_legnth = strlen(register_table[0].name);

  for (i=0; i < NUM_OF_REGISTERS; i++)
    if (strncmp (register_table[i].name, token, reg_name_legnth) == 0)
      return i; /* if found - return index of the register in the table */
  return NOT_FOUND; /* in case not found */
}/*********************************************************************************************************************/



/***********************************************************************************************************************/
/*                                               decimal_to_binary:                                                    */
/* this function accepts a signed integer in decimal representation and an array to return the given number as         */
/* a character string in binary representation.The array must be big enough.The converison is done in the 2's          */
/* complements method - negetive num msb is '1' and positive '0'. Instead of "flip-floping" bits and adding '1'		   */
/* for negetive values, because the representation used here is of a character string, an alternative method  		   */
/* for negetive values has been used here: adding 2^bits_num to the negetive value and converting the result,  		   */
/* that's equivalent to [(~(abs((double)(decimal_num))))+1].                                                   		   */
/* NOTE: Inorder to be portable, this function computes the num of bits at run_time using the sizeof operator. 		   */
/* If any adjustments are needed for the num of bits,it should be done by the calling routine / proccess.      		   */
/***********************************************************************************************************************/
char * convert_decimal_to_binary(signed int decimal_num, char *binary_num)
{
    const unsigned char BASE = 2; /* base for conversion. this func conerts to base 2 (binary). */
    unsigned int bits_in_num = BITS_IN_BYTE * sizeof(signed int); /* total bit amount */
    unsigned char remainder = 0; /* util remainder variable for computing */
    unsigned int i; /*index for binary representation chararacter array */
    signed int mask = 1; /* util lsb for masking powers of 2 - 2,4,8,16, etc... */
    char sign = '+'; /* positive\negetive sign indicator. default is set for positive. */

    if (decimal_num < 0) /* check if negetive */
    { /* for negetive values - increment them by pow(2, bits_in_num) */
      sign = '-'; /* update sign indicator minus */
      mask = mask << (bits_in_num-1); /* no space for shifting all way through because it will overflow to zero) */
      decimal_num += mask; /* increment num with mask value which is [100...00]. */
    }

    binary_num[bits_in_num] = '\0'; /* set the last array cell as a terminal sentinel */

    /* start from end of array and descend in order to have it in right order */
    i = bits_in_num;
    while (i > 0)
    {
        remainder = decimal_num % BASE; /* save the remainder */
        binary_num[--i] = remainder + '0'; /* convert it to ascii and pass it to array */
        decimal_num /= BASE; /* cut the least significant digit */
    }

    if (sign == '-')
      binary_num[i] = '1'; /* if negetive - set the first array cell (msb) to '1' for 2's complement representation.*/

    return binary_num; /*return pointer to the array holding the binary representation */
}/*******************************************************************************************************************/



/******************************************************************************************************************/
/*                                         check_num                                                              */
/* this function accepts number given from input in a character string representation, and checks if it's         */
/* a legal integer number. it returns 1 is it's legal and 0 if it is not legal or if the reference is NULL.       */
/******************************************************************************************************************/
int check_num (char* num)
{
    int is_num = NO; /* initialize boolean flag */

    if (num == NULL) /* validation check for prevention run time errors in case of invalid pointer from caller */
        return is_num; /* zero (flag is off) */

    if((atoi(num) < INT_MIN) || (atoi(num) > INT_MAX))
         fprintf(stderr, "Warning number %d has exceded the int limit\n", atoi(num));

    /* if not null, check first charachter */
    if (isdigit(*num) || ((*num) == '+') || ((*num) == '-'))
    {
        /* skip sign if needed */
        if (((*num) == '+') || ((*num) == '-'))
            num++;
        /* validate that each character is a digit */
        while ((*num != '\0') && isdigit(*num))
            num++;

        /* incase we got so far (end of string), turn on flag */
        if ((*num) == '\0' || (*num) == '\n')
           is_num = YES;
    }
    return is_num; /* return flag result to caller according to the legality of num */
}/*************************************************************************************************************/


/*************************************************************************************************************/
/*                                      check_legal_addressing_method                                        */
/* this function accepts an opcode index and an actual address method that was given as an istruction on     */
/* the assemly source code on the source/target operand. The function checks if the given method is legal    */
/* for this opcode and this operand and returns 1 if yes, and 0 if not.                                      */
/* inorder to serve both source and target operands, a third paramter should be send as a parameter to       */
/* distinguish by the two, and the opcode table with consists the legal methods for each opcode.             */
/*************************************************************************************************************/
int check_legal_addressing_method (int op_index, int actual_method, char operand_role, opcode *op_table)
{
  char *util_ptr = NULL;
  actual_method += '0'; /* itoa flag - add '48' in order to convert int value to ascii */
  /* check if the actual method is one of the allowed methods in the opcode table apropiate field (source/target) */
  if (operand_role == SOURCE)
    util_ptr = strchr(op_table[op_index].source_operand_legal_methods,  (char)actual_method);
  else if (operand_role == TARGET)
    util_ptr = strchr(op_table[op_index].target_operand_legal_methods, (char)actual_method);
  if (util_ptr) /* if actual method found in permitted list */
    return YES;
  else /* actual method not found */
    return NO;
}/**************************************************************************************************************/



/*********************************************************************************************************************/
/* 											convert_binary_to_decimal											     */
/*********************************************************************************************************************/
/* This function recevies a character string of binary digits and converts it into it's equivalent decimal int number*/
/*********************************************************************************************************************/
unsigned int convert_binary_to_decimal(const char * binary_num)
{

  unsigned int decimal_num; /* the number that will be reutrned as the result */
  int num_length = strlen(binary_num); /* smount of digits in string */
  int i = num_length-1; /* start from right last significant bit */ 
  double power = 0;  /* power used for conversing, initially 0, incrementing along the conversing */
  int power_temp_result; /* temporary variable for computations */ 
  int atoi_mask = '0'; /* mask to convery from ascii value to numberic int */

  /* compute lsb decimal int value */
  power_temp_result = power_num(BINARY_BASE, power); 
  decimal_num = (int)((binary_num[i] - atoi_mask) * power_temp_result);
  power++;
	
  /* go left towards the most significant bit */ 
  while (--i >= 0)
  { /* for each digit character: */
	/* compute the current power of 2 accroding to bit location*/ 
    power_temp_result = power_num(BINARY_BASE, power);    
    /* convert ascii to int & multy by the temp result and add it to the current value of the decimal num */
    decimal_num += (int)((binary_num[i] - atoi_mask) * power_temp_result); 
    power++; /* increment power to according to the next bit */ 
  }
  return decimal_num; 
}/*************************************************************************************************************/

/********************************************************************************************************************/
/* 													power_num 													    */
/********************************************************************************************************************/
/* This function is the basic math power function, but it supports the specific needs for doubling integer varyables*/
/* incontrast to math.h function "pow" which receives a double base and double power.                               */
/********************************************************************************************************************/
int power_num (int base, int power)
{
	int i; /* index */
	int result = base; 
	
	if (power == 0) /* base case 1- power in 0 is always one '1' */
		return 1;
	else if (power == 1) /* base case 2 - each number with power 1 is the number itself */
		return base;
	
	/* for all other cases - multiply the number by iteself (the base) "power" amount of times */
	for (i = 1; i < power; i++)
		result = result * base; 
	
	return result; /* reutn to caller */
}/*********************************************************************************************************************/


