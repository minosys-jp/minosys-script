#include "engine.h"
#include "minosysscr_api.h"
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>

// TODO: Ptr<Var> による Var * 部分の書き換えが必要

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

Var *Var::clone() {
  Var *v = NULL;

  switch (vtype) {
  case VT_NULL:
    return new Var();

  case VT_INT:
    return new Var(inum);

  case VT_DNUM:
    return new Var(dnum);

  case VT_STRING:
    return new Var(str);

  case VT_INST:
    return new Var(new Instance(inst.getvalue()));

  case VT_POINTER:
    v = new Var();
    v->vtype = VT_POINTER;
    v->pointer = pointer;
    return v;

  case VT_ARRAY:
    v = new Var();
    v->vtype = VT_ARRAY;
    v->arrayhash = arrayhash;
    return  v;

  case VT_FUNC:
    return new Var(func);

  case VT_MEMBER:
    return new Var(member);
  }
  return NULL;
}

Var::~Var() {
}

PackageMinosys::~PackageMinosys() {
  delete top;
}

Ptr<Var> PackageMinosys::start(const string &fname, vector<Ptr<Var> > &args) {
  auto p = top->funcs.find(fname);
 if (p != top->funcs.end()) {
    Content *c = p->second;
    unordered_map<string, Ptr<Var> > amap;

    // ビルトイン関数
    if (fname == "type") {
      return typefunc(args);
    } else if (fname == "convert") {
      return convertfunc(args);
    } else if (fname == "print") {
      printfunc(args);
      return NULL;
    } else if (fname == "exit") {
      exitfunc(args);
    }

    for (int i = 0; i < args.size(); ++i) {
      amap[c->arg[i]] = args[i];
    }
    eng->varmark.push_back(eng->vars.size());
    eng->topmark.push_back(eng->paramstack.size());
    eng->vars.push_back(amap);
    Ptr<Var> rv = callfunc(c);
    if (!rv.empty()) {
      if (eng->topmark.back() >= eng->paramstack.size()) {
        eng->paramstack.erase(
          eng->paramstack.begin() + eng->topmark.back(),
          eng->paramstack.end() - eng->paramstack.size() - 1
        );
      }
      rv = eng->paramstack.back();
    }
    eng->vars.erase(
      eng->vars.begin() + eng->varmark.back(),
      eng->vars.end()
    );
    eng->varmark.pop_back();
    eng->topmark.pop_back();
    return rv;
  }
  return Ptr<Var>();
}

Ptr<Var> PackageMinosys::callfunc(Content *c) {
  while (c) {
    switch (c->tag) {
    case LexBase::LT_BEGIN:	// TODO
    case LexBase::LT_IF:	// TODO
    case LexBase::LT_FOR:	// TODO
    case LexBase::LT_WHILE:	// TODO
    case LexBase::LT_BREAK:	// TODO
    case LexBase::LT_CONTINUE:	// TODO
    case LexBase::LT_RETURN:
      if (c->pc.size() >= 1) {
        return evaluate(c);
      }
      return NULL;

    default: // 演算子
      evaluate(c);
      eng->paramstack.pop_back();
    }
    c = c->next;
  }
  return Ptr<Var>();
}

Ptr<Var> PackageMinosys::evaluate(Content *c) {
  // TODO:
  return Ptr<Var>();
}

Ptr<Var> PackageMinosys::typefunc(const std::vector<Ptr<Var> > &args) {
  Var *v;
  if (!args.empty()) {
    v = new Var((int)args[0].get()->vtype);
  } else {
    v = new Var(-1);
  }
  eng->paramstack.push_back(Ptr<Var>(v));
  return eng->paramstack.back();
}

Ptr<Var> PackageMinosys::convertfunc(const std::vector<Ptr<Var> > &args) {
  if (args.size() < 2
    || args[1].get()->vtype != VT_INT) {
    eng->paramstack.push_back(args[0].get()->clone());
  } else {
    switch (args[1].get()->inum) {
    case VT_INT:
      switch (args[0].get()->vtype) {
      case VT_INT:
        eng->paramstack.push_back(new Var(args[0].get()->inum));
        break;

      case VT_DNUM:
        eng->paramstack.push_back(new Var((int)args[0].get()->dnum));
        break;

      case VT_STRING:
        eng->paramstack.push_back(new Var(atoi(args[0].get()->str.c_str())));
        break;

      default:
        eng->paramstack.push_back(args[0].get()->clone());
      }
      break;

    case VT_DNUM:
      switch (args[0].get()->vtype) {
      case VT_INT:
        eng->paramstack.push_back(new Var((double)args[0].get()->inum));
        break;

      case VT_DNUM:
        eng->paramstack.push_back(new Var(args[0].get()->dnum));
        break;

      case VT_STRING:
        eng->paramstack.push_back(new Var(atof(args[0].get()->str.c_str())));
        break;

      default:
        eng->paramstack.push_back(args[0].get()->clone());
      }
      break;

    case VT_STRING:
      switch (args[0].get()->vtype) {
      case VT_INT:
        eng->paramstack.push_back(new Var(to_string(args[0].get()->inum)));
        break;

      case VT_DNUM:
        eng->paramstack.push_back(new Var(to_string(args[0].get()->dnum)));
        break;

      case VT_STRING:
        eng->paramstack.push_back(new Var(args[0].get()->str));
        break;

      default:
        eng->paramstack.push_back(args[0].get()->clone());
      }
      break;

    default:
      eng->paramstack.push_back(args[0].get()->clone());
    }
  }
  return eng->paramstack.back();
}

void PackageMinosys::printfunc(const std::vector<Ptr<Var> > &args) {
  int count = 0;

  for (auto p = args.begin(); p != args.end(); ++p) {
    switch (p->get()->vtype) {
    case VT_INT:
      printf("%d", p->get()->inum);
      break;

    case VT_DNUM:
      printf("%g", p->get()->dnum);
      break;

    case VT_STRING:
      printf("%.*s", (int)p->get()->str.size(), p->get()->str.data());
      break;

    case VT_INST:
      {
        vector<Ptr<Var> > args;
        Ptr<Var> r = this->start("toString", args);
        if (!r.empty()) {
          vector<Ptr<Var> > args;
          args.push_back(r.get()->clone());
          printfunc(args);
          eng->paramstack.pop_back();
        }
      }
      break;

    case VT_ARRAY:
      printf("{");
      for (auto pc = p->get()->arrayhash.begin(); pc != p->get()->arrayhash.end(); ++pc, ++count) {
        if (count) {
          printf(",");
        }
        string s = pc->first.toString();
        printf("%.*s: ", (int)s.size(), s.data());
        vector<Ptr<Var> > args;
        args.push_back(pc->second);
        printfunc(args);
      }
      printf("}");
      break;

    case VT_FUNC:
      {
        const string &pac = p->get()->func.get()->first;
        const string &fname = p->get()->func.get()->second;
        if (pac.empty()) {
          printf("%.*s", (int)fname.size(), fname.data());
        } else {
          printf("%.*s.%.*s", (int)pac.size(), pac.data(), (int)fname.size(), fname.data());
        }
      }
      break;

    case VT_MEMBER:
      {
        vector<Ptr<Var> > args;
        args.push_back(p->get()->member.get()->first);
        printfunc(args);
        string &mem = p->get()->member.get()->second;
        printf(".%.*s", (int)mem.size(), mem.data());
      }
      break;

    default:
      ;
    }
  }
}

void PackageMinosys::exitfunc(const std::vector<Ptr<Var> > &args) {
  int code = 0;
  if (!args.empty()) {
    switch (args[0].get()->vtype) {
    case VT_INT:
      code = args[0].get()->inum;
      break;

    case VT_DNUM:
      code = (int)args[0].get()->dnum;
      break;

    case VT_STRING:
      code = atoi(args[0].get()->str.c_str());
      break;
    }
  }
  throw new ExitException(code);
}

PackageDlopen::~PackageDlopen() {
  dlclose(dlhandle);
}

Ptr<Var> PackageDlopen::start(const string &fname, vector<Ptr<Var> > &args) {
  void *p = dlsym(dlhandle, "start");
  if (p) {
    int (*pstart)(void *, void *, const char *, void *) =
      (int (*)(void *, void *, const char *, void *))p;
    Ptr<Var> rval;
    int r = (*pstart)((void **)&rval, eng, fname.c_str(), &args);
    if (r == 0) {
      return rval;
    }
  }
  return Ptr<Var>();
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
        PackageMinosys *pm = new PackageMinosys();
        pm->ptype = PackageBase::PT_MINOSYS;
        pm->top = top;
        pm->eng = this;
        packages[pacname] = pm;
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
        PackageDlopen *pd = new PackageDlopen();
        pd->ptype = PackageBase::PT_DLOPEN;
        pd->name = pacname;
        pd->path = pt;
        pd->dlhandle = d;
        pd->eng = this;
        packages[pacname] = pd;
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
        PackageMinosys *pm = new PackageMinosys();
        pm->ptype = PackageBase::PT_MINOSYS;
        pm->name = pacname;
        pm->path = pt;
        pm->top = top;
        pm->eng = this;
        packages[pacname] = pm;
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

Ptr<Var> Engine::start(const string &pname, const string &fname, vector<Ptr<Var> > &args) {
  auto p = packages.find(pname);
  if (p != packages.end()) {
    return p->second.getvalue().start(fname, args);
  }
  return Ptr<Var>();
}

Var *Engine::searchVar(const string &vname) {
  // search block local
  for (int i = vars.size() - 1; i >= varmark.back(); --i) {
    unordered_map<string, Ptr<Var> > &map = vars[i];
    auto p = map.find(vname);
    if (p != map.end()) {
      return &p->second.getvalue();
    }
  }

  // search instance variable
  if (vars.size() - 1 >= varmark.back()) {
    unordered_map<string, Ptr<Var> > &map = vars[varmark.back()];
    auto p = map.find("this");
    if (p != map.end()) {
      // instance found
      Instance *inst = &p->second.getvalue().inst.getvalue();
      auto pi = inst->vars.find(vname);
      if (pi != inst->vars.end()) {
        return &pi->second.getvalue();
      }
    }
  }

  // search global variables
  auto pg = globalvars.find(vname);
  if (pg != globalvars.end()) {
    return &pg->second.getvalue();
  }

  return NULL;
}

