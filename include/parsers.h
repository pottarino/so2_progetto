#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* line;
    int line_number;
    char* filename;
} FileText;

typedef struct {
    FileText* headers;
    FileText* variables;
    FileText* main_body;
    char* filename;
    int h_count, v_count, m_count;
} ParsedProgram;

typedef struct {
    char** lines;
    int* line_numbers;
    int* continues;
    int count;
    char* filename;
} CodeLine;

typedef struct {
    CodeLine* variables;
    CodeLine* typedefs;
    int variables_count;
    int typedefs_count;
} ParsedGlobal;

typedef struct {
    CodeLine codeLine;
    char* formattedCodeLine;
} ParsedCodeLine;

ParsedProgram first_parsing(char * const text, char* const filename);
ParsedGlobal parseGlobal(ParsedProgram p);
ParsedCodeLine parseCodeLine(CodeLine c);

void init_codeline(CodeLine* cl, const char* filename);
void add_fragment(CodeLine* cl, const char* text, int line_num);
void append_filetext(FileText** dest, int* count, const char* line, int line_number, const char* filename);

#endif // PARSER_H