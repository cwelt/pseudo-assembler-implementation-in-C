/************************************************************************************************************/
/* symbols.h - constants and data structure defentions and uti lfunction declarations for handeling symbols */
 /***********************************************************************************************************/
 
#ifndef MAMAN14_SYMBOLS_H_
#define MAMAN14_SYMBOLS_H_

#include "assembler.h"
#define SYMBOL_MAX_LENGTH 30 /* max length allowed for a symbols label name */
#define BITS_IN_REGISTER 12 /* amount of bits in a single register on this ISA */ 

typedef enum type_of_symbol {external, relocatable} symbol_type;  /* enumuration of possible symbol types */
typedef enum context_of_symbol {external_context, data, code} symbol_context; /* enumuration of possible symbol context */

/* symbol record */ 
typedef struct symbol_node 
{
    char name[SYMBOL_MAX_LENGTH+1];  /* name of symbol (label) - 30 + \'0' */
    unsigned int address_decimal_value; /* decimal value symbol adress */
    symbol_type type; /* external or relocatable */
    symbol_context context; /* data or code */
    unsigned int source_line; /* assembly file source line for error reporting */ 
    struct symbol_node *next; /* pointer to next symbol in table */ 
} symbol;

/* entry record that will be used for generating the ".ent" file */ 
typedef struct entry_node
{
    char name[SYMBOL_MAX_LENGTH+1];  /* name of symbol (label) 30 + \'0' */
    unsigned int address_decimal_value; /* decimal value symbol address of symbol */
    unsigned int source_line; /* source line in assembly file for error reports */ 
    struct entry_node *next; /* pointer to next record on table */ 
} entry;

/* extern record that will be used for generating the ".ext" file */ 
typedef struct external_node
{
    char name[SYMBOL_MAX_LENGTH+1];  /* name of symbol (label) 30 + \'0' */
    unsigned int address_decimal_value; /* decimal value symbol adress */
    struct external_node *next; /* pointer to next record on table */ 
} external_symbol;


/* record for saving symbols that were used in direct addressing method in mchine instructions */ 
typedef struct direct_addressing_symbol
{
  char name[SYMBOL_MAX_LENGTH+1]; /* symbol name 30 + \'0' */ 
  unsigned int address; /* IC address of symbol */
  unsigned int source_line; /* source line in assembly file for error report */
  struct direct_addressing_symbol *next; /* pointer to next record on table */ 
} direct_address_symbol;

/* record for saving symbols that were used in distance addressing method in mchine instructions */ 
typedef struct distance_addressing_symbols
{
  char name1[SYMBOL_MAX_LENGTH+1]; /* first symbol name 30 + \'0' */ 
  char name2[SYMBOL_MAX_LENGTH+1]; /* second symbol name 30 + \'0' */ 
  unsigned int address_of_instruction; /* IC address of instruction that referenced these symbols */ 
  unsigned int address_to_replace; /* IC address of the code image index needed to be replace with the distance */
  unsigned int source_line; /* assembly source line for error reporting */
  struct distance_addressing_symbols *next; /* pointer to next record on table */ 
} distance_addressing_symbol;

/* util function declarations - see full detaild API at symbols.c */
symbol * check_symbol_existence (symbol *head, char *name);
symbol * add_new_symbol (symbol *head, char *name, unsigned int decimal_address, symbol_type type, symbol_context context, unsigned int line);
entry * add_new_entry(entry *head, char *name, unsigned int line);
external_symbol * add_new_extern (external_symbol *head, char *name, unsigned int address);
direct_address_symbol * add_direct_address_symbol (direct_address_symbol *head, char *symbol_name, unsigned int address, unsigned int line);
distance_addressing_symbol *add_distance_address_symbols (distance_addressing_symbol *head, char *name1, char *name2, unsigned int address_of_instruction, unsigned int address_to_replace, unsigned int line);
void relocate_data_symbols_address (symbol *head, unsigned int IC);
void set_missing_addresses (direct_address_symbol *head, symbol *symbol_table, external_symbol **extern_table, char missing_address_line[][BITS_IN_REGISTER+1], unsigned int *error_flag);
void set_missing_distances (distance_addressing_symbol *head, symbol *symbol_table, char missing_distance_line[][BITS_IN_REGISTER+1], unsigned int *error_flag);
void set_entry_addresses (entry *entry_table, symbol *symbol_table, unsigned int *error_flag);
void free_symbol_table (symbol *head);
void free_entry_table (entry *head);
void free_extern_table (external_symbol *head);
void free_direct_address_symbol (direct_address_symbol *head);
void free_distance_addressing_symbol (distance_addressing_symbol *head);

#endif /* MAMAN14_SYMBOLS_H_ */

