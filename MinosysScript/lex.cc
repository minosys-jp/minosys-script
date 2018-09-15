#include "lex.h"
#include <cstdlib>
#include <ctype.h>
#include <cstring>

using namespace std;
using namespace minosys;

LexBase::LexBase() {
  has_uc = false;
  uc = 0;
  this->rp = this->state = this->pushstate = 0;
  tagMap["this"] = LT_THIS;
  tagMap["super"] = LT_SUPER;
  tagMap["if"] = LT_IF;
  tagMap["else"] = LT_ELSE;
  tagMap["for"] = LT_FOR;
  tagMap["while"] = LT_WHILE;
  tagMap["break"] = LT_BREAK;
  tagMap["continue"] = LT_CONTINUE;
  tagMap["function"] = LT_FUNCDEF;
  tagMap["return"] = LT_RETURN;
  tagMap["global"] = LT_GLOBAL;
  tagMap["new"] = LT_NEW;
  tagMap["class"] = LT_CLASS;
  tagMap["import"] = LT_IMPORT;
  tagMap["function"] = LT_FUNCDEF;
}

void LexBase::ungetc(int c) {
  has_uc = true;
  uc = c;
}

LexBase::LexTag LexBase::analyze() {
  bool isDnum = false, isHTML = false;
  int c;
  this->itoken = 0;
  this->dtoken = 0.0;

  token.clear();
  while ((c = this->getc()) != -1 && c != '\0') {
    switch (this->state) {
    case 0:
      if (!isspace(c)) {
        if (c == '/') {
          this->state = 1;
        } else if (isalpha(c) || c == '_') {
          // 予約語
          this->state = 20;
          token.push_back(c);
        } else if (c == '$') {
          // 変数
          token.push_back((char)c);
          this->state = 30;
        } else if (c == '"') {
          // 文字列定数
          this->state = 40;
        } else if (c == '\'') {
          // 文字列定数
          this->state = 50;
        } else if (c == '0') {
          // 数値または16進数
          this->state = 60;
        } else if (isdigit(c)) {
          // 数値
          this->state = 63;
          token.push_back(c);
        } else if (c == '<') {
          // HTML 開始
          this->state = 100;
          token.push_back(c);
        } else {
          if (c == ';') return LT_NL;
          if (c == '{') return LT_BEGIN;
          if (c == '}') return LT_BEND;
          if (c == '+') {
            this->state = 70;
          } else if (c == '-') {
            this->state = 71;
          } else if (c == '*') {
            this->state = 72;
          } else if (c == '&') {
            this->state = 73;
          } else if (c == '|') {
            this->state = 74;
          } else if (c == '^') {
            this->state = 75;
          } else if (c == '>') {
            this->state = 76;
          } else if (c == '=') {
            this->state = 78;
          } else if (c == '!') {
            this->state = 79;
          } else if (c == '%') {
            this->state = 80;
          } else {
            if (c == '(') {
              ++rp;
            } else if (c == ')') {
              if (--rp == 0) {
                this->state = this->pushstate;
              }
            }
            token.push_back((char)c);
            return LT_OP;
          }
        }
      }
      break;

    case 1:
      if (c == '=') {
        token = "/=";
        this->state = 0;
        return LT_OP;
      } else if (c == '/') {
        // 行末まで無視する
        this->state = 10;
      } else if (c == '*') {
        // 終端記号まで無視する
        this->state = 12;
      } else {
        this->ungetc(c);
        token = "/";
        this->state = 0;
        return LT_OP;
      }
      break;

    case 10:
      if (c == '\n') {
        this->state = 0;
      }
      break;

    case 12:
      if (c == '*') {
        this->state = 13;
      }
      break;

    case 13:
      if (c == '/') {
        this->state = 0;
      } else {
        this->state = 12;
        this->ungetc(c);
      }
      break;

    case 20: // 予約語
      if (isalnum(c) || c == '_') {
        token.push_back((char)c);
      } else {
        this->ungetc(c);
        this->state = 0;
        if (token == "VT_NULL") {
          itoken = 0;
          return LT_INT;
        }
        if (token == "VT_INT") {
          itoken = 1;
          return LT_INT;
        }
        if (token == "VT_STRING") {
          itoken = 2;
          return LT_INT;
        }
        if (token == "VT_INST") {
          itoken = 3;
          return LT_INT;
        }
        if (token == "VT_POINTER") {
          itoken = 4;
          return LT_INT;
        }
        if (token == "VT_ARRAY") {
          itoken = 5;
          return LT_INT;
        }
        auto p = tagMap.find(token);
        if (p != tagMap.end()) {
          return p->second;
        }
        return LT_TAG;
      }
      break;

    case 30: // 変数
      if (isalnum(c) || c == '_') {
        token.push_back((char)c);
      } else {
        this->ungetc(c);
        this->state = 0;
        return LT_VAR;
      }
      break;

    case 40: // 文字列定数１
      if (c == '\\') {
        this->state = 41;
      } else if (c != '"') {
        token.push_back(c);
      } else {
        this->state = 0;
        return LT_STRING;
      }
      break;

    case 41: // バックスラッシュ定数
      if (c == 'n') {
        token.push_back('\n');
      } else if (c == 'r') {
        token.push_back('\r');
      } else if (c == '\\') {
        token.push_back('\\');
      }
      this->state = 40;
      break;

    case 50: // 文字列定数２
      if (c == '\\') {
        this->state = 51;
      } else if (c != '\'') {
        token.push_back(c);
      } else {
        this->state = 0;
        return LT_STRING;
      }
      break;

    case 51: // バックスラッシュ定数
      if (c == 'n') {
        token.push_back('\n');
      } else if (c == 'r') {
        token.push_back('\r');
      } else if (c == '\\') {
        token.push_back('\\');
      }
      this->state = 50;
      break;

    case 60: // 16進数
      if (c == 'x' || c == 'X') {
        this->state = 61;
      } else if (isdigit(c)) {
        this->state = 63;
        this->ungetc((char)c);
      } else {
        this->state = 0;
        this->ungetc((char)c);
        token.push_back('0');
        return LT_INT;
      }
      break;

    case 61: // 16進数
      if (isxdigit(c)) {
        itoken *= 16;
        itoken += (c >= 'a' && c <= 'f') ? (c - 'a' + 10) : (c >= 'A' && c <= 'F') ? (c - 'A' + 10) : (c - '0');
      } else {
        this->state = 0;
        this->ungetc((char)c);
        return LT_INT;
      }
      break;

    case 63: // 数値
      if (isdigit(c)) {
        token.push_back(c);
      } else if (c == '.' && !isDnum) {
        token.push_back(c);
        isDnum = true;
      } else {
        this->state = 0;
        this->ungetc((char)c);
        if (isDnum) {
          dtoken = atof(token.c_str());
          return LT_DNUM;
        } else {
          itoken = atoi(token.c_str());
          return LT_INT;
        }
      }
      break;

    case 70: // +
      if (c == '+') {
        token = "++";
      } else if (c == '=') {
        token = "+=";
      } else {
        this->ungetc(c);
        token = "+";
      }
      this->state = 0;
      return LT_OP;

    case 71: // - 
      if (c == '-') {
        token = "--";
      } else if (c == '=') {
        token = "-=";
      } else {
        token = "-";
        this->ungetc(c);
      }
      this->state = 0;
      return LT_OP;

    case 72: // *
      if (c == '=') {
        token = "*=";
      } else {
        token = "*";
        this->ungetc(c);
      }
      this->state = 0;
      return LT_OP;

    case 73: // &
      if (c == '&') {
        token = "&&";
      } else if (c == '=') {
        token = "&=";
      } else {
        token = "&";
        this->ungetc(c);
      }
      this->state = 0;
      return LT_OP;

    case 74: // |
      if (c == '|') {
        token = "||";
      } else if (c == '=') {
        token = "|=";
      } else {
        token = "|";
        this->ungetc(c);
      }
      this->state = 0;
      return LT_OP;

    case 75: // ^
      if (c == '=') {
        token = "^=";
      } else {
        token = "^";
        this->ungetc(c);
      }
      this->state = 0;
      return LT_OP;

    case 76: // >
      if (c == '>') {
        this->state = 77;
      } else if (c == '=') {
        token = ">=";
        this->state = 0;
        return LT_OP;
      } else {
        token = ">";
        this->ungetc(c);
        this->state = 0;
        return LT_OP;
      }
      break;

    case 77:
      if (c == '=') {
        token = ">>=";
      } else {
        token = ">>";
        this->ungetc(c);
      }
      this->state = 0;
      return LT_OP;

    case 78: // '='
      if (c == '=') {
        token = "==";
      } else {
        token = "=";
        this->ungetc(c);
      }
      state = 0;
      return LT_OP;

    case 79: // '!'
      if (c == '=') {
        token = "!=";
      } else {
        token = "!";
        this->ungetc(c);
      }
      state = 0;
      return LT_OP;

    case 80: // '%'
      if (c == '=') {
        token = "%=";
      } else {
        token = "%";
        this->ungetc(c);
      }
      state = 0;
      return LT_OP;

    case 100: // HTML
      if (c == '<') {
        this->state = 101;
      } else if (c == '=') {
        this->state = 0;
        token = "<=";
        return LT_OP;
      } else if (c == '!') {
        token.push_back('<');
        token.push_back('!');
        this->state = 110;
        isHTML = true;
      } else if (isalpha(c)) {
        token.push_back('<');
        token.push_back((char)c);
        this->state = 120;
        isHTML = true;
      } else {
        this->ungetc(c);
        this->state = 0;
        token = "<";
        return LT_OP;
      }
      break;

    case 101:
      if (c == '=') {
        token ="<<=";
      } else {
        token = "<<";
        this->ungetc(c);
        this->state = 0;
      }
      return LT_OP;

    case 110:
      if (c == '>') {
        this->state = 120;
      }
      token.push_back(c);
      break;

    case 120:
      if (c == '$') {
        if (!token.empty()) {
          this->ungetc(c);
          return LT_HTML;
        }
        token.push_back((char)c);
        this->state = 121;
      } else {
        token.push_back((char)c);
      }
      break;

    case 121:
      if (c == '@') {
        this->state = 122;
      } else {
        this->ungetc(c);
        this->state = 123;
      }
      break;

    case 122:
      if (isalnum(c) || c == '_') {
        token.push_back((char)c);
      } else {
        if (!token.empty()) {
          return LT_TAG;
        }
        if (c == '.') {
          token = ".";
          return LT_OP;
        } else if (c == '(') {
          ++rp;
          this->pushstate = 120;
	  this->state = 0;
          return LT_OP;
        }
      }
      break;

    case 123:
      if (isalnum(c) || c == '_') {
        token.push_back((char)c);
      } else {
        if (!token.empty()) {
           return LT_VAR;
        }
        if (c == '.') {
          token = ".";
          return LT_OP;
        } else if (c == '(') {
          ++rp;
          this->pushstate = 120;
          this->state = 0;
          return LT_OP;
        }
      }
      break;
    }
  }

  if (isHTML) {
    return LT_HTML;
  }
  return LT_NULL;
}

#ifdef DEBUG
int main() {
  const static char q[] = "test($x, $y) { return $x + $y - 100.0; }";
  LexString lex(q, sizeof(q));
  LexBase::LexTag tag;
  const char *name[] = {
    "LT_NULL", "LT_NL", "LT_TAG", "LT_BEGIN", "LT_BEND", "LT_OP", "LT_STRING", "LT_INT", "LT_DNUM", "LT_VAR", "LT_HTML"
  };
  while ((tag = lex.analyze()) != LexBase::LT_NULL) {
    printf("[%s", name[tag]);
    switch (tag) {
    case LexBase::LT_TAG:
    case LexBase::LT_VAR:
    case LexBase::LT_OP:
    case LexBase::LT_STRING:
    case LexBase::LT_HTML:
      printf(":%s", lex.token.c_str());
      break;

    case LexBase::LT_INT:
      printf(":%d", lex.itoken);
      break;

    case LexBase::LT_DNUM:
      printf(":%g", lex.dtoken);
      break;
    }
    printf("]");
  }
  return 0;
}
#endif //DEBUG

