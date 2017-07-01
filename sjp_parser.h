#ifndef SJP_PARSER_H
#define SJP_PARSER_H

#include "sjp_common.h"
#include "sjp_lexer.h"

#define MODULE_NAME SJP_PARSER

enum SJP_EVENT {
  SJP_NONE = 0,

  SJP_NULL,
  SJP_TRUE,
  SJP_FALSE,
  SJP_STRING,
  SJP_NUMBER,
  SJP_OBJECT_BEG,
  SJP_OBJECT_END,
  SJP_ARRAY_BEG,
  SJP_ARRAY_END,
};

#define SJP_EVENT_MAX 10

enum SJP_PARSER_STATE {
  SJP_PARSER_VALUE = 0,

  SJP_PARSER_PARTIAL, // finishing a partial value

  // object states:          read               expect
  SJP_PARSER_OBJ_NEW,    // '{'                either key (string) or '}'
  SJP_PARSER_OBJ_KEY,    // key (string)       ':'
  SJP_PARSER_OBJ_COLON,  // colon              value
  SJP_PARSER_OBJ_VALUE,  // value              either ',' or '}'
  SJP_PARSER_OBJ_NEXT,   // ','                key (string)

  // array states:           read               expect
  SJP_PARSER_ARR_NEW,    // '['                either item (value) or ']'
  SJP_PARSER_ARR_ITEM,   // item (value)       either ']' or ','
  SJP_PARSER_ARR_NEXT,   // ','                item (value)
};

struct sjp_event {
  enum SJP_EVENT type;

  const char *text;
  size_t n;
  double d;
};

enum {
  SJP_PARSER_MIN_STACK  = 16,
};

struct sjp_parser {
  char *stack;
  size_t top;
  size_t nstack;

  char *buf;
  size_t off;
  size_t nbuf;

  struct sjp_token spill;
  enum SJP_RESULT rspill;  // return value of spilled call
  int has_spilled;

  struct sjp_lexer lex;
};

// Initializes the parser state.  The parser has both a stack and a
// value buffer.
//
// Returns SJP_OK on success.
//
// Returns SJP_INVALID_PARAMS if:
//   stack == NULL or nstack < SJP_PARSER_MIN_STACK
// or if:
//   nbuf > 0 and (buf == NULL or nbuf <= SJP_LEX_RESTART_SIZE)
//
// If nbuf == 0, buf must be NULL and the lexer is not buffered.
enum SJP_RESULT sjp_parser_init(struct sjp_parser *p, char *stack, size_t nstack, char *buf, size_t nbuf);

// Resets a the parser to its initial state.
//
// This can be used on any parser with a valid (non-NULL and unfreed)
// stack and input buffer.
void sjp_parser_reset(struct sjp_parser *p);

// Feeds more data to the parser.
void sjp_parser_more(struct sjp_parser *p, char *data, size_t n);

// Tells the parser that we've reached the end of the stream.
static inline void sjp_parser_eos(struct sjp_parser *p)
{
  sjp_parser_more(p, NULL, 0);
}

// Fetches the next json event.
//
// Return values:
//
//   SJP_OK            evt has the next event and the text of that event
//
//   SJP_MORE          the parser requires more data to complete the
//                      next event.  Partial data may be returned.
//
//   SJP_PARTIAL       partial data from the next string or number
//                      value.  More data is not required.
//
//   SJP_INVALID       an error occured
//
// To explain restarts (SJP_MORE and SJP_PARTIAL):
//
// If the parser exhausts the input data before it has completely parsed
// an event, it will return SJP_MORE.  If the parser is parsing a
// string, number, or keyword when it exhausts the input data, it will
// either return the partial data in the event (event type is
// SJP_STRING or SJP_NUMBER) or it will buffer the data and the event
// type will be SJP_NONE.  If the parser is not parsing a string,
// number, or keyword, the event type will be SJP_NONE.
//
// When SJP_MORE is returned, the caller must give the parser more data
// with sjp_parser_more() or close the parser with sjp_parser_close().
//
// The parser has buffered the string, number, or keyword, and will
// overflow the buffer before the event can be finished, it will return
// SJP_PARTIAL with the partial data for the event.  The caller must
// not give the parser more data when SJP_PARTIAL is returned.
//
// If SJP_MORE is returned with partial data or SJP_PARTIAL is
// returned.  The event will be completed after one or more subsequent
// calls to sjp_parser_next().  Note that during partial returns, the
// event has not been fully parsed, and a subsequent call to
// sjp_parser_next() may return SJP_INVALID.
enum SJP_RESULT sjp_parser_next(struct sjp_parser *p, struct sjp_event *evt);

// Closes the parser.  If the parser is not in a state that's valid to
// close, returns an error.
//
// The caller is responsible for free'ing any allocated buffers.
enum SJP_RESULT sjp_parser_close(struct sjp_parser *p);

// Returns the current state of the parser.  Undefined if the parser
// is uninitialized or closed.
int sjp_parser_state(struct sjp_parser *p);

#undef MODULE_NAME

#endif /* SJP_PARSER_H */

