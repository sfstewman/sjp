#ifndef JSON_LEXER_H
#define JSON_LEXER_H

#include <stdlib.h>

#define MODULE_NAME JSON_LEXER

// Lexer state required to resume lexing if the input is exhausted
// before a full token can be emitted
//
// state is not kept for arrays and objects because their delimiters are
// single characters
enum JSON_LEX_STATE {
  JSON_LEX_VALUE,    // waiting on json value

  JSON_LEX_KEYWORD,  // parsing keywords (true,false,null)

  JSON_LEX_STR,
  JSON_LEX_STR_ESC1, // read '\' but not the escape character

  // these escape allows restart inside of a \uXYZW escape
  JSON_LEX_STR_ESC2, // waiting for X
  JSON_LEX_STR_ESC3, // waiting for Y
  JSON_LEX_STR_ESC4, // waiting for Z
  JSON_LEX_STR_ESC5, // waiting for W

  // number states
  JSON_LEX_NUM_NEG,  // read leading '-' sign
  JSON_LEX_NUM_DIG0, // read leading zero digit
  JSON_LEX_NUM_DIG,  // read digit for the integer part
  JSON_LEX_NUM_DOT,  // read '.'
  JSON_LEX_NUM_DIGF, // read digit for the fractional part
  JSON_LEX_NUM_EXP,  // read exponent char ('e' or 'E')
  JSON_LEX_NUM_ESGN, // read exponent sign
  JSON_LEX_NUM_EDIG, // read exponent digit
};

// max supported chars in a number
#define JSON_LEX_NUM_MAXLEN 32

/* Lex JSON tokens.  Makes no attempt to ensure that the JSON has a
 * valid structure, only that its tokens are valid.
 */
struct json_lexer {
  size_t sz;
  size_t off;
  size_t line;
  size_t lbeg;
  size_t prev_lbeg;
  char *data;

  // buffer to allow restart during keyword/string/number states
  char buf[JSON_LEX_NUM_MAXLEN];
  enum JSON_LEX_STATE state;
};

enum JSON_LEX_RESULT {
  JSON_INVALID = -1,
  JSON_OK      =  0,
  JSON_MORE    =  1,   // return partial token, needs more input
  JSON_PARTIAL =  2,   // return partial token, does not need more input
};

enum JSON_TOKEN {
  JSON_NONE,    // no input

  JSON_NULL,
  JSON_TRUE,
  JSON_FALSE,
  JSON_STRING,
  JSON_NUMBER,
  JSON_OCURLY   = '{',
  JSON_CCURLY   = '}',
  JSON_OBRACKET = '[',
  JSON_CBRACKET = ']',
  JSON_COMMA    = ',',
  JSON_COLON    = ':',
};

struct json_token {
  size_t n;
  const char *value;
  enum JSON_TOKEN type;
};

// Initializes the lexer state, reseting its state
void json_lexer_init(struct json_lexer *l);

// Sets the lexer data, resets the buffer offset.  The lexer may modify
// the data.
//
// The caller may pass the same data buffer to the lexer.
void json_lexer_more(struct json_lexer *l, char *data, size_t n);

// Returns the next token or partial token
//
// If the return is a partial token, the buffer is exhausted.  If the
// token type is not JSON_NONE, the lexer expects a partial token
int json_lexer_token(struct json_lexer *l, struct json_token *tok);

#undef MODULE_NAME

#endif /* JSON_LEXER_H */

