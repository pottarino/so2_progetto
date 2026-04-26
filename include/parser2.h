//
// Created by potta on 20/04/2026.
//

#ifndef SO2_PARSER2_H
#define SO2_PARSER2_H
#include "parsers.h"
ParsedHeaders parseHeaders(ParsedProgram* pp);
ParsedGlobal parseGlobal(ParsedProgram p);
ParsedMain parseMainProgram(ParsedProgram p);
ParsedProgram parse_program(const char *text, char *filename);
#endif //SO2_PARSER2_H