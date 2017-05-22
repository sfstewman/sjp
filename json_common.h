#ifndef JSON_COMMON_H
#define JSON_COMMON_H

#define MODULE_NAME JSON_COMMON

enum JSON_RESULT {
  JSON_INTERNAL_ERROR   = -127, // internal error occured

  JSON_UNCLOSED_ARRAY   = -9,   // array is unclosed at the end of input
  JSON_UNCLOSED_OBJECT  = -8,   // array is unclosed at the end of input
  JSON_INVALID_PARAMS   = -7,   // invalid parameters

  JSON_UNFINISHED_INPUT = -6,   // unfinished input (after end-of-stream stream)
  JSON_INVALID_INPUT    = -5,   // invalid character encountered
  JSON_INVALID_CHAR     = -4,   // invalid utf8 or invalid bytes in string
  JSON_INVALID_ESCAPE   = -3,   // invalid escape sequence
  JSON_INVALID_U16PAIR  = -2,   // invalid surrogate pairs

  JSON_INVALID          = -1,   // 
  JSON_OK               =  0,
  JSON_MORE             =  1,   // return partial token, needs more input
  JSON_PARTIAL          =  2,   // return partial token, does not need more input
};

#define JSON_ERROR(ret) ((ret) < 0)

#undef MODULE_NAME

#endif /* JSON_COMMON_H */

