#include "sjp_parser.h"

#include <assert.h>
#include <string.h>

#if SJP_DEBUG
#  define SHOULD_NOT_REACH() abort()
#else
#  define SHOULD_NOT_REACH()
#endif /* SJP_DEBUG */

static int jp_pushstate(struct sjp_parser *p, enum SJP_PARSER_STATE st);
static int jp_popstate(struct sjp_parser *p);

static int jp_getstate(struct sjp_parser *p);
static void jp_setstate(struct sjp_parser *p, enum SJP_PARSER_STATE st);

void sjp_parser_reset(struct sjp_parser *p)
{
  struct sjp_token zero_tok = { 0 };
  sjp_lexer_init(&p->lex);
  p->top = 0;
  jp_pushstate(p,SJP_PARSER_VALUE);
  p->off = 0;

  p->spill = zero_tok;
  p->rspill = 0;
  p->has_spilled = 0;
}

enum SJP_RESULT sjp_parser_init(struct sjp_parser *p, char *stack, size_t nstack, char *buf, size_t nbuf)
{
  if (nstack < SJP_PARSER_MIN_STACK || stack == NULL) {
    return SJP_INVALID_PARAMS;
  }

  if (nbuf > 0 && buf == NULL) {
    return SJP_INVALID_PARAMS;
  }

  if (nbuf > 0 && nbuf <= SJP_LEX_RESTART_SIZE) {
    return SJP_INVALID_PARAMS;
  }

  p->stack = stack;
  p->nstack = nstack;

  p->buf = buf;
  p->nbuf = nbuf;

  sjp_parser_reset(p);

  return SJP_OK;
}

#define PUSHSTATE(p,st) do{        \
  int _e = jp_pushstate((p),(st)); \
  if (SJP_ERROR(_e)) { return _e; } \
} while(0)

#define POPSTATE(p) do{      \
  int _e = jp_popstate((p)); \
  if (SJP_ERROR(_e)) { return _e; } \
} while(0)

static int jp_pushstate(struct sjp_parser *p, enum SJP_PARSER_STATE st)
{
  if (p->top >= p->nstack) {
    return SJP_TOO_MUCH_NESTING;
  }

  p->stack[p->top++] = st;

  return SJP_OK;
}

static int jp_getstate(struct sjp_parser *p)
{
  return p->stack[p->top-1];
}

static void jp_setstate(struct sjp_parser *p, enum SJP_PARSER_STATE st)
{
  p->stack[p->top-1] = st;
}

static int jp_popstate(struct sjp_parser *p)
{
  if (p->top == 0) {
    return SJP_INTERNAL_ERROR;
  }

  p->top--;
  return SJP_OK;
}

static int parse_value(struct sjp_parser *p, struct sjp_event *evt, int ret, struct sjp_token *tok)
{
  switch (tok->type) {
    case SJP_TOK_NULL:   evt->type = SJP_NULL;   break;
    case SJP_TOK_TRUE:   evt->type = SJP_TRUE;   break;
    case SJP_TOK_FALSE:  evt->type = SJP_FALSE;  break;
    case SJP_TOK_STRING: evt->type = SJP_STRING; break;
    case SJP_TOK_NUMBER:
      evt->type = SJP_NUMBER;
      evt->d = tok->dbl;
      break;

    case '{':
      evt->type = SJP_OBJECT_BEG;
      PUSHSTATE(p, SJP_PARSER_OBJ_NEW);
      return ret;

    case '[':
      evt->type = SJP_ARRAY_BEG;
      PUSHSTATE(p, SJP_PARSER_ARR_NEW);
      return ret;

    default:
      return SJP_INVALID_INPUT;
  }

  if (ret != SJP_OK) {
    PUSHSTATE(p, SJP_PARSER_PARTIAL);
  }

  return ret;
}

// Fills buffer with data from token, updates buffer offset
// and token.
//
// Returns 0 if the buffer could accomodate all of the data, 
//         1 if not
static int fillbuf(struct sjp_parser *p, struct sjp_token *tok)
{
  size_t nfill = p->nbuf - p->off;
  if (nfill > tok->n) {
    nfill = tok->n;
  }
  memcpy(&p->buf[p->off], tok->value, nfill);
  p->off += nfill;
  tok->value += nfill;
  tok->n -= nfill;
  return tok->n > 0;
}

static void returnbuf(struct sjp_parser *p, struct sjp_token *tok)
{
  tok->value = &p->buf[0];
  tok->n = p->off;
  p->off = 0;
}

static enum SJP_RESULT spill(struct sjp_parser *p, struct sjp_token *tok, enum SJP_RESULT ret)
{
  assert(p->spill.n == 0);
  assert(p->spill.value == NULL);
  assert(p->rspill == 0);
  assert(tok->n > 0);
  assert(tok->value != NULL);

  p->spill.n = tok->n;
  p->spill.value = tok->value;
  p->rspill = ret;
  p->has_spilled = 1;

  returnbuf(p,tok);

  return ret;
}

static enum SJP_RESULT next_token(struct sjp_parser *p, struct sjp_token *tok)
{
  enum SJP_RESULT ret;

  // fast exit if we're unbuffered
  if (p->nbuf == 0) {
    return sjp_lexer_token(&p->lex, tok);
  }

next_token:
  // The parser has a buffer and a marker for spilled data.  Data is
  // spilled when we have some buffered data and the next token would
  // overrun our buffer.  In this case, we save the spilled data and
  // return the buffered data.
  //
  // If we have spilled data, this is used instead of asking the lexer
  // for the next token.
  if (p->spill.n > 0) { // spilled data...
    *tok = p->spill;
    ret = p->rspill;
    p->spill.n = 0;
    p->spill.value = NULL;
    p->rspill = 0;
  } else {
    // Otherwise ask the lexer for the next token
    ret = sjp_lexer_token(&p->lex, tok);
    if (SJP_ERROR(ret)) {
      return ret;
    }
  }

  switch (ret) {
    case SJP_OK:
      {
        // reset spilled state, note that spill() will set the spilled
        // state again if necessary
        p->has_spilled = 0;

        // fast exit if buffer is empty
        if (p->off == 0) {
          return ret;
        }
      }
      /* fallthrough */

    case SJP_MORE:
      {
        // fast exit if (SJP_MORE, SJP_TOK_NONE, *)
        if (tok->type == SJP_TOK_NONE) {
          return ret;
        }

        // Fill buffer from token data.  If the token still has more
        // data, mark the data as spilled and return the buffer.
        if (fillbuf(p, tok)) {
          return spill(p, tok, ret);
        }

        // No spill.  Return the buffer if:
        //   1. the buffer is full
        //   2. ret is SJP_OK
        //   3. we've already spilled
        //
        // XXX - add a high-water mark to try and avoid spilling next
        // call?
        if (ret == SJP_OK || p->has_spilled || p->off >= p->nbuf) {
          returnbuf(p,tok);
          return ret;
        }

        // Return (SJP_MORE, SJP_TOK_NONE, "") to request more data...
        if (ret != SJP_OK && p->off < p->nbuf && !p->has_spilled) {
          tok->type = SJP_TOK_NONE;
          tok->value = "";
          tok->n = 0;
          return SJP_MORE;
        }
      }

    case SJP_PARTIAL:
      // If return is SJP_PARTIAL:
      //
      //   If the data would overflow the buffer, mark the data as spilled
      //   and return the buffer.  Done.
      //
      //   Otherwise, append the data to the buffer, go to state 2.
      {
        if (fillbuf(p, tok)) {
          spill(p, tok, ret);
          return SJP_MORE;
        }

        // if we didn't spill, request more from the lexer
        goto next_token;
      }

    default:
      SHOULD_NOT_REACH();
      return SJP_INTERNAL_ERROR;
  }
}

enum SJP_RESULT sjp_parser_next(struct sjp_parser *p, struct sjp_event *evt)
{
  struct sjp_token tok = {0};
  int st,ret;

restart:
  evt->text = NULL;
  evt->n = 0;
  evt->d = 0;

  // XXX - stream of values?
  if (p->top == 0) {
    // TODO: this was initially meant to close the stream of input after
    // the first value is read, but currently the stack should always
    // have one item.
    return SJP_INTERNAL_ERROR;
  }

  st = jp_getstate(p);

  if (ret = next_token(p, &tok), SJP_ERROR(ret)) {
    return ret;
  }

  if (ret == SJP_MORE && tok.type == SJP_NONE) {
    return ret;
  }

  evt->text = tok.value;
  evt->n = tok.n;
  if (evt->type == SJP_TOK_NUMBER) {
    evt->d = tok.dbl;
  }

  switch (st) {
    case SJP_PARSER_VALUE:
      return parse_value(p, evt, ret, &tok);

    case SJP_PARSER_PARTIAL:
      if (ret == SJP_OK) {
        POPSTATE(p);
      }

      switch (tok.type) {
        case SJP_TOK_NULL:
          evt->type = SJP_NULL;
          break;

        case SJP_TOK_TRUE:
          evt->type = SJP_TRUE;
          break;

        case SJP_TOK_FALSE:
          evt->type = SJP_FALSE;
          break;

        case SJP_TOK_STRING:
          evt->type = SJP_STRING;
          break;

        case SJP_TOK_NUMBER:
          evt->type = SJP_NUMBER;
          evt->d = tok.dbl;
          break;

        default:
          /* should not reach here */
          SHOULD_NOT_REACH();
          return SJP_INTERNAL_ERROR;
      }
      return ret;

    case SJP_PARSER_OBJ_NEW:
      switch (tok.type) {
        case '}':
          POPSTATE(p);

          evt->type = SJP_OBJECT_END;
          return SJP_OK;

        case SJP_TOK_STRING:
          evt->type = SJP_STRING;
          jp_setstate(p, SJP_PARSER_OBJ_KEY);
          if (ret != SJP_OK) {
            PUSHSTATE(p,SJP_PARSER_PARTIAL);
          }
          return ret;

        default:
          return SJP_INVALID_KEY;
      }

    case SJP_PARSER_OBJ_KEY:
      if (tok.type != ':') {
        return SJP_INVALID_INPUT;
      }

      jp_setstate(p, SJP_PARSER_OBJ_COLON);
      goto restart;

    case SJP_PARSER_OBJ_COLON:
      jp_setstate(p, SJP_PARSER_OBJ_VALUE);
      return parse_value(p, evt, ret, &tok);

    case SJP_PARSER_OBJ_VALUE:
      switch (tok.type) {
        case ',':
          jp_setstate(p, SJP_PARSER_OBJ_NEXT);
          goto restart;

        case '}':
          POPSTATE(p);

          evt->type = SJP_OBJECT_END;
          return SJP_OK;

        default:
          return SJP_INVALID_INPUT;
      }

    case SJP_PARSER_OBJ_NEXT:
      if (tok.type != SJP_TOK_STRING) {
        return SJP_INVALID_KEY;
      }

      evt->type = SJP_STRING;
      jp_setstate(p, SJP_PARSER_OBJ_KEY);
      if (ret != SJP_OK) {
        PUSHSTATE(p, SJP_PARSER_PARTIAL);
      }
      return ret;

    case SJP_PARSER_ARR_NEW:
      if (tok.type == ']') {
        POPSTATE(p);

        evt->type = SJP_ARRAY_END;
        return SJP_OK;
      }

      jp_setstate(p, SJP_PARSER_ARR_ITEM);
      return parse_value(p, evt, ret, &tok);

    case SJP_PARSER_ARR_ITEM:
      switch (tok.type) {
        case ',':
          jp_setstate(p, SJP_PARSER_ARR_NEXT);
          goto restart;

        case ']':
          POPSTATE(p);

          evt->type = SJP_ARRAY_END;
          return SJP_OK;

        default:
          return SJP_INVALID_INPUT;
      }

    case SJP_PARSER_ARR_NEXT:
      jp_setstate(p, SJP_PARSER_ARR_ITEM);
      return parse_value(p, evt, ret, &tok);

    default:
      return SJP_INTERNAL_ERROR;
  }

  SHOULD_NOT_REACH();
  return SJP_INTERNAL_ERROR;
}

void sjp_parser_more(struct sjp_parser *p, char *data, size_t n)
{
  sjp_lexer_more(&p->lex, data, n);
}

int sjp_parser_state(struct sjp_parser *p)
{
    return jp_getstate(p);
}

enum SJP_RESULT sjp_parser_close(struct sjp_parser *p)
{
  int ret;
  if (ret = sjp_lexer_close(&p->lex), SJP_ERROR(ret)) {
    return ret;
  }

  if (jp_getstate(p) == SJP_PARSER_PARTIAL) {
    POPSTATE(p);
  }

  if (p->top > 1) {
    int i;

    // look at what we're waiting for...
    for (i=p->top-1; i >= 1; i--) {
      switch (p->stack[i]) {
        case SJP_PARSER_VALUE:
        case SJP_PARSER_PARTIAL:
          break;

        case SJP_PARSER_OBJ_NEW:
        case SJP_PARSER_OBJ_KEY:
        case SJP_PARSER_OBJ_COLON:
        case SJP_PARSER_OBJ_VALUE:
        case SJP_PARSER_OBJ_NEXT:
          return SJP_UNCLOSED_OBJECT;

        case SJP_PARSER_ARR_NEW:
        case SJP_PARSER_ARR_ITEM:
        case SJP_PARSER_ARR_NEXT:
          return SJP_UNCLOSED_ARRAY;

        default:
          return SJP_INTERNAL_ERROR;  // unknown state
      }
    }

    return SJP_INTERNAL_ERROR;
  }

  return SJP_OK;
}

