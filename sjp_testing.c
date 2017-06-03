#include "sjp_testing.h"

#include <stdio.h>
#include <string.h>

const char testing_close_marker[] = { 0, 'c', 'l', 'm', 'k', 0 };
const char testing_end_of_stream[] = { 0, 'e', 'o', 's', 0 };

int ntest = 0;
int nfail = 0;

const char *ret2name(enum SJP_RESULT ret)
{
  switch (ret) {
    case SJP_TOO_MUCH_NESTING:
      return "TOO_MUCH_NESTING";

    case SJP_INTERNAL_ERROR:   
      return "INTERNAL_ERROR";

    case SJP_INVALID_KEY:
      return "INVALID_KEY";

    case SJP_UNCLOSED_ARRAY:   
      return "UNCLOSED_ARRAY";

    case SJP_UNCLOSED_OBJECT:  
      return "UNCLOSED_OBJECT";

    case SJP_INVALID_PARAMS:   
      return "INVALID_PARAMS";

    case SJP_UNFINISHED_INPUT: 
      return "UNFINISHED_INPUT";

    case SJP_INVALID_INPUT:    
      return "INVALID_INPUT";

    case SJP_INVALID_CHAR:     
      return "INVALID_CHAR";

    case SJP_INVALID_ESCAPE:   
      return "INVALID_ESCAPE";

    case SJP_INVALID_U16PAIR:  
      return "INVALID_U16PAIR";

    /*
    case SJP_INVALID:
      return "INVALID";
    */

    case SJP_OK:
      return "OK";

    case SJP_MORE:
      return "MORE";

    case SJP_PARTIAL:
      return "PARTIAL";

    default:
      return "UNKNOWN";
  }
}

const char *tok2name(enum SJP_TOKEN type)
{
  switch (type) {
    case SJP_TOK_NONE:     return "NONE";
    case SJP_TOK_NULL:     return "NULL";
    case SJP_TOK_TRUE:     return "TRUE";
    case SJP_TOK_FALSE:    return "FALSE";
    case SJP_TOK_STRING:   return "STR";
    case SJP_TOK_NUMBER:   return "NUM";
    case SJP_TOK_OCURLY:   return "{";
    case SJP_TOK_CCURLY:   return "}";
    case SJP_TOK_OBRACKET: return "[";
    case SJP_TOK_CBRACKET: return "]"; 
    case SJP_TOK_COMMA:    return ",";
    case SJP_TOK_COLON:    return ":";
    case SJP_TOK_EOS:      return "EOS";
    default:            return "UNKNOWN";
  }
}

const char *lst2name(enum SJP_LEX_STATE st)
{
  switch (st) {
    case SJP_LST_VALUE:    return "VALUE";
    case SJP_LST_KEYWORD:  return "KEYWORD";
    case SJP_LST_STR:      return "STR";
    case SJP_LST_STR_ESC1: return "STR_ESC1";
    case SJP_LST_STR_ESC2: return "STR_ESC2";
    case SJP_LST_STR_ESC3: return "STR_ESC3";
    case SJP_LST_STR_ESC4: return "STR_ESC4";
    case SJP_LST_STR_ESC5: return "STR_ESC5";
    case SJP_LST_NUM_NEG:  return "NUM_NEG";
    case SJP_LST_NUM_DIG0: return "NUM_DIG0";
    case SJP_LST_NUM_DIG:  return "NUM_DIG";
    case SJP_LST_NUM_DOT:  return "NUM_DOT";
    case SJP_LST_NUM_DIGF: return "NUM_DIGF";
    case SJP_LST_NUM_EXP:  return "NUM_EXP";
    case SJP_LST_NUM_ESGN: return "NUM_ESGN";
    case SJP_LST_NUM_EDIG: return "NUM_EDIG";
    default: return "UNK";
  }
}

const char *pst2name(enum SJP_PARSER_STATE st)
{
  switch (st) {
    case SJP_PARSER_VALUE:     return "VALUE";
    case SJP_PARSER_PARTIAL:   return "PARTIAL";
    case SJP_PARSER_OBJ_NEW:   return "OBJ_NEW";
    case SJP_PARSER_OBJ_KEY:   return "OBJ_KEY";
    case SJP_PARSER_OBJ_COLON: return "OBJ_COLON";
    case SJP_PARSER_OBJ_VALUE: return "OBJ_VALUE";
    case SJP_PARSER_OBJ_NEXT:  return "OBJ_NEXT";
    case SJP_PARSER_ARR_NEW:   return "ARR_NEW";
    case SJP_PARSER_ARR_ITEM:  return "ARR_ITEM";
    case SJP_PARSER_ARR_NEXT:  return "ARR_NEXT";
    default: return "UNKNOWN";
  }
}

const char *evt2name(enum SJP_EVENT evt)
{
  switch (evt) {
    case SJP_NONE: return "NONE";
    case SJP_NULL: return "NULL";
    case SJP_TRUE: return "TRUE";
    case SJP_FALSE: return "FALSE";
    case SJP_STRING: return "STRING";
    case SJP_NUMBER: return "NUMBER";
    case SJP_OBJECT_BEG: return "OBJECT_BEG";
    case SJP_OBJECT_END: return "OBJECT_END";
    case SJP_ARRAY_BEG: return "ARRAY_BEG";
    case SJP_ARRAY_END: return "ARRAY_END";
    default: return "UNKNOWN";
  }
}

