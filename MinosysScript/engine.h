#ifndef ENGINE_H_

#include <vector>
#include <stack>
#include <unordered_map>
#include <string>
#include <cstdio>
#include "content.h"
#include "Ptr.h"

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
  Ptr<std::pair<std::string, std::string> > func;
  Ptr<std::pair<Ptr<Var>, std::string> > member;
  Ptr<Instance> inst;
  void *pointer;
  std::unordered_map<VarKey, Ptr<Var>, VarKey::Hash> arrayhash;

  Var() : vtype(VT_NULL) {}
  Var(int inum) : vtype(VT_INT) { this->inum = inum; }
  Var(double dnum) : vtype(VT_DNUM) { this->dnum = dnum; }
  Var(const std::string &c) : vtype(VT_STRING),  str(c) {}
  Var(Instance *i) : vtype(VT_INST), inst(i) {}
  Var(const Ptr<std::pair<std::string, std::string> > &fnpair) : vtype(VT_FUNC), func(fnpair) {}
  Var(const Ptr<std::pair<Ptr<Var>, std::string> > &mpair) : vtype(VT_MEMBER), member(mpair) {}
  Var(const Var &v);
  ~Var();
  Var &operator = (const Var &v);
  Var *clone();
  bool isTrue() const;
};

class Instance {
  friend Var;

 public:
  MinosysClassDef *def;
  std::unordered_map<std::string, Ptr<Var> > vars;
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
   virtual Ptr<Var> start(const std::string &fname, std::vector<Ptr<Var> > &args) = 0;
   virtual ~PackageBase() {}
};

class PackageMinosys : public PackageBase {
 public:
   ContentTop *top;
   Ptr<Var> start(const std::string &fname, std::vector<Ptr<Var> > &args);
   Ptr<Var> typefunc(const std::vector<Ptr<Var> > &args);
   Ptr<Var> convertfunc(const std::vector<Ptr<Var> > &args);
   void printfunc(const std::vector<Ptr<Var> > &args);
   void exitfunc(const std::vector<Ptr<Var> > &args);
   Ptr<Var> callfunc(const std::string &fname, Content *c);
   Ptr<Var> evaluate(Content *c);
   ~PackageMinosys();
};

class PackageDlopen : public PackageBase {
 public:
  void *dlhandle;
  Ptr<Var> start(const std::string &fname, std::vector<Ptr<Var> > &args);
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
  std::unordered_map<std::string, Ptr<PackageBase> > packages;
  std::unordered_map<std::string, Ptr<Var> > globalvars;
  std::vector<std::unordered_map<std::string, Ptr<Var> > > vars;
  std::vector<int> varmark;
  std::vector<Ptr<Var> > paramstack;
  std::vector<int> topmark;
  std::vector<Content *> callstack;
  std::vector<int> callmark;
  std::vector<std::pair<std::string, std::string> > headers;
  Engine(const std::vector<std::string> &searchPaths) : searchPaths(searchPaths), ar(NULL) {}
  ~Engine();
  bool analyzePackage(const std::string &pacname, bool current = false);
  void setArchive(const std::string &arname);
  Ptr<Var> start(const std::string &pname, const std::string &fname, std::vector<Ptr<Var> > &args);
  Var *searchVar(const std::string &vname);

 private:
   void analyzeArchive(FILE *f);
};

} // minosys

#endif // ENGINE_H_

