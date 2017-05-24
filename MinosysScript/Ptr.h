#ifndef PTR_H_
#define PTR_H_

#include "exception.h"

namespace minosys {

template <class T>
class PtrRef {
 private:
  int count;
  T *value;

 public:
  PtrRef<T>() : count(0), value(NULL) {}
  PtrRef<T>(T *value) : count(0), value(value) {}
  ~PtrRef<T>() {
    if (value) {
      delete value;
    }
  }
  void inc() {
    ++count;
  }
  int release() {
    return --count;
  }
  T *get() const {
    return value;
  }
  T &getvalue() {
    if (!value) {
      throw new NullException();
    }
    return *value;
  }
};

template<class T>
class Ptr {
 private:
  PtrRef<T> *ref;

 public:
  Ptr<T>() : ref(NULL) {}
  Ptr<T>(const Ptr<T> &p) : ref(p.ref) {
    if (ref) {
      ref->inc();
    }
  }
  Ptr<T>(T *value) : ref(new PtrRef<T>(value)) {}
  Ptr<T> &operator =(const Ptr<T> &p) {
    if (ref) {
      if (ref->release() < 0) {
        delete ref;
      }
    }
    ref = p.ref;
  }
  void inc() {
    if (ref) {
      ref->inc();
    }
  }
  int release() {
    if (ref) {
      return ref->release();
    } else {
      throw new NullException();
    }
  }
  bool empty() const {
    return ref == NULL;
  }
  T *get() const {
    if (ref) {
      return ref->get();
    } else {
      throw new NullException();
    }
  }
  T &getvalue() {
    if (ref) {
      return ref->getvalue();
    } else {
      throw new NullException();
    }
  }
  T &operator *() {
    return getvalue();
  }
};

} // minosys

#endif // PTR_H_

