#ifndef JSON_TESTING_H
#define JSON_TESTING_H

#include "json_lexer.h"
#include "json_parser.h"

#define MODULE_NAME JSON_TESTING

#ifndef TEST_LOG_LEVEL
#  define TEST_LOG_LEVEL 0
#endif /* TEST_LOG_LEVEL */

#if TEST_LOG_LEVEL > 0 
#  define LOG(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#  define LOG(fmt, ...) 
#endif

extern int ntest;
extern int nfail;

const char *ret2name(enum JSON_RESULT ret);

const char *lst2name(enum JSON_LEX_STATE st);
const char *pst2name(enum JSON_PARSER_STATE st);

const char *tok2name(enum JSON_TOKEN type);
const char *evt2name(enum JSON_EVENT evt);

// special marker to cause the test function to close the stream
extern const char testing_close_marker[];

struct lexer_output {
  enum JSON_RESULT ret;
  enum JSON_TOKEN type;
  const char *value;
};

static inline int lex_is_sentinel(struct lexer_output *out)
{
  return out->ret == JSON_OK && out->type == JSON_TOK_NONE && out->value == NULL;
}

int lexer_test_inputs(struct json_lexer *lex, const char *inputs[], struct lexer_output *outputs);

struct parser_output {
  enum JSON_RESULT ret;
  enum JSON_EVENT type;
  const char *text;
};

#undef MODULE_NAME

#endif /* JSON_TESTING_H */

