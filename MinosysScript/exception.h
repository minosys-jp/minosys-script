#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include "lex.h"
#include <vector>
#include <list>
#include <unordered_map>

namespace minosys {

class Exception {
 public:
  int e;
  std::string er;
  Exception(int e, const std::string &str) {
    this->e = e;
    this->er = str;
  }
};

class SyntaxException : public Exception {
 public:
  int line;
  SyntaxException(int line) : line(line), Exception(1, "Syntax Error") {
  }
};

class RuntimeException : public Exception {
 public:
  RuntimeException(int e, std::string str) : Exception(e, str) {}
};

class ExitException : public Exception {
 public:
  int exitcode;
  ExitException(int exitcode) : exitcode(exitcode), Exception(0, "exit") {}
};

class NullException : public Exception {
 public:
  NullException() : Exception(2, "NULL Exception") {}
};

} // minosys

#endif // EXCEPTION_H_

