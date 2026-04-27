#ifndef PARSERS_H
#define PARSERS_H
#include <stdbool.h>
#include <stddef.h>

#include "errors.h"

typedef enum {
    TOKEN_UNKNOWN,
    TOKEN_KEYWORD,
    TOKEN_TYPE,
    TOKEN_MODIFIER
} TokenType;



typedef struct {
    char** items;
    int count;
} Dictionary;

typedef struct {
    char* text;
    StatusCode statusCode;
} FileRead;

extern Dictionary dict_keywords;
extern Dictionary dict_types;
extern Dictionary dict_modifiers;
char* replace_extension(const char* filename, const char* new_ext);
void init_syntax();
void add_to_dict(Dictionary* dict, const char* word);
int is_qualifier(const char* word);
int is_keyword(const char* word);
TokenType get_token_type(const char* word);
int is_known_type(const char* word);
char** split(const char* src, const char *splitter);
void free_split(char** words);
FileRead file_reader(char* file);
int allocate_more(void ** pointer, int *old_size, size_t element_size);
int free_unused(void **pointer, int new_size);
void clean_newline(char* str);
int starts_with(const char* str, const char* prefix);
#endif