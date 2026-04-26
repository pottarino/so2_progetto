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
    char** lines;
    int* line_numbers;
    int count;
    char* filename;
} CodeLine;

typedef struct {
    CodeLine codeLine;
    char* formattedCodeLine;
} ParsedCodeLine;

typedef struct {
    ParsedCodeLine* headers;
    ParsedCodeLine* variables;
    ParsedCodeLine* main_body;
    char* filename;
    int h_count, v_count, m_count;
} ParsedProgram;



typedef struct {
    ParsedCodeLine *variable_lines;
    int variables_count;
    ParsedCodeLine *instructions;
    int instructions_count;
} ParsedMain;

typedef struct {
    ParsedCodeLine* variable_lines;
    ParsedCodeLine* typedefs;
    int variables_count;
    int typedefs_count;
}ParsedGlobal;

typedef struct {
    char** headers; //temp
    int count;
}ParsedHeaders;
ParsedCodeLine parseCodeLine(CodeLine const c);
void init_codeline(CodeLine* cl, const char* filename);
void add_fragment(CodeLine* cl, const char* text, int line_num);
void append_filetext(FileText** dest, int* count, const char* line, int line_number, const char* filename);
void free_parsed_program(ParsedProgram* p);
void free_parsed_parts(ParsedGlobal* pg, ParsedMain* pm) ;
void free_headers(ParsedHeaders* ph);
void free_codeline(CodeLine* cl) ;
#endif // PARSER_H