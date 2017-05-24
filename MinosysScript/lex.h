#ifndef LEXBASE_H_
#define LEXBASE_H_

#include <cstdio>
#include <cstdio>
#include <string>
#include <unordered_map>

namespace minosys {

class LexBase {
 public:
  enum LexTag {
    LT_NULL = 0, LT_NL, LT_TAG, LT_BEGIN, LT_BEND, LT_OP, LT_STRING, LT_INT, LT_DNUM, LT_VAR, LT_HTML, LT_FUNC, LT_FUNCDEF, LT_FUNCHTML, LT_THIS, LT_SUPER, LT_IF, LT_ELSE, LT_FOR, LT_WHILE, LT_BREAK, LT_CONTINUE, LT_RETURN, LT_GLOBAL, LT_NEW, LT_CLASS, LT_IMPORT
  } lexTag;
  LexBase();
  std::string token;
  int itoken;
  double dtoken;
  LexTag analyze();

 protected:
  int uc, state, pushstate, rp;
  bool has_uc;
  FILE *f;
  virtual int getc() = 0;
  void ungetc(int c);

 private:
  std::unordered_map<std::string, LexTag> tagMap;
};

class LexString : public LexBase {
 public:
  LexString(const char *p, int len) {
    this->p = p;
    this->len = len;
    this->pos = 0;
  }

 private:
  const char *p;
  int pos, len;
  int getc() {
    if (has_uc) {
      has_uc = false;
      return uc;
    }
    if (pos == len) {
      return -1;
    }
    return (int)p[pos++] & 255;
  }
};

class LexFile : public LexBase {
 public:
  LexFile(std::FILE *f) {
    this->f = f;
  }

 private:
  std::FILE *f;
  int getc() {
    if (has_uc) {
      has_uc = false;
      return uc;
    }
    return std::fgetc(f);
  }
};

} // minosys

#endif // LEXBASE_H_
