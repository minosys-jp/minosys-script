#include "engine.h"
#include "minosysscr_api.h"
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <iostream>

using namespace std;
using namespace minosys;

bool VarKey::operator == (const VarKey &k) const {
  if (vtype != k.vtype) return false;
  switch (vtype) {
  case VT_INT: return u.inum == k.u.inum;
  case VT_DNUM: return u.dnum == k.u.dnum;
  case VT_STRING: return *u.str == *k.u.str;
  }
  return false;
}

bool VarKey::operator != (const VarKey &k) const {
  return !(*this == k);
}

string VarKey::toString() const {
  switch (vtype) {
  case VT_INT:
    return to_string(this->u.inum);

  case VT_DNUM:
    return to_string(this->u.dnum);

  case VT_STRING:
    return *this->u.str;
  }
  return "";
}

Var::Var(const Var &v) : vtype(v.vtype) {
  switch (vtype) {
  case VT_NULL:
    break;

  case VT_INT:
    this->inum = v.inum;
    break;

  case VT_DNUM:
    this->dnum = v.dnum;
    break;

  case VT_STRING:
    this->str = v.str;
    break;

  case VT_INST:
    this->inst = v.inst;
    break;

  case VT_POINTER:
    this->pointer = v.pointer;
    break;

  case VT_ARRAY:
    this->arrayhash = v.arrayhash;
    break;

  case VT_FUNC:
    this->func = v.func;
    break;

  case VT_MEMBER:
    this->member = v.member;
    break;
  }
}

Var & Var::operator = (const Var &v) {
  vtype = v.vtype;
  switch (vtype) {
  case VT_NULL:
    this->inum = 0;
    break;

  case VT_INT:
    this->inum = v.inum;
    break;

  case VT_DNUM:
    this->dnum = v.dnum;
    break;

  case VT_STRING:
    this->str = v.str;
    break;

  case VT_INST:
    this->inst = v.inst;
    break;

  case VT_POINTER:
    this->pointer = v.pointer;
    break;

  case VT_ARRAY:
    this->arrayhash = v.arrayhash;
    break;
  }
}

shared_ptr<Var> Var::clone() {
  switch (vtype) {
  case VT_NULL:
    return make_shared<Var>();

  case VT_INT:
    return make_shared<Var>(inum);

  case VT_DNUM:
    return make_shared<Var>(dnum);

  case VT_STRING:
    return make_shared<Var>(str);

  case VT_INST:
    return make_shared<Var>(inst);

  case VT_POINTER:
    {
      shared_ptr<Var> v = make_shared<Var>();
      v->vtype = VT_POINTER;
      v->pointer = pointer;
      return v;
    }

  case VT_ARRAY:
    return make_shared<Var>(arrayhash);

  case VT_FUNC:
    return make_shared<Var>(func);

  case VT_MEMBER:
    return make_shared<Var>(member);
  }
  return make_shared<Var>();
}

bool Var::isTrue() const {
  switch(vtype) {
  case VT_NULL:
    return false;

  case VT_INT:
    return inum != 0;

  case VT_DNUM:
    return dnum != 0.0;

  case VT_STRING:
    return str != "";

  case VT_INST:
    return inst.get() != NULL;

  case VT_ARRAY:
    return !arrayhash.empty();

  case VT_POINTER:
    return pointer != NULL;

  case VT_FUNC:
    return func.first.empty() && func.second.empty();

  case VT_MEMBER:
    return !member.first && member.second.empty();

  default:
    return false;
  }
}

bool Var::operator == (const Var &v) const {
  switch (this->vtype) {
  case VT_NULL:
    return v.vtype == VT_NULL;

  case VT_INT:
    switch (v.vtype) {
    case VT_INT:
      return this->inum == v.inum;

    case VT_DNUM:
      return this->inum == v.dnum;
    }
    break;

  case VT_DNUM:
    switch (v.vtype) {
    case VT_INT:
      return this->dnum == v.inum;

    case VT_DNUM:
      return this->dnum == v.dnum;
    }
    break;

  case VT_STRING:
    if (v.vtype == VT_STRING) {
      return this->str == v.str;
    }
    break;

  case VT_INST:
    if (v.vtype == VT_INST) {
      return this->inst == v.inst;
    }
    break;

  case VT_POINTER:
    if (v.vtype == VT_POINTER) {
      return this->pointer == v.pointer;
    }
    break;

  case VT_ARRAY:
    if (v.vtype == VT_ARRAY) {
      return this->arrayhash == v.arrayhash;
    }
    break;

  case VT_FUNC:
    if (v.vtype == VT_FUNC) {
      return this->func == v.func;
    }
    break;

  case VT_MEMBER:
    if (v.vtype == VT_MEMBER) {
      return this->member == v.member;
    }
    break;
  }

  return false;
}

Var::~Var() {
}


#define OPMAP(cc,name) opmap[cc] = [](PackageMinosys *p, Content *c){ return p->eval_op_##name(c); }
#define BUILTINMAP(map,cc,name) map[cc] = [](PackageMinosys *p, const vector<shared_ptr<Var> > &args) { return p->func##name (args); }
#undef BUILTIN
#define BUILTIN(name) shared_ptr<Var> PackageMinosys::func##name (const vector<shared_ptr<Var> > &args)

PackageMinosys::PackageMinosys() {
  OPMAP(".", dot);
  OPMAP("?", 3term);
  OPMAP("=", assign);
  OPMAP("+=", assignplus);
  OPMAP("*=", assignmultiply);
  OPMAP("-=", assignminus);
  OPMAP("/=", assigndiv);
  OPMAP("%=", assignmod);
  OPMAP("&=", assignand);
  OPMAP("|=", assignor);
  OPMAP("^=", assignxor);
  OPMAP("!", monoNot);
  OPMAP("~", negate);
  OPMAP("-m", monoMinus);
  OPMAP("++x", preIncr);
  OPMAP("x++", postIncr);
  OPMAP("--x", preDecr);
  OPMAP("x--", postDecr);
  OPMAP("<", lt);
  OPMAP("<=", lteq);
  OPMAP(">", gt);
  OPMAP(">=", gteq);
  OPMAP("!=", neq);
  OPMAP("==", eq);
  OPMAP("+", plus);
  OPMAP("-", minus);
  OPMAP("*", multiply);
  OPMAP("/", div);
  OPMAP("%", mod);
  OPMAP("&", and);
  OPMAP("|", or);
  OPMAP("^", xor);
  OPMAP("&&", logand);
  OPMAP("||", logor);
  OPMAP("[", leftarray);

  BUILTINMAP(builtinmap, "type", type);
  BUILTINMAP(builtinmap, "convert", convert);
  BUILTINMAP(builtinmap, "print", print);
  BUILTINMAP(builtinmap, "exit", exit);

  BUILTINMAP(stringmap, "at", at);
  BUILTINMAP(stringmap, "empty", empty);
  BUILTINMAP(stringmap, "length", length);
  BUILTINMAP(stringmap, "index", index);
  BUILTINMAP(stringmap, "rindex", rindex);
  BUILTINMAP(stringmap, "substr", substr);
}

PackageMinosys::~PackageMinosys() {
  delete top;
}

// パッケージ関数呼び出し
shared_ptr<Var> PackageMinosys::start(const string &fname, vector<shared_ptr<Var> > &args) {
  auto p = top->funcs.find(fname);
 if (p == top->funcs.end()) {
    // ビルトイン関数
    auto pb = builtinmap.find(fname);
    if (pb != builtinmap.end()) {
      return (pb->second)(this, args);
    }
    // string 固有関数
    if (!args.empty() || args.at(0)->vtype == VT_STRING) {
      auto ps = stringmap.find(fname);
      if (ps != stringmap.end()) {
        return (ps->second)(this, args);
      }
    }
 } else {
    Content *c = p->second;
    unordered_map<string, shared_ptr<Var> > amap;

    // TODO: 仮引数に過不足がある場合はデフォルト推定する
    if (c->arg.size() != args.size()) {
cout << "c->arg:" << c->arg.size() << ", args:" << args.size() << endl;
      throw RuntimeException(903, string("Arg size not matched:") + fname);
    }

    for (int i = 0; i < args.size(); ++i) {
      amap[c->arg[i]] = args[i];
    }
    eng->varmark.push_back(eng->vars.size());
    eng->topmark.push_back(eng->paramstack.size());
    eng->callmark.push_back(eng->callstack.size());
    eng->vars.push_back(amap);
    shared_ptr<Var> rv = callfunc(fname, c->pc.at(0));
    if (eng->topmark.back() > eng->paramstack.size()) {
      eng->paramstack.erase(
        eng->paramstack.begin() + eng->topmark.back(),
        eng->paramstack.end() - eng->paramstack.size()
      );
    }
    eng->vars.erase(
      eng->vars.begin() + eng->varmark.back(),
      eng->vars.end()
    );
    eng->callstack.erase(
      eng->callstack.begin() + eng->callmark.back(),
      eng->callstack.end()
    );
    eng->varmark.pop_back();
    eng->topmark.pop_back();
    eng->callmark.pop_back();
    return rv;
  }
  throw RuntimeException(900, string("Unknown function/method:") + fname);
}

// 関数呼び出し
shared_ptr<Var> PackageMinosys::callfunc(const string &fname, Content *c) {
  while (c) {
    bool redo = false;
    switch (c->tag) {
    case LexBase::LT_BEGIN:
      if (!c->pc.empty()) {
        eng->callstack.push_back(c);
        c = c->pc.at(0);
        redo = true;
      }
      break;

    case LexBase::LT_IF:
      {
        shared_ptr<Var> r = evaluate(c->pc.at(0));
        if (r && r.get()->isTrue()) {
          eng->callstack.push_back(c);
          c = c->pc.at(1);
          redo = true;
        } else if (c->pc.size() == 3) {
          eng->callstack.push_back(c);
          c = c->pc.at(2);
          redo = true;
        }
      }
      break;

    case LexBase::LT_FOR:
      {
        evaluate(c->pc.at(0));
        shared_ptr<Var> r = evaluate(c->pc.at(1));
        if (r && r->isTrue()) {
          eng->callstack.push_back(c);
          c = c->pc.at(3);
          redo = true;
        }
      }
      break;

    case LexBase::LT_WHILE:
      {
        shared_ptr<Var> r = evaluate(c->pc.at(0));
        if (r && r.get()->isTrue()) {
          eng->callstack.push_back(c);
          c = c->pc.at(1);
          redo = true;
        }
      }
      break;

    case LexBase::LT_BREAK:
      {
        auto p = top->labels.find(fname);
        if (p != top->labels.end() && !c->op.empty()) {
          auto pl = p->second.find(c->pc.at(0)->op);
          if (pl != p->second.end()) {
            int mark = eng->callstack.size() - eng->callmark.back() - pl->second.nest;
            if (mark == 0) {
              eng->callstack.erase(eng->callstack.begin() + pl->second.nest, eng->callstack.end());
              c = pl->second.content;
              break;
            }
          }
        }
        if (eng->callstack.size() > eng->callmark.back()) {
          c = eng->callstack.back();
          eng->callstack.pop_back();
        }
      }
      break;

    case LexBase::LT_CONTINUE:
      {
        auto p = top->labels.find(fname);
        if (p != top->labels.end() && !c->op.empty()) {
          auto pl = p->second.find(c->pc.at(0)->op);
          if (pl != p->second.end()) {
            int mark = eng->callstack.size() - eng->callmark.back() - pl->second.nest;
            if (mark == 0) {
              eng->callstack.erase(eng->callstack.begin() + pl->second.nest, eng->callstack.end());
              c = pl->second.content;
              redo = true;
              break;
            }
          }
        }
        if (eng->callstack.size() > eng->callmark.back()) {
          c = eng->callstack.back();
          eng->callstack.pop_back();
          redo = true;
        }
      }
      break;


    case LexBase::LT_RETURN:
      if (c->pc.size() >= 1) {
        return evaluate(c->pc.at(0));
      }
      return make_shared<Var>();

    default: // 演算子
      evaluate(c);
    }
    if (redo) {
      continue;
    }
    c = c->next;
    if (!c && eng->callstack.size() > eng->callmark.back()) {
      c = eng->callstack.back();
      eng->callstack.pop_back();
      if (c->tag == LexBase::LT_FOR) {
        evaluate(c->pc.at(2));
        shared_ptr<Var> r = evaluate(c->pc.at(1));
        if (r && r.get()->isTrue()) {
          eng->callstack.push_back(c);
          c = c->pc.at(3);
        } else {
          c = c->next;
        }
      } else if (c->tag == LexBase::LT_WHILE) {
        shared_ptr<Var> r = evaluate(c->pc.at(0));
        if (r && r.get()->isTrue()) {
          eng->callstack.push_back(c);
          c = c->pc.at(1);
        } else {
          c = c->next;
        }
      } else {
        c = c->next;
      }
    }
  }
  return make_shared<Var>();
}

// 変数型を返す
BUILTIN(type) {
  int vtype = -1;
  if (!args.empty()) {
    vtype = (int)args[0]->vtype;
  }
  return make_shared<Var>(vtype);
}

// 変数間の型変換
BUILTIN(convert) {
  if (args.size() < 2
    || args[1]->vtype != VT_INT) {
    return args[0]->clone();
  } else {
    switch (args[1]->inum) {
    case VT_INT:
      switch (args[0]->vtype) {
      case VT_INT:
        return make_shared<Var>(args[0]->inum);

      case VT_DNUM:
        return make_shared<Var>((int)args[0]->dnum);

      case VT_STRING:
        return make_shared<Var>(atoi(args[0]->str.c_str()));

      default:
        return args[0]->clone();
      }
      break;

    case VT_DNUM:
      switch (args[0]->vtype) {
      case VT_INT:
        return make_shared<Var>((double)args[0]->inum);

      case VT_DNUM:
        return make_shared<Var>(args[0]->dnum);

      case VT_STRING:
        return make_shared<Var>(atof(args[0]->str.c_str()));
        break;

      default:
        return args[0]->clone();
      }
      break;

    case VT_STRING:
      switch (args[0]->vtype) {
      case VT_INT:
        return make_shared<Var>(to_string(args[0]->inum));

      case VT_DNUM:
        return make_shared<Var>(to_string(args[0]->dnum));

      case VT_STRING:
        return make_shared<Var>(args[0]->str);

      default:
        return args[0]->clone();
      }
      break;

    default:
      return args[0]->clone();
    }
  }
  return make_shared<Var>();
}

// print 関数; 表示文字数を返す
BUILTIN(print) {
  int count = 0, len = 0;

  for (auto p = args.begin(); p != args.end(); ++p) {
    switch ((*p)->vtype) {
    case VT_INT:
      len += printf("%d", (*p)->inum);
      break;

    case VT_DNUM:
      len += printf("%g", (*p)->dnum);
      break;

    case VT_STRING:
      len += printf("%.*s", (int)(*p)->str.size(), (*p)->str.data());
      break;

    case VT_INST:
      {
        vector<shared_ptr<Var> > args;
        shared_ptr<Var> r = this->start("toString", args);
        if (r) {
          vector<shared_ptr<Var> > args;
          shared_ptr<Var> v(r->clone());
          args.push_back(v);
          shared_ptr<Var> vr = funcprint(args);
          if (vr->vtype == VT_INT) {
            len += vr->inum;
          }
        }
      }
      break;

    case VT_ARRAY:
      len += printf("{");
      for (auto pc = (*p)->arrayhash.begin(); pc != (*p)->arrayhash.end(); ++pc, ++count) {
        if (count) {
          len += printf(",");
        }
        string s = pc->first.toString();
        len += printf("%.*s: ", (int)s.size(), s.data());
        vector<shared_ptr<Var> > args;
        args.push_back(pc->second);
        shared_ptr<Var> vr = funcprint(args);
        if (vr->vtype == VT_INT) {
          len += vr->inum;
        }
      }
      len += printf("}");
      break;

    case VT_FUNC:
      {
        const string &pac = (*p)->func.first;
        const string &fname = (*p)->func.second;
        if (pac.empty()) {
          len += printf("%.*s", (int)fname.size(), fname.data());
        } else {
          len += printf("%.*s.%.*s", (int)pac.size(), pac.data(), (int)fname.size(), fname.data());
        }
      }
      break;

    case VT_MEMBER:
      {
        vector<shared_ptr<Var> > args;
        args.push_back((*p)->member.first);
        shared_ptr<Var> vr =  funcprint(args);
        if (vr->vtype == VT_INT) {
          len += vr->inum;
        }
        string &mem = (*p)->member.second;
        len += printf(".%.*s", (int)mem.size(), mem.data());
      }
      break;

    default:
      ;
    }
  }
  return make_shared<Var>(len);
}

// 終了関数
BUILTIN(exit) {
  int code = 0;
  if (!args.empty()) {
    switch ((*args[0]).vtype) {
    case VT_INT:
      code = (*args[0]).inum;
      break;

    case VT_DNUM:
      code = (int)(*args[0]).dnum;
      break;

    case VT_STRING:
      code = atoi((*args[0]).str.c_str());
      break;
    }
  }
  throw new ExitException(code);
}

// string: s.at(pos)
BUILTIN(at) {
  if (args.size() == 2) {
    shared_ptr<Var> a2 = args.at(1);
    int pos = 0;
    switch (a2->vtype) {
    case VT_INT:
      pos = a2->inum;
      break;

    case VT_DNUM:
      pos = (int)a2->dnum;
      break;

    default:
      throw new RuntimeException(1005, "illegal argument");
    }
    shared_ptr<Var> a1 = args.at(0);
    if (-pos > 0 && -pos <= a1->str.size()) {
      pos = a1->str.size() + pos;
    }
    if (pos >= 0 && pos < a1->str.size()) {
      return make_shared<Var>(a1->str.substr(pos, 1));
    }
    return make_shared<Var>("");
  }
  throw new RuntimeException(1005, "illegal argument");
}

// string: s.empty()
BUILTIN(empty) {
  return make_shared<Var>(args.at(0)->str.empty() ? 1 : 0);
}

// string: s.length()
BUILTIN(length) {
  return make_shared<Var>((int)(args.at(0)->str.size()));
}

// string: s.index(s2, [start])
BUILTIN(index) {
  if (args.size() >= 2) {
    int pos = 0;
    shared_ptr<Var> a1 = args.at(0);
    shared_ptr<Var> a2 = args.at(1);
    if (a2->vtype != VT_STRING) {
      return make_shared<Var>(-1);
    }
    if (args.size() > 2) {
      shared_ptr<Var> a3 = args.at(2);
      switch (a3->vtype) {
      case VT_INT:
        pos = a3->inum;
        break;

      case VT_DNUM:
        pos = (int)a3->dnum;
        break;

      default:
        return make_shared<Var>(-1);
      }
    }
    if (-pos > 0 && -pos <= a1->str.size()) {
      pos = a1->str.size() + pos;
    }
    if (pos >= 0 && pos < a1->str.size()) {
      return make_shared<Var>((int)a1->str.find(a2->str, pos));
    }
  }
  return make_shared<Var>(-1);
}

// string: s.rindex(s2, [start])
BUILTIN(rindex) {
  if (args.size() >= 2) {
    int pos = 0;
    shared_ptr<Var> a1 = args.at(0);
    shared_ptr<Var> a2 = args.at(1);
    if (a2->vtype != VT_STRING) {
      return make_shared<Var>(-1);
    }
    if (args.size() > 2) {
      shared_ptr<Var> a3 = args.at(2);
      switch (a3->vtype) {
      case VT_INT:
        pos = a3->inum;
        break;

      case VT_DNUM:
        pos = (int)a3->dnum;
        break;

      default:
        return make_shared<Var>(-1);
      }
    }
    if (-pos > 0 && -pos <= a1->str.size()) {
      pos = a1->str.size() + pos;
    }
    if (pos >= 0 && pos < a1->str.size()) {
      return make_shared<Var>((int)a1->str.rfind(a2->str, pos));
    }
  }
  return make_shared<Var>(-1);
}

// string s.substr(start, [size])
BUILTIN(substr) {
  if (args.size() >= 2) {
    shared_ptr<Var> a1 = args.at(0);
    shared_ptr<Var> a2 = args.at(1);
    int start = 0, width = a1->str.size();
    switch (a2->vtype) {
    case VT_INT:
      start = a2->inum;
      break;

    case VT_DNUM:
      start = (int)a2->dnum;
      break;

    default:
      return a1;
    }

    if (args.size() > 2) {
      shared_ptr<Var> a3 = args.at(2);
      switch (a3->vtype) {
      case VT_INT:
        width = a3->inum;
        break;

      case VT_DNUM:
        width = (int)a3->dnum;
        break;

      default:
        return a1;
      }
    }
    if (width < 0) {
      width = a1->str.size();
    }

    if (-start > 0 && -start <= a1->str.size()) {
      start = a1->str.size() + start;
    }
    if (start + width > a1->str.size()) {
      width = a1->str.size() - start;
    }
    return make_shared<Var>(a1->str.substr(start, width));
  }
  return args.at(0);
}


PackageDlopen::~PackageDlopen() {
  dlclose(dlhandle);
}

shared_ptr<Var> PackageDlopen::start(const string &fname, vector<shared_ptr<Var> > &args) {
  void *p = dlsym(dlhandle, fname.c_str());
  if (p) {
    int (*pstart)(void *, void *, const char *, void *) =
      (int (*)(void *, void *, const char *, void *))p;
    shared_ptr<Var> rval;
    int r = (*pstart)((void **)&rval, eng, fname.c_str(), &args);
    if (r == 0) {
      return rval;
    }
  }
  return make_shared<Var>();
}

Engine::~Engine() {
  if (ar) {
    delete ar;
  }
}

void Engine::setArchive(const string &arname) {
  for (auto p = searchPaths.begin(); p != searchPaths.end(); ++p) {
    string path = *p + "/" + arname + ".minosys";
    char sig[4];
    FILE *f = fopen(path.c_str(), "r");
    if (f) {
      if (fread(&sig[0], 1, sizeof(sig), f) == sizeof(sig)
        && sig[0] == 'm' && sig[1] == 'p' && sig[2] == 'k' && sig[3] == '2') {
        ar = new Archive();
        ar->f = f;
        ar->createMap();
        return;
      }
      fclose(f);
    }
  }
}

void Engine::Archive::createMap() {
  int nelem, maxlen = 100;
  char *buf = new char[100];
  fread(&nelem, 1, sizeof(int), f);
  vector<Elem> vecElem;

  for (int i = 0; i < nelem; ++i) {
    Elem e;
    fread(&e.key_offset, 1, sizeof(int), f);
    fread(&e.key_length, 1, sizeof(int), f);
    fread(&e.val_offset, 1, sizeof(int), f);
    fread(&e.val_length, 1, sizeof(int), f);
    vecElem.push_back(e);
  }
  for (auto p = vecElem.begin(); p != vecElem.end(); ++p) {
    fseek(f, p->key_offset, SEEK_SET);
    if (maxlen < p->key_length) {
      maxlen = p->key_length;
      if (buf) delete buf;
      buf = new char[maxlen];
    }
    fread(buf, 1, p->key_length, f);
    string key(buf, p->key_length);
    map[key] = pair<int, int>(p->val_offset, p->val_length);
  }
  delete buf;
}

string Engine::Archive::findMap(const std::string &name) {
  auto p = map.find(name);
  if (p != map.end()) {
    string s(p->second.second, '\0');
    fseek(f, p->second.first, SEEK_SET);
    fread((void *)s.data(), 1, p->second.second, f);
    return s;
  }
  return string();
}

bool Engine::analyzePackage(const string &pacname, bool current) {
  auto p = packages.find(pacname);
  if (p != packages.end()) {
    // 定義済み
    return true;
  }
  if (ar) {
    auto pa = ar->map.find(pacname);
    if (pa != ar->map.end()) {
      // アーカイブに発見; minosys script でなければならない
      string s = ar->findMap(pacname);
      LexString lex(s.data(), (int)s.size());
      ContentTop *top = new ContentTop();
      if (top->yylex(&lex)) {
        // minosys script として認識
        shared_ptr<PackageMinosys> pm(new PackageMinosys());
        pm->ptype = PackageBase::PT_MINOSYS;
        pm->top = top;
        pm->eng = this;
        packages[pacname] = dynamic_pointer_cast<PackageBase>(pm);
        if (current) {
          packages[""] = packages[pacname];
        }
        for (auto vpac = top->imports.begin(); vpac != top->imports.end(); ++vpac) {
          analyzePackage(*vpac);
        }
        return true;
      }
    }
  }

  for (auto vp = searchPaths.begin(); vp != searchPaths.end(); ++vp) {
    string pt = *vp + "/" + pacname + ".minosys";
    void *d = dlopen(pt.c_str(), RTLD_LAZY);
    if (d) {
      void *st = dlsym(d, "start");
      if (st) {
        // binary package を発見
        shared_ptr<PackageDlopen> pd = make_shared<PackageDlopen>();
        pd->ptype = PackageBase::PT_DLOPEN;
        pd->name = pacname;
        pd->path = pt;
        pd->dlhandle = d;
        pd->eng = this;
        packages[pacname] = dynamic_pointer_cast<PackageBase>(pd);
        if (current) {
          packages[""] = packages[pacname];
        }
        return true;
      }
      dlclose(d);
    }
    FILE *f = fopen(pt.c_str(), "r");
    if (f) {
      LexFile lexf(f);
      ContentTop *top = new ContentTop();
      if (top->yylex(&lexf)) {
        fclose(f);
        // minosys script を発見
        shared_ptr<PackageMinosys> pm = make_shared<PackageMinosys>();
        pm->ptype = PackageBase::PT_MINOSYS;
        pm->name = pacname;
        pm->path = pt;
        pm->top = top;
        pm->eng = this;
        packages[pacname] = dynamic_pointer_cast<PackageBase>(pm);
        if (current) {
          packages[""] = packages[pacname];
        }
        for (auto vpac = top->imports.begin(); vpac != top->imports.end(); ++vpac) {
          analyzePackage(*vpac);
        }
        return true;
      }
      fclose(f);
    }
  }
  return false;
}

shared_ptr<Var> Engine::start(const string &pname, const string &fname, vector<shared_ptr<Var> > &args) {
  auto p = packages.find(pname);
  if (p != packages.end()) {
    // カレントパッケージ名を設定する
    string prevPackageName = this->currentPackageName;
    this->currentPackageName = pname;

    // パッケージ関数呼び出し
    shared_ptr<Var> r = p->second->start(fname, args);

    // カレントパッケージ名を復帰する
    this->currentPackageName = prevPackageName;
    return r;
  }
  return make_shared<Var>();
}

shared_ptr<Var> &Engine::searchVar(const string &vname, bool bLHS) {
  // search block local
  for (int i = vars.size() - 1; i >= varmark.back(); --i) {
    unordered_map<string, shared_ptr<Var> > &map = vars[i];
    auto p = map.find(vname);
    if (p != map.end()) {
      return p->second;
    }
  }

  // search instance variable
  if (vars.size() - 1 >= varmark.back()) {
    unordered_map<string, shared_ptr<Var> > &map = vars[varmark.back()];
    auto p = map.find("this");
    if (p != map.end()) {
      // instance found
      Instance *inst = p->second->inst.get();
      auto pi = inst->vars.find(vname);
      if (pi != inst->vars.end()) {
        return pi->second;
      }
    }
  }

  // search global variables
  auto pg = globalvars.find(vname);
  if (pg != globalvars.end()) {
    return pg->second;
  }

  if (bLHS) {
    // create a new local variable 
    unordered_map<string, shared_ptr<Var> > &map = vars[varmark.back()];
    map[vname] = make_shared<Var>();
    return map[vname];
  }

  // 未定義の変数を使用した
  throw RuntimeException(901, string("undefined variable:") + vname);
}

