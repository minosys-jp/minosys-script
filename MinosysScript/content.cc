#include "content.h"
#include <algorithm>

using namespace std;
using namespace minosys;

MinosysClassDef::~MinosysClassDef() {
  if (!members.empty()) {
    for(auto i = members.begin(); i != members.end(); ++i) {
      delete i->second;
    }
  }
}

string MinosysClassDef::toStringParentClass() {
  string s;
  for (auto i = this->parentClass.begin(); i != this->parentClass.end(); ++i) {
    if (i != this->parentClass.begin()) s += ".";
    s += *i;
  }
  return s;
}

string MinosysClassDef::toString(const string &name) {
  string s;
  string parent = this->toStringParentClass();
  s += string("class ") + name;
  if (!parent.empty()) {
    s += string(": ") + parent;
  }
  s += " {";
  for (auto i = vars.begin(); i != vars.end(); ++i) {
    s += string(",") + *i;
  }
  for (auto i = members.begin(); i != members.end(); ++i) {
    s += i->second->toString();
  }
  s += "}\n";
  return s;
}

Content::~Content() {
  if (this->next != this) {
    delete this->next;
  }
  for (int i = 0; i < pc.size(); ++i) {
    delete pc[i];
  }
}

int ContentTop::getContentToken(ContentToken &t, LexBase *lex) {
  if (!listContent.empty()) {
    t = listContent.front();
    listContent.pop_front();
  } else {
    LexBase::LexTag tag = lex->analyze();
    t = ContentToken(tag, lex);
    if (tag == LexBase::LT_NULL) return -1;
  }
  return 0;
}

Content *ContentTop::yylex(LexBase *lex) {
  Content *c;
  Content *t = NULL;
  while ((c = yylex_block(lex)) != NULL) {
    if (!t) {
      t = c;
    } else {
      t->last->next = c;
      t->last = c->last;
    }
  }
  return t;
}

Content *ContentTop::yylex_block(LexBase *lex) {
  LexBase::LexTag tag;
  ContentToken token, token2;
  Content *t = NULL;
  string labelname;

  // check label
  if (getContentToken(token, lex) < 0) return NULL;

  if (token.tag == LexBase::LT_TAG) {
    if (getContentToken(token2, lex) < 0) {
      listContent.push_back(token);
    } else if (token2.tag != LexBase::LT_OP || token2.token != ":") {
      listContent.push_back(token2);
      listContent.push_back(token);
    } else {
      labelname = token.token;
    }
  } else {
    listContent.push_back(token);
  }

  if (getContentToken(token, lex) >= 0) {
    if (token.tag == LexBase::LT_BEGIN) {
      this->nest++;
      t = new Content(LexBase::LT_BEGIN, "");
      t->label = labelname;
      Content *c = yylex(lex);
      if (c) {
        t->pc.push_back(c);
      }
      if (getContentToken(token, lex) < 0
        || token.tag != LexBase::LT_BEND) {
        delete t;
        t = NULL;
      }
      this->nest--;
    } else if (token.tag == LexBase::LT_BEND) {
      listContent.push_back(token);
    } else {
      listContent.push_back(token);
      t = yylex_sentence(labelname, lex);
    }
  }
  if (!t && !labelname.empty()) {
    t = new Content(LexBase::LT_NL, "");
    t->label = labelname;
  }
  if (!labelname.empty()) {
    this->labels[this->parseFunc][labelname] = Label(this->nest, t);
  }
  return t;
}

Content *ContentTop::yylex_sentence(const string &label, LexBase *lex) {
  Content *t = NULL;
  ContentToken token;

 loop:
  if (getContentToken(token, lex) < 0) {
    return NULL;
  }
  if (token.tag == LexBase::LT_NL) {
    t = new Content(token.tag, token.token);
    return t;
  }

  if (token.tag == LexBase::LT_CLASS) {
    if (getContentToken(token, lex) >= 0) {
      if (token.tag == LexBase::LT_TAG) {
        string cname = token.token;
        MinosysClassDef *def = yylex_class(lex);
        if (def) {
          this->defines[cname] = def;
        }
      } else {
        listContent.push_back(token);
      }
    }
    goto loop;
  } else if (token.tag == LexBase::LT_IMPORT) {
    if (getContentToken(token, lex) >= 0) {
      if (token.tag == LexBase::LT_STRING) {
        string name = token.token;
        if (getContentToken(token, lex) >= 0) {
          if (token.tag == LexBase::LT_NL) {
            this->imports.push_back(name);
          }
        }
      }
    }
    goto loop;
  } else if (token.tag == LexBase::LT_GLOBAL) {
    string range = token.token;
    if (getContentToken(token, lex) >= 0
      && token.tag == LexBase::LT_VAR) {
      string varname = token.token;
      if (getContentToken(token, lex) >= 0) {
        if (token.tag == LexBase::LT_NL) {
          t = new Content(LexBase::LT_TAG, range);
          t->pc.push_back(new Content(LexBase::LT_VAR, varname));
        } else if (token.tag == LexBase::LT_OP && token.token == "=") {
          t = new Content(LexBase::LT_TAG, range);
          t->pc.push_back(new Content(LexBase::LT_VAR, varname));
          t->pc.push_back(yylex_eval(lex));
          if (getContentToken(token, lex) < 0
            || token.tag == LexBase::LT_NL) {
            delete t;
            t = NULL;
          }
        }
      }
    }
  } else if (token.tag == LexBase::LT_IF) {
    Content *cond = yylex_eval(lex);
    Content *cif = yylex_block(lex);
    Content *celse = NULL;
    if (getContentToken(token, lex) >= 0) {
      if (token.tag == LexBase::LT_ELSE) {
        celse = yylex_block(lex);
      } else {
        listContent.push_back(token);
      }
    }

    if (cond && cif) {
      t = new Content(LexBase::LT_IF, "");
      t->pc.push_back(cond);
      t->pc.push_back(cif);
      t->label = label;
      if (celse) {
        t->pc.push_back(celse);
      }
    } else {
      if (cond) delete cond;
      if (cif) delete cif;
      if (celse) delete celse;
    }
  } else if (token.tag == LexBase::LT_FUNCDEF) {
    if (getContentToken(token, lex) >= 0
      && token.tag == LexBase::LT_TAG) {
      t = new Content(LexBase::LT_FUNCDEF, token.token);
      this->parseFunc = token.token;
      yylex_arg(t, lex);
      Content *def = yylex_block(lex);
      t->pc.push_back(def);
      this->funcs[this->parseFunc] = def;
      this->parseFunc = "";
    }
  } else if (token.tag == LexBase::LT_FOR) {
    t = yylex_for(label, lex);
  } else if (token.tag == LexBase::LT_WHILE) {
    Content *cond = yylex_eval(lex);
    Content *b = yylex_block(lex);
    if (cond && b) {
      t = new Content(LexBase::LT_WHILE, "");
      t->label = label; 
      t->pc.push_back(cond);
      t->pc.push_back(b);
    } else {
      if (cond) delete cond;
      if (b) delete b;
    }
  } else if (token.tag == LexBase::LT_BREAK) {
    if (getContentToken(token, lex) >= 0) {
      if (token.tag == LexBase::LT_NL) {
        t = new Content(LexBase::LT_BREAK, "");
      } else if (token.tag == LexBase::LT_STRING) {
        t = new Content(LexBase::LT_BREAK, token.token);
        if (getContentToken(token, lex) < 0
          || token.tag != LexBase::LT_NL) {
          delete t;
          return NULL;
        }
      }
    }
  } else if (token.tag == LexBase::LT_CONTINUE) {
    if (getContentToken(token, lex) >= 0) {
      if (token.tag == LexBase::LT_NL) {
        t = new Content(LexBase::LT_CONTINUE, "");
      } else if (token.tag == LexBase::LT_STRING) {
        t = new Content(LexBase::LT_CONTINUE, token.token);
        t->arg.push_back(token.token);
        if (getContentToken(token, lex) < 0
          || token.tag != LexBase::LT_NL) {
          delete t;
          return NULL;
        }
      }
    }
  } else if (token.tag == LexBase::LT_RETURN) {
    if (getContentToken(token, lex) >= 0) {
      if (token.tag == LexBase::LT_NL) {
        t = new Content(LexBase::LT_RETURN, "");
      } else {
        listContent.push_front(token);
        t = new Content(LexBase::LT_RETURN, "");
        t->pc.push_back(yylex_eval(lex));
        if (getContentToken(token, lex) < 0
          || token.tag != LexBase::LT_NL) {
          delete t;
          return NULL;
        }
      }
    }
  }

  if (!t) {
    listContent.push_back(token);
    Content *c = yylex_eval(lex);
    if (c) c->label = label;
    if (getContentToken(token, lex) >= 0) {
      if (token.tag == LexBase::LT_NL) {
        t = c;
      } else {
        listContent.push_back(token);
        delete c;
      }
    }
  }
  return t;
}

Content *ContentTop::yylex_for(const string &label, LexBase *lex) {
  ContentToken token;
  Content *t = NULL;

  if (getContentToken(token, lex) < 0) return NULL;
  if (token.tag != LexBase::LT_OP || token.token != "(") {
    listContent.push_back(token);
    return NULL;
  }

  Content *c1 = yylex_eval(lex);
  if (getContentToken(token, lex) < 0) return NULL;
  if (token.tag != LexBase::LT_NL) {
    listContent.push_back(token);
    return NULL;
  }
  Content *c2 = yylex_eval(lex);
  if (getContentToken(token, lex) < 0) return NULL;
  if (token.tag != LexBase::LT_NL) {
    listContent.push_back(token);
    return NULL;
  }
  Content *c3 = yylex_eval(lex);
  if (getContentToken(token, lex) < 0) return NULL;
  if (token.tag != LexBase::LT_OP && token.token != ")") {
    listContent.push_back(token);
    return NULL;
  }
  Content *b = yylex_block(lex);
  if (c1 && c2 && c3 && b) {
    t = new Content(LexBase::LT_TAG, "for");
    t->label = label;
    t->pc.push_back(c1);
    t->pc.push_back(c2);
    t->pc.push_back(c3);
    t->pc.push_back(b);
  } else {
    if (c1) delete c1;
    if (c2) delete c2;
    if (c3) delete c3;
    if (b) delete b;
  }
  return t;
}

void ContentTop::yylex_arg(Content *c, LexBase *lex) {
  ContentToken token;

  if (getContentToken(token, lex) < 0) return;
  if (token.tag != LexBase::LT_OP || token.token != "(") {
    listContent.push_back(token);
    return;
  }
  while (true) {
    if (getContentToken(token, lex) < 0) return;
    if (token.tag == LexBase::LT_VAR) {
      c->arg.push_back(token.token);
    } else if (token.tag == LexBase::LT_OP) {
      if (token.token == ")") break;
      if (token.token != ",") {
        listContent.push_back(token);
        break;
      }
    } else {
      listContent.push_back(token);
      break;
    }
  }
}

Content *ContentTop::yylex_eval(LexBase *lex) {
  ContentToken token;
  Content *t = NULL, *c1 = yylex_eval3(lex), *c2 = NULL, *c3 = NULL;
  if (getContentToken(token, lex) < 0) return c1;
  if (token.tag == LexBase::LT_OP && token.token == "?") {
    c2 = yylex_eval(lex);
    if (getContentToken(token, lex) < 0) return c1;
    if (token.tag == LexBase::LT_OP && token.token == ":") {
      c3 = yylex_eval(lex);
      t = new Content(LexBase::LT_OP, "?");
      t->pc.push_back(c1);
      t->pc.push_back(c2);
      t->pc.push_back(c3);
    }
  } else {
    listContent.push_back(token);
  }
  if (!t) {
    if (c2) delete c2;
    if (c3) delete c3;
    t = c1;
  }
  return t;
}

Content *ContentTop::yylex_eval3(LexBase *lex) {
  ContentToken token;
  Content *t = NULL, *c1 = yylex_evalbody(lex);

  while (getContentToken(token, lex) >= 0) {
    if (token.tag != LexBase::LT_OP) {
      listContent.push_back(token);
      break;
    }
    if (token.token == "[") {
      Content *c2 = yylex_eval(lex);
      if (getContentToken(token, lex) < 0) break;
      if (token.tag != LexBase::LT_OP || token.token != "]") {
        listContent.push_back(token);
        break;
      }
      if (!t) {
        t = new Content(LexBase::LT_OP, "[");
        t->pc.push_back(c1);
      }
      t->pc.push_back(c2);
    } else if (token.token == "(") {
      listContent.push_back(token);
      t = new Content(LexBase::LT_FUNC, "");
      t->pc.push_back(c1);
      if (yylex_func(t, lex) < 0) {
        delete t;
        return NULL;
      }
    } else {
      listContent.push_back(token);
      break;
    }
  }

  if (!t) {
    t = c1;
  }
  return t;
}

int ContentTop::yylex_func(Content *t, LexBase *lex) {
  ContentToken token;

  if (getContentToken(token, lex) < 0) return -1;
  if (token.tag != LexBase::LT_OP || token.token != "(") {
    listContent.push_back(token);
    return -1;
  }
  Content *c = NULL;
  while (true) {
    c = yylex_eval(lex);
    if (getContentToken(token, lex) < 0) return -1;
    if (token.tag == LexBase::LT_OP) {
      if (token.token == ")") {
        break;
      }
      if (token.token != ",") {
        listContent.push_back(token);
        break;
      }
      t->pc.push_back(c);
      c = NULL;
    } else {
      listContent.push_back(token);
      delete c;
      c = NULL;
      break;
    }
  }
  if (c) {
    t->pc.push_back(c);
  }
  return 0;
}

Content *ContentTop::yylex_evalbody(LexBase *lex) {
  ContentToken token;
  Content *t = NULL;
  if (getContentToken(token, lex) < 0) return NULL;

  // 終端記号
  if (token.tag == LexBase::LT_OP
   && (token.token == ")" || token.token == "]" || token.token == ",")) {
    listContent.push_front(token);
    return NULL;
  }
  
  //単項演算子 
  if (token.tag == LexBase::LT_OP && token.token == "+") {
    t = yylex_rhs(lex);
  } else if (token.tag == LexBase::LT_OP && token.token == "-") {
    t = new Content(LexBase::LT_OP, "-");
    t->pc.push_back(yylex_rhs(lex));
  } else if (token.tag == LexBase::LT_OP && token.token == "++") {
    t = new Content(LexBase::LT_OP, "++x");
    t->pc.push_back(yylex_lhs(lex));
  } else if (token.tag == LexBase::LT_OP && token.token == "--") {
    t = new Content(LexBase::LT_OP, "--x");
    t->pc.push_back(yylex_lhs(lex));
  } else if (token.tag == LexBase::LT_OP && token.token == "!") {
    t = new Content(LexBase::LT_OP, "!");
    t->pc.push_back(yylex_rhs(lex));
  } else if (token.tag == LexBase::LT_OP && token.token == "~") {
    t = new Content(LexBase::LT_OP, "~");
    t->pc.push_back(yylex_rhs(lex));
  } else if (token.tag == LexBase::LT_OP && token.token == "new") {
    t = yylex_new(lex);
  } else if (token.tag == LexBase::LT_THIS || token.tag == LexBase::LT_SUPER || token.tag == LexBase::LT_VAR || token.tag == LexBase::LT_TAG) {
    listContent.push_back(token);
    Content *lhs = yylex_lhs(lex);
    if (getContentToken(token, lex) >= 0 && token.tag == LexBase::LT_OP) {
      if (token.token == "++") {
        t = new Content(LexBase::LT_OP, "x++");
        t->pc.push_back(lhs);
      } else if (token.token == "--") {
        t = new Content(LexBase::LT_OP, "x--");
        t->pc.push_back(lhs);
      } else if (token.token == "=" || token.token == "+="
        || token.token == "-=" || token.token == "*="
        || token.token == "-=" || token.token == "&="
        || token.token == "|=" || token.token == "^="
        || token.token == "<<=" || token.token == ">>=") {
        // 代入演算子
        t = new Content(LexBase::LT_OP, token.token);
        t->pc.push_back(lhs);
        t->pc.push_back(yylex_eval(lex));
      } else {
        listContent.push_front(token);
        t = yylex_rhs2(lhs, lex);
      }
    } else {
      if (lhs) {
        listContent.push_front(token);
        t = yylex_rhs2(lhs, lex);
      } else {
        listContent.push_back(token);
      }
    }
  } else {
    listContent.push_back(token);
  }

  if (!t) {
    t = yylex_rhs(lex);
  }
  return t;
}

Content *ContentTop::yylex_lhs(LexBase *lex) {
  ContentToken token;
  Content *t = NULL;

  if (getContentToken(token, lex) < 0) return NULL;
  t = new Content(token.tag, token.token);

  while (getContentToken(token, lex) >= 0) {
     if (token.tag == LexBase::LT_OP) {
       if (token.token == "[") {
         t->pc.push_back(yylex_eval(lex));
         if (getContentToken(token, lex) < 0
           || token.tag != LexBase::LT_OP || token.token != "]") {
           delete t;
           t = NULL;
           break;
         }
       } else if (token.token == ".") {
         if (getContentToken(token, lex) < 0) break;
         Content *t2 = new Content(LexBase::LT_OP, ".");
         t2->pc.push_back(t);
         t2->pc.push_back(new Content(token.tag, token.token));
         t = t2;
       } else {
         listContent.push_back(token);
         break;
       }
     } else {
       listContent.push_back(token);
       break;
     }
  }
  return t;
}

Content *ContentTop::yylex_rhs(LexBase *lex) {
  ContentToken token;
  Content *c1 = yylex_comp(lex);

  if (getContentToken(token, lex) < 0) return c1;
  if (token.tag == LexBase::LT_OP) {
    if (token.token == "&" || token.token == "|"
      || token.token == "^" || token.token == "&&"
      || token.token == "||") {
      Content *c2 = yylex_eval(lex);
      if (c2) {
        Content *t = new Content(LexBase::LT_OP, token.token);
        t->pc.push_back(c1);
        t->pc.push_back(c2);
        return t;
      }
    }
    listContent.push_back(token);
  } else {
    listContent.push_back(token);
  }
  return c1;
}

Content *ContentTop::yylex_rhs2(Content *c, LexBase *lex) {
  this->savedLHS = c;
  return yylex_rhs(lex);
}

Content *ContentTop::yylex_comp(LexBase *lex) {
  ContentToken token;
  Content *c1 = yylex_arith(lex);

  if (getContentToken(token, lex) < 0) return c1;
  if (token.tag != LexBase::LT_OP
   || (token.token != "<" && token.token != "<="
     && token.token != ">" && token.token != ">="
     && token.token != "!=" && token.token != "==")) {
    listContent.push_back(token);
    return c1;
  }
  string opname = token.token;
  Content *c2 = yylex_eval(lex);
  if (c2) {
    Content *t = new Content(LexBase::LT_OP, opname);
    t->pc.push_back(c1);
    t->pc.push_back(c2);
    return t;
  }
  return c1;
}

Content *ContentTop::yylex_arith(LexBase *lex) {
  ContentToken token;
  Content *c1 = yylex_term(lex);

  if (getContentToken(token, lex) < 0) return c1;
  if (token.tag == LexBase::LT_OP) {
    if (token.token == "+" || token.token == "-") {
      Content *c2 = yylex_eval(lex);
      if (c2) {
        Content *t = new Content(LexBase::LT_OP, token.token);
        t->pc.push_back(c1);
        t->pc.push_back(c2);
        return t;
      }
    }
  }
  listContent.push_back(token);
  return c1;
}

Content *ContentTop::yylex_term(LexBase *lex) {
  ContentToken token;
  Content *c1 = yylex_mono(lex);

  if (getContentToken(token, lex) < 0) return c1;
  if (token.tag == LexBase::LT_OP) {
    if (token.token == "*" || token.token == "/" || token.token == "%") {
      Content *c2 = yylex_eval(lex);
      if (c2) {
        Content *t = new Content(LexBase::LT_OP, token.token);
        t->pc.push_back(c1);
        t->pc.push_back(c2);
        return t;
      }
    }
  }
  listContent.push_back(token);
  return c1;
}

Content *ContentTop::yylex_mono(LexBase *lex) {
  ContentToken token;
  Content *t = NULL;

  if (savedLHS) {
    Content *c = savedLHS;
    savedLHS = NULL;
    return c;
  }
  if (getContentToken(token, lex) < 0) return t;
  if (token.tag == LexBase::LT_INT) {
    t = new Content(token.inum);
  } else if (token.tag == LexBase::LT_DNUM) {
    t = new Content(token.dnum);
  } else if (token.tag == LexBase::LT_STRING) {
    t = new Content(token.tag, token.token);
  } else if (token.tag == LexBase::LT_VAR || token.tag == LexBase::LT_TAG || token.tag == LexBase::LT_THIS || token.tag == LexBase::LT_SUPER) {
    ContentToken token2;
    if (getContentToken(token2, lex) < 0) {
      t = new Content(token.tag, token.token);
    } else if (token2.tag != LexBase::LT_OP || token2.token != ".") {
      listContent.push_back(token2);
      t = new Content(token.tag, token.token);
    } else {
      t = new Content(LexBase::LT_OP, ".");
      t->pc.push_back(new Content(token.tag, token.token));
      t->pc.push_back(yylex_mono(lex));
    }
  } else if (token.tag == LexBase::LT_OP && token.token == "(") {
    t = yylex_eval(lex);
    if (getContentToken(token, lex) >= 0) {
      if (token.tag != LexBase::LT_OP || token.token != ")") {
        listContent.push_back(token);
      }
    }
  } else if (token.tag == LexBase::LT_BEGIN) {
    t = new Content(LexBase::LT_OP, "array");
    while (getContentToken(token, lex) >= 0) {
      if (token.tag == LexBase::LT_BEND) break;
      listContent.push_back(token);
      t->pc.push_back(yylex_eval(lex));
      if (getContentToken(token, lex) < 0) break;
      if (token.tag == LexBase::LT_BEND) break;
      if (token.tag == LexBase::LT_OP) {
        if (token.token == ",") {
          // do nothing
        } else {
          listContent.push_back(token);
          break;
        }
      } else {
        listContent.push_back(token);
        break;
      }
    }
  }
  return t;
}

MinosysClassDef *ContentTop::yylex_class(LexBase *lex) {
  MinosysClassDef *def = NULL;
  ContentToken token;
  vector<string> pnames;

  if (getContentToken(token, lex) < 0) return NULL;
  if (token.tag == LexBase::LT_OP) {
    if (token.token == ":") {
      // parent Class
      while (true) {
        if (getContentToken(token, lex) < 0) return NULL;
        if (token.tag == LexBase::LT_TAG) {
          pnames.push_back(token.token);
        } else if (token.tag == LexBase::LT_BEGIN) {
          listContent.push_back(token);
          break;
        } else if (token.tag != LexBase::LT_OP || token.token != ".") {
          listContent.push_back(token);
          return NULL;
        }
      }
    } else {
      listContent.push_back(token);
      return NULL;
    }
  } else if (token.tag == LexBase::LT_BEGIN) {
    listContent.push_back(token);
  } else {
    listContent.push_back(token);
    return NULL;
  }
  def = new MinosysClassDef();
  if (!pnames.empty()) {
    def->parentClass = pnames;
  }

  do {
    if (getContentToken(token, lex) < 0) break;
    if (token.tag != LexBase::LT_BEGIN) break;
    while (1) {
      if (getContentToken(token, lex) < 0) goto loop_out;
      if (token.tag == LexBase::LT_BEND) {
        return def;
      }
      if (token.tag == LexBase::LT_VAR) {
        string varname = token.token;
        if (getContentToken(token, lex) < 0
          || token.tag != LexBase::LT_NL) goto loop_out;
        def->vars.push_back(varname);
      } else if (token.tag == LexBase::LT_TAG && token.token == "function") {
        if (getContentToken(token, lex) < 0
          || token.tag != LexBase::LT_TAG) goto loop_out;
        string memname = token.token;
        Content *c = new Content(LexBase::LT_FUNCDEF, memname);
        yylex_arg(c, lex);
        Content *b = yylex_block(lex);
        if (b) {
          c->pc.push_back(b);
          def->members[memname] = c;
        } else {
          delete c;
          goto loop_out;
        }
      } else {
        goto loop_out;
      }
    }
  } while(0);
 loop_out:
  delete def;
  return NULL;
}

Content *ContentTop::yylex_new(LexBase *lex) {
  ContentToken token;
  Content *t = new Content(LexBase::LT_OP, "new");

  if (getContentToken(token, lex) >= 0) {
    if (token.tag == LexBase::LT_TAG) {
      t->arg.push_back(token.token);
    }
    while (getContentToken(token, lex) >= 0) {
      if (token.tag == LexBase::LT_NL) {
        listContent.push_back(token);
        break;
      }
      if (token.tag == LexBase::LT_OP) {
        if (token.token == "."
         && getContentToken(token, lex) >= 0
         && token.tag == LexBase::LT_TAG) {
           t->arg.push_back(token.token);
        } else if (token.token == "(") {
          listContent.push_back(token);
          if (yylex_func(t, lex) < 0) {
            delete t;
            t = NULL;
          }
          break;
        }
      }
    }
  }
  return t;
}

string ContentTop::toString() {
  Content *t = this->top;
  string si = toStringImports();
  string sd = toStringDefines();
  string sc;
  if (t) {
    sc = t->toString();
  }
  string s = si + sd + sc + "\n";
  return s;
}

string ContentTop::toStringImports() {
  string s = "import {";
  for (auto i = imports.begin(); i != imports.end(); ++i) {
    if (i != imports.begin()) s += ",";
    s += *i;
  }
  s += "}\n";
  return s;
}

string ContentTop::toStringDefines() {
  string s;
  if (!defines.empty()) {
    for (auto i = defines.begin(); i != defines.end(); ++i) {
      s += i->second->toString(i->first);
    }
  }
  return s;
}

string Content::toString() {
  string s;
  Content *t = this;

  do {
    s += t->toStringInt();
    t = t->next;
  } while(t);
  return s;
}

string Content::toStringInt() {
  string s;

  if (!this->label.empty()) {
    s += this->label + ":";
  }
  s += "[";
  switch (this->tag) {
  case LexBase::LT_VAR:
    s += string("VAR:") + this->op;
    break;

  case LexBase::LT_TAG:
    s += string("TAG:") + this->op;
    break;

  case LexBase::LT_OP:
    s += string("OP:") + this->op;
    break;

  case LexBase::LT_FUNCDEF:
    s += string("FUNCDEF:") + this->op;
    break;

  case LexBase::LT_FUNC:
    s += string("FUNC:") + this->op;
    break;

  case LexBase::LT_BEGIN:
    s += string("BEGIN:");
    break;

  case LexBase::LT_NL:
    s += string("NL");
    break;

  case LexBase::LT_STRING:
    s += string("\"") + this->op + "\"";
    break;

  case LexBase::LT_INT:
    s += to_string(this->inum);
    break;

  case LexBase::LT_DNUM:
    s += to_string(this->dnum);
    break;

  case LexBase::LT_IF: s+= "if"; break;
  case LexBase::LT_FOR: s += "for"; break;
  case LexBase::LT_WHILE: s += "while"; break;
  case LexBase::LT_CONTINUE: s += string("continue:") + this->op; break;
  case LexBase::LT_BREAK: s += string("break:") + this->op; break;
  case LexBase::LT_NEW: s += "new"; break;
  case LexBase::LT_RETURN: s += "return"; break;

  default:
    s += string("???:") + this->op;
  }
  s += "]";
  if (!this->arg.empty()) {
    s += "(";
    for (auto p = this->arg.begin(); p != this->arg.end(); ++p) {
      if (p != this->arg.begin()) s += ",";
      s += *p;
    }
    s += ")";
  }
  if (!this->pc.empty()) {
    for (auto p = this->pc.begin(); p != this->pc.end(); ++p) {
      s += "[";
      s += (*p)->toString();
      s += "]";
    }
  }

  return s;
}

