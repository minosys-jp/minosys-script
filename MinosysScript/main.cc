#include "engine.h"
#include <iostream>
#include <unistd.h>
#include <vector>
#include <string>
#include <memory>

using namespace std;
using namespace minosys;

int main(int argc, char **argv) {
  vector<string> sp;
  int c;
  string ar;

  while ((c = getopt(argc, argv, "a:d:")) != -1) {
    switch (c) {
    case 'a':
      ar = optarg;
      break;

    case 'd':
      sp.push_back(optarg);
      break;
    }
  }

  argc -= optind;
  argv += optind;

  if (argc < 1) {
    cout << "usage: minosysscr [-a <ar>][-d <dir>] <file>" << endl;
    return 1;
  }

  Engine eng(sp);
  eng.setArchive(argv[0]);
  if (!eng.analyzePackage(argv[0], true)) {
    cout << "package:" << argv[0] << " not found" << endl;
    return 1;
  }
  vector<shared_ptr<Var> > args;
  try {
    shared_ptr<Var> r = eng.start(argv[0], "init", args);
    cout << "return type: " << r->vtype << endl;
    switch (r->vtype) {
    case VT_INT:
      cout << "return value:" << r->inum << endl;
      break;

    case VT_DNUM:
      cout << "return value:" << r->dnum << endl;
      break;

    case VT_STRING:
      cout << "return value:" << r->str << endl;
      break;
    }
  } catch (const RuntimeException &e) {
    cout << "RuntimeException: number=" << e.e << ", message=" << e.er << endl;
  }
  return 0;
}

