/************************************************************************************************************************/
/*                                        	 	   data_manager.c                                                       */
/************************************************************************************************************************/
/* data_manager module is responsible for handleing the data image, parsing the .data and .string machine instructions  */
/* validating the legality, syntax and context of the data , and saving it in data base. Up to MAX_DATA_OUTPUT_LINES    */
/* could be saved for each file.                                                                                        */
/************************************************************************************************************************/

#include "assembler.h"
#include "data_manager.h"

/***********************************************************************************************************/
/*                                          get_data                                                       */
/***********************************************************************************************************/
/* This function is used to validate ".data" arguments before updating them in the data image. It recevies */
/* an input line of characters, and validates it contains a list of legal intergers separated by commas.   */
/* If legal - the function converts the numbers from string characters into integers and saves them in a   */
/* target array that is past by the calling routine, and returns the amount of numbers saved in the list.  */
/* if not legal, the function returns 0.                                                                   */
/***********************************************************************************************************/
int get_data (char* line, int converted_num_list[], int *amount_of_args, unsigned int line_num, unsigned int *error_flag)
{
    unsigned char has_unsatisfied_comma; /* indicator if there is a comma following current token or not */
    char current_token[MAX_LINE_LENGTH]; /* array for saving current argumet parsed */ 
    unsigned int token_length; /* length of current data token */
    char *util_ptr = line; /* util pointer for parsing the data */
    int scan_success = 0; /* scanf indicator of success or failure */
    *amount_of_args = 0; /* initialize amount of numeric arguments in input line */

    /* validation check for preventing run time errors in case of invalid pointer from caller */
    if (line != NULL) 
    { /* skip spaces if necessary */ 
      while(isspace(*util_ptr))
        util_ptr++;
      current_token[0] = '\0';
      
      /* while there are more non white characters - fetch each token at a time */
      while (((scan_success = sscanf(util_ptr, "%[^\b\t ,]", current_token)) > 0)) 
      {
        has_unsatisfied_comma = 0;
        /* validate the token is a legal interger - if so save it */
        if (check_num(current_token)) /* if legal - convert to int and save in array */
          converted_num_list[(*amount_of_args)++] = atoi(current_token);
        else
        { /* if not legal, print error and return apropiate signal */
          fprintf(stderr, "Error: Line %d: \".data\" encounterd an illegal interger: \"%s\"!\n", line_num, current_token);
          (*error_flag)++;
          *amount_of_args = 0;
          return *amount_of_args;
        }

        /* advance to next token to see if it's a comma */
        token_length = strlen(current_token);
        util_ptr = util_ptr + token_length;
        while(isspace(*util_ptr)) /* skip spaces */
          util_ptr++;
        if (*util_ptr == ',') 
        { /* if comma found, turn on flag */ 
          has_unsatisfied_comma = 1;
          util_ptr++;
          while(isspace(*util_ptr)) /* skip spaces if necessary */ 
            util_ptr++;
          continue;
        }
		
		/* if there is no comma and no other chacrters after the last argument, end process (success) */
        else if((strcmp(util_ptr, "") == 0)) 
          return *amount_of_args;
		
		/* if there are more arguments but no comma delimeter before them - report error */ 
        else if (*util_ptr != '\n' && *util_ptr != '\0')
        {
          fprintf(stderr, "Error: Line %d: \".data\" missing comma delimeter after number %d!\n", 
          								line_num, converted_num_list[(*amount_of_args)-1]);
          (*error_flag)++;
          *amount_of_args = 0;
          return *amount_of_args;
        }
      }
		
	  /* if no legal number found - report error */	
      if(*amount_of_args == 0)
      {
        fprintf(stderr, "Error: Line %d: \".data\" command received no number arguments!\n", line_num);
        (*error_flag)++;
        *amount_of_args = 0;
        return *amount_of_args;
      }
		
	  /* if 2 commas were found with no argument in between them - report error */ 	
      else if(*util_ptr == ',')
      {
        fprintf(stderr, "Error: Line %d: \".data\" syntax error: two consecutive comma delimeters with out a number in between!\n", line_num);
        (*error_flag)++;
        *amount_of_args = 0;
        return *amount_of_args;
      }
		
	  /* if we got to end of input line - check if there is an extra comma with no argument after */ 
      else if((strcmp(util_ptr, "") == 0))
      {
       if(has_unsatisfied_comma) /* if so - report error */ 
       {
         fprintf(stderr, "Error: Line %d: \".data\" syntax error: unsatesfied comma delimeter at end of line!\n", line_num);
         (*error_flag)++;
         *amount_of_args = 0;
         return *amount_of_args;
       }
       else /* if okay - return amount of successfuly fecthed arguments */ 
         return *amount_of_args;
      }
    }
    return *amount_of_args; /* return result to caller - in case of NULL pointer or empty line the result is 0 */
} /******************************************************************************************************************/

/************************************************************************************************************************/
/*                                              set_data                                                                */
/************************************************************************************************************************/
/* This function is responsible for updating .data arguments fetched by get_data into the data image memory. It get an  */
/* the number in a array of intergers in decimal value, converts them into binary representation and updates database.  */
/* up to MAX_OUTPUT_DATA_LINES could be updatedfor each file. it returns amount of numbers updated in database or zero  */
/* if a memory overflow error has occured.                                                                              */
/************************************************************************************************************************/
int set_data (char data_image[MAX_OUTPUT_DATA_LINES][BITS_IN_REGISTER+1], unsigned int *DC, 
			  int data_arguments[MAX_AMOUNT_OF_ARGS], int amount_of_args, unsigned int line_num, unsigned int *error_flag)
{
  int i; /* index counter of numbers to update in data image */ 
  static char num_in_binary[BITS_IN_BYTE * sizeof(signed int)+1]; /* array to hold binary represtation of each number */
  char *util_ptr; /* utility char pointer */ 
	
  /* as long there more arguments to update and more free memory space left on data image */ 	
  for (i=0; i < amount_of_args && *DC < MAX_OUTPUT_DATA_LINES; i++) 
  { /* converto from decimal int to binary string character representation */ 
    convert_decimal_to_binary(data_arguments[i], num_in_binary); /* get the binary representation */
    util_ptr = num_in_binary + strlen(num_in_binary) - BITS_IN_REGISTER; /* skip reduant prefix by jumping to end and cuting 12 lsb*/

    if (*DC < MAX_OUTPUT_DATA_LINES) /* assure there is free memory space */
    { /* update database with binary representation of current number */ 
      strcpy(data_image[*DC], util_ptr);
      (*DC)++; /* update data counter to point to next free memory cell in the data image */ 
    }
    else /* in out of memory space - report erorr */ 
    {
      fprintf(stderr, "Error: Line %d: Not enough free memory space for saving more data!\n", line_num);
      error_flag++;
      return 0; /* return zero at failre */ 
    }
  }
  return i; /* if success - return amount of numbers updated */
} /***********************************************************************************************************************/


/**************************************************************************************************************************/
/*                                      	    get_string                                                                */
/* This function is used to validate ".string" arguments before updating them in the data image. It recevies an input line*/
/* of character strings, and validates it contains a list of legal characters separated by comma delimiters and surounded */
/* by double quote charcters at start and end. it legal - it saves each character separately in an array and in the end it*/
/* adds the terminate zero null character to mark end of string. The function return the amount of characters successfuly */
/* saved, including the terminatiing zero. If error the function return zero.                                             */
/**************************************************************************************************************************/
int get_string (char* line, char *char_arguments, int *amount_of_args, unsigned int line_num, unsigned int *error_flag)
{
    char *util_ptr; /* util pointer for parsing the data */
    char *end_of_string_ptr; /* pointer to end of string argument */ 
    *amount_of_args = 0; /* initialize amount of numeric arguments in input line */

    if (line != NULL) /* validation check for preventing run time errors in case of invalid pointer from caller */
    {
      while(isspace(*line)) /* skip spaces if necessary */ 
        line++;
	
	  /*validate first character is double quotes marking the begining of the string */
      util_ptr = strchr(line, '\"'); 
      if(util_ptr != NULL)
      {
        if (*util_ptr != *line) /* if not - report error */ 
        {
          fprintf(stderr, "Error: Line %d: \'.string\' syntax error: missing double quotes (\") at beginning of string!\n", line_num);
          (*error_flag)++;
          return *amount_of_args;
        }
		
		/* validate there is a terminating double quote at end of string */ 
		end_of_string_ptr = strrchr (line, '\"'); /* point to last occourence of double quote */ 
		if ((end_of_string_ptr != NULL) && (end_of_string_ptr > util_ptr)) /* validate its not the first one */ 
		{        
		    util_ptr++;
		    /* if valid - as long we havn't reached end of string, save each character */ 
		    while(util_ptr != end_of_string_ptr && *util_ptr != '\n' && *util_ptr != '\0')
		    {
		      *char_arguments++ = *util_ptr++;
		      (*amount_of_args)++; /* update amount of characters saved so far */ 
		    }
			
			/* when we reached the end of string */
		    if(*util_ptr == '\"')
		    {
		      *char_arguments = '\0'; /* add the null terminator character */ 
		      (*amount_of_args) += 1; /* increment length by 1 for '\0' terminator */
		      return *amount_of_args; /* return successfully total amount of characters saved */
		    }
        }
        else /* no double quote character at end of string - report error */ 
        {
          fprintf(stderr, "Error: Line %d: \'.string\' syntax error: missing double quotes (\") at end of string!\n", line_num);
          (*error_flag)++;
          return *amount_of_args;
        }
      } /* if no double quotes at all - report error */ 
      else
      {
        fprintf(stderr, "Error: Line %d: \'.string\' syntax error: missing double quotes (\") before and after the string!\n", line_num);
        (*error_flag)++;
        return *amount_of_args;
      }
    } /* if the string argument passed by calling routine was a Null pointer - report error */
    else
    {
      fprintf(stderr, "Error: Line %d: \'.string\' argument error: NULL char pointer given as argument!\n", line_num);
      (*error_flag)++;
      return *amount_of_args;
    }
}/*************************************************************************************************************************/


/**************************************************************************************************************************/
/*                                    		      set_string                                                              */
/**************************************************************************************************************************/
/* This function gets a string characters saved by the get_string function from the .string instrunction. The function is */
/* in charge of converting the characters from their Ascii values into their coresponding binary representation values,and*/
/* and save the converted representation in the data image memory, by saving whole 12 bit cell word for each individual   */
/* charcater. At the end another line of zero's is added to mark end of string. Upto MAX_OUTPUT_DATA_LINES could be saved */
/* in data image for each file. The function returns the amount of characters succesfully saved or zero in case of memort */
/* overflow error.                                                                                                        */
/**************************************************************************************************************************/
int set_string (char data_image[MAX_OUTPUT_DATA_LINES][BITS_IN_REGISTER+1], unsigned int *DC, 
				char char_arguments[MAX_STRING_LENGTH], int amount_of_args, unsigned int line_num, unsigned int *error_flag)
{
  int i; /* index for counting amount of characters updated */ 
  static char num_in_binary[BITS_IN_BYTE * sizeof(unsigned int)+1]; /* array to save binary representation of each char */
  char *util_ptr; /* utility temp char pointer */ 

  /* while there are more character arguments and more free memory space */ 
  for (i=0; i < amount_of_args && *DC < MAX_OUTPUT_DATA_LINES; i++)
  { /* convery ascii value to binary */ 
    convert_decimal_to_binary((unsigned)char_arguments[i], num_in_binary); /* get the binary representation */
    util_ptr = num_in_binary + strlen(num_in_binary) - BITS_IN_REGISTER; /* skip reduant prefix by jumping to end and cuting 12 lsb*/
    
    /* assure there is free memory space */
    if (*DC < MAX_OUTPUT_DATA_LINES) 
    { /* if so - save binary representation in database */ 
      strcpy(data_image[*DC], util_ptr);
      (*DC)++; /* update Data Counter to point to next free memory cell in data image */ 
    }
    else /* if out of memory - report error */
    {
      fprintf(stderr, "Error: Line %d: Not enough free memory space for saving more data!\n", line_num);
      error_flag++;
      return 0; /* return zero in case of error */ 
    }
  }
  return i; /* return succesfully amount of characters updated in data base */ 
} /************************************************************************************************************************/


