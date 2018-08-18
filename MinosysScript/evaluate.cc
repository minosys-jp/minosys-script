#include "engine.h"
#include "lex.h"
#include "minosysscr_api.h"
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <iostream>

using namespace std;
using namespace minosys;

// 式の評価
shared_ptr<Var> PackageMinosys::evaluate(Content *c) {
  switch (c->tag) {
  case LexBase::LT_NULL:	// nullptr
    return make_shared<Var>();

  case LexBase::LT_INT:	// 整数
    return make_shared<Var>(c->inum);

  case LexBase::LT_DNUM:	// 浮動小数点
    return make_shared<Var>(c->dnum);

  case LexBase::LT_STRING:	// 文字列
    return make_shared<Var>(c->op);

  case LexBase::LT_VAR:	// 変数
    return eval_var(c);

  case LexBase::LT_TAG:	// 関数名
    return eval_functag(c);

  case LexBase::LT_FUNC:	// 関数呼び出し
  case LexBase::LT_OP:	// 演算子
  default:
    cout << "unknown operator: (" << (int)c->tag << ")" << c->op << endl;
  }
  return make_shared<Var>();
}

// 変数値の評価
shared_ptr<Var> PackageMinosys::eval_var(Content *c) {
  return eng->searchVar(c->op);
}

// 関数名の評価
// 実際の関数呼び出しは eval_func で行われる
// また、一般には "." 演算子でパッケージ名と結合されることに注意
shared_ptr<Var> PackageMinosys::eval_functag(Content *c) {
  pair<string, string> func("", c->op);
  return make_shared<Var>(func);
}

