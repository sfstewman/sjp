#ifndef JSON_COMMON_H
#define JSON_COMMON_H

#define MODULE_NAME JSON_COMMON

enum JSON_RESULT {
  JSON_INVALID = -1,
  JSON_OK      =  0,
  JSON_MORE    =  1,   // return partial token, needs more input
  JSON_PARTIAL =  2,   // return partial token, does not need more input
};


#undef MODULE_NAME

#endif /* JSON_COMMON_H */

