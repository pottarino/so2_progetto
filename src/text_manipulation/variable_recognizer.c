//
// Created by potta on 02/04/2026.
//

#include "utility.h"
#include <ctype.h>

int is_name_valid(char *name) {
    if (!name || name[0] == '\0')  return 0;
    if (isdigit(name[0])) return 0;
    for (int i = 0; name[i] != '\0'; i++) {
        if (!isalnum(name[i]) && name[i] != '_') return 0;
    }
    if (is_keyword(name)) return 0;
    return 1;
}
