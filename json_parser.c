#include "json_parser.h"

static int jp_pushstate(struct json_parser *p, enum JSON_PARSER_STATE st);
static int jp_popstate(struct json_parser *p);

static int jp_getstate(struct json_parser *p);
static void jp_setstate(struct json_parser *p, enum JSON_PARSER_STATE st);

void json_parser_reset(struct json_parser *p)
{
  json_lexer_init(&p->lex);
  p->top = 0;
  jp_pushstate(p,JSON_PARSER_VALUE);
  p->off = 0;
}

int json_parser_init(struct json_parser *p, char *stack, size_t nstack, char *buf, size_t nbuf)
{
  if (nstack < JSON_PARSER_MIN_STACK || nbuf < JSON_PARSER_MIN_BUFFER) {
    return JSON_INVALID_PARAMS;
  }

  if (stack == NULL || buf == NULL) {
    return JSON_INVALID_PARAMS;
  }

  p->stack = stack;
  p->nstack = nstack;

  p->buf = buf;
  p->nbuf = nbuf;

  json_parser_reset(p);

  return JSON_OK;
}

int json_parser_close(struct json_parser *p)
{
  int ret;
  if (ret = json_lexer_close(&p->lex), JSON_ERROR(ret)) {
    return ret;
  }

  if (p->top > 1) {
    int i;

    // look at what we're waiting for...
    for (i=p->top-1; i >= 1; i--) {
      switch (p->stack[i]) {
        case JSON_PARSER_VALUE:
        case JSON_PARSER_PARTIAL:
          break;

        case JSON_PARSER_OBJ_NEW:
        case JSON_PARSER_OBJ_KEY:
        case JSON_PARSER_OBJ_COLON:
        case JSON_PARSER_OBJ_VALUE:
        case JSON_PARSER_OBJ_NEXT:
          return JSON_UNCLOSED_OBJECT;

        case JSON_PARSER_ARR_NEW:
        case JSON_PARSER_ARR_ITEM:
        case JSON_PARSER_ARR_NEXT:
          return JSON_UNCLOSED_ARRAY;
      }
    }

    return JSON_INVALID;
  }

  return JSON_OK;
}

static int jp_pushstate(struct json_parser *p, enum JSON_PARSER_STATE st)
{
  if (p->top >= p->nstack) {
    return JSON_INVALID;
  }

  p->stack[p->top++] = st;

  return JSON_OK;
}

static int jp_getstate(struct json_parser *p)
{
  return p->stack[p->top-1];
}

static void jp_setstate(struct json_parser *p, enum JSON_PARSER_STATE st)
{
  p->stack[p->top-1] = st;
}

static int jp_popstate(struct json_parser *p)
{
  if (p->top == 0) {
    return JSON_INVALID;
  }

  p->top--;
  return JSON_OK;
}

static int parse_value(struct json_parser *p, struct json_event *evt, int ret, struct json_token *tok)
{
  switch (tok->type) {
    case JSON_TOK_NULL:   evt->type = JSON_NULL;   break;
    case JSON_TOK_TRUE:   evt->type = JSON_TRUE;   break;
    case JSON_TOK_FALSE:  evt->type = JSON_FALSE;  break;
    case JSON_TOK_STRING: evt->type = JSON_STRING; break;
    case JSON_TOK_NUMBER: evt->type = JSON_NUMBER; break;

    case '{':
      evt->type = JSON_OBJECT_BEG;
      if (jp_pushstate(p, JSON_PARSER_OBJ_NEW) != JSON_OK) {
        return JSON_INVALID;
      }
      return ret;

    case '[':
      evt->type = JSON_ARRAY_BEG;
      if (jp_pushstate(p, JSON_PARSER_ARR_NEW) != JSON_OK) {
        return JSON_INVALID;
      }
      return ret;

    default:
      return JSON_INVALID;
  }

  if (ret != JSON_OK) {
    if (jp_pushstate(p, JSON_PARSER_PARTIAL) != JSON_OK) {
      return JSON_INVALID;
    }
  }

  return ret;
}

int json_parser_next(struct json_parser *p, struct json_event *evt)
{
  struct json_token tok = {0};
  int st,ret;

restart:
  // XXX - stream of values?
  if (p->top == 0) {
    return JSON_INVALID;
  }

  st = jp_getstate(p);

  ret = json_lexer_token(&p->lex, &tok);
  if (ret == JSON_INVALID) {
    return ret;
  }

  if (ret == JSON_MORE && tok.type == JSON_NONE) {
    return ret;
  }

  evt->text = tok.value;
  evt->n = tok.n;

  switch (st) {
    case JSON_PARSER_VALUE:
      return parse_value(p, evt, ret, &tok);

    case JSON_PARSER_PARTIAL:
      if (ret == JSON_OK) {
        if (jp_popstate(p) != JSON_OK) {
          return JSON_INVALID;
        }
      }
      switch (tok.type) {
        case JSON_TOK_NULL:
          evt->type = JSON_NULL;
          break;

        case JSON_TOK_TRUE:
          evt->type = JSON_TRUE;
          break;

        case JSON_TOK_FALSE:
          evt->type = JSON_FALSE;
          break;

        case JSON_TOK_STRING:
          evt->type = JSON_STRING;
          break;

        case JSON_TOK_NUMBER:
          evt->type = JSON_NUMBER;
          break;

        default:
          return JSON_INVALID;
      }
      return ret;

    case JSON_PARSER_OBJ_NEW:
      switch (tok.type) {
        case '}':
          if (jp_popstate(p) != JSON_OK) {
            return JSON_INVALID;
          }

          evt->type = JSON_OBJECT_END;
          return JSON_OK;

        case JSON_TOK_STRING:
          evt->type = JSON_STRING;
          jp_setstate(p, JSON_PARSER_OBJ_KEY);
          if (ret != JSON_OK) {
            jp_pushstate(p, JSON_PARSER_PARTIAL);
          }
          return ret;

        default:
          return JSON_INVALID;
      }

    case JSON_PARSER_OBJ_KEY:
      if (tok.type != ':') {
        return JSON_INVALID;
      }

      jp_setstate(p, JSON_PARSER_OBJ_COLON);
      goto restart;

    case JSON_PARSER_OBJ_COLON:
      jp_setstate(p, JSON_PARSER_OBJ_VALUE);
      return parse_value(p, evt, ret, &tok);

    case JSON_PARSER_OBJ_VALUE:
      switch (tok.type) {
        case ',':
          jp_setstate(p, JSON_PARSER_OBJ_NEXT);
          goto restart;

        case '}':
          if (jp_popstate(p) != JSON_OK) {
            return JSON_INVALID;
          }

          evt->type = JSON_OBJECT_END;
          return JSON_OK;

        default:
          return JSON_INVALID;
      }

    case JSON_PARSER_OBJ_NEXT:
      if (tok.type != JSON_TOK_STRING) {
        return JSON_INVALID;
      }

      evt->type = JSON_STRING;
      jp_setstate(p, JSON_PARSER_OBJ_KEY);
      if (ret != JSON_OK) {
        jp_pushstate(p, JSON_PARSER_PARTIAL);
      }
      return ret;

    case JSON_PARSER_ARR_NEW:
      if (tok.type == ']') {
          if (jp_popstate(p) != JSON_OK) {
            return JSON_INVALID;
          }

          evt->type = JSON_ARRAY_END;
          return JSON_OK;
      }

      jp_setstate(p, JSON_PARSER_ARR_ITEM);
      return parse_value(p, evt, ret, &tok);

    case JSON_PARSER_ARR_ITEM:
      switch (tok.type) {
        case ',':
          jp_setstate(p, JSON_PARSER_ARR_NEXT);
          goto restart;

        case ']':
          if (jp_popstate(p) != JSON_OK) {
            return JSON_INVALID;
          }

          evt->type = JSON_ARRAY_END;
          return JSON_OK;

        default:
          return JSON_INVALID;
      }

    case JSON_PARSER_ARR_NEXT:
      jp_setstate(p, JSON_PARSER_ARR_ITEM);
      return parse_value(p, evt, ret, &tok);

    default:
      return JSON_INVALID;
  }

  return JSON_INVALID;
}

void json_parser_more(struct json_parser *p, char *data, size_t n)
{
  json_lexer_more(&p->lex, data, n);
}
