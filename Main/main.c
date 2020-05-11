/**************************************************************************************************************************/
/* 2015A - 20465 : Final Project (Maman 14       **************************************************************************/
/* @authors:  Alex Spayev & Chanan Welt     *******************************************************************************/
/* @date 20/03/2015          		    ***********************************************************************************/
/**************************************************************************************************************************/
/*	util header files inclusions (each header API could be found in the header file itself	*/  
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "main.h" 
#include "symbols.h" 
#include "assembler.h" 
#include "data_manager.h" 
#include "code_manager.h"

/* declaring a util function that will be used for processing a single assembly file and converting it to raw binary data */ 
int assemble_file (obj_file_struct *obj_record, entry **ent, external_symbol**ext, 
         			 opcode table[NUM_OF_OPCODES], assembly_register reg_table[NUM_OF_REGISTERS]);

/**************************************************************************************************************************/
/*                                                    main.c                                                              */
/**************************************************************************************************************************/
/* main.c is the main interface between the assembler program and the user and operating system. Main.c is responsible of */
/* controlling the process and flow of the program, I/O, transfering data and reporting compilation status for each file. */
/* main inputs assembly files (".as" name suffix) as command line arguments. It processes them individuadlly,each file at */
/* time, by transfering it to the assembler. If the file was successfully assembled, the assmebler returns main raw binary*/
/* data of the source code, as an intermediate stage of translating it to machine language. Main.c is in charge of the    */
/* processing of the raw binary data and converting to an object file in hexadecimal base, and additional files if needed */
/* entry and extern declarations. If any error occours during process of a certain assembly file, no output files would   */
/* be generated for this specific file. Main prints out a detailed report of success/failure for each file.               */
/* Warning: at most MAX_FILES could be compiled with each program run. If more than MAX_FILES are been requested at a     */
/* single program run, only the first MAX_FILES files world be processed.                                                 */
/**************************************************************************************************************************/
int main (int argc, char *argv[])
{
    /* data defention */ 
    FILE *ifp; /* input file pointer for each assembly file to be compiled */
    obj_file_struct obj_file_record; /* struct for saving raw data for generating an object file */
    entry *entry_file_record; /* records for saving raw data for generating an entry file if needed */
    external_symbol *extern_file_record; /* records for saving raw data for generating an extern file if needed */
    char *argument_name; /* name of current command line argument */
    char file_name[FILE_MAX_NAME]; /* name of current file being processed */ 
    opcode opcode_table[NUM_OF_OPCODES]; /* opcode table */
    assembly_register register_table[NUM_OF_REGISTERS]; /* register table */
    const unsigned int suffix_length = strlen(file_name_suffix); /* length of ".as" */
    const unsigned int allowed_length = FILE_MAX_NAME - suffix_length; /* max length allowed for file name */
    unsigned int arg_name_length; /* actual file name length */
    unsigned int file_counter = 0; /* total amount counter of assembly files being compiled */  

  	
  	/*Threshhold Validations */ 
    if (argc > MAX_FILES+1) /* validation #1 - not too much files */
         fprintf(stderr, "%s: Warning: Maximum files allowed is %d. The remaining files won't be processed.\n"
         																				, argv[0], MAX_FILES);
    if (argc == 1) /* validation #2 - at least one file existance */
    {
        fprintf(stderr, "%s: Error: no files found for the assmebler!\n", argv[0]);
        exit (NO_FILES_FOUND_ON_INPUT);
    }

	/* Preparation Activity:   set constant opcode and register tables that would be used for all files */
    set_opcode_table(opcode_table);
    set_register_table(register_table);
	
	/* File Processor */ 
    while (--argc > 0 && file_counter < MAX_FILES)  
    {	/* as long there are more files to process and max amount not exceeded */
        file_counter++; /* update file counter */
        argument_name = *++argv; /* advance to next command line arg and save it's name */ 
        arg_name_length = strlen(argument_name); /* save it's length */ 
        if (arg_name_length > allowed_length) /* make sure name isn't too long */
        { /* if file name too long - can't save it - continue to next file */ 
          fprintf(stderr, "%s:Error: File name \"%s\" is too long!\n", argv[0], *argv);
          continue; 
        }
        strcpy(file_name, argument_name); /* if not too long - save file name */
        strcat(file_name, file_name_suffix); /* concatinate the suffix ".as" */
		
		/* try to open the given assembly file in read mode */
        if ((ifp = fopen(file_name, "r")) == NULL) /* failure */ 
        {
            fprintf(stderr,"System failure occoured in attempt to open file \"%s\"\n", file_name);
            continue;
        }
        else /* success */ 
        {	/* print title of compilation report for current assembly file */ 
            fprintf(stderr, "======================================================================\n");
            fprintf(stderr, "----------------------------------------------------------------------\n");
            fprintf(stderr, "\t\tCompilation report for file \"%s\"\n", file_name);
            fprintf(stderr, "----------------------------------------------------------------------\n");
            
            /* call the assembler and transfer relevant data and input file pointer */ 
            strcpy(obj_file_record.name, file_name);
            obj_file_record.file_ptr = ifp;
            assemble_file (&obj_file_record, &entry_file_record, &extern_file_record, opcode_table, register_table);

            /* validate no errors occoured during compilation */
            if (obj_file_record.errors == 0) /* success */
            { /* if no errors - generate necessary output files */
             
              /* (A) Generate Object File */
              if(generate_object_file (obj_file_record, *argv) == EXIT_SUCCESS)
                fprintf(stderr, "\t(*)File \"%s%s\"  has been successfully generated\n", *argv, object_name_suffix);
              else fprintf(stderr, "System failure occured in attempt to generate file \"%s%s\"!\n",
              																	    *argv,object_name_suffix);
              /* (B) Generate Entry File if any entries have been registerd */
              if (entry_file_record != NULL)
                if(generate_entry_file(entry_file_record, *argv) == EXIT_SUCCESS)
                  fprintf(stderr, "\t(*)File \"%s%s\" has been successfully generated\n", *argv, entry_name_suffix);
                else fprintf(stderr, "System failure occured in attempt to generate file \"%s%s\"!\n",
              																	    *argv,entry_name_suffix);
			  /* (C) Generate Extern File if any extern declarations have been made */	
              if (extern_file_record != NULL)
                if(generate_extern_file(extern_file_record, *argv) == EXIT_SUCCESS)
                  fprintf(stderr, "\t(*)File \"%s%s\" has been successfully generated\n", *argv, extern_name_suffix);
                else fprintf(stderr, "System failure occured in attempt to generate file \"%s%s\"!\n",
              																	    *argv,extern_name_suffix);
            }
            else /* report failure */
            {   
            	fprintf(stderr, "----------------------------------------------------------------------\n");
            	fprintf(stderr, "Compilation Failed: %d errors found during process.\n",obj_file_record.errors);
            	fprintf(stderr, "----------------------------------------------------------------------\n");
            }
            
            /* print end of report for current file */
            fprintf(stderr, "======================================================================\n"); 
            
            /* Clean Up Job: Release allocated memory, buffers & I/O file pointers back to Operating System */
            free_entry_table(entry_file_record);
            free_extern_table(extern_file_record);
            fclose(obj_file_record.file_ptr);
            fflush(NULL);
        } /* end of current file proccess */
    } /* continue to next file */ 
    
    /* final sum report */ 
    fprintf(stderr, "----------------------------------------------------------------------\n");
    fprintf(stderr, "\nTotal amount of files processed: %d \n", file_counter); 
    fprintf(stderr, "Timestamp on current PC: "); 
    system("date");  
    fprintf(stderr, "----------------------------------------------------------------------\n");
    fprintf(stderr, "======================================================================\n\n"); 
    
    exit (EXIT_SUCCESS); /* return to operating system / calling routine */
}/*************************************************************************************************************************/



/**************************************************************************************************************************/
/*                                              generate_object_file                                                      */
/**************************************************************************************************************************/
/* generate_object_file: This function receives raw data after the assembler work, which contains the binary coding and   */
/* counters for code and data segments in each image.Based on the raw data it converts it to hexadecimal and outputs it to*/
/* an object file, containing the information needed for next stage of compiling process of linker/loader.                */
/* It returns zero for success and non zero for error.                                                                    */
/**************************************************************************************************************************/
int generate_object_file (obj_file_struct file_record, char *file_name)
{
  FILE *ofp; /* output file pointer */
  unsigned int current_address = BASE_START;
  char object_file_name[FILE_MAX_NAME];
  int i;

  /* (1) create a name for the object file */
  strcpy(object_file_name, file_name);
  strcat(object_file_name, object_name_suffix);

  /* (2) try to create the object file */
  if((ofp = fopen(object_file_name, "w")) == NULL)
  {
    fprintf(stderr, "Error: System failure while attempting to create file \"%s\"\n", object_file_name);
    return(EXIT_FAILURE);
  }

  /* (3) print title of file */
  fprintf(ofp, "\tBase %d Address \t Base %d machine code\n\n", OBJECT_BASE, OBJECT_BASE);

  /* (4) print instruction and data counters (IC and DC) */
  fprintf(ofp, "%35X    %X\n", file_record.IC, file_record.DC);

  /* (5) print address list and machine code in hexadecimal base */
  for (i=0; i < file_record.IC; i++)
    fprintf(ofp, "\t %7X \t\t\t\t\t %03X\n",current_address++, convert_binary_to_decimal(file_record.code_image[i]));
  for (i=0; i < file_record.DC; i++)
    fprintf(ofp, "\t %7X \t\t\t\t\t %03X\n",current_address++, convert_binary_to_decimal(file_record.data_image[i]));

  /* (6) close file and return to caller */
  return (fclose(ofp));
}/*************************************************************************************************************************/


/**************************************************************************************************************************/
/*                                              generate_entry_file                                                       */
/**************************************************************************************************************************/
/* generate_entry_file: This function receives raw data after the assembler work, which contains the symbols which were   */
/* marked as enteries for additional exteranl files that would be using them. Based on the symbol name and source defined */
/* address, the function converts the data to hexadecimal and outputs it to a entry file for further processing.          */
/* It returns zero for success and non zero for error.                                                                    */
/**************************************************************************************************************************/
int generate_entry_file (entry *entry_file_record, char *file_name)
{
  FILE *ofp; /* output file pointer */
  char entry_file_name[FILE_MAX_NAME];

  /* (1) create a name for the object file */
  strcpy(entry_file_name, file_name);
  strcat(entry_file_name, entry_name_suffix);

  /* (2) try to create the entry file */
  if((ofp = fopen(entry_file_name, "w")) == NULL)
  {
    fprintf(stderr, "Error: System failure while attempting to create file \"%s\"\n", entry_file_name);
    return(EXIT_FAILURE);
  }

  /* (3) print entry symbols list and their addresses as defined in source file */
  while (entry_file_record != NULL)
  {
    fprintf(ofp, "%-30s %X\n", entry_file_record->name, entry_file_record->address_decimal_value);
    entry_file_record = entry_file_record -> next;
  }

  /* (4) close file and return to caller */
  return (fclose(ofp));
}/************************************************************************************************************************/


/**************************************************************************************************************************/
/*                                              generate_extern_file                                                      */
/**************************************************************************************************************************/
/* generate_extern_file: This function receives raw data after the assembler work, which contains the symbols which were  */
/* marked as external for current files and are actually defined else where. Based on the symbol name and source line     */
/* address that is needed to be replaced later on,the function converts the data to hexadecimal and outputs it to extern  */ 
/* file for further processing of linkage. It returns zero for success and non zero for error.                            */
/**************************************************************************************************************************/
int generate_extern_file (external_symbol *extern_file_record, char *file_name)
{
  FILE *ofp; /* output file pointer */
  char extern_file_name[FILE_MAX_NAME];

  /* (1) create a name for the object file */
  strcpy(extern_file_name, file_name);
  strcat(extern_file_name, extern_name_suffix);

  /* (2) try to create the extern file */
  if((ofp = fopen(extern_file_name, "w")) == NULL)
  {
    fprintf(stderr, "Error: System failure while attempting to create file \"%s\"\n", extern_file_name);
    return(EXIT_FAILURE);
  }

  /* (3) print extern symbols list used and addresses for future linking and loading */
  while (extern_file_record != NULL)
  {
    fprintf(ofp, "%-30s %X\n", extern_file_record->name, extern_file_record->address_decimal_value);
    extern_file_record = extern_file_record -> next;
  }

  /* (4) close file and return to caller */
  return (fclose(ofp));
} /*************************************************************************************************************************/


