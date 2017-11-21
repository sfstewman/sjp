#include "sjp_parser.h"

#define TEST_LOG_LEVEL 0
#include "sjp_testing.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <assert.h>

#define DEFAULT_STACK 16
#define NO_BUF         0
#define SMALL_BUF    128

static int parser_is_sentinel(struct parser_output *out)
{
  return out->ret == SJP_OK && out->type == SJP_NONE && out->text == NULL;
}

static int parser_test_inputs(struct sjp_parser *p, const char *inputs[], struct parser_output *outputs)
{
  int i, j, more, close, eos;
  char inbuf[2048];

  i=0;
  j=0;
  more=1;
  eos=0;
  close=0;

  while(1) {
    enum SJP_RESULT ret;
    struct sjp_event evt = {0};
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
        sjp_parser_reset(p);
        close=0;
      }

      eos=0;

      if (inputs[i] == testing_close_marker) {
        close=1;
        LOG("[CLOSE] %s\n", "");
      } else if (inputs[i] == testing_end_of_stream) {
        eos=1;
        LOG("[EOS] %s\n", inbuf);
        sjp_parser_eos(p);
      } else {
        snprintf(inbuf, sizeof inbuf, "%s", inputs[i]);
        LOG("[MORE] %s\n", inbuf);
        sjp_parser_more(p, inbuf, strlen(inbuf));
      }
      i++;
    }

    ret = close ? sjp_parser_close(p) : sjp_parser_next(p, &evt);

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

    if ((evt.n != outlen) || (outlen > 0 && evt.n > 0 && memcmp(evt.text,outputs[j].text,outlen) != 0)) {
      printf("i=%d, j=%d, expected text '%s' but found '%s'\n",
          i,j, outputs[j].text ? outputs[j].text : "<NULL>", buf);
      return -1;
    }

    if (outputs[j].flags & SJP_TEST_NUMBER) {
      assert(evt.type == SJP_NUMBER);
      if (evt.extra.d != outputs[j].num) {
        printf("i=%d, j=%d, expected number %f but found %f\n", i,j, outputs[j].num, evt.extra.d);
        return -1;
      }
    }

    more = (ret == SJP_MORE) || close || eos;
    j++;
  }

  if (!close) {
    int ret;
    if (ret = sjp_parser_close(p), SJP_ERROR(ret)) {
      return -1;
    }
  }

  return 0;
}

void run_parser_test(const char *name, size_t nstack, size_t nbuf, const char *inputs[], struct parser_output outputs[])
{
  int ret;
  struct sjp_parser p = { 0 };
  char *stack, *buf;

  ntest++;

  stack = malloc(nstack);
  buf = (nbuf > 0) ? malloc(nbuf) : NULL;

  if (stack == NULL || (nbuf > 0 && buf == NULL)) {
    printf("could not allocate stack  of %zu bytes or buffer of %zu bytes\n", nstack,nbuf);
    goto failed;
  }

  if (ret = sjp_parser_init(&p, stack, nstack, buf, nbuf), ret != SJP_OK) {
    printf("error initializing the parser (ret=%d %s)\n", ret, ret2name(ret));
    goto failed;
  }

  if (ret = parser_test_inputs(&p, inputs, outputs), ret != 0) {
    goto failed;
  }

  goto cleanup;

failed:
  nfail++;
  printf("FAILED: %s\n", name);

  if (p.stack != NULL || p.buf != NULL) {
    sjp_parser_close(&p);
  }

cleanup:
  free(stack);
  free(buf);
}

void test_values(void)
{
  const char *inputs[] = {
    "\"foo bar baz\"",
    testing_close_marker,

    "1",
    testing_end_of_stream,
    testing_close_marker,

    "1.1",
    testing_end_of_stream,
    testing_close_marker,

    "1.35e-2",
    testing_end_of_stream,
    testing_close_marker,

    "true",
    testing_close_marker,

    "false",
    testing_close_marker,

    "null",
    testing_close_marker,

    NULL
  };

  struct parser_output outputs[] = {
    { SJP_OK, SJP_STRING, "foo bar baz" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_OK, SJP_NONE, "" },

    { SJP_MORE, SJP_NUMBER, "1" },
    { SJP_OK, SJP_NUMBER, "", SJP_TEST_NUMBER, 1 },
    { SJP_OK, SJP_NONE, "" },

    { SJP_MORE, SJP_NUMBER, "1.1" },
    { SJP_OK, SJP_NUMBER, "", SJP_TEST_NUMBER, 1.1 },
    { SJP_OK, SJP_NONE, "" },

    { SJP_MORE, SJP_NUMBER, "1.35e-2" },
    { SJP_OK, SJP_NUMBER, "", SJP_TEST_NUMBER, 1.35e-2 },
    { SJP_OK, SJP_NONE, "" },

    { SJP_OK, SJP_TRUE, "true" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_OK, SJP_NONE, "" },

    { SJP_OK, SJP_FALSE, "false" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_OK, SJP_NONE, "" },

    { SJP_OK, SJP_NULL, "null" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_OK, SJP_NONE, "" },

    { SJP_OK, SJP_NONE, NULL }, // end sentinel
  };

  run_parser_test(__func__, DEFAULT_STACK, NO_BUF, inputs, outputs);
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
    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_OBJECT_END, "}" },

    { SJP_MORE, SJP_NONE, "" },

    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_OK, SJP_STRING, "bar" },
    { SJP_OK, SJP_OBJECT_END, "}" },

    { SJP_MORE, SJP_NONE, "" },

    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_OK, SJP_STRING, "bar" },
    { SJP_OK, SJP_STRING, "baz" },
    { SJP_OK, SJP_NUMBER, "2378" },
    { SJP_OK, SJP_OBJECT_END, "}" },

    { SJP_OK, SJP_NONE, NULL }, // end sentinel
  };

  run_parser_test(__func__, DEFAULT_STACK, NO_BUF, inputs, outputs);
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
    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_ARRAY_END, "]" },

    { SJP_MORE, SJP_NONE, "" },

    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_OK, SJP_ARRAY_END, "]" },

    { SJP_MORE, SJP_NONE, "" },

    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_OK, SJP_STRING, "bar" },
    { SJP_OK, SJP_NUMBER, "2378", SJP_TEST_NUMBER, 2378 },
    { SJP_OK, SJP_ARRAY_END, "]" },

    { SJP_OK, SJP_NONE, NULL }, // end sentinel
  };

  run_parser_test(__func__, DEFAULT_STACK, NO_BUF, inputs, outputs);
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
    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_OBJECT_END, "}" },

    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_ARRAY_END, "]" },

    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_OBJECT_END, "}" },
    { SJP_OK, SJP_ARRAY_END, "]" },

    { SJP_OK, SJP_ARRAY_END, "]" },

    { SJP_MORE, SJP_NONE, "" },

    { SJP_OK, SJP_ARRAY_BEG, "[" },

    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_OK, SJP_STRING, "bar" },

    { SJP_OK, SJP_STRING, "baz" },
    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_NUMBER, "123" },
    { SJP_OK, SJP_NUMBER, "456" },
    { SJP_OK, SJP_ARRAY_END, "]" },

    { SJP_OK, SJP_OBJECT_END, "}" },

    { SJP_OK, SJP_ARRAY_END, "]" },

    { SJP_MORE, SJP_NONE, "" },

    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "bar" },
    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_NUMBER, "123" },
    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "baz" },
    { SJP_OK, SJP_TRUE, "true" },
    { SJP_OK, SJP_OBJECT_END, "}" },
    { SJP_OK, SJP_ARRAY_END, "]" },

    { SJP_OK, SJP_STRING, "quux" },
    { SJP_OK, SJP_NUMBER, "-23.56e+5" },
    { SJP_OK, SJP_OBJECT_END, "}" },

    { SJP_OK, SJP_OBJECT_END, "}" },

    { SJP_MORE, SJP_NONE, "" },

    { SJP_OK, SJP_NONE, NULL }, // end sentinel
  };

  run_parser_test(__func__, DEFAULT_STACK, NO_BUF, inputs, outputs);
}

void test_restarts_1(void)
{
  const char *inputs[] = {
    "[",
    "{} ]",

    "{ \"some key that ",
    "we break\" : \"some other string that we",
    " break\", \"short key\" : 12345",
    ".6789 }",

    // "{ \"foo\" : { \"bar\" : [ 123, { \"baz\" : true } ], \"quux\": -23.56e+5 } }",
    NULL
  };

  struct parser_output outputs[] = {
    { SJP_OK, SJP_ARRAY_BEG, "[" },

    { SJP_MORE, SJP_NONE, "" },

    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_OBJECT_END, "}" },
    { SJP_OK, SJP_ARRAY_END, "]" },

    { SJP_MORE, SJP_NONE, "" },


    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_MORE, SJP_STRING, "some key that " },
    { SJP_OK, SJP_STRING, "we break" },

    { SJP_MORE, SJP_STRING, "some other string that we" },
    { SJP_OK, SJP_STRING, " break" },

    { SJP_OK, SJP_STRING, "short key" },
    { SJP_MORE, SJP_NUMBER, "12345" },
    { SJP_OK, SJP_NUMBER, ".6789" },

    { SJP_OK, SJP_OBJECT_END, "}" },


    { SJP_OK, SJP_NONE, NULL }, // end sentinel
  };

  run_parser_test(__func__, DEFAULT_STACK, NO_BUF, inputs, outputs);
}

void test_buffered_1(void)
{
  const char *inputs[] = {
    "{ \"some key that ",
    "we break\" : \"some other string that we",
    " break\", \"short key\" : 12345",
    ".6789 }",

    // "{ \"foo\" : { \"bar\" : [ 123, { \"baz\" : true } ], \"quux\": -23.56e+5 } }",
    NULL
  };

  struct parser_output outputs[] = {
    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_MORE, SJP_NONE, "" },

    { SJP_OK, SJP_STRING, "some key that we break" },

    { SJP_MORE, SJP_NONE, "" },

    { SJP_OK, SJP_STRING, "some other string that we break" },

    { SJP_OK, SJP_STRING, "short key" },
    { SJP_MORE, SJP_NONE, "" },

    { SJP_OK, SJP_NUMBER, "12345.6789" },

    { SJP_OK, SJP_OBJECT_END, "}" },


    { SJP_OK, SJP_NONE, NULL }, // end sentinel
  };

  run_parser_test(__func__, DEFAULT_STACK, SMALL_BUF, inputs, outputs);
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
    { SJP_MORE, SJP_STRING, "foo" },
    { SJP_UNFINISHED_INPUT, SJP_NONE, NULL },

    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNFINISHED_INPUT, SJP_NONE, NULL },

    { SJP_MORE, SJP_NUMBER, "123.5e" },
    { SJP_UNFINISHED_INPUT, SJP_NONE, NULL },

    // test arrays
    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNCLOSED_ARRAY, SJP_NONE, NULL },

    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNCLOSED_ARRAY, SJP_NONE, NULL },

    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNCLOSED_ARRAY, SJP_NONE, NULL },

    // test objects
    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNCLOSED_OBJECT, SJP_NONE, NULL },

    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNCLOSED_OBJECT, SJP_NONE, NULL },

    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNCLOSED_OBJECT, SJP_NONE, NULL },

    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_OK, SJP_STRING, "bar" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNCLOSED_OBJECT, SJP_NONE, NULL },

    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_OK, SJP_STRING, "bar" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNCLOSED_OBJECT, SJP_NONE, NULL },

    // test nested things
    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_NUMBER, "123" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNCLOSED_ARRAY, SJP_NONE, NULL },

    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "bar" },
    { SJP_OK, SJP_STRING, "baz" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNCLOSED_OBJECT, SJP_NONE, NULL },

    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_NUMBER, "123" },
    { SJP_OK, SJP_STRING, "bar" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNCLOSED_ARRAY, SJP_NONE, NULL },

    { SJP_OK, SJP_ARRAY_BEG, "[" },
    { SJP_OK, SJP_STRING, "foo" },
    { SJP_OK, SJP_OBJECT_BEG, "{" },
    { SJP_OK, SJP_STRING, "bar" },
    { SJP_OK, SJP_STRING, "baz" },
    { SJP_MORE, SJP_NONE, "" },
    { SJP_UNCLOSED_OBJECT, SJP_NONE, NULL },

    { SJP_OK, SJP_NONE, NULL }, // end sentinel
  };

  run_parser_test(__func__, DEFAULT_STACK, NO_BUF, inputs, outputs);
}

int main(void)
{
  test_values();
  test_simple_objects();
  test_simple_arrays();
  test_nested_arrays_and_objects();
  test_restarts_1();

  test_buffered_1();

  test_detect_unclosed_things();

  printf("%d tests, %d failures\n", ntest,nfail);
  return nfail == 0 ? 0 : 1;
}



