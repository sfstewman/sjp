#include "json_parser.h"

#define TEST_LOG_LEVEL 0
#include "json_testing.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEFAULT_STACK 16
#define DEFAULT_BUF   JSON_PARSER_MIN_BUFFER 

static int parser_is_sentinel(struct parser_output *out)
{
  return out->ret == JSON_OK && out->type == JSON_NONE && out->text == NULL;
}

static int parser_test_inputs(struct json_parser *p, const char *inputs[], struct parser_output *outputs)
{
  int i, j, more, close;
  char inbuf[2048];

  i=0;
  j=0;
  more=1;
  close=0;

  while(1) {
    enum JSON_RESULT ret;
    struct json_event evt = {0};
    char buf[1024];
    size_t n, outlen;

    LOG("[[ i=%d, j=%d | %d %s ]]\n",
        i,j, p->stack[p->top-1], pst2name(p->stack[p->top-1]));
#if TEST_LOG_LEVEL > 0
    printf("STACK:");
    for (int k=0; k < p->top; k++) {
      printf(" %s", pst2name(p->stack[k]));
    }
    printf("\n");
#endif /* TEST_LOG_LEVEL > 0 */

    if (parser_is_sentinel(&outputs[j])) {
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
            j, outputs[i].ret, outputs[i].type, outputs[i].text);
        return -1;
      }

      if (close) {
        LOG("[RESET]%s\n","");
        json_parser_reset(p);
        close=0;
      }

      if (inputs[i] != testing_close_marker) {
        snprintf(inbuf, sizeof inbuf, "%s", inputs[i]);
        LOG("[MORE] %s\n", inbuf);
        json_parser_more(p, inbuf, strlen(inbuf));
      } else {
        close=1;
        LOG("[CLOSE] %s\n", "");
      }
      i++;
    }

    ret = close ? json_parser_close(p) : json_parser_next(p, &evt);

    n = evt.n < sizeof buf ? evt.n : sizeof buf-1;
    memset(buf, 0, sizeof buf);
    if (n > 0) {
      LOG("[VAL ] %zu chars in text\n", n);
      memcpy(buf, evt.text, n);
    }

    LOG("[TOK ] %3d %3d %8s %8s | %s\n",
        ret, evt.type,
        ret2name(ret), evt2name(evt.type),
        buf);

    if (ret != outputs[j].ret) {
      printf("i=%d, j=%d, expected return %d (%s), but found %d (%s)\n",
          i,j,
          outputs[j].ret, ret2name(outputs[j].ret),
          ret, ret2name(ret));
      return -1;
    }

    if (evt.type != outputs[j].type) {
      printf("i=%d, j=%d, expected type %d (%s), but found %d (%s)\n",
          i,j,
          outputs[j].type, evt2name(outputs[j].type),
          evt.type, evt2name(evt.type));
      return -1;
    }

    outlen = (outputs[j].text != NULL) ? strlen(outputs[j].text) : 0;

    if (evt.n != outlen) {
      printf("i=%d, j=%d, expected text '%s' but found '%s'\n",
          i,j, outputs[j].text ? outputs[j].text : "<NULL>", buf);
      return -1;
    }

    more = (ret == JSON_MORE) || close;
    j++;
  }

  if (!close) {
    int ret;
    if (ret = json_parser_close(p), JSON_ERROR(ret)) {
      return -1;
    }
  }

  return 0;
}

void run_parser_test(const char *name, size_t nstack, size_t nbuf, const char *inputs[], struct parser_output outputs[])
{
  int ret;
  struct json_parser p = { 0 };
  char *stack, *buf;

  ntest++;

  stack = malloc(nstack);
  buf = malloc(nbuf);

  if (stack == NULL || buf == NULL) {
    printf("could not allocate stack  of %zu bytes or buffer of %zu bytes\n", nstack,nbuf);
    goto failed;
  }

  if (ret = json_parser_init(&p, stack, nstack, buf, nbuf), ret != JSON_OK) {
    printf("error initializing the parser (ret=%d %s)\n", ret, ret2name(ret));
    goto failed;
  }

  if (ret = parser_test_inputs(&p, inputs, outputs), ret != 0) {
    goto failed;
  }

  goto cleanup;

failed:
  nfail++;
  printf("FAILED: %s\n", __func__);

  if (p.stack != NULL || p.buf != NULL) {
    json_parser_close(&p);
  }

cleanup:
  free(stack);
  free(buf);
}

void test_simple_objects(void)
{
  const char *inputs[] = {
    "{}",
    "{ \"foo\" : \"bar\" }",
    "{ \"foo\" : \"bar\", \"baz\" : 2378 }",
    NULL
  };

  struct parser_output outputs[] = {
    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_OBJECT_END, "}" },

    { JSON_MORE, JSON_NONE, "" },

    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_OK, JSON_STRING, "bar" },
    { JSON_OK, JSON_OBJECT_END, "}" },

    { JSON_MORE, JSON_NONE, "" },

    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_OK, JSON_STRING, "bar" },
    { JSON_OK, JSON_STRING, "baz" },
    { JSON_OK, JSON_NUMBER, "2378" },
    { JSON_OK, JSON_OBJECT_END, "}" },

    { JSON_OK, JSON_NONE, NULL }, // end sentinel
  };

  run_parser_test(__func__, DEFAULT_STACK, DEFAULT_BUF, inputs, outputs);
}

void test_simple_arrays(void)
{
  const char *inputs[] = {
    "[]",
    "[ \"foo\" ]",
    "[ \"foo\",\"bar\", 2378 ]",
    NULL
  };

  struct parser_output outputs[] = {
    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_ARRAY_END, "]" },

    { JSON_MORE, JSON_NONE, "" },

    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_OK, JSON_ARRAY_END, "]" },

    { JSON_MORE, JSON_NONE, "" },

    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_OK, JSON_STRING, "bar" },
    { JSON_OK, JSON_NUMBER, "2378" },
    { JSON_OK, JSON_ARRAY_END, "]" },

    { JSON_OK, JSON_NONE, NULL }, // end sentinel
  };

  run_parser_test(__func__, DEFAULT_STACK, DEFAULT_BUF, inputs, outputs);
}

void test_nested_arrays_and_objects(void)
{
  const char *inputs[] = {
    "[ {}, [], [ {} ] ]",
    "[ { \"foo\" : \"bar\", \"baz\" : [ 123, 456 ]} ]",
    "{ \"foo\" : { \"bar\" : [ 123, { \"baz\" : true } ], \"quux\": -23.56e+5 } }",
    NULL
  };

  struct parser_output outputs[] = {
    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_OBJECT_END, "}" },

    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_ARRAY_END, "]" },

    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_OBJECT_END, "}" },
    { JSON_OK, JSON_ARRAY_END, "]" },

    { JSON_OK, JSON_ARRAY_END, "]" },

    { JSON_MORE, JSON_NONE, "" },

    { JSON_OK, JSON_ARRAY_BEG, "[" },

    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_OK, JSON_STRING, "bar" },

    { JSON_OK, JSON_STRING, "baz" },
    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_NUMBER, "123" },
    { JSON_OK, JSON_NUMBER, "456" },
    { JSON_OK, JSON_ARRAY_END, "]" },

    { JSON_OK, JSON_OBJECT_END, "}" },

    { JSON_OK, JSON_ARRAY_END, "]" },

    { JSON_MORE, JSON_NONE, "" },

    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "bar" },
    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_NUMBER, "123" },
    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "baz" },
    { JSON_OK, JSON_TRUE, "true" },
    { JSON_OK, JSON_OBJECT_END, "}" },
    { JSON_OK, JSON_ARRAY_END, "]" },

    { JSON_OK, JSON_STRING, "quux" },
    { JSON_OK, JSON_NUMBER, "-23.56e+5" },
    { JSON_OK, JSON_OBJECT_END, "}" },

    { JSON_OK, JSON_OBJECT_END, "}" },

    { JSON_MORE, JSON_NONE, "" },

    { JSON_OK, JSON_NONE, NULL }, // end sentinel
  };

  run_parser_test(__func__, DEFAULT_STACK, DEFAULT_BUF, inputs, outputs);
}

void test_restarts_1(void)
{
  const char *inputs[] = {
    "[",
    "{} ]",

    "{ \"some key that",
    "we break\" : \"some other key that we",
    "break\", \"short key\" : 12345",
    ".6789 }",

    // "{ \"foo\" : { \"bar\" : [ 123, { \"baz\" : true } ], \"quux\": -23.56e+5 } }",
    NULL
  };

  struct parser_output outputs[] = {
    { JSON_OK, JSON_ARRAY_BEG, "[" },

    { JSON_MORE, JSON_NONE, "" },

    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_OBJECT_END, "}" },
    { JSON_OK, JSON_ARRAY_END, "]" },

    { JSON_MORE, JSON_NONE, "" },


    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_MORE, JSON_STRING, "some key that" },
    { JSON_OK, JSON_STRING, "we break" },

    { JSON_MORE, JSON_STRING, "some other key that we" },
    { JSON_OK, JSON_STRING, "break" },

    { JSON_OK, JSON_STRING, "short key" },
    { JSON_MORE, JSON_NUMBER, "12345" },
    { JSON_OK, JSON_NUMBER, ".6789" },

    { JSON_OK, JSON_OBJECT_END, "}" },


    { JSON_OK, JSON_NONE, NULL }, // end sentinel
  };

  run_parser_test(__func__, DEFAULT_STACK, DEFAULT_BUF, inputs, outputs);
}

void test_detect_unclosed_things(void)
{
  const char *inputs[] = {
    // make sure we return the lexer errors
    "\"foo",
    testing_close_marker,

    "nul",
    testing_close_marker,

    "123.5e",
    testing_close_marker,

    // test arrays
    "[",
    testing_close_marker,

    "[ \"foo\"",
    testing_close_marker,

    "[ \"foo\", ",
    testing_close_marker,

    // test objects
    "{",
    testing_close_marker,

    "{ \"foo\"",
    testing_close_marker,

    "{ \"foo\" : ",
    testing_close_marker,

    "{ \"foo\" : \"bar\"",
    testing_close_marker,

    "{ \"foo\" : \"bar\", ",
    testing_close_marker,

    // test nested structures
    "{ \"foo\" : [ 123,",
    testing_close_marker,

    "{ \"foo\" : { \"bar\" : \"baz\"",
    testing_close_marker,

    "[ \"foo\", [ 123, \"bar\"",
    testing_close_marker,

    "[ \"foo\", { \"bar\" : \"baz\"",
    testing_close_marker,

    NULL
  };

  struct parser_output outputs[] = {
    // test lexer errors
    { JSON_MORE, JSON_STRING, "foo" },
    { JSON_UNFINISHED_INPUT, JSON_NONE, NULL },

    { JSON_MORE, JSON_NONE, "" },
    { JSON_UNFINISHED_INPUT, JSON_NONE, NULL },

    { JSON_MORE, JSON_NUMBER, "123.5e" },
    { JSON_UNFINISHED_INPUT, JSON_NONE, NULL },

    // test arrays
    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_MORE, JSON_NONE, "" },
    { JSON_UNCLOSED_ARRAY, JSON_NONE, NULL },

    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_MORE, JSON_NONE, "" },
    { JSON_UNCLOSED_ARRAY, JSON_NONE, NULL },

    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_MORE, JSON_NONE, "," },
    { JSON_UNCLOSED_ARRAY, JSON_NONE, NULL },

    // test objects
    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_MORE, JSON_NONE, "" },
    { JSON_UNCLOSED_OBJECT, JSON_NONE, NULL },

    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_MORE, JSON_NONE, "" },
    { JSON_UNCLOSED_OBJECT, JSON_NONE, NULL },

    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_MORE, JSON_NONE, ":" },
    { JSON_UNCLOSED_OBJECT, JSON_NONE, NULL },

    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_OK, JSON_STRING, "bar" },
    { JSON_MORE, JSON_NONE, "" },
    { JSON_UNCLOSED_OBJECT, JSON_NONE, NULL },

    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_OK, JSON_STRING, "bar" },
    { JSON_MORE, JSON_NONE, "," },
    { JSON_UNCLOSED_OBJECT, JSON_NONE, NULL },

    // test nested things
    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_OK, JSON_ARRAY_BEG, "{" },
    { JSON_OK, JSON_NUMBER, "123" },
    { JSON_MORE, JSON_NONE, "," },
    { JSON_UNCLOSED_ARRAY, JSON_NONE, NULL },

    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "bar" },
    { JSON_OK, JSON_STRING, "baz" },
    { JSON_MORE, JSON_NONE, "" },
    { JSON_UNCLOSED_OBJECT, JSON_NONE, NULL },

    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_NUMBER, "123" },
    { JSON_OK, JSON_STRING, "baz" },
    { JSON_MORE, JSON_NONE, "" },
    { JSON_UNCLOSED_ARRAY, JSON_NONE, NULL },

    { JSON_OK, JSON_ARRAY_BEG, "[" },
    { JSON_OK, JSON_STRING, "foo" },
    { JSON_OK, JSON_OBJECT_BEG, "{" },
    { JSON_OK, JSON_STRING, "bar" },
    { JSON_OK, JSON_STRING, "baz" },
    { JSON_MORE, JSON_NONE, "" },
    { JSON_UNCLOSED_OBJECT, JSON_NONE, NULL },

    { JSON_OK, JSON_NONE, NULL }, // end sentinel
  };

  run_parser_test(__func__, DEFAULT_STACK, DEFAULT_BUF, inputs, outputs);
}

int main(void)
{
  test_simple_objects();
  test_simple_arrays();
  test_nested_arrays_and_objects();
  test_restarts_1();

  test_detect_unclosed_things();

  printf("%d tests, %d failures\n", ntest,nfail);
  return nfail == 0 ? 0 : 1;
}



