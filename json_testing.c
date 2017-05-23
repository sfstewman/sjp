#include "json_testing.h"

#include <stdio.h>
#include <string.h>

const char testing_close_marker[] = { 0, 'c', 'l', 'm', 'k', 0 };

int ntest = 0;
int nfail = 0;

const char *ret2name(enum JSON_RESULT ret)
{
  switch (ret) {
    case JSON_TOO_MUCH_NESTING:
      return "TOO_MUCH_NESTING";

    case JSON_INTERNAL_ERROR:   
      return "INTERNAL_ERROR";

    case JSON_INVALID_KEY:
      return "INVALID_KEY";

    case JSON_UNCLOSED_ARRAY:   
      return "UNCLOSED_ARRAY";

    case JSON_UNCLOSED_OBJECT:  
      return "UNCLOSED_OBJECT";

    case JSON_INVALID_PARAMS:   
      return "INVALID_PARAMS";

    case JSON_UNFINISHED_INPUT: 
      return "UNFINISHED_INPUT";

    case JSON_INVALID_INPUT:    
      return "INVALID_INPUT";

    case JSON_INVALID_CHAR:     
      return "INVALID_CHAR";

    case JSON_INVALID_ESCAPE:   
      return "INVALID_ESCAPE";

    case JSON_INVALID_U16PAIR:  
      return "INVALID_U16PAIR";

    /*
    case JSON_INVALID:
      return "INVALID";
    */

    case JSON_OK:
      return "OK";

    case JSON_MORE:
      return "MORE";

    case JSON_PARTIAL:
      return "PARTIAL";

    default:
      return "UNKNOWN";
  }
}

const char *tok2name(enum JSON_TOKEN type)
{
  switch (type) {
    case JSON_TOK_NONE:     return "NONE";
    case JSON_TOK_NULL:     return "NULL";
    case JSON_TOK_TRUE:     return "TRUE";
    case JSON_TOK_FALSE:    return "FALSE";
    case JSON_TOK_STRING:   return "STR";
    case JSON_TOK_NUMBER:   return "NUM";
    case JSON_TOK_OCURLY:   return "{";
    case JSON_TOK_CCURLY:   return "}";
    case JSON_TOK_OBRACKET: return "[";
    case JSON_TOK_CBRACKET: return "]"; 
    case JSON_TOK_COMMA:    return ",";
    case JSON_TOK_COLON:    return ":";
    default:            return "UNKNOWN";
  }
}

const char *lst2name(enum JSON_LEX_STATE st)
{
  switch (st) {
    case JSON_LST_VALUE:    return "VALUE";
    case JSON_LST_KEYWORD:  return "KEYWORD";
    case JSON_LST_STR:      return "STR";
    case JSON_LST_STR_ESC1: return "STR_ESC1";
    case JSON_LST_STR_ESC2: return "STR_ESC2";
    case JSON_LST_STR_ESC3: return "STR_ESC3";
    case JSON_LST_STR_ESC4: return "STR_ESC4";
    case JSON_LST_STR_ESC5: return "STR_ESC5";
    case JSON_LST_NUM_NEG:  return "NUM_NEG";
    case JSON_LST_NUM_DIG0: return "NUM_DIG0";
    case JSON_LST_NUM_DIG:  return "NUM_DIG";
    case JSON_LST_NUM_DOT:  return "NUM_DOT";
    case JSON_LST_NUM_DIGF: return "NUM_DIGF";
    case JSON_LST_NUM_EXP:  return "NUM_EXP";
    case JSON_LST_NUM_ESGN: return "NUM_ESGN";
    case JSON_LST_NUM_EDIG: return "NUM_EDIG";
    default: return "UNK";
  }
}

const char *pst2name(enum JSON_PARSER_STATE st)
{
  switch (st) {
    case JSON_PARSER_VALUE:     return "VALUE";
    case JSON_PARSER_PARTIAL:   return "PARTIAL";
    case JSON_PARSER_OBJ_NEW:   return "OBJ_NEW";
    case JSON_PARSER_OBJ_KEY:   return "OBJ_KEY";
    case JSON_PARSER_OBJ_COLON: return "OBJ_COLON";
    case JSON_PARSER_OBJ_VALUE: return "OBJ_VALUE";
    case JSON_PARSER_OBJ_NEXT:  return "OBJ_NEXT";
    case JSON_PARSER_ARR_NEW:   return "ARR_NEW";
    case JSON_PARSER_ARR_ITEM:  return "ARR_ITEM";
    case JSON_PARSER_ARR_NEXT:  return "ARR_NEXT";
    default: return "UNKNOWN";
  }
}

const char *evt2name(enum JSON_EVENT evt)
{
  switch (evt) {
    case JSON_NONE: return "NONE";
    case JSON_NULL: return "NULL";
    case JSON_TRUE: return "TRUE";
    case JSON_FALSE: return "FALSE";
    case JSON_STRING: return "STRING";
    case JSON_NUMBER: return "NUMBER";
    case JSON_OBJECT_BEG: return "OBJECT_BEG";
    case JSON_OBJECT_END: return "OBJECT_END";
    case JSON_ARRAY_BEG: return "ARRAY_BEG";
    case JSON_ARRAY_END: return "ARRAY_END";
    default: return "UNKNOWN";
  }
}

