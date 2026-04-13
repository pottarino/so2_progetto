//
// Created by Showmae on 08-Apr-26.
//

#ifndef SO2_ERRORS_H
#define SO2_ERRORS_H
typedef enum {
    NO_ERROR = 0,
    INPUT_ERROR = 1,
    FILE_OPEN_ERROR = 2,
    FILE_CLOSE_ERROR = 3,
    FILE_READ_ERROR = 4,
    FILE_WRITE_ERROR = 5
} StatusCode;

typedef enum {
  VARIABLE_NAME_ERROR,
    VARIABLE_TYPE_ERROR,
    VARIABLE_UNUSED_ERROR,
} VariableError;
#endif //SO2_ERRORS_H
