#include "json_lexer.h"

#define TEST_LOG_LEVEL 0
#include "json_testing.h"

#include <stdio.h>
#include <string.h>

int lexer_test_inputs(struct json_lexer *lex, const char *inputs[], struct lexer_output *outputs)
{
  int i, j, more, close;
  char inbuf[2048];

  json_lexer_init(lex);

  i=0;
  j=0;
  more=1;
  close=0;

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

      if (close) {
        LOG("[RESET]%s\n","");
        json_lexer_init(lex);
        close=0;
      }

      if (inputs[i] != testing_close_marker) {
        snprintf(inbuf, sizeof inbuf, "%s", inputs[i]);
        LOG("[MORE] %s\n", inbuf);
        json_lexer_more(lex, inbuf, strlen(inbuf));
      } else {
        close=1;
        LOG("[CLOSE] %s\n", "");
      }
      i++;
    }

    ret = close ? json_lexer_close(lex) : json_lexer_token(lex, &tok);

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

    more = (ret == JSON_MORE) || JSON_ERROR(ret) || close;
    j++;
  }

  return 0;
}

void test_simple_array(void)
{
  const char *inputs[] = {
    "[ true, false, null, \"foo\" ]",
    NULL
  };

  struct lexer_output outputs[] = {
    { JSON_OK, '['        , "["     },
    { JSON_OK, JSON_TOK_TRUE  , "true"  },
    { JSON_OK, ','        , ","     },
    { JSON_OK, JSON_TOK_FALSE , "false" },
    { JSON_OK, ','        , ","     },
    { JSON_OK, JSON_TOK_NULL  , "null"  },
    { JSON_OK, ','        , ","     },
    { JSON_OK, JSON_TOK_STRING, "foo"   },
    { JSON_OK, ']'        , "]"     },
    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct json_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

void test_simple_object(void)
{
  const char *inputs[] = {
    "{ \"foo\" : true, \"bar\" : \"baz\", \"quux\" : null }",
    NULL
  };

  struct lexer_output outputs[] = {
    { JSON_OK, '{'        , "{"     },

    { JSON_OK, JSON_TOK_STRING, "foo"   },
    { JSON_OK, ':'        , ":"     },
    { JSON_OK, JSON_TOK_TRUE  , "true"  },
    { JSON_OK, ','        , ","     },

    { JSON_OK, JSON_TOK_STRING, "bar"   },
    { JSON_OK, ':'        , ":"     },
    { JSON_OK, JSON_TOK_STRING, "baz"   },
    { JSON_OK, ','        , ","     },

    { JSON_OK, JSON_TOK_STRING, "quux"  },
    { JSON_OK, ':'        , ":"     },
    { JSON_OK, JSON_TOK_NULL  , "null"  },
    { JSON_OK, '}'        , "}"     },

    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct json_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

void test_string_with_escapes(void)
{
  const char *inputs[] = {
    "\"this string \\\"has\\\" double quote escapes\"",
    "\"this string has \\\\ some escape c\\bsequences \\/ combinations\"",
    "\"this string tests form-feed\\fand carriage-return \\r\\nand newline\"",
    "\"this string tests \\ttab\"",
    "\"this string has unicode escapes: G\\u00fcnter \\u2318 \\u65E5 \\u672C \\u8A9E\"",
    NULL
  };

  struct lexer_output outputs[] = {
    { JSON_OK, JSON_TOK_STRING, "this string \"has\" double quote escapes" },
    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_STRING, "this string has \\ some escape c\bsequences / combinations" },
    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_STRING, "this string tests form-feed\fand carriage-return \r\nand newline" },
    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_STRING, "this string tests \ttab" },
    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_STRING, "this string has unicode escapes: G\u00fcnter \u2318 \u65E5 \u672C \u8A9E" },
    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct json_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

void test_simple_restarts(void)
{
  const char *inputs[] = {
    // keywords broken across the buffer
    "[ tr",
    "ue, n",
    "ull",
    ", fals",
    "e, \"some",
    " string\", ",

    // break keyword and string over three inputs
    "tr",
    "u",
    "e, ",
    "\"some ",
    "other ",
    "string\" ]",

    NULL
  };

  struct lexer_output outputs[] = {
    { JSON_OK, '[', "[" },

    { JSON_MORE, JSON_TOK_NONE, "tr" },
    { JSON_OK, JSON_TOK_TRUE, "true" },

    { JSON_OK, ',', "," },

    { JSON_MORE, JSON_TOK_NONE, "n" },
    { JSON_OK, JSON_TOK_NULL, "null" },

    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_OK, ',', "," },

    { JSON_MORE, JSON_TOK_NONE, "fals" },
    { JSON_OK, JSON_TOK_FALSE, "false" },

    { JSON_OK, ',', "," },

    { JSON_MORE, JSON_TOK_STRING, "some" },
    { JSON_OK, JSON_TOK_STRING, " string" },

    { JSON_OK, ',', "," },

    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_NONE, "tr" },
    { JSON_MORE, JSON_TOK_NONE, "tru" },
    { JSON_OK, JSON_TOK_TRUE, "true" },

    { JSON_OK, ',', "," },

    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "some " },
    { JSON_MORE, JSON_TOK_STRING, "other " },
    { JSON_OK, JSON_TOK_STRING, "string" },

    { JSON_OK, ']', "]" },

    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct json_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }

}

void test_string_with_restarts_and_escapes(void)
{
  const char *inputs[] = {
    "\"this set of strings \\",
    "\"break\\\" the escapes\"",

    "\"the breaks \\",
    "nare sometimes for longer \\",
    "\\u sequences: \"",
    
    "\"like \\",
    "u00fc and \\u",
    "00fc and \\u0",
    "0fc and \\u00",
    "fc and \\u00f",
    "c\"",

    "\"and \\",
    "u2318 and \\u",
    "2318 and \\u2",
    "318 and \\u23",
    "18 and \\u231",
    "8\"",

    "\"and \\",
    "u65E5 and \\u",
    "65E5 and \\u6",
    "5E5 and \\u65",
    "E5 and \\u65E",
    "5\"",

    "\"and \\",
    "u8A9E and \\u",
    "8A9E and \\u8",
    "A9E and \\u8A",
    "9E and \\u8A9",
    "E\"",

    NULL
  };

  struct lexer_output outputs[] = {
    { JSON_MORE, JSON_TOK_STRING, "this set of strings " },
    { JSON_OK, JSON_TOK_STRING, "\"break\" the escapes" },

    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "the breaks " },
    { JSON_MORE, JSON_TOK_STRING, "\nare sometimes for longer " },
    { JSON_OK, JSON_TOK_STRING, "\\u sequences: " },

    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "like " },
    { JSON_MORE, JSON_TOK_STRING, "\u00fc and " },
    { JSON_MORE, JSON_TOK_STRING, "\u00fc and " },
    { JSON_MORE, JSON_TOK_STRING, "\u00fc and " },
    { JSON_MORE, JSON_TOK_STRING, "\u00fc and " },
    { JSON_PARTIAL, JSON_TOK_STRING, "\u00fc" },
    { JSON_OK, JSON_TOK_STRING, "" },

    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "and " },
    { JSON_MORE, JSON_TOK_STRING, "\u2318 and " },
    { JSON_MORE, JSON_TOK_STRING, "\u2318 and " },
    { JSON_MORE, JSON_TOK_STRING, "\u2318 and " },
    { JSON_PARTIAL, JSON_TOK_STRING, "\u2318" },
    { JSON_MORE, JSON_TOK_STRING, " and " },
    { JSON_PARTIAL, JSON_TOK_STRING, "\u2318" },
    { JSON_OK, JSON_TOK_STRING, "" },

    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "and " },
    { JSON_MORE, JSON_TOK_STRING, "\u65E5 and " },
    { JSON_MORE, JSON_TOK_STRING, "\u65E5 and " },
    { JSON_MORE, JSON_TOK_STRING, "\u65E5 and " },
    { JSON_PARTIAL, JSON_TOK_STRING, "\u65E5" },
    { JSON_MORE, JSON_TOK_STRING, " and " },
    { JSON_PARTIAL, JSON_TOK_STRING, "\u65E5" },
    { JSON_OK, JSON_TOK_STRING, "" },

    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "and " },
    { JSON_MORE, JSON_TOK_STRING, "\u8A9E and " },
    { JSON_MORE, JSON_TOK_STRING, "\u8A9E and " },
    { JSON_MORE, JSON_TOK_STRING, "\u8A9E and " },
    { JSON_PARTIAL, JSON_TOK_STRING, "\u8A9E" },
    { JSON_MORE, JSON_TOK_STRING, " and " },
    { JSON_PARTIAL, JSON_TOK_STRING, "\u8A9E" },
    { JSON_OK, JSON_TOK_STRING, "" },

    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct json_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: test_simple\n");
  }
}

void test_string_with_surrogate_pairs(void)
{
  const char *inputs[] = {
    "\"this string has a surrogate pair: \\uD840\\uDE13\"",

    "\"this string splits the surrogate pair: \\uD840",
    "\\uDE13\"",

    "\"another split \\uD8",
    "40\\u",
    "DE13\"",

    NULL
  };

  struct lexer_output outputs[] = {
    { JSON_OK, JSON_TOK_STRING, "this string has a surrogate pair: \xf0\xa0\x88\x93" },
    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "this string splits the surrogate pair: " },
    { JSON_OK, JSON_TOK_STRING, "\xf0\xa0\x88\x93" },
    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "another split " },
    { JSON_MORE, JSON_TOK_STRING, "" },
    { JSON_OK, JSON_TOK_STRING, "\xf0\xa0\x88\x93" },
    { JSON_MORE, JSON_TOK_NONE, "" },

    /*
    { JSON_MORE, JSON_TOK_STRING, "this string splits the surrogate pair: ",
    { JSON_MORE, JSON_TOK_NONE, "" },
    */

    { JSON_OK, JSON_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct json_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

void test_numbers(void)
{
  const char *inputs[] = {
    "3 57 0.451 10.2343 -3.4 -0.3 5.4e-23 0.93e+7 7e2 ",

    NULL
  };

  struct lexer_output outputs[] = {
    { JSON_OK, JSON_TOK_NUMBER  , "3" },
    { JSON_OK, JSON_TOK_NUMBER  , "57" },
    { JSON_OK, JSON_TOK_NUMBER  , "0.451" },
    { JSON_OK, JSON_TOK_NUMBER  , "10.2343" },
    { JSON_OK, JSON_TOK_NUMBER  , "-3.4" },
    { JSON_OK, JSON_TOK_NUMBER  , "-0.3" },

    { JSON_OK, JSON_TOK_NUMBER  , "5.4e-23" },
    { JSON_OK, JSON_TOK_NUMBER  , "0.93e+7" },
    { JSON_OK, JSON_TOK_NUMBER  , "7e2" },

    { JSON_MORE, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct json_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

void test_number_restarts(void)
{
  const char *inputs[] = {
    "3",
    "456 5",
    ".37 54.19e",
    "+5 54.19",
    "e-16 54.1",
    "9E07 54.19e",
    "+3  54.19e",
    "17 ",

    NULL
  };

  struct lexer_output outputs[] = {
    { JSON_MORE, JSON_TOK_NUMBER, "3" },
    { JSON_OK, JSON_TOK_NUMBER  , "456" },

    { JSON_MORE, JSON_TOK_NUMBER, "5" },
    { JSON_OK, JSON_TOK_NUMBER  , ".37" },

    { JSON_MORE, JSON_TOK_NUMBER, "54.19e" },
    { JSON_OK, JSON_TOK_NUMBER  , "+5" },

    { JSON_MORE, JSON_TOK_NUMBER, "54.19" },
    { JSON_OK, JSON_TOK_NUMBER  , "e-16" },

    { JSON_MORE, JSON_TOK_NUMBER, "54.1" },
    { JSON_OK, JSON_TOK_NUMBER  , "9E07" },

    { JSON_MORE, JSON_TOK_NUMBER, "54.19e" },
    { JSON_OK, JSON_TOK_NUMBER  , "+3" },

    { JSON_MORE, JSON_TOK_NUMBER, "54.19e" },
    { JSON_OK, JSON_TOK_NUMBER  , "17" },

    { JSON_OK, JSON_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct json_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

void test_invalid_keywords(void)
{
  const char *inputs[] = {
    "trup",
    testing_close_marker,

    "raisin",
    testing_close_marker,

    "fals",
    testing_close_marker,

    NULL
  };

  struct lexer_output outputs[] = {
    { JSON_INVALID_INPUT, JSON_TOK_NONE, "trup" },
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_INPUT, JSON_TOK_NONE, "raisin" },
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_NONE, "fals" },
    { JSON_UNFINISHED_INPUT, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct json_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

void test_invalid_numbers(void)
{
  const char *inputs[] = {
    "--1",
    testing_close_marker,

    "-Q",
    testing_close_marker,

    "0123",
    testing_close_marker,

    "+123",
    testing_close_marker,

    "1.A",
    testing_close_marker,

    "12.e+3",
    testing_close_marker,

    "12ea",
    testing_close_marker,

    "12e1.9",
    testing_close_marker,

    NULL
  };

  struct lexer_output outputs[] = {
    { JSON_INVALID_INPUT, JSON_TOK_NUMBER, "--" },
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_INPUT, JSON_TOK_NUMBER, "-Q" },
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_INPUT, JSON_TOK_NUMBER, "01" },
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_INPUT, JSON_TOK_NONE, "+123" },
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_INPUT, JSON_TOK_NUMBER, "1.A" },
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_INPUT, JSON_TOK_NUMBER, "12.e" },
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_INPUT, JSON_TOK_NUMBER, "12ea" },
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_NUMBER, "12e1" },
    { JSON_INVALID_INPUT, JSON_TOK_NONE, ".9" },
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct json_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

void test_unterminated_strings(void)
{
  const char *inputs[] = {
    "\"this is an unterminated string",
    testing_close_marker,

    "\"this is an unterminated string with a partial escape\\",
    testing_close_marker,

    "\"this is an unterminated string with a partial escape\\u",
    testing_close_marker,

    "\"this is an unterminated string with a partial escape\\u1",
    testing_close_marker,

    "\"this is an unterminated string with a partial escape\\u12",
    testing_close_marker,

    "\"this is an unterminated string with a partial escape\\u123",
    testing_close_marker,

    "\"this is an unterminated string with only half a surrogate pair \\uD840",
    testing_close_marker,

    NULL
  };

  struct lexer_output outputs[] = {
    { JSON_MORE, JSON_TOK_STRING, "this is an unterminated string" },
    { JSON_UNFINISHED_INPUT, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "this is an unterminated string with a partial escape" },
    { JSON_UNFINISHED_INPUT, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "this is an unterminated string with a partial escape" },
    { JSON_UNFINISHED_INPUT, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "this is an unterminated string with a partial escape" },
    { JSON_UNFINISHED_INPUT, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "this is an unterminated string with a partial escape" },
    { JSON_UNFINISHED_INPUT, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "this is an unterminated string with a partial escape" },
    { JSON_UNFINISHED_INPUT, JSON_TOK_NONE, "" },

    { JSON_MORE, JSON_TOK_STRING, "this is an unterminated string with only half a surrogate pair " },
    { JSON_UNFINISHED_INPUT, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct json_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

void test_invalid_strings(void)
{
  const char *inputs[] = {
    "\"only partial escape \\ \"",
    testing_close_marker,

    "\"only partial escape\\u\"",
    testing_close_marker,

    "\"only partial escape\\u1\"",
    testing_close_marker,

    "\"only partial escape\\u12\"",
    testing_close_marker,

    "\"only partial escape\\u123\"",
    testing_close_marker,

    "\"invalid escape \\Q\"",
    testing_close_marker,

    "\"only half a surrogate pair \\uD840\"",
    testing_close_marker,

    NULL
  };

  struct lexer_output outputs[] = {
    { JSON_INVALID_ESCAPE, JSON_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_ESCAPE, JSON_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_ESCAPE, JSON_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_ESCAPE, JSON_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_ESCAPE, JSON_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_ESCAPE, JSON_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_INVALID_U16PAIR, JSON_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { JSON_OK, JSON_TOK_NONE, "" },

    { JSON_OK, JSON_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct json_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

int main(void)
{
  test_simple_array();
  test_simple_object();

  test_string_with_escapes();
  test_simple_restarts();
  test_string_with_restarts_and_escapes();

  test_string_with_surrogate_pairs();

  test_numbers();
  test_number_restarts();

  test_invalid_keywords();
  test_invalid_numbers();
  test_unterminated_strings();

  test_invalid_strings();

  printf("%d tests, %d failures\n", ntest,nfail);
  return nfail == 0 ? 0 : 1;
}


