#include "json_testing.h"

#include <stdio.h>
#include <string.h>

int ntest = 0;
int nfail = 0;

const char *ret2name(enum JSON_RESULT ret)
{
  switch (ret) {
    case JSON_INVALID:
      return "INVALID";

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

int lexer_test_inputs(struct json_lexer *lex, const char *inputs[], struct lexer_output *outputs)
{
  int i, j, more;
  char inbuf[2048];

  json_lexer_init(lex);

  i=0;
  j=0;
  more=1;

  while(1) {
    enum JSON_RESULT ret;
    struct json_token tok = {0};
    char buf[1024];
    size_t n;

    LOG("[[ i=%d, j=%d | %d %s ]]\n",
        i,j, lex->state, lst2name(lex->state));

    if (lex_is_sentinel(&outputs[j])) {
      if (inputs[i] != NULL) {
        printf("expected outputs finished, but there are still more inputs:\n"
            "current input %d: %s\n",
            i, inputs[i]);
        return -1;
      }

      return 0;
    }

    if (more) {
      if (inputs[i] == NULL) {
        printf("expected input finished, but there are still more outputs:\n"
            "current output %d: %d %d %s\n",
            j, outputs[i].ret, outputs[i].type, outputs[i].value);
        return -1;
      }

      snprintf(inbuf, sizeof inbuf, "%s", inputs[i]);
      LOG("[MORE] %s\n", inbuf);
      json_lexer_more(lex, inbuf, strlen(inbuf));
      i++;
    }

    ret = json_lexer_token(lex, &tok);

    n = tok.n < sizeof buf ? tok.n : sizeof buf-1;
    memset(buf, 0, sizeof buf);
    if (n > 0) {
      LOG("[VAL ] %zu chars in value\n", n);
      memcpy(buf, tok.value, n);
    }

    LOG("[TOK ] %3d %3d %8s %8s | %s\n",
        ret, tok.type,
        ret2name(ret), tok2name(tok.type),
        buf);

    if (ret != outputs[j].ret) {
      printf("i=%d, j=%d, expected return %d (%s), but found %d (%s)\n",
          i,j,
          outputs[j].ret, ret2name(outputs[j].ret),
          ret, ret2name(ret));
      return -1;
    }

    if (tok.type != outputs[j].type) {
      printf("i=%d, j=%d, expected type %d (%s), but found %d (%s)\n",
          i,j,
          outputs[j].type, tok2name(outputs[j].type),
          tok.type, tok2name(tok.type));
      return -1;
    }

    if (tok.n != strlen(outputs[j].value)) {
      printf("i=%d, j=%d, expected value '%s' but found '%s'\n",
          i,j, outputs[j].value, buf);
      return -1;
    }

    more = (ret == JSON_MORE); 
    j++;
  }

  return 0;
}

