#ifndef CONTENT_H_
#define CONTENT_H_

#include "lex.h"
#include <vector>
#include <list>
#include <unordered_map>
#include "exception.h"

namespace minosys {

class Content;
struct MinosysClassDef {
  std::vector<std::string> parentClass;
  std::vector<std::string> vars;
  std::unordered_map<std::string, Content *> members;
  ~MinosysClassDef();
  std::string toString(const std::string &name);
  std::string toStringParentClass();
};

class Content {
 public:
  LexBase::LexTag tag;
  std::string op;
  std::string label;
  int inum;
  double dnum;
  std::vector<std::string> arg;
  std::vector<Content *>pc;

  Content *next, *last;

  Content() : tag(LexBase::LT_NULL), next(NULL), last(this) {}
  Content(LexBase::LexTag t, std::string o) : tag(t), op(o) {
    next = NULL;
    last = this;
  }
  Content(int itoken) : tag(LexBase::LT_INT), inum(itoken) {
    next = NULL;
    last = this;
  }
  Content(double dtoken) : tag(LexBase::LT_DNUM), dnum(dtoken) {
    next = NULL;
    last = this;
  }
  ~Content();
  std::string toString();
  std::string toStringInt();
};

struct ContentToken {
  LexBase::LexTag tag;
  std::string token;
  int inum;
  double dnum;
  void init() {
    token.clear();
    inum = 0;
    dnum = 0.0;
  }
  ContentToken() {
    init();
  }
  ContentToken(LexBase::LexTag t) {
    init();
    this->tag = t;
  }
  ContentToken(int itoken) : ContentToken(LexBase::LT_INT) {
    this->inum = itoken;
  }
  ContentToken(double dtoken) : ContentToken(LexBase::LT_DNUM) {
    this->dnum = dtoken;
  }
  ContentToken(LexBase::LexTag t, LexBase *c) {
    init();
    this->tag = t;
    if (t == LexBase::LT_INT) {
      this->inum = c->itoken;
    } else if (t == LexBase::LT_DNUM) {
      this->dnum = c->dtoken;
    } else {
      this->token = c->token;
    }
  }
};

struct Label {
  int nest;
  Content *content;
  Label() : nest(0), content(NULL) {}
  Label(int n, Content *c) : nest(n), content(c) {}
};

class ContentTop {
 public:
  Content *top;
  Content *last;
  int nest;
  std::list<ContentToken> listContent;
  std::unordered_map<std::string, std::unordered_map<std::string, Label> > labels;
  Content *savedLHS;
  std::string parseFunc;
  std::vector<std::string> imports;
  std::unordered_map<std::string, Content *> funcs;
  std::unordered_map<std::string, MinosysClassDef *> defines;
  ContentTop() : top(NULL), last(NULL), savedLHS(NULL), nest(0) {}
  Content *yylex(LexBase *lex);
  std::string toStringImports();
  std::string toStringDefines();
  std::string toString();

 private:
  Content *yylex_block(LexBase *lex);
  Content *yylex_sentence(const std::string &label, LexBase *lex);
  void yylex_arg(Content *c, LexBase *lex);
  Content *yylex_eval(LexBase *lex);
  Content *yylex_eval3(LexBase *lex);
  int yylex_func(Content *t, LexBase *lex);
  Content *yylex_evalbody(LexBase *lex);
  Content *yylex_for(const std::string &label, LexBase *lex);
  Content *yylex_lhs(LexBase *lex);
  Content *yylex_rhs(LexBase *lex);
  Content *yylex_rhs2(Content *c, LexBase *lex);
  Content *yylex_comp(LexBase *lex);
  Content *yylex_shift(LexBase *lex);
  Content *yylex_arith(LexBase *lex);
  Content *yylex_term(LexBase *lex);
  Content *yylex_mono(LexBase *lex);
  Content *yylex_new(LexBase *lex);
  MinosysClassDef *yylex_class(LexBase *lex);
  int getContentToken(ContentToken &t, LexBase *lex);
  void setLabel(Content *t, const std::string &label);
};

} // minosys

#endif // CONTENT_H_

