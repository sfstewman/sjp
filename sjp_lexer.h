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

// Buffer size to allow lexer to restart reading keyword/string/number
// states.
//
// Keywords are 'true', 'false', and 'null', which in utf8 require 4 or
// 5 bytes.
//
// Strings only need a restart buffer for the \uXYZW escape sequence,
// which only requires 4 bytes of storage.
//
// Numbers, on the other hand, require more.  This buffer should allow
// us to parse either uint64_t, int64_t or double.
//
// For uint64_t: 64 bits, so 64 / log2(10) = 19.3, so 20 digits
// For int64_t:  63 bits, so 63 / log2(10) = 19.0, plus a sign, so 20
//               digits
//
// For double:   53 bits in the significand, a sign, a decimal point,
//               and an exponent.
//
//   significand: 53 / log2(10) = 15.9, or 16 digits
//   sign:        one digit
//   exponent:    range is -1023 to 1024 plus 'e', so 6 chars
//
//   This gives 16 + 1 + 6 = 23.
//
// Round up to the nearest (small) power of two to offer some padding,
// especially for double numbers where people do odd things.
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
  size_t ncp;

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

  /* extra (type-dependent) information about the token */
  union {
    size_t ncp; // SJP_TOK_STRING: number of codepoints fully parsed in string
    double dbl; // SJP_TOK_NUMBER: double precision value of number
  } extra;
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

// Returns the next complete token or partial token.  Both the l and the
// tok parameter must be non-NULL.
//
// If the return value is SJP_OK:
//   On return, if tok->type is SJP_TOK_NONE, this indicates an
//   end-of-stream (sjp_lexer_eos has previously been called).
//
//   Otherwise, the lexer returns a complete token.
//
// If the return value is SJP_MORE:
//   The lexer input buffer is exhausted, and the lexer cannot advance
//   further until more input is provided via sjp_lexer_more().
//
//   On return, if tok->type is SJP_TOK_NONE, this does *not* indicate
//   an end-of-stream, only that the lexer cannot return a partial
//   token.  Otherwise, a partial token is returned.
//
// If the return value is SJP_PARTIAL:
//   Only a partial token is available, but the lexer input buffer is
//   not exhausted and sjp_lexer_more() should not be called.
//
//   On return, tok->type should never be SJP_TOK_NONE.
//
// If the return value is negative, the lexer encountered an error.
//
// After an end-of-stream or an error, the lexer should be reset
// via sjp_lexer_init() before additional calls are made.
//
// Tokens:
//   On return, the tok->type field will have the type of token
//   returned, if any.
//
//   For return values of SJP_MORE, the only values of tok->type should
//   be SJP_TOK_NONE, SJP_TOK_STRING, or SJP_TOK_NUMBER.
//
//   Currently SJP_PARTIAL will only be returned in certain cases of
//   parsing \u escapes on strings.  However, this may change in
//   subsequent versions.
//
enum SJP_RESULT sjp_lexer_token(struct sjp_lexer *l, struct sjp_token *tok);

// Closes the lexer state.  If the lexer is not at a position where it
// can be stopped (ie: it's waiting on the close " of a string), the
// lexer returns SJP_INVALID.
enum SJP_RESULT sjp_lexer_close(struct sjp_lexer *l);

#undef MODULE_NAME

#endif /* SJP_LEXER_H */

