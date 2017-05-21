#include "json_lexer.h"

#include "json_testing.h"

#include <stdio.h>

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

int main(void)
{
  test_simple_array();
  test_simple_object();

  test_string_with_escapes();
  test_simple_restarts();
  test_string_with_restarts_and_escapes();

  test_numbers();

  printf("%d tests, %d failures\n", ntest,nfail);
  return nfail == 0 ? 0 : 1;
}


