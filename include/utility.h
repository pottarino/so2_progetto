#ifndef PARSERS_H
#define PARSERS_H
#include <stdbool.h>
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

extern Dictionary dict_keywords;
extern Dictionary dict_types;
extern Dictionary dict_modifiers;

void init_syntax();
void add_to_dict(Dictionary* dict, const char* word);
int is_qualifier(const char* word);
int is_keyword(const char* word);
TokenType get_token_type(const char* word);
int is_known_type(char* word);
char** split(const char* src, const char *splitter);
void free_split(char** words);

int is_variable_declaration(const char* line);

#endif