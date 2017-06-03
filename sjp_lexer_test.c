#include "sjp_lexer.h"

#define TEST_LOG_LEVEL 0
#include "sjp_testing.h"

#include <stdio.h>
#include <string.h>

int lexer_test_inputs(struct sjp_lexer *lex, const char *inputs[], struct lexer_output *outputs)
{
  int i, j, more, close, eos;
  char inbuf[2048];

  sjp_lexer_init(lex);

  i=0;
  j=0;
  more=1;
  eos=0;
  close=0;

  for(;;) {
    enum SJP_RESULT ret;
    struct sjp_token tok = {0};
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
        sjp_lexer_init(lex);
        close=0;
      }

      eos=0;

      if (inputs[i] == testing_close_marker) {
        close=1;
        LOG("[CLOSE] %s\n", "");
      } else if (inputs[i] == testing_end_of_stream) {
        eos=1;
        LOG("[EOS] %s\n", inbuf);
        sjp_lexer_eos(lex);
      } else {
        snprintf(inbuf, sizeof inbuf, "%s", inputs[i]);
        LOG("[MORE] %s\n", inbuf);
        sjp_lexer_more(lex, inbuf, strlen(inbuf));
      }
      i++;
    }

    ret = close ? sjp_lexer_close(lex) : sjp_lexer_token(lex, &tok);

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

    if (outputs[j].value != NULL) {
      if (tok.n != strlen(outputs[j].value) || memcmp(tok.value,outputs[j].value,tok.n) != 0) {
        printf("i=%d, j=%d, expected value '%s' but found '%s'\n",
            i,j, outputs[j].value, buf);
        return -1;
      }
    } else {
      if (tok.n != 0) {
        printf("i=%d, j=%d, expected value <null> but found '%s'\n", i,j, buf);
        return -1;
      }
    }

    if (outputs[j].checknum) {
      if (tok.dbl != outputs[j].num) {
        printf("i=%d, j=%d, expected number %f but found %f\n", i,j, outputs[j].num, tok.dbl);
        return -1;
      }
    }

    more = (ret == SJP_MORE) || SJP_ERROR(ret) || close || eos;
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
    { SJP_OK, '['        , "["     },
    { SJP_OK, SJP_TOK_TRUE  , "true"  },
    { SJP_OK, ','        , ","     },
    { SJP_OK, SJP_TOK_FALSE , "false" },
    { SJP_OK, ','        , ","     },
    { SJP_OK, SJP_TOK_NULL  , "null"  },
    { SJP_OK, ','        , ","     },
    { SJP_OK, SJP_TOK_STRING, "foo"   },
    { SJP_OK, ']'        , "]"     },
    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

void test_simple_object(void)
{
  const char *inputs[] = {
    "{ \"foo\" : true, \"bar\" : \"baz\", \"quux\" : null }",
    testing_end_of_stream,
    NULL
  };

  struct lexer_output outputs[] = {
    { SJP_OK, '{'        , "{"     },

    { SJP_OK, SJP_TOK_STRING, "foo"   },
    { SJP_OK, ':'        , ":"     },
    { SJP_OK, SJP_TOK_TRUE  , "true"  },
    { SJP_OK, ','        , ","     },

    { SJP_OK, SJP_TOK_STRING, "bar"   },
    { SJP_OK, ':'        , ":"     },
    { SJP_OK, SJP_TOK_STRING, "baz"   },
    { SJP_OK, ','        , ","     },

    { SJP_OK, SJP_TOK_STRING, "quux"  },
    { SJP_OK, ':'        , ":"     },
    { SJP_OK, SJP_TOK_NULL  , "null"  },
    { SJP_OK, '}'        , "}"     },

    { SJP_MORE, SJP_TOK_NONE, "" },
    { SJP_OK, SJP_TOK_EOS, NULL },

    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

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
    { SJP_OK, SJP_TOK_STRING, "this string \"has\" double quote escapes" },
    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_STRING, "this string has \\ some escape c\bsequences / combinations" },
    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_STRING, "this string tests form-feed\fand carriage-return \r\nand newline" },
    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_STRING, "this string tests \ttab" },
    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_STRING, "this string has unicode escapes: G\u00fcnter \u2318 \u65E5 \u672C \u8A9E" },
    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

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
    { SJP_OK, '[', "[" },

    { SJP_MORE, SJP_TOK_NONE, "tr" },
    { SJP_OK, SJP_TOK_TRUE, "true" },

    { SJP_OK, ',', "," },

    { SJP_MORE, SJP_TOK_NONE, "n" },
    { SJP_OK, SJP_TOK_NULL, "null" },

    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_OK, ',', "," },

    { SJP_MORE, SJP_TOK_NONE, "fals" },
    { SJP_OK, SJP_TOK_FALSE, "false" },

    { SJP_OK, ',', "," },

    { SJP_MORE, SJP_TOK_STRING, "some" },
    { SJP_OK, SJP_TOK_STRING, " string" },

    { SJP_OK, ',', "," },

    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_NONE, "tr" },
    { SJP_MORE, SJP_TOK_NONE, "tru" },
    { SJP_OK, SJP_TOK_TRUE, "true" },

    { SJP_OK, ',', "," },

    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "some " },
    { SJP_MORE, SJP_TOK_STRING, "other " },
    { SJP_OK, SJP_TOK_STRING, "string" },

    { SJP_OK, ']', "]" },

    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

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
    { SJP_MORE, SJP_TOK_STRING, "this set of strings " },
    { SJP_OK, SJP_TOK_STRING, "\"break\" the escapes" },

    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "the breaks " },
    { SJP_MORE, SJP_TOK_STRING, "\nare sometimes for longer " },
    { SJP_OK, SJP_TOK_STRING, "\\u sequences: " },

    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "like " },
    { SJP_MORE, SJP_TOK_STRING, "\u00fc and " },
    { SJP_MORE, SJP_TOK_STRING, "\u00fc and " },
    { SJP_MORE, SJP_TOK_STRING, "\u00fc and " },
    { SJP_MORE, SJP_TOK_STRING, "\u00fc and " },
    { SJP_PARTIAL, SJP_TOK_STRING, "\u00fc" },
    { SJP_OK, SJP_TOK_STRING, "" },

    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "and " },
    { SJP_MORE, SJP_TOK_STRING, "\u2318 and " },
    { SJP_MORE, SJP_TOK_STRING, "\u2318 and " },
    { SJP_MORE, SJP_TOK_STRING, "\u2318 and " },
    { SJP_PARTIAL, SJP_TOK_STRING, "\u2318" },
    { SJP_MORE, SJP_TOK_STRING, " and " },
    { SJP_PARTIAL, SJP_TOK_STRING, "\u2318" },
    { SJP_OK, SJP_TOK_STRING, "" },

    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "and " },
    { SJP_MORE, SJP_TOK_STRING, "\u65E5 and " },
    { SJP_MORE, SJP_TOK_STRING, "\u65E5 and " },
    { SJP_MORE, SJP_TOK_STRING, "\u65E5 and " },
    { SJP_PARTIAL, SJP_TOK_STRING, "\u65E5" },
    { SJP_MORE, SJP_TOK_STRING, " and " },
    { SJP_PARTIAL, SJP_TOK_STRING, "\u65E5" },
    { SJP_OK, SJP_TOK_STRING, "" },

    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "and " },
    { SJP_MORE, SJP_TOK_STRING, "\u8A9E and " },
    { SJP_MORE, SJP_TOK_STRING, "\u8A9E and " },
    { SJP_MORE, SJP_TOK_STRING, "\u8A9E and " },
    { SJP_PARTIAL, SJP_TOK_STRING, "\u8A9E" },
    { SJP_MORE, SJP_TOK_STRING, " and " },
    { SJP_PARTIAL, SJP_TOK_STRING, "\u8A9E" },
    { SJP_OK, SJP_TOK_STRING, "" },

    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

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
    { SJP_OK, SJP_TOK_STRING, "this string has a surrogate pair: \xf0\xa0\x88\x93" },
    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "this string splits the surrogate pair: " },
    { SJP_OK, SJP_TOK_STRING, "\xf0\xa0\x88\x93" },
    { SJP_MORE, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "another split " },
    { SJP_MORE, SJP_TOK_STRING, "" },
    { SJP_OK, SJP_TOK_STRING, "\xf0\xa0\x88\x93" },
    { SJP_MORE, SJP_TOK_NONE, "" },

    /*
    { SJP_MORE, SJP_TOK_STRING, "this string splits the surrogate pair: ",
    { SJP_MORE, SJP_TOK_NONE, "" },
    */

    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

void test_numbers(void)
{
  const char *inputs[] = {
    "3 57 0.451 10.2343 -3.4 -0.3 5.4e-23 0.93e+7 7e2 ",
    testing_close_marker,

    "3",
    testing_end_of_stream,
    testing_close_marker,

    "3.5",
    testing_end_of_stream,
    testing_close_marker,

    "-3.5",
    testing_end_of_stream,
    testing_close_marker,

    "3.592e+3",
    testing_end_of_stream,
    testing_close_marker,

    NULL
  };

  struct lexer_output outputs[] = {
    { SJP_OK, SJP_TOK_NUMBER  , "3",       1, 3 },
    { SJP_OK, SJP_TOK_NUMBER  , "57",      1, 57 },
    { SJP_OK, SJP_TOK_NUMBER  , "0.451",   1, 0.451 },
    { SJP_OK, SJP_TOK_NUMBER  , "10.2343", 1, 10.2343 },
    { SJP_OK, SJP_TOK_NUMBER  , "-3.4",    1, -3.4 },
    { SJP_OK, SJP_TOK_NUMBER  , "-0.3",    1, -0.3 },

    { SJP_OK, SJP_TOK_NUMBER  , "5.4e-23", 1, 5.4e-23 },
    { SJP_OK, SJP_TOK_NUMBER  , "0.93e+7", 1, 0.93e7 },
    { SJP_OK, SJP_TOK_NUMBER  , "7e2",     1, 7e2 },

    { SJP_MORE, SJP_TOK_NONE, "" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_NUMBER  , "3" },
    // { SJP_MORE, SJP_TOK_NUMBER, "" },
    { SJP_OK, SJP_TOK_NUMBER, "", 1, 3 },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_NUMBER  , "3.5" },
    { SJP_OK, SJP_TOK_NUMBER, "", 1, 3.5 },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_NUMBER  , "-3.5" },
    { SJP_OK, SJP_TOK_NUMBER, NULL, 1, -3.5 },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_NUMBER  , "3.592e+3" },
    { SJP_OK, SJP_TOK_NUMBER, NULL, 1, 3.592e3 },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

  if (ret = lexer_test_inputs(&lex, inputs, outputs), ret != 0) {
    nfail++;
    printf("FAILED: %s\n", __func__);
  }
}

void test_numbers_with_close(void)
{
  const char *inputs[] = {
    "1",
    testing_close_marker,

    "1.1",
    testing_close_marker,

    NULL
  };

  struct lexer_output outputs[] = {
    { SJP_MORE, SJP_TOK_NUMBER  , "1" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_NUMBER  , "1.1" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

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
    { SJP_MORE, SJP_TOK_NUMBER, "3" },
    { SJP_OK, SJP_TOK_NUMBER  , "456", 1, 3456 },

    { SJP_MORE, SJP_TOK_NUMBER, "5" },
    { SJP_OK, SJP_TOK_NUMBER  , ".37", 1, 5.37 },

    { SJP_MORE, SJP_TOK_NUMBER, "54.19e" },
    { SJP_OK, SJP_TOK_NUMBER  , "+5", 1, 54.19e5 },

    { SJP_MORE, SJP_TOK_NUMBER, "54.19" },
    { SJP_OK, SJP_TOK_NUMBER  , "e-16", 1, 54.19e-16 },

    { SJP_MORE, SJP_TOK_NUMBER, "54.1" },
    { SJP_OK, SJP_TOK_NUMBER  , "9E07", 1, 54.19e7 },

    { SJP_MORE, SJP_TOK_NUMBER, "54.19e" },
    { SJP_OK, SJP_TOK_NUMBER  , "+3", 1, 54.19e3 },

    { SJP_MORE, SJP_TOK_NUMBER, "54.19e" },
    { SJP_OK, SJP_TOK_NUMBER  , "17", 1, 54.19e17 },

    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

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


    // test for partial keywords, truncated by the end of the stream
    "t",
    testing_end_of_stream,
    testing_close_marker,

    "tr",
    testing_end_of_stream,
    testing_close_marker,

    "tru",
    testing_end_of_stream,
    testing_close_marker,

    "true",
    testing_end_of_stream,
    testing_close_marker,

    "f",
    testing_end_of_stream,
    testing_close_marker,

    "fa",
    testing_end_of_stream,
    testing_close_marker,

    "fal",
    testing_end_of_stream,
    testing_close_marker,

    "fals",
    testing_end_of_stream,
    testing_close_marker,

    "false",
    testing_end_of_stream,
    testing_close_marker,

    "n",
    testing_end_of_stream,
    testing_close_marker,

    "nu",
    testing_end_of_stream,
    testing_close_marker,

    "nul",
    testing_end_of_stream,
    testing_close_marker,

    "null",
    testing_end_of_stream,
    testing_close_marker,

    NULL
  };

  struct lexer_output outputs[] = {
    { SJP_INVALID_INPUT, SJP_TOK_NONE, "trup" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_INPUT, SJP_TOK_NONE, "raisin" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_NONE, "fals" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" },


    // test for partial keywords, truncated by the end of the stream
    { SJP_MORE, SJP_TOK_NONE, "t" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_TRUE, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_NONE, "tr" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_TRUE, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_NONE, "tru" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_TRUE, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_OK, SJP_TOK_TRUE, "true" },
    { SJP_MORE, SJP_TOK_NONE, "" },
    { SJP_OK, SJP_TOK_EOS, "" },
    { SJP_OK, SJP_TOK_NONE, "" },


    { SJP_MORE, SJP_TOK_NONE, "f" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_FALSE, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_NONE, "fa" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_FALSE, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_NONE, "fal" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_FALSE, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_NONE, "fals" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_FALSE, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_OK, SJP_TOK_FALSE, "false" },
    { SJP_MORE, SJP_TOK_NONE, "" },
    { SJP_OK, SJP_TOK_EOS, "" },
    { SJP_OK, SJP_TOK_NONE, "" },


    { SJP_MORE, SJP_TOK_NONE, "n" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NULL, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_NONE, "nu" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NULL, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_NONE, "nul" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NULL, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_OK, SJP_TOK_NULL, "null" },
    { SJP_MORE, SJP_TOK_NONE, "" },
    { SJP_OK, SJP_TOK_EOS, "" },
    { SJP_OK, SJP_TOK_NONE, "" },


    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

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
    { SJP_INVALID_INPUT, SJP_TOK_NUMBER, "--" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_INPUT, SJP_TOK_NUMBER, "-Q" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_INPUT, SJP_TOK_NUMBER, "01" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_INPUT, SJP_TOK_NONE, "+123" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_INPUT, SJP_TOK_NUMBER, "1.A" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_INPUT, SJP_TOK_NUMBER, "12.e" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_INPUT, SJP_TOK_NUMBER, "12ea" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_NUMBER, "12e1" },
    { SJP_INVALID_INPUT, SJP_TOK_NONE, ".9" },
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

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

    // same tests but indicate EOS
    "\"this is an unterminated string",
    testing_end_of_stream,
    testing_close_marker,

    "\"this is an unterminated string with a partial escape\\",
    testing_end_of_stream,
    testing_close_marker,

    "\"this is an unterminated string with a partial escape\\u",
    testing_end_of_stream,
    testing_close_marker,

    "\"this is an unterminated string with a partial escape\\u1",
    testing_end_of_stream,
    testing_close_marker,

    "\"this is an unterminated string with a partial escape\\u12",
    testing_end_of_stream,
    testing_close_marker,

    "\"this is an unterminated string with a partial escape\\u123",
    testing_end_of_stream,
    testing_close_marker,

    "\"this is an unterminated string with only half a surrogate pair \\uD840",
    testing_end_of_stream,
    testing_close_marker,

    NULL
  };

  struct lexer_output outputs[] = {
    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string with a partial escape" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string with a partial escape" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string with a partial escape" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string with a partial escape" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string with a partial escape" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" },

    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string with only half a surrogate pair " },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" },

    // same tests but indicate EOS
    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_STRING, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string with a partial escape" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_STRING, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string with a partial escape" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_STRING, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string with a partial escape" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_STRING, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string with a partial escape" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_STRING, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string with a partial escape" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_STRING, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_MORE, SJP_TOK_STRING, "this is an unterminated string with only half a surrogate pair " },
    { SJP_UNFINISHED_INPUT, SJP_TOK_STRING, "" },
    { SJP_UNFINISHED_INPUT, SJP_TOK_NONE, "" }, // for close call

    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

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

    "\"invalid \xFF utf8 bytes\"",
    testing_close_marker,

    "\"invalid\\t \xFF utf8 bytes on the slow path\"",
    testing_close_marker,

    NULL
  };

  struct lexer_output outputs[] = {
    { SJP_INVALID_ESCAPE, SJP_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_ESCAPE, SJP_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_ESCAPE, SJP_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_ESCAPE, SJP_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_ESCAPE, SJP_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_ESCAPE, SJP_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_U16PAIR, SJP_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_CHAR, SJP_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_INVALID_CHAR, SJP_TOK_STRING, "" },  // XXX - should return up to the invalid part of the token
    { SJP_OK, SJP_TOK_NONE, "" },

    { SJP_OK, SJP_TOK_NONE, NULL }, // end sentinel
  };

  ntest++;

  int ret;
  struct sjp_lexer lex = { 0 };

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
  test_numbers_with_close();
  test_number_restarts();

  test_invalid_keywords();
  test_invalid_numbers();
  test_unterminated_strings();

  test_invalid_strings();

  printf("%d tests, %d failures\n", ntest,nfail);
  return nfail == 0 ? 0 : 1;
}


