/**********************************************************************************************************************/
/*                   Header File - main.h: Generic defentions for handleing I/O file interface.                       */
/**********************************************************************************************************************/
#ifndef MAMAN14_MAIN_H_
#define MAMAN14_MAIN_H_

	#include "symbols.h" 
	#define FILE_MAX_NAME 260 /* max length a file name according to unix systems */
	#define MAX_FILES 100 /* max files to be processed with each run of the program */ 
	#define NO_FILES_FOUND_ON_INPUT 1 /* Exception Error signal in case no files were fiven on input */ 

	/* util functions for handeling output files */	
	int generate_object_file (obj_file_struct file_record, char *file_name);
	int generate_entry_file (entry *entry_file_record, char *file_name);
	int generate_extern_file (external_symbol *extern_file_record, char *file_name);
	
	/* constant suffixes of the possible input/output files */	
	const char file_name_suffix[]  = ".as";
	const char object_name_suffix[] = ".ob";
	const char entry_name_suffix[] = ".ent";
	const char extern_name_suffix[] = ".ext";

#endif /* MAMAN14_MAIN_H_ */
/**********************************************************************************************************************/
