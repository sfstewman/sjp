#include "sjp_lexer.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <assert.h>

#include "hoerhmann.h"

#define BORDER_DELIMS "{}[]:,"

static int jl_eos(struct sjp_lexer *l)
{
  return l->data == NULL;
}

static int jl_getc(struct sjp_lexer *l)
{
  int ch;

  if (l->data == NULL || l->off >= l->sz) {
    return EOF;
  }

  ch = (unsigned char)l->data[l->off++];
  if (ch == '\n') {
    l->line++;
    l->prev_lbeg = l->lbeg;
    l->lbeg = l->off;
  }

  return ch;
}

static void jl_ungetc(struct sjp_lexer *l, int ch)
{
  if (l->off > 0) {
    l->off--;
    l->data[l->off] = ch;

    if (ch == '\n') {
      l->line--;
      l->lbeg = l->prev_lbeg;
    }
  }
}

// Initializes the lexer state, reseting its state
void sjp_lexer_init(struct sjp_lexer *l)
{
  l->sz = 0;
  l->off = 0;
  l->data = NULL;

  l->line = 0;
  l->lbeg = 0;
  l->prev_lbeg = 0;

  l->u8st = 0;
  l->u8cp = 0;

  memset(l->buf, 0, sizeof l->buf);
  l->state = SJP_LST_VALUE;
}

// Sets the lexer data, resets the buffer offset.  The lexer may modify
// the data.
//
// The caller may pass the same data buffer to the lexer.
void sjp_lexer_more(struct sjp_lexer *l, char *data, size_t n)
{
  l->data = data;
  l->sz = n;
  l->off = 0;
}

static int parse_kw(struct sjp_lexer *l, struct sjp_token *tok)
{
  int ch;
  size_t off0;
  const char *kw = NULL;
  char *b = NULL;

  off0 = l->off;

  ch = (l->state == SJP_LST_KEYWORD) ? l->buf[0] : jl_getc(l);
  switch (ch) {
    case 't':
      kw = "rue";
      tok->type = SJP_TOK_TRUE;
      break;

    case 'f':
      kw = "alse";
      tok->type = SJP_TOK_FALSE;
      break;

    case 'n':
      kw = "ull";
      tok->type = SJP_TOK_NULL;
      break;

    default:
      assert(l->state != SJP_LST_KEYWORD);
      jl_ungetc(l,ch);
      goto invalid;
  }

  l->buf[0] = ch;
  b = &l->buf[1];

  if (l->state == SJP_LST_KEYWORD) {
    for (b = &l->buf[1]; *b != '\0'; b++, kw++) {
      assert(*kw == *b);
      continue;
    }
  }

  while (*kw) {
    ch = jl_getc(l);
    if (ch == EOF) {
      // XXX - should restart state
      break;
    }

    if (*kw != ch) {
      goto invalid;
    }

    *b = ch;
    b++;
    kw++;
  }

  if (*kw != 0) {
    goto partial;
  }

  // on restart, return internal buffer
  if (l->state == SJP_LST_KEYWORD) {
    tok->value = &l->buf[0];
    tok->n = b - l->buf;
  } else {
    tok->value = &l->data[off0];
    tok->n = l->off - off0;
  }

  // XXX - this doesn't check that the next character
  // is a delimiter or whitespace.  the json grammar
  // doesn't appare to clearly require this, though.
  //
  // The grammar essentially requires some kind of delimitter
  // after a keyword, so omiting the check is probably okay.
  //
  // Need to revisit this, though.

  l->state = SJP_LST_VALUE;
  return SJP_OK;

partial:
  if (jl_eos(l)) {
    tok->value = NULL;
    tok->n = 0;
    return SJP_UNFINISHED_INPUT;
  }

  *b = 0;
  l->state = SJP_LST_KEYWORD;
  tok->type = SJP_TOK_NONE;
  tok->value = l->buf;
  tok->n = b - l->buf;
  return SJP_MORE;

invalid:
  l->state = SJP_LST_VALUE;
  tok->type = SJP_TOK_NONE;
  tok->value = &l->data[off0];
  tok->n = l->sz - off0;
  return SJP_INVALID_INPUT;
}

static int tohex(int ch)
{
  // XXX - make this portable?
  if (ch >= '0' && ch <= '9') {
    return ch-'0';
  }

  if (ch >= 'A' && ch <= 'F') {
    return ch-'A'+10;
  }

  if (ch >= 'a' && ch <= 'f') {
    return ch-'a'+10;
  }

  return -1;
}

static int utf8_enc(char buf[4], long cp)
{
  if (cp < 0x80) {
    buf[0] = cp;
    return 1;
  }

  if (cp < 0x800) {
    buf[0] = 0x80 | 0x40 | (cp >> 6);
    buf[1] = 0x80 | (cp & 0x3F);
    return 2;
  }

  if (cp < 0x10000) {
    buf[0] = 0x80 | 0x40 | 0x20 | (cp >> 12);
    buf[1] = 0x80 | ((cp >> 6) & 0x3F);
    buf[2] = 0x80 | (cp & 0x3F);
    return 3;
  }

  buf[0] = 0x80 | 0x40 | 0x20 | 0x10 | (cp >> 18);
  buf[1] = 0x80 | ((cp >> 12) & 0x3F);
  buf[2] = 0x80 | ((cp >>  6) & 0x3F);
  buf[3] = 0x80 | (cp & 0x3F);
  return 4;
}

static int u16cp(const char *buf)
{
  return (tohex(buf[0]) << 12) + (tohex(buf[1]) << 8) + (tohex(buf[2]) << 4) + tohex(buf[3]);
}

static int u16pair(const char buf[8])
{
  int p1 = u16cp(&buf[0]);
  int p2 = u16cp(&buf[4]);
  int cp = 0x10000 + ((p1 - 0xD800) << 10) | ((p2 - 0xDC00));

  // fprintf(stderr, "0x%04X 0x%04X %d\n", p1,p2,cp);
  return cp;
}

static int parse_str(struct sjp_lexer *l, struct sjp_token *tok)
{
  size_t off0, outInd;
  int ch;

  off0 = l->off;
  outInd = off0;
  tok->type = SJP_TOK_STRING;

  switch (l->state) {
    case SJP_LST_STR_ESC1: goto read_esc;
    case SJP_LST_STR_ESC2: goto read_udig1; 
    case SJP_LST_STR_ESC3: goto read_udig2; 
    case SJP_LST_STR_ESC4: goto read_udig3; 
    case SJP_LST_STR_ESC5: goto read_udig4;

    case SJP_LST_STR_PAIR0: goto read_pair0;
    case SJP_LST_STR_PAIR1: goto read_pair1;
    case SJP_LST_STR_PAIR2: goto read_pair2;
    case SJP_LST_STR_PAIR3: goto read_pair3;
    case SJP_LST_STR_PAIR4: goto read_pair4;
    case SJP_LST_STR_PAIR5: goto read_pair5;

    case SJP_LST_VALUE:
      l->state = SJP_LST_STR;
      l->u8st  = 0;
      l->u8cp  = 0;
      /* fallthrough */

    case SJP_LST_STR:
      goto fast_path;

    default:
      /* should never reach */
      return SJP_INTERNAL_ERROR;
  }

  // fast path: no escapes, scan for next '"'
fast_path:
  while (ch = jl_getc(l), ch != EOF) {
    if (u8_decode(&l->u8st, &l->u8cp, (uint32_t)ch) == UTF8_REJECT) {
      l->state = SJP_LST_VALUE;
      return SJP_INVALID_CHAR;
    }

    if (ch == '"') {
        l->state = SJP_LST_VALUE;
        tok->value = &l->data[off0];
        tok->n = l->off - off0 - 1; // last -1 is to omit the trailing "
        return SJP_OK;
    }

    // control characters aren't allowed.  RFC 7159 defines them
    // as U+0000 to U+001F
    if (ch < 0x1f) {
      l->state = SJP_LST_VALUE;
      return SJP_INVALID_CHAR;
    }

    if (ch == '\\') {
      // have to rewrite the string for escapes,
      // so we can't use the fast path.
      // 
      // jump into the slow path at the point we read the escape
      // character
      outInd = l->off-1;
      goto read_esc;
    }
  }

  // if the fast-path loop terminates without a '"':
  //   1) if EOS, then we have unfinished input
  //   2) otherwise return a partial result
  if (jl_eos(l)) {
    return SJP_UNFINISHED_INPUT;
  }

  tok->value = &l->data[off0];
  tok->n = l->off - off0;
  return SJP_MORE;

  // slow_path:
  //   string has escapes, so we need to rewrite it

  // outInd MUST be set correctly at this point

  while (ch = jl_getc(l), ch != EOF) {
    long cp;
    int hexdig;

    if (u8_decode(&l->u8st, &l->u8cp, (uint32_t)ch) == UTF8_REJECT) {
      l->state = SJP_LST_VALUE;
      return SJP_INVALID_CHAR;
    }

    if (ch == '"') {
      l->state = SJP_LST_VALUE;
      tok->value = &l->data[off0];
      tok->n = outInd - off0;
      return SJP_OK;
    }

    // control characters aren't allowed.  RFC 7159 defines them
    // as U+0000 to U+001F
    if (ch < 0x1f) {
      l->state = SJP_LST_VALUE;
      return SJP_INVALID_CHAR;
    }

    if (ch != '\\') {
      l->data[outInd++] = ch;
      continue;
    }

read_esc:
    // handle escapes
    ch = jl_getc(l);
    switch (ch) {
      case EOF:
        l->state = SJP_LST_STR_ESC1;
        goto partial;

      default:
        l->state = SJP_LST_VALUE;
        return SJP_INVALID_ESCAPE;

      case '"': case '\\': case '/':
        l->data[outInd++] = ch;
        break;

      case 'b':
        l->data[outInd++] = '\b';
        break;

      case 'f':
        l->data[outInd++] = '\f';
        break;

      case 'n':
        l->data[outInd++] = '\n';
        break;

      case 'r':
        l->data[outInd++] = '\r';
        break;

      case 't':
        l->data[outInd++] = '\t';
        break;

      case 'u':
        // \uXYZW escape

        // to enable restarts, we suck the digits into l->buf
        // and only calculate the codepoint once we have all of the
        // digits

read_udig1:
        if (ch = jl_getc(l), ch == EOF) {
          l->state = SJP_LST_STR_ESC2;
          goto partial;
        }

        if (hexdig = tohex(ch), hexdig < 0) {
          // jl_ungetc(l, ch);
          l->state = SJP_LST_VALUE;
          return SJP_INVALID_ESCAPE;
        }

        l->buf[0] = ch;

read_udig2:
        if (ch = jl_getc(l), ch == EOF) {
          l->state = SJP_LST_STR_ESC3;
          goto partial;
        }

        if (hexdig = tohex(ch), hexdig < 0) {
          // jl_ungetc(l, ch);
          l->state = SJP_LST_VALUE;
          return SJP_INVALID_ESCAPE;
        }

        l->buf[1] = ch;  // in case of restart

read_udig3:
        if (ch = jl_getc(l), ch == EOF) {
          l->state = SJP_LST_STR_ESC4;
          goto partial;
        }

        if (hexdig = tohex(ch), hexdig < 0) {
          // jl_ungetc(l, ch);
          l->state = SJP_LST_VALUE;
          return SJP_INVALID_ESCAPE;
        }

        l->buf[2] = ch;  // in case of restart

read_udig4:
        if (ch = jl_getc(l), ch == EOF) {
          l->state = SJP_LST_STR_ESC5;
          goto partial;
        }

        if (hexdig = tohex(ch), hexdig < 0) {
          // jl_ungetc(l, ch);
          l->state = SJP_LST_VALUE;
          return SJP_INVALID_ESCAPE;
        }

        l->buf[3] = ch;  // in case of restart

        // check if the character is a surrogate pair
        if (l->buf[0] == 'D' && tohex(l->buf[1]) >= 8) {
          goto read_pair0;
        }

/* encode_bmp: */
        // now calculate unicode char for BMP
        cp = u16cp(&l->buf[0]);
        goto encode_utf8;

read_pair0:
        if (ch = jl_getc(l), ch == EOF) {
          l->state = SJP_LST_STR_PAIR0;
          goto partial;
        }

        // TODO - optionally allow invalid U+DCXX sequences
        // if the next character isn't a '\'
        if (ch != '\\') {
          l->state = SJP_LST_VALUE;
          return SJP_INVALID_U16PAIR;
        }

read_pair1:
        if (ch = jl_getc(l), ch == EOF) {
          l->state = SJP_LST_STR_PAIR1;
          goto partial;
        }

        // TODO - optionally allow invalid U+DCXX sequences
        // if the next character is a valid escape character
        if (ch != 'u') {
          l->state = SJP_LST_VALUE;
          return SJP_INVALID_U16PAIR;
        }

read_pair2:
        if (ch = jl_getc(l), ch == EOF) {
          l->state = SJP_LST_STR_PAIR2;
          goto partial;
        }

        if (hexdig = tohex(ch), hexdig < 0) {
          // jl_ungetc(l, ch);
          l->state = SJP_LST_VALUE;
          return SJP_INVALID_ESCAPE;
        }

        l->buf[4] = ch;  // in case of restart

read_pair3:
        if (ch = jl_getc(l), ch == EOF) {
          l->state = SJP_LST_STR_PAIR3;
          goto partial;
        }

        if (hexdig = tohex(ch), hexdig < 0) {
          // jl_ungetc(l, ch);
          l->state = SJP_LST_VALUE;
          return SJP_INVALID_ESCAPE;
        }

        l->buf[5] = ch;  // in case of restart

read_pair4:
        if (ch = jl_getc(l), ch == EOF) {
          l->state = SJP_LST_STR_PAIR4;
          goto partial;
        }

        if (hexdig = tohex(ch), hexdig < 0) {
          // jl_ungetc(l, ch);
          l->state = SJP_LST_VALUE;
          return SJP_INVALID_ESCAPE;
        }

        l->buf[6] = ch;  // in case of restart

read_pair5:
        if (ch = jl_getc(l), ch == EOF) {
          l->state = SJP_LST_STR_PAIR5;
          goto partial;
        }

        if (hexdig = tohex(ch), hexdig < 0) {
          // jl_ungetc(l, ch);
          l->state = SJP_LST_VALUE;
          return SJP_INVALID_ESCAPE;
        }

        l->buf[7] = ch;  // in case of restart

        cp = u16pair(&l->buf[0]);
        goto encode_utf8;

encode_utf8:
        // now encode the codepoint into utf8 in l->buf
        {
          int i,nb;
          nb = utf8_enc(&l->buf[0], cp);

          // make sure we have room to emit the utf8 in the string.
          // otherwise, return a PARTIAL with data from the buffer and 
          //
          // utf8 encodes in 1-4 bytes, and \uXYZW is six bytes, so this
          // only matters on a restart.  Because it's a restart, there's
          // no existing string, so emit the buffer
          assert(l->off >= outInd);
          if ((int64_t)nb > (int64_t)(l->off - outInd)) {
            assert(outInd == off0);
            tok->value = &l->buf[0];
            tok->n = nb;
            l->state = SJP_LST_STR;
            return SJP_PARTIAL;
          }

          // otherwise keep going...
          for (i=0; i < nb; i++) {
            l->data[outInd++] = l->buf[i];
            // fprintf(stderr, "0x%02X ", (unsigned char)l->buf[i]);
          }
          // fprintf(stderr, "\n");
        }

        break;
    }
  }

partial:
  // if the slow-path loop terminates without a '"':
  //   1) if EOS, then we have unfinished input
  //   2) otherwise return a partial result
  if (jl_eos(l)) {
    return SJP_UNFINISHED_INPUT;
  }

  tok->value = &l->data[off0];
  tok->n = outInd - off0;
  return SJP_MORE;
}

static int parse_num(struct sjp_lexer *l, struct sjp_token *tok)
{
  size_t off0;
  int ch;

  off0 = l->off;
  tok->type = SJP_TOK_NUMBER;
  tok->value = &l->data[off0];

  // ch = jl_getc(l);
  switch (l->state) {
    case SJP_LST_VALUE:
      l->buf[0] = 1; // offset in buffer
      ch = jl_getc(l);
      break;

    // restarts after reading:
    case SJP_LST_NUM_NEG:
      ch = jl_getc(l);
      goto st_neg;   // leading '-'

    case SJP_LST_NUM_DIG0: goto st_dig0; // leading '0'
    case SJP_LST_NUM_DIG:  goto st_dig;  // leading '1' .. '9'
    case SJP_LST_NUM_DOT:  goto st_dot;  // decimal dot '.'
    case SJP_LST_NUM_DIGF: goto st_digf; // '0' .. '9' after '.'
    case SJP_LST_NUM_EXP:  goto st_exp;  // 'e' or 'E'
    case SJP_LST_NUM_ESGN:
      ch = jl_getc(l);
      goto st_esign; // '+' or '-' after e/E
    case SJP_LST_NUM_EDIG: goto st_edig; // exponent digit

    default:
      return SJP_INTERNAL_ERROR; // should never reach here!
  }

  if (ch == '-') {
    l->state = SJP_LST_NUM_NEG;
    ch = jl_getc(l);
  }

st_neg:
  if (ch == EOF) {
    goto more;
  }

  if (ch == '0') {
    goto st_dig0;
  }

  if (ch < '1' || ch > '9') {
    goto invalid;
  }

st_dig:
  l->state = SJP_LST_NUM_DIG;
  for(;;) {
    ch = jl_getc(l);
    if (ch == EOF) {
      if (jl_eos(l)) {
        goto finish;
      }
      goto more;
    }

    if (ch == '.') {
      goto st_dot;
    }

    if (ch == 'e' || ch == 'E') {
      goto st_exp;
    }

    if (ch < '0' || ch > '9') {
      jl_ungetc(l,ch);
      goto finish;
    }
  }

st_dig0:
  l->state = SJP_LST_NUM_DIG0;
  ch = jl_getc(l);
  if (ch == EOF) {
    if (jl_eos(l)) {
      goto finish;
    }
    goto more;
  }

  if (ch == '.') {
    goto st_dot;
  }

  if (ch == 'e' || ch == 'E') {
    goto st_exp;
  }

  // numbers cannot have leading zeros
  if (ch >= '1' && ch <= '9') {
    goto invalid;
  }

  // otherwise we're done...
  jl_ungetc(l,ch);
  goto finish;

st_dot:
  l->state = SJP_LST_NUM_DOT;
  ch = jl_getc(l);
  if (ch == EOF) {
    if (jl_eos(l)) {
      goto invalid;
    }
    goto more;
  }

  if (ch < '0' || ch > '9') {
    // '.' must be followed by at least one digit
    goto invalid;
  }

st_digf:
  l->state = SJP_LST_NUM_DIGF;
  for (;;) {
    ch = jl_getc(l);
    if (ch == EOF) {
      if (jl_eos(l)) {
        goto finish;
      }
      goto more;
    }

    if (ch == 'e' || ch == 'E') {
      goto st_exp;
    }

    if (ch < '0' || ch > '9') {
      jl_ungetc(l,ch);
      goto finish;
    }
  }

st_exp:
  l->state = SJP_LST_NUM_EXP;
  ch = jl_getc(l);

  if (ch == '-' || ch == '+') {
    l->state = SJP_LST_NUM_ESGN;
    ch = jl_getc(l);
  }

st_esign:
  if (ch == EOF) {
    if (jl_eos(l)) {
      goto invalid;
    }
    goto more;
  }

  if (ch < '0' || ch > '9') {
    goto invalid;
  }

st_edig:
  l->state = SJP_LST_NUM_EDIG;
  for (;;) {
    ch = jl_getc(l);
    if (ch == EOF) {
      if (jl_eos(l)) {
        goto finish;
      }
      goto more;
    }

    if (ch < '0' || ch > '9') {
      jl_ungetc(l,ch);
      goto finish;
    }
  }

more:
  {
    size_t n;

    tok->n = l->off - off0;
    n = sizeof l->buf - l->buf[0];
    if (n >= tok->n) {
      n = tok->n;
    }

    if (n > 0) {
      int off = l->buf[0];
      memcpy(&l->buf[off], tok->value, n);
      l->buf[0] = off + n;
    }
    return SJP_MORE;
  }

finish:
  {
    size_t n;

    l->state = SJP_LST_VALUE;
    tok->n = l->off - off0;
    n = sizeof l->buf - l->buf[0];
    if (n >= tok->n) {
      n = tok->n;
    }

    if (n > 0) {
      int off = l->buf[0];
      memcpy(&l->buf[off], tok->value, n);
      l->buf[0] = off + n;
    }

    if ((unsigned char)l->buf[0] < sizeof l->buf) {
      char *ep= NULL;
      int ind = l->buf[0];

      l->buf[ind] = '\0';
      tok->extra.dbl = strtod(&l->buf[1], &ep);
      // XXX - handle overflow, underflow cases?
      assert(ep && *ep == '\0');
    }
    return SJP_OK;
  }

invalid:
  l->state = SJP_LST_VALUE;
  tok->n = l->off - off0;
  return SJP_INVALID_INPUT;
}

static int parse_value(struct sjp_lexer *l, struct sjp_token *tok)
{
  int ch;

  // skip whitespace
  tok->type = SJP_TOK_NONE;
  tok->value = NULL;

  while (ch = jl_getc(l), ch != EOF) {
  // Skip whitespace
  // XXX - is this the right definition of whitespace in json?
    if (!isspace(ch)) {
      jl_ungetc(l,ch);
      break;
    }
  }

  if (ch == EOF) {
    if (!jl_eos(l)) {
      return SJP_MORE;
    }
    tok->type = SJP_TOK_EOS;
    tok->value = NULL;
    return SJP_OK;
  }

  ch = l->data[l->off];
  switch (ch) {
    case '[': case ']': case '{': case '}': case ':': case ',':
      tok->type = ch;
      tok->value = &l->data[l->off];
      tok->n = 1;
      l->off++;
      return SJP_OK;

    case '"':
      // strip off the first character...
      l->off++;
      return parse_str(l,tok);

    case '-': case '0': case '1': case '2':
    case '3': case '4': case '5': case '6':
    case '7': case '8': case '9':
      return parse_num(l,tok);

    default:
      return parse_kw(l,tok);
  }
}

// Returns the next token or partial token
//
// If the return is a partial token, the buffer is exhausted.  If the
// token type is not SJP_TOK_NONE, the lexer expects a partial token
int sjp_lexer_token(struct sjp_lexer *l, struct sjp_token *tok)
{
  switch (l->state) {
  case SJP_LST_VALUE:
    return parse_value(l,tok);

  case SJP_LST_KEYWORD:
    return parse_kw(l,tok);

  case SJP_LST_STR:
  case SJP_LST_STR_ESC1:
  case SJP_LST_STR_ESC2:
  case SJP_LST_STR_ESC3:
  case SJP_LST_STR_ESC4:
  case SJP_LST_STR_ESC5:
  case SJP_LST_STR_PAIR0:
  case SJP_LST_STR_PAIR1:
  case SJP_LST_STR_PAIR2:
  case SJP_LST_STR_PAIR3:
  case SJP_LST_STR_PAIR4:
  case SJP_LST_STR_PAIR5:
    return parse_str(l,tok);

  case SJP_LST_NUM_NEG:
  case SJP_LST_NUM_DIG0:
  case SJP_LST_NUM_DIG:
  case SJP_LST_NUM_DOT:
  case SJP_LST_NUM_DIGF:
  case SJP_LST_NUM_EXP:
  case SJP_LST_NUM_ESGN:
  case SJP_LST_NUM_EDIG:
    return parse_num(l,tok);

  default:
    return SJP_INTERNAL_ERROR;
  }
} 

int sjp_lexer_close(struct sjp_lexer *l)
{
  switch (l->state) {
    case SJP_LST_VALUE:
    case SJP_LST_NUM_DIG0:
    case SJP_LST_NUM_DIG:
    case SJP_LST_NUM_DIGF:
    case SJP_LST_NUM_EDIG:
      l->state = SJP_LST_VALUE;
      return SJP_OK;

    default:
      l->state = SJP_LST_VALUE;
      return SJP_UNFINISHED_INPUT;
  }
}
