//
// Created by potta on 02/04/2026.
//

#ifndef SO2_UTILITY_H
#define SO2_UTILITY_H
void append_str(char** dest, const char* src);
char* filereader(char* filedest);
const char** reserved_keywords;
int is_keyword(char* name);
int reserved_keywords_count;
int custom_keywords_count;
void add_reserved_keyword(char *new_keyword);
#endif