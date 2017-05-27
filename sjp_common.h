#ifndef SJP_COMMON_H
#define SJP_COMMON_H

#define MODULE_NAME SJP_COMMON

enum SJP_RESULT {
  SJP_INTERNAL_ERROR   = -128, // internal error occured

  SJP_TOO_MUCH_NESTING = -10,  // invalid character encountered
  SJP_INVALID_KEY      = -9,   // invalid character encountered

  SJP_UNCLOSED_ARRAY   = -8,   // array is unclosed at the end of input
  SJP_UNCLOSED_OBJECT  = -7,   // array is unclosed at the end of input
  SJP_INVALID_PARAMS   = -6,   // invalid parameters

  SJP_INVALID_INPUT    = -5,   // invalid character encountered
  SJP_INVALID_CHAR     = -4,   // invalid utf8 or invalid bytes in string
  SJP_INVALID_ESCAPE   = -3,   // invalid escape sequence
  SJP_INVALID_U16PAIR  = -2,   // invalid surrogate pairs

  SJP_UNFINISHED_INPUT = -1,   // unfinished input (after end-of-stream stream)

  SJP_OK               =  0,
  SJP_MORE             =  1,   // return partial token, needs more input
  SJP_PARTIAL          =  2,   // return partial token, does not need more input
};

#define SJP_ERROR(ret) ((ret) < 0)

#undef MODULE_NAME

#endif /* SJP_COMMON_H */

