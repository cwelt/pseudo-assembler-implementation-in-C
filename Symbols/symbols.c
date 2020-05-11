/**************************************************************************************************************************/
/*                                                     symbols.c                                                          */
/**************************************************************************************************************************/
/* This module is responsible for handleing all the data processing of symbols - symbols data structures managment,       */
/* syntax and semantic validations, entry, extern, data and code context defention of symbols, direct and distance address*/
/* methods, and freeing allocated memory used for the compilation process.                                                */
/**************************************************************************************************************************/
#include "symbols.h" /* data strucutres and defentions of smbol management */ 
#include "assembler.h"
#include "data_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*************************************************************************************************************************/
/*                                               check_symbol_syntax                                                     */
/*************************************************************************************************************************/
/* This function gets a string containing a lable's name, and checks if it is a legal name by make a seris of validations*/
/* If legal, It saves the labels name into a char pointer passed by caller, and returns it's length.                     */
/* If illegal it reutrns length zero.                                                                                    */
/*************************************************************************************************************************/
int check_symbol_syntax (char* input_line, char* symbol_name, assembly_register *reg_table, opcode *op_table, unsigned int line_num, unsigned int *error_flag, unsigned char *symbol_flag)
{
  unsigned char symbol_length;
  char *util_ptr;

  /*validate that the label starts on first column of the line */
  if(isalpha(*input_line))
  { /* calculate length of label and check if it's legal */
    util_ptr = strrchr(input_line, ':');

    /* check if this is a new symbol definition or just a extern/entry declaration */
    if (util_ptr == NULL) /* if no ':', it's a extern\entry declaration, and not a new symbol definition */
    { /* advance to end of label name in order to calculate its length */
      util_ptr = input_line;
      while (!isspace(*util_ptr) && (*util_ptr) != '\0' && (*util_ptr) != '\n' && (*util_ptr) != ',')
        util_ptr++;
    }
    symbol_length = util_ptr - input_line;
    if (symbol_length <= SYMBOL_MAX_LENGTH)
    { /* isolate label's name from the input line and save it */
      strncpy(symbol_name, input_line, symbol_length);
      *(symbol_name + symbol_length) = '\0'; /* terminate name with the null character */
      for (util_ptr = input_line; util_ptr && isalnum(*util_ptr); util_ptr++) /* check name legality */
        ;      /* scan the name till it's end and validate that each character is alpha numeric */
      if (((util_ptr - input_line) == (symbol_length))) /* check if the name scan succeded all way through; */
        if (register_lookup(symbol_name, reg_table) == NOT_FOUND) /* validate name is not a register name */
          if (opcode_lookup(symbol_name, op_table) == NOT_FOUND) /* validate name is not a opcode name */
          { /* if we got this far, then the given input line has a label */
            *symbol_flag = YES;     /* turn on indicator for current input line*/
            return symbol_length;  /* return the length of the label's name */
          }
          /* if not valid - print appropiate error, update error flag and return zero */ 
          else
            fprintf(stderr, "  : line %d: Label name may not be a command's(opcode) name!\n", line_num);
        else
          fprintf(stderr, "Error: line %d: Label name may not be a register's name!\n", line_num);
      else
        fprintf(stderr, "Error: line %d: Label name may contain only alphabetic characters and numbers!\n", line_num);
    }
    else
      fprintf(stderr, "Error: line %d: Label name exceeded max length which is 30 characters at most.\n", line_num);
  }
  else if (*input_line == '\b' || *input_line == '\t' ||  *input_line == ' ')
    fprintf(stderr, "Error: line %d: Label must start on first column of the line!\n", line_num);
  else
    fprintf(stderr, "Error: line %d: Label name must start with an alphabetic letter!\n", line_num);

  (*error_flag)++;
  return 0;
}/*********************************************************************************************************************/


/*********************************************************************************************************************/
/*                                             check_symbol_existence                                                */
/*********************************************************************************************************************/
/* This function receives a label name and checks if a symbol containing this name has already been registers in the */
/* symbol table. if the symbol exists at the table, the function would return a pointer to it and NULL if it isn't.  */
/*********************************************************************************************************************/
symbol *check_symbol_existence (symbol *head, char *name)
{
  symbol *util_ptr; /* utillity pointer for searching through the symbol table */
  /* scan the entire symbol list */
  for (util_ptr = head; util_ptr != NULL; util_ptr = util_ptr -> next)
  { /* check if the given label is identical to the current symbols name */
    if (strcmp (name, util_ptr -> name) == 0)
      break; /* if so, we are pointing at the desired symbol */
  }
  /* return result to caller (incase head was pointing to a null adress, the result would return null) */
  return util_ptr;
}/**********************************************************************************************************************/

/*************************************************************************************************************************/
/*                                                  add_new_symbol                                                       */
/*************************************************************************************************************************/
/* This function is responsible for saving symbols that were referenced to in the assembly source file, in a symbol table*/
/* The table will save each symbol's name, address, type and context in order to enable later address replacement and    */
/* relocations if necessary on code image. Thisfunction handles both data and code context symbols and extern symbols too*/
/* It returns a pointer ot the updated symbol table if update was successful and Null if error occured.                  */
/*************************************************************************************************************************/
symbol * add_new_symbol (symbol *head, char *name, unsigned int decimal_address, symbol_type type, symbol_context context, unsigned int line)
{
  symbol *util_ptr; /* utillity pointer for new record */
  util_ptr = (symbol*)calloc(1, sizeof(symbol)); /* try to allocate memomry for new symbol record */ 

  /* if allocation was succesful - update relevant required data */ 
  if (util_ptr != NULL) 
  {
    strcpy(util_ptr -> name, name); /* label name */
    util_ptr -> address_decimal_value = decimal_address; /* address of DC or IC or 0 if it is external */
    util_ptr -> type = type; /* extern or relocatable */ 
    util_ptr -> context = context; /* data or code */ 
    util_ptr -> source_line = line; /* source line of assembly file for error report if necessary */
    util_ptr -> next = head; /* add symbol to symbol table */
    head = util_ptr; /* update head of table accordingly */ 
    return head; /* if successful - return head of updated table */
  }
  else /* if memort allocation failed - return Null */ 
    return NULL;
}/*********************************************************************************************************************/

/*************************************************************************************************************************/
/*                                                   add_new_entry                                                       */
/*************************************************************************************************************************/
/* This function is reponsible for saving entry symbols to a special table that will be used later for genreating the    */
/* entry file (".ent"). It is done after the whole scan of the assembly file, when each symbol is already registerd.     */
/*************************************************************************************************************************/
entry * add_new_entry (entry *head, char *name, unsigned int line)
{
  entry *util_ptr; /* utillity pointer for manipulating linked list */
  entry *new_entry; /* utillity pointer for new record */
  
  /* try to allocate memory for new record */ 
  new_entry = (entry*)calloc(1, sizeof(entry));
 
  if (new_entry != NULL) /* if memory allocation succeded */
  { /* save required data - label name and address of symbol defention */ 
    strcpy(new_entry -> name, name);
    new_entry -> source_line = line;

    util_ptr = head; /* point to head of current entry linked list ("table") */
    
    /* if current list is not empty, add new record to the tail of the list */ 
    if (util_ptr != NULL) 
    {
      while ((util_ptr -> next) != NULL)
        util_ptr = util_ptr -> next;
      util_ptr -> next = new_entry;
    }
    else /* current list is empty - define new record as the head of the list */ 
    {
      new_entry -> next = head;
      head = new_entry;
    }
    
    return head; /* return head of successfully updated list */ 
  }
  else /* if memory allocation falied - return NULL */
    return NULL;
}/*********************************************************************************************************************/


/*************************************************************************************************************************/
/*                                                 add_new_extern                                                        */
/*************************************************************************************************************************/
/* This function is reponsible for saving external symbols to a special table that will be used later for genreating the */
/* extern file (".ext"). It is done after the whole scan of the assembly file, when each symbol is already registerd.    */
/*************************************************************************************************************************/
external_symbol * add_new_extern (external_symbol *head, char *name, unsigned int address)
{
  external_symbol *new_extern; /* pointer for new record */ 
  
  /* try to allocate memory for new record */ 
  new_extern = (external_symbol*)calloc(1, sizeof(external_symbol));

  /* if memory allocation succeded - save required data */ 
  if (new_extern != NULL)
  {
    strcpy(new_extern -> name, name); /* symbol's name */ 
    new_extern -> address_decimal_value = address + BASE_START; /* address of reference to this symbol in the code image */
	
	/* add the extern symbol to the table */ 
    new_extern -> next = head;
    head = new_extern;
    return head; /* if successful - return head of updated table */
  }
  else /* if memory allocation failed - return NULL */
    return NULL;
}/************************************************************************************************************************/

/*************************************************************************************************************************/
/*                                    		    add_direct_address_symbol                                                */
/*************************************************************************************************************************/
/* This function saves record of symbols used in machine instruction with direct addressing methods, saving required info*/
/* needed for later replacement of the address defined for the symbol in the code image memory. This information will be */
/* used later after the entri file scan inorder to update the code image with the missing adresses. It reutrn a pointer  */
/* to the head of te updated table containg ing the new record or NULL on case of error of memory allocation failure.    */
/*************************************************************************************************************************/
direct_address_symbol * add_direct_address_symbol (direct_address_symbol *head, char *symbol_name, unsigned int address,
																									 unsigned int line)
{
  direct_address_symbol *util_ptr; /* utillity pointer  */
  
  /* try to allocate memort for new record */ 
  util_ptr = (direct_address_symbol*)calloc(1, sizeof(direct_address_symbol)); 
	
  /* if memory allocation succeded - save relevant data required */ 	
  if (util_ptr != NULL)
  {
    strcpy(util_ptr -> name, symbol_name); /* copy symbols label name */
    util_ptr -> address = address; /* copy address IC counter in code image that need replacement */ 
    util_ptr -> source_line = line; /* source line of assembly source file, for error report if necessary */
    util_ptr -> next = head; /* add new record to existing table */ 
    head = util_ptr; /* update head of table */
    return head; /* return head of table if successful */
  }
  else /* in case of error in allocation memory-return NULL */
    return NULL;
}/*********************************************************************************************************************/

/**********************************************************************************************************************/
/*                                             add_distance_address_symbols                                           */
/**********************************************************************************************************************/
/* This function saves symbols used in machine instruction with distance addressing methods. It saves the symbol names*/
/*,The source line instrunction address and the address if the additional word needed to be replaced with the distance*/
/* This information is later passed to an appropiate function to compute distances and update code image accordingly. */
/* On success a head of the table containg the new record is record is returned. On error - null is reutrned.         */
/**********************************************************************************************************************/
distance_addressing_symbol *add_distance_address_symbols (distance_addressing_symbol *head, char *name1, char *name2,
							 unsigned int address_of_instruction, unsigned int address_to_replace, unsigned int line)
{
  distance_addressing_symbol *util_ptr; /* utillity pointer  */
  
  /* allocate memort for a new record */
  util_ptr = (distance_addressing_symbol*)calloc(1, sizeof(distance_addressing_symbol));
	
  /* if memory allocation successful - save requried data */ 	
  if (util_ptr != NULL)
  {
    strcpy(util_ptr -> name1, name1); /* copy first symbol's name from argument to new reocord */
    strcpy(util_ptr -> name2, name2); /* copy secind symbol's name from argument to new reocord */
    util_ptr -> address_of_instruction = address_of_instruction; /* copy address of origin instruction */
    util_ptr -> address_to_replace = address_to_replace; /* copy address needed for distance assignment in image */ 
    util_ptr -> source_line = line; /* copy source line in assembly file for error report if necessary */ 
    util_ptr -> next = head; /* add new record to existin's table (list) */ 
    head = util_ptr; /* update head of table (list) accordingly */
    return head; /* success - return updated head of list */ 
  }
  else /* error in allocating memory - retrun NULL to caller */
    return NULL;
}/***********************************************************************************************************************/

/************************************************************************************************************************/
/*                                       update_data_symbols_address                                                    */
/************************************************************************************************************************/
/* This function receives an IC (instruction counter) and a symbol table, and updated each symbol which is marked as    */
/* data context, to it's relocatable address, after adding the offset of the amount of machine instruction output lines.*/
/************************************************************************************************************************/
void relocate_data_symbols_address (symbol *head, unsigned int IC)
{
  symbol *util_ptr; /* utillity pointer for searching through the symbol table */
  /* scan the entire symbol list */
  for (util_ptr = head; util_ptr != NULL; util_ptr = util_ptr -> next)
  { /* check if the given label is marked as with data address */
    if (util_ptr -> context == data)
      util_ptr -> address_decimal_value += IC; /* if so, relcoate it's address by offset of IC */
  }
}/***********************************************************************************************************************/


/************************************************************************************************************************/
/*                                			  set_missing_addresses                                                     */
/************************************************************************************************************************/
/* This function sets the addresses of symbols the direct addressing method symbols, which we did not know at first scan*/
/* It receives information that was saved during scanning process, containing symbols which address are required, source*/
/* line address in code image that is needed to be replaced, and additional data on external declared symbols in order  */
/* to be effecient and at the same time, while scanning symbol table for replacement, this function also saves relevant */
/* data for the extern symbols file (".ext") saving the address reference to external symbols used in the instrunctions.*/ 
/************************************************************************************************************************/
void set_missing_addresses (direct_address_symbol *head, symbol *symbol_table, external_symbol **extern_table, 
							char missing_address_line[][BITS_IN_REGISTER+1], unsigned int *error_flag)
{
  direct_address_symbol *direct_ptr = head; /* utillity pointer for relevant symbol table for replacement */
  symbol *symbol_ptr; /* symbol pointer for symbol table */ 
  external_symbol *external_ptr; /* pointer for extern symbol table */ 
  unsigned int decimal_address; /* address of symbol in decimal representation */
  char temp_binary_address[MAX_LINE_LENGTH]; /* address of in symbol in binary representation */ 
  char const relocatable_postfix[] = "10"; /* binary suffix of direct method relocatable symbols */  
  char const external_postfix[] = "01"; /* binary suffix of direct method external symbols */  
  char *char_ptr; /* utility temp pointer */ 
	
  /* scan the whole table of symbols referenced in direct addressing method */ 	
  for (; direct_ptr != NULL; direct_ptr = direct_ptr -> next)
  { /*look for a match in symbol table for a symbol with the same label name */ 
    symbol_ptr = check_symbol_existence(symbol_table, direct_ptr -> name);
 
    /* if not found - report error */
    if (symbol_ptr == NULL)
    {
      fprintf(stderr, "Error: Line %d: Symbol \"%s\" is not defined!\n", direct_ptr -> source_line, direct_ptr -> name);
      (*error_flag)++;
      continue;
    }

    /* if found - if not external add BASE_START offset to original address  */ 
    if (symbol_ptr -> type == external) /* if extern - address is zero (unknown) no need for offset */ 
    	decimal_address = symbol_ptr -> address_decimal_value;
    else /* if not extern add offset of BASE_START */
    	decimal_address = symbol_ptr -> address_decimal_value + BASE_START;
    
    /* convert address of symbol found from decimal to binary */
    convert_decimal_to_binary(decimal_address, temp_binary_address);
    char_ptr = temp_binary_address + strlen(temp_binary_address) - BITS_IN_ADDITIONAL_WORD;
    
    /* if it is a external marked symbol, add relevant suffix and save information in extern table */
    if (symbol_ptr -> type == external)
    {
      strcat(char_ptr, external_postfix);
      /* if it's external save it's location at external's file */
      external_ptr = add_new_extern(*extern_table, direct_ptr -> name, direct_ptr -> address);
      if (external_ptr != NULL)
        *extern_table = external_ptr;
      else
      { /* if no memory space available for saving extern record in extern file - report error */
        fprintf(stderr, "Error: Not enough memory on disk to save external symbol \"%s\"!\n", direct_ptr -> name);
        (*error_flag)++;
      }
    } 
    else /* if not external - add relocatable binary suffix */
      strcat(char_ptr, relocatable_postfix);
      
    /* update code image in the relevant index with the desired address */  
    strncpy(missing_address_line[direct_ptr->address], char_ptr, BITS_IN_REGISTER);
  }
}/***********************************************************************************************************************/


/**********************************************************************************************************************/
/*                             				     set_missing_distances                                                */
/**********************************************************************************************************************/
/* This function is in charge of updating code image of word which represent the max absolute distance beetween their */
/* symbols addresses defention and source line of machine instruction. It is given a table of all symbols which were  */
/* marked as "distance addressing method" relevant, with their addresses for replacement in code image and the address*/
/* of the machine instruction source inorder to compute the max distance.                                             */
/**********************************************************************************************************************/
void set_missing_distances (distance_addressing_symbol *head, symbol *symbol_table, char missing_distance_line[][BITS_IN_REGISTER+1], unsigned int *error_flag)
{
  distance_addressing_symbol *distance_ptr = head; /* utillity pointer to head of relevant symbol table */
  symbol *symbol_ptr1; /* util pointer to first symbol */
  symbol *symbol_ptr2; /* util pointer to second symbol */
  int distance1, distance2, distance3; /* 3 variable to save the three distances and compare them */
  int max_distance; /* variable to hold the result of max distance from withing the three */ 
  char temp_binary_address[MAX_LINE_LENGTH]; /* array to hold the binary representation of the max distance */ 
  char *char_ptr; /* utility char pointer */ 
  const char absolute[] = "00"; /* ARE constant suffix for the word that is going to be updated in code image */ 

  /* loop at all entries of the distance relevant symbol table */
  for (; distance_ptr != NULL; distance_ptr = distance_ptr -> next)
  { /* for each entry - validate both symbols actually exist in symbol table */
    symbol_ptr1 = check_symbol_existence(symbol_table, distance_ptr -> name1);
    symbol_ptr2 = check_symbol_existence(symbol_table, distance_ptr -> name2);

	/* if one the symbols does not exist on symbol table - report error */ 
    if (symbol_ptr1 == NULL)
    {
      fprintf(stderr, "Error: Line %d: Symbol \"%s\" is not defined!\n", distance_ptr -> source_line, distance_ptr -> name1);
      (*error_flag)++;
      continue;
    }
    if (symbol_ptr2 == NULL)
    {
      fprintf(stderr, "Error: Line %d: Symbol \"%s\" is not defined!\n", distance_ptr -> source_line, distance_ptr -> name2);
      (*error_flag)++;
      continue;
    }

	/* if both symbols found - calcualte their distances (bwtween the symbols themselfs, and betweem them and source line */ 
    distance1 = (symbol_ptr1 -> address_decimal_value) - (symbol_ptr2 -> address_decimal_value);
    distance2 = (distance_ptr -> address_of_instruction) - (symbol_ptr1 -> address_decimal_value);
    distance3 = (distance_ptr -> address_of_instruction) - (symbol_ptr2 -> address_decimal_value);
    
    /* convert distances to their absoulte value, incase of negative result */ 
    distance1 = abs(distance1);
    distance2 = abs(distance2);
    distance3 = abs(distance3);
	
	/* calculate the max result fro mwithin the three distances found */ 
    max_distance = ((distance1 >= distance2) ? distance1 : distance2);
    max_distance = ((max_distance >= distance3) ? max_distance : distance3);

	/* convert result to binary representation, add ARE bits and update code image in database */
    convert_decimal_to_binary(max_distance, temp_binary_address);
    char_ptr = temp_binary_address + strlen(temp_binary_address) - BITS_IN_ADDITIONAL_WORD;
    strcat(char_ptr, absolute);
    strncpy(missing_distance_line[distance_ptr->address_to_replace], char_ptr, BITS_IN_REGISTER);
  }
}/************************************************************************************************************************/


/*********************************************************************************************************/
/*                                        set_entry_addresses                                            */
/*********************************************************************************************************/
/* This function is responsible for gathering addressed of symbols declared as entry. The information is */
/* eventually used to generate an entry file (".ent"). If a declaration for a entry is made and no symbol*/
/* is found on symbol tabel with the same label, an error is reported.                                   */
/*********************************************************************************************************/ 
void set_entry_addresses (entry *entry_table, symbol *symbol_table, unsigned int *error_flag)
{
  entry *entry_ptr; /* util entry pointer to scan the entry table */
  symbol *symbol_ptr; /* util symbol pointer to scan the symbol table */
  
  /* as long as there are more entry enteties at the enntry table - scan it */
  for (entry_ptr = entry_table; entry_ptr != NULL; entry_ptr = entry_ptr -> next)
  { /* compare each one with the label in symbol table */
    for(symbol_ptr = symbol_table; symbol_ptr != NULL; symbol_ptr = symbol_ptr -> next)
      if((strcmp(entry_ptr -> name, symbol_ptr -> name) == 0))
      { /* if match found, update address of symbol in entry table and continue to next entry */ 
        entry_ptr -> address_decimal_value = BASE_START + symbol_ptr -> address_decimal_value;
        break;
      }
    if (symbol_ptr == NULL) /* if entry symbol not found on symbol table - report error */
    {
      fprintf(stderr, "Error: Line %d: Entry symbol %s does not exist in the symbol table!\n", entry_ptr->source_line, entry_ptr->name);
      (*error_flag)++;
    }
  }
}/********************************************************************************************************/

/*********************************************************************************************************/
/*                                          free_symbol_table                                            */
/*********************************************************************************************************/
/* This Function releases reserved dynamic memory for the symbol table  back to operating system         */
/*********************************************************************************************************/
void free_symbol_table (symbol *head)
{
  symbol *current_node_ptr; /* pointer of current node to be freed */
  symbol *ptr_at_front; /* util pointer to be up front to hold the remaining of list */

  /* (1) point to head of table */
  current_node_ptr = head;
  /* (2) scan the table as lone there are more records left in it to be freed */
  while (current_node_ptr != NULL)
  { /* (3) make back up pointer to point to next record in table, for not getting lost trapped memory */
    ptr_at_front = current_node_ptr -> next;
    free(current_node_ptr); /* (4) free memory of current record */
    current_node_ptr = ptr_at_front; /* (5) advance to backup next record on front */
  }
}/*********************************************************************************************************/


/*********************************************************************************************************/
/*                                          free_entry_table                                             */
/*********************************************************************************************************/
/* This Function releases reserved dynamic memory for the entry table  back to operating system.         */
/*********************************************************************************************************/
void free_entry_table (entry *head)
{
  entry *current_node_ptr; /* pointer of current node to be freed */
  entry *ptr_at_front; /* util pointer to be up front to hold the remaining of list */

  /* (1) point to head of table */
  current_node_ptr = head;
  /* (2) scan the table as lone there are more records left in it to be freed */
  while (current_node_ptr != NULL)
  { /* (3) make back up pointer to point to next record in table, for not getting lost trapped memory */
    ptr_at_front = current_node_ptr -> next;
    free(current_node_ptr); /* (4) free memory of current record */
    current_node_ptr = ptr_at_front; /* (5) advance to backup next record on front */
  }
}/*********************************************************************************************************/


/*********************************************************************************************************/
/*                                          free_extern_table                                            */
/*********************************************************************************************************/
/* This Function releases reserved dynamic memory for the extern table  back to operating system.        */
/*********************************************************************************************************/
void free_extern_table (external_symbol *head)
{
  external_symbol *current_node_ptr; /* pointer of current node to be freed */
  external_symbol *ptr_at_front; /* util pointer to be up front to hold the remaining of list */

  /* (1) point to head of table */
  current_node_ptr = head;
  /* (2) scan the table as lone there are more records left in it to be freed */
  while (current_node_ptr != NULL)
  { /* (3) make back up pointer to point to next record in table, for not getting lost trapped memory */
    ptr_at_front = current_node_ptr -> next;
    free(current_node_ptr); /* (4) free memory of current record */
    current_node_ptr = ptr_at_front; /* (5) advance to backup next record on front */
  }
}/********************************************************************************************************/

/*********************************************************************************************************/
/*                                      free_direct_address_symbol                                       */
/*********************************************************************************************************/
/* This Function releases reserved dynamic memory for the table of direct addressing used symbols back to*/
/* the operating system.                                                                                 */
/*********************************************************************************************************/
void free_direct_address_symbol (direct_address_symbol *head)
{
  direct_address_symbol *current_node_ptr; /* pointer of current node to be freed */
  direct_address_symbol *ptr_at_front; /* util pointer to be up front to hold the remaining of list */

  /* (1) point to head of table */
  current_node_ptr = head;
  /* (2) scan the table as lone there are more records left in it to be freed */
  while (current_node_ptr != NULL)
  { /* (3) make back up pointer to point to next record in table, for not getting lost trapped memory */
    ptr_at_front = current_node_ptr -> next;
    free(current_node_ptr); /* (4) free memory of current record */
    current_node_ptr = ptr_at_front; /* (5) advance to backup next record on front */
  }
}/********************************************************************************************************/

/*********************************************************************************************************/
/*                                     free_distance_addressing_symbol                                   */
/*********************************************************************************************************/
/* This Function releases reserved dynamic memory for the table of distance addressing used symbols back */
/* to the operating system.                                                                              */
/*********************************************************************************************************/
void free_distance_addressing_symbol (distance_addressing_symbol *head)
{
  distance_addressing_symbol *current_node_ptr; /* pointer of current node to be freed */
  distance_addressing_symbol *ptr_at_front; /* util pointer to be up front to hold the remaining of list */

  /* (1) point to head of table */
  current_node_ptr = head;
  /* (2) scan the table as lone there are more records left in it to be freed */
  while (current_node_ptr != NULL)
  { /* (3) make back up pointer to point to next record in table, for not getting lost trapped memory */
    ptr_at_front = current_node_ptr -> next;
    free(current_node_ptr); /* (4) free memory of current record */
    current_node_ptr = ptr_at_front; /* (5) advance to backup next record on front */
  }
}/*********************************************************************************************************/
