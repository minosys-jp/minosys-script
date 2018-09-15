#ifndef ENGINE_H_

#include <vector>
#include <stack>
#include <unordered_map>
#include <string>
#include <cstdio>
#include <memory>
#include "content.h"

namespace minosys {

class Instance;
enum VTYPE {
  VT_NULL, VT_INT, VT_DNUM, VT_STRING, VT_INST, VT_POINTER, VT_ARRAY, VT_FUNC, VT_MEMBER
};

struct VarKey {
  VTYPE vtype;
  union {
    int inum;
    double dnum;
    std::string *str;
  } u;
  VarKey() : vtype(VT_INT) {}
  VarKey(int inum) : vtype(VT_INT) {
    u.inum = inum;
  }
  VarKey(double dnum) : vtype(VT_DNUM) {
    u.dnum = dnum;
  }
  VarKey(const std::string &s) : vtype(VT_STRING) {
    u.str = new std::string(s);
  }
  VarKey(const VarKey &k) : vtype(k.vtype) {
    switch (vtype) {
    case VT_INT:
      u.inum = k.u.inum;
      break;

    case VT_DNUM:
      u.dnum = k.u.dnum;
      break;

    case VT_STRING:
      u.str = new std::string(*k.u.str);
      break;
    }
  }
  ~VarKey() {
    if (vtype == VT_STRING) delete u.str;
  }
  bool operator == (const VarKey &k) const;
  bool operator != (const VarKey &k) const;
  std::string toString() const;
  struct Hash {
    size_t operator()(const VarKey &v) const {
      switch(v.vtype) {
      case VT_INT:
        return std::hash<int>()(v.u.inum);

      case VT_DNUM:
        return std::hash<double>()(v.u.dnum);

      case VT_STRING:
        return std::hash<std::string>()(*v.u.str);
      }
      return 0;
    }
  };
};

struct Var {
  VTYPE vtype;
  int inum;
  double dnum;
  std::string str;
  std::pair<std::string, std::string> func;
  std::pair<std::shared_ptr<Var>, std::string> member;
  std::shared_ptr<Instance> inst;
  void *pointer;
  std::unordered_map<VarKey, std::shared_ptr<Var>, VarKey::Hash> arrayhash;

  Var() : vtype(VT_NULL) {}
  Var(int inum) : vtype(VT_INT) { this->inum = inum; }
  Var(double dnum) : vtype(VT_DNUM) { this->dnum = dnum; }
  Var(const std::string &c) : vtype(VT_STRING),  str(c) {}
  Var(Instance *i) : vtype(VT_INST), inst(i) {}
  Var(const std::shared_ptr<Instance> &i) : vtype(VT_INST), inst(i) {}
  Var(const std::pair<std::string, std::string> &fnpair) : vtype(VT_FUNC), func(fnpair) {}
  Var(const std::pair<std::shared_ptr<Var>, std::string> &mpair) : vtype(VT_MEMBER), member(mpair) {}
  Var(const std::unordered_map<VarKey, std::shared_ptr<Var>, VarKey::Hash> ah) : vtype(VT_ARRAY), arrayhash(ah) {}
  Var(const Var &v);
  ~Var();
  Var &operator = (const Var &v);
  bool operator == (const Var &v) const;
  std::shared_ptr<Var> clone();
  bool isTrue() const;
};

class Instance {
  friend Var;

 public:
  MinosysClassDef *def;
  std::unordered_map<std::string, std::shared_ptr<Var> > vars;
  Instance() : def(NULL) {}
  Instance(const Instance &i) : def(i.def), vars(i.vars) {}
};

class Engine;
class PackageBase {
 public:
   enum PTYPE {
     PT_MINOSYS, PT_DLOPEN
   } ptype;
   Engine *eng;
   std::string name;
   std::string path;
   virtual std::shared_ptr<Var> start(const std::string &fname, std::vector<std::shared_ptr<Var> > &args) = 0;
   virtual ~PackageBase() {}
};

class PackageMinosys : public PackageBase {
 private:
   std::shared_ptr<Var> eval_var(Content *c);
   std::shared_ptr<Var> eval_functag(Content *c);
   std::shared_ptr<Var> eval_func(Content *c);
   std::shared_ptr<Var> eval_op(Content *c);

   std::unordered_map<std::string, std::function<std::shared_ptr<Var>(PackageMinosys *, Content *)> > opmap;
#define OP(x) std::shared_ptr<Var> eval_op_##x(Content *c);

   OP(dot);
   OP(3term);
   OP(monoNot);
   OP(negate);
   OP(monoMinus);
   OP(assign);
   OP(assignplus);
   OP(assignminus);
   OP(assignmultiply);
   OP(assigndiv);
   OP(assignmod);
   OP(assignand);
   OP(assignor);
   OP(assignxor);
   OP(assignlsh);
   OP(assignrsh);
   OP(preIncr);
   OP(postIncr);
   OP(preDecr);
   OP(postDecr);
   OP(lt);
   OP(lteq);
   OP(gt);
   OP(gteq);
   OP(neq);
   OP(eq);
   OP(plus);
   OP(minus);
   OP(multiply);
   OP(div);
   OP(mod);
   OP(logand);
   OP(logor);
   OP(and);
   OP(or);
   OP(xor);
   OP(lsh);
   OP(rsh);
   OP(leftarray);

   std::shared_ptr<Var>& createVar(const std::string &vname, std::vector<Content *> &pc);
   std::shared_ptr<Var>* createVarIndex(const VarKey &key, std::shared_ptr<Var> *pv);
   std::string createMulString(int count, const std::string &s);

 public:
   ContentTop *top;
   std::shared_ptr<Var> start(const std::string &fname, std::vector<std::shared_ptr<Var> > &args);

   std::unordered_map<std::string, std::function<std::shared_ptr<Var>(PackageMinosys *, std::vector<std::shared_ptr<Var> >)> > builtinmap;
   std::unordered_map<std::string, std::function<std::shared_ptr<Var>(PackageMinosys *, std::vector<std::shared_ptr<Var> >)> > stringmap;

#define BUILTIN(bb) std::shared_ptr<Var> func##bb (const std::vector<std::shared_ptr<Var> > &args)
   BUILTIN(type);
   BUILTIN(convert);
   BUILTIN(print);
   BUILTIN(exit);

   BUILTIN(empty);
   BUILTIN(length);
   BUILTIN(at);
   BUILTIN(index);
   BUILTIN(rindex);
   BUILTIN(substr);

   std::shared_ptr<Var> callfunc(const std::string &fname, Content *c);
   std::shared_ptr<Var> evaluate(Content *c);
   PackageMinosys();
   ~PackageMinosys();
};

class PackageDlopen : public PackageBase {
 public:
  void *dlhandle;
  std::shared_ptr<Var> start(const std::string &fname, std::vector<std::shared_ptr<Var> > &args);
  ~PackageDlopen();
};

class Engine {
 public:
  struct Archive {
    std::FILE *f;
    std::unordered_map<std::string, std::pair<int, int> > map;
    std::string findMap(const std::string &name);
    void createMap();

    struct Elem {
      int key_offset, key_length;
      int val_offset, val_length;
    };
    ~Archive() {
      if (f) {
        std::fclose(f);
      }
    }
  };
  Archive *ar;
  std::vector<std::string> searchPaths;
  std::unordered_map<std::string, std::shared_ptr<PackageBase> > packages;
  std::unordered_map<std::string, std::shared_ptr<Var> > globalvars;
  std::vector<std::unordered_map<std::string, std::shared_ptr<Var> > > vars;
  std::vector<int> varmark;
  std::vector<std::shared_ptr<Var> > paramstack;
  std::vector<int> topmark;
  std::vector<Content *> callstack;
  std::vector<int> callmark;
  std::vector<std::pair<std::string, std::string> > headers;
  std::string currentPackageName;
  Engine(const std::vector<std::string> &searchPaths) : searchPaths(searchPaths), ar(NULL) {}
  ~Engine();
  bool analyzePackage(const std::string &pacname, bool current = false);
  void setArchive(const std::string &arname);
  std::shared_ptr<Var> start(const std::string &pname, const std::string &fname, std::vector<std::shared_ptr<Var> > &args);
  std::shared_ptr<Var> &searchVar(const std::string &vname, bool bLHS = false);

 private:
   void analyzeArchive(FILE *f);
};

} // minosys

#endif // ENGINE_H_

