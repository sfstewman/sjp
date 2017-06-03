#ifndef SJP_LEXER_H
#define SJP_LEXER_H

#include "sjp_common.h"

#include <stdlib.h>

#define MODULE_NAME SJP_LEXER

// Lexer state required to resume lexing if the input is exhausted
// before a full token can be emitted
//
// state is not kept for arrays and objects because their delimiters are
// single characters
enum SJP_LEX_STATE {
  SJP_LST_VALUE,    // waiting on json value

  SJP_LST_KEYWORD,  // parsing keywords (true,false,null)

  SJP_LST_STR,
  SJP_LST_STR_ESC1, // read '\' but not the escape character

  // these escape allows restart inside of a \uXYZW escape
  SJP_LST_STR_ESC2, // waiting for X
  SJP_LST_STR_ESC3, // waiting for Y
  SJP_LST_STR_ESC4, // waiting for Z
  SJP_LST_STR_ESC5, // waiting for W

  // states for waiting for second \uXYZW sequence in a surrogate pair
  SJP_LST_STR_PAIR0, // waiting for '\'
  SJP_LST_STR_PAIR1, // waiting for 'u'
  SJP_LST_STR_PAIR2, // waiting for X
  SJP_LST_STR_PAIR3, // waiting for Y
  SJP_LST_STR_PAIR4, // waiting for Z
  SJP_LST_STR_PAIR5, // waiting for W

  // number states
  SJP_LST_NUM_NEG,  // read leading '-' sign
  SJP_LST_NUM_DIG0, // read leading zero digit
  SJP_LST_NUM_DIG,  // read digit for the integer part
  SJP_LST_NUM_DOT,  // read '.'
  SJP_LST_NUM_DIGF, // read digit for the fractional part
  SJP_LST_NUM_EXP,  // read exponent char ('e' or 'E')
  SJP_LST_NUM_ESGN, // read exponent sign
  SJP_LST_NUM_EDIG, // read exponent digit
};

// buffer size to allow lexer to restart reading keyword/string/number
// states
enum { SJP_LEX_RESTART_SIZE = 32 };

/* Lex JSON tokens.  Makes no attempt to ensure that the JSON has a
 * valid structure, only that its tokens are valid.
 */
struct sjp_lexer {
  size_t sz;
  size_t off;
  size_t line;
  size_t lbeg;
  size_t prev_lbeg;
  char *data;
  uint32_t u8st;
  uint32_t u8cp;

  // buffer to allow restart during keyword/string/number states
  char buf[SJP_LEX_RESTART_SIZE];
  enum SJP_LEX_STATE state;
};

enum SJP_TOKEN {
  SJP_TOK_NONE,    // no input

  SJP_TOK_NULL,
  SJP_TOK_TRUE,
  SJP_TOK_FALSE,
  SJP_TOK_STRING,
  SJP_TOK_NUMBER,
  SJP_TOK_OCURLY   = '{',
  SJP_TOK_CCURLY   = '}',
  SJP_TOK_OBRACKET = '[',
  SJP_TOK_CBRACKET = ']',
  SJP_TOK_COMMA    = ',',
  SJP_TOK_COLON    = ':',

  SJP_TOK_EOS      = 0xff,
};

struct sjp_token {
  size_t n;
  const char *value;
  enum SJP_TOKEN type;
};

// Initializes the lexer state, reseting its state
void sjp_lexer_init(struct sjp_lexer *l);

// Sets the lexer data, resets the buffer offset.  The lexer may modify
// the data.
//
// The caller may pass the same data buffer to the lexer.
void sjp_lexer_more(struct sjp_lexer *l, char *data, size_t n);

// Tells the lexer that we've reached the end of the stream.
static inline void sjp_lexer_eos(struct sjp_lexer *l)
{
  sjp_lexer_more(l, NULL, 0);
}

// Returns the next token or partial token
//
// If the return is a partial token, the buffer is exhausted.  If the
// token type is not SJP_NONE, the lexer expects a partial token
enum SJP_RESULT sjp_lexer_token(struct sjp_lexer *l, struct sjp_token *tok);

// Closes the lexer state.  If the lexer is not at a position where it
// can be stopped (ie: it's waiting on the close " of a string), the
// lexer returns SJP_INVALID.
enum SJP_RESULT sjp_lexer_close(struct sjp_lexer *l);

#undef MODULE_NAME

#endif /* SJP_LEXER_H */

