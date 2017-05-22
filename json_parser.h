#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "json_common.h"
#include "json_lexer.h"

#define MODULE_NAME JSON_PARSER

enum JSON_EVENT {
  JSON_NONE = 0,

  JSON_NULL,
  JSON_TRUE,
  JSON_FALSE,
  JSON_STRING,
  JSON_NUMBER,
  JSON_OBJECT_BEG,
  JSON_OBJECT_END,
  JSON_ARRAY_BEG,
  JSON_ARRAY_END,
};

enum JSON_PARSER_STATE {
  JSON_PARSER_VALUE = 0,

  JSON_PARSER_PARTIAL, // finishing a partial value

  // object states:          read               expect
  JSON_PARSER_OBJ_NEW,    // '{'                either key (string) or '}'
  JSON_PARSER_OBJ_KEY,    // key (string)       ':'
  JSON_PARSER_OBJ_COLON,  // colon              value
  JSON_PARSER_OBJ_VALUE,  // value              either ',' or '}'
  JSON_PARSER_OBJ_NEXT,   // ','                key (string)

  // array states:           read               expect
  JSON_PARSER_ARR_NEW,    // '['                either item (value) or ']'
  JSON_PARSER_ARR_ITEM,   // item (value)       either ']' or ','
  JSON_PARSER_ARR_NEXT,   // ','                item (value)
};

struct json_event {
  enum JSON_EVENT type;

  const char *text;
  size_t n;
};

enum {
  JSON_PARSER_MIN_STACK  = 16,
  JSON_PARSER_MIN_BUFFER = 1024,
};

struct json_parser {
  char *stack;
  size_t top;
  size_t nstack;

  char *buf;
  size_t off;
  size_t nbuf;

  struct json_lexer lex;
};

// Initializes the parser state.  The parser has both a stack and a
// value buffer.
//
// In nstack < JSON_PARSER_MIN_STACK or nbuf < JSON_PARSER_MIN_BUFFER,
// returns JSON_INVALID.
//
// TODO: allow no buffer to be allocated?
int json_parser_init(struct json_parser *p, char *stack, size_t nstack, char *vbuf, size_t nbuf);

// Resets a the parser to its initial state.
//
// This can be used on any parser with a valid (non-NULL and unfreed)
// stack and input buffer.
void json_parser_reset(struct json_parser *p);

// Feeds more data to the parser.
void json_parser_more(struct json_parser *p, char *data, size_t n);

// Fetches the next json event.
//
// Return values:
//
//   JSON_OK            evt has the next event and the text of that event
//
//   JSON_MORE          the parser requires more data to complete the
//                      next event.  Partial data may be returned.
//
//   JSON_PARTIAL       partial data from the next string or number
//                      value.  More data is not required.
//
//   JSON_INVALID       an error occured
//
// To explain restarts (JSON_MORE and JSON_PARTIAL):
//
// If the parser exhausts the input data before it has completely parsed
// an event, it will return JSON_MORE.  If the parser is parsing a
// string, number, or keyword when it exhausts the input data, it will
// either return the partial data in the event (event type is
// JSON_STRING or JSON_NUMBER) or it will buffer the data and the event
// type will be JSON_NONE.  If the parser is not parsing a string,
// number, or keyword, the event type will be JSON_NONE.
//
// When JSON_MORE is returned, the caller must give the parser more data
// with json_parser_more() or close the parser with json_parser_close().
//
// The parser has buffered the string, number, or keyword, and will
// overflow the buffer before the event can be finished, it will return
// JSON_PARTIAL with the partial data for the event.  The caller must
// not give the parser more data when JSON_PARTIAL is returned.
//
// If JSON_MORE is returned with partial data or JSON_PARTIAL is
// returned.  The event will be completed after one or more subsequent
// calls to json_parser_next().  Note that during partial returns, the
// event has not been fully parsed, and a subsequent call to
// json_parser_next() may return JSON_INVALID.
int json_parser_next(struct json_parser *p, struct json_event *evt);

// Closes the parser.  If the parser is not in a state that's valid to
// close, returns an error.
//
// The caller is responsible for free'ing any allocated buffers.
int json_parser_close(struct json_parser *p);


#undef MODULE_NAME

#endif /* JSON_PARSER_H */

