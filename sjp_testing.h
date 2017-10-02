#ifndef SJP_TESTING_H
#define SJP_TESTING_H

#include "sjp_lexer.h"
#include "sjp_parser.h"

#define MODULE_NAME SJP_TESTING

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

const char *ret2name(enum SJP_RESULT ret);

const char *lst2name(enum SJP_LEX_STATE st);
const char *pst2name(enum SJP_PARSER_STATE st);

const char *tok2name(enum SJP_TOKEN type);
const char *evt2name(enum SJP_EVENT evt);

// special marker to cause the test function to close the stream
extern const char testing_close_marker[];
extern const char testing_end_of_stream[];

enum SJP_TEST_FLAGS {
  SJP_TEST_NUM_CODEPOINTS = 1 << 0,
  SJP_TEST_NUMBER         = 1 << 1,
};

struct lexer_output {
  enum SJP_RESULT ret;
  enum SJP_TOKEN type;
  const char *value;
  enum SJP_TEST_FLAGS flags;
  double num;
  size_t ncp;
};

static inline int lex_is_sentinel(struct lexer_output *out)
{
  return out->ret == SJP_OK && out->type == SJP_TOK_NONE && out->value == NULL;
}

int lexer_test_inputs(struct sjp_lexer *lex, const char *inputs[], struct lexer_output *outputs);

struct parser_output {
  enum SJP_RESULT ret;
  enum SJP_EVENT type;
  const char *text;
  enum SJP_TEST_FLAGS flags;
  double num;
  size_t ncp;
};

#undef MODULE_NAME

#endif /* SJP_TESTING_H */

