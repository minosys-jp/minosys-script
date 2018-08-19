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
    return eval_func(c);

  case LexBase::LT_OP:	// 演算子
    return eval_op(c);

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

// 関数呼び出し
shared_ptr<Var> PackageMinosys::eval_func(Content *c) {
  // [0]: 関数名
  shared_ptr<Var> func = evaluate(c->pc.at(0));

  // [1~]: 引数
  vector<shared_ptr<Var> > args;
  for (int i = 1; i < c->pc.size(); i++) {
    args.push_back(evaluate(c->pc.at(i)));
  }

  // パッケージ名の抽出
  // TODO: メンバーの場合の抽出処理
  if (func->vtype != VT_FUNC) {
    throw RuntimeException(1000, "Function calls non-function");
  }

  string pname;
  if (func->func.first.empty()) {
    // カレントパッケージ
    pname = eng->currentPackageName;
  } else {
    // 別のパッケージ
    pname = func->func.first;
  }
  auto found = eng->packages.find(pname);
  if (found == eng->packages.end()) {
    throw RuntimeException(1001, string("Package not found:") + pname);
  }
  shared_ptr<PackageBase> base = found->second;

  // 関数呼び出しおよび結果の返却
  return base->start(func->func.second, args);
}

// 演算子の評価
shared_ptr<Var> PackageMinosys::eval_op(Content *c) {
  if (c->op == "=") {
    return eval_op_assign(c);
  }
  throw RuntimeException(1002, string("operator not defined:") + c->op);
}

// 配列を考慮して変数を作成する
shared_ptr<Var> &PackageMinosys::createVar(const string &vname, vector<Content *> &pc) {
  shared_ptr<Var> &v = eng->searchVar(vname, true);

  if (!pc.empty()) {
    for (int i = 0; i < pc.size(); i++) {
      if (v->vtype != VT_ARRAY) {
        // 配列でなければ配列化する
        v->vtype = VT_ARRAY;
      }
      shared_ptr<Var> idx = evaluate(pc.at(i));
      switch (idx->vtype) {
      case VT_INT:
        {
          VarKey key(idx->inum);
          v = createVarIndex(key, v);
        }
        break;

      case VT_DNUM:
        {
          VarKey key(idx->dnum);
          v = createVarIndex(key, v);
        }
        break;

      case VT_STRING:
        {
          VarKey key(idx->str);
          v = createVarIndex(key, v);
        }
        break;

      default:
        throw RuntimeException(1003, "hash index is not int/dnum/string");
      }
    }
  }
  return v;
}

// 配列要素を検索する。なければ作成する
shared_ptr<Var> &PackageMinosys::createVarIndex(const VarKey &key, shared_ptr<Var> &v) {
  auto p = v->arrayhash.find(key);
  if (p != v->arrayhash.end()) {
    // 配列要素が見つかったので v を置き換える
    v = p->second;
  } else {
    // 配列要素が見つからなかったので、作成する
    v->arrayhash[key] = make_shared<Var>();
    v = v->arrayhash[key];
  }
  return v;
}

// 代入演算子の評価
shared_ptr<Var> PackageMinosys::eval_op_assign(Content *c) {
  // 左辺
  Content *lhs = c->pc.at(0);

  // 変数を探す; なければ作成する
  shared_ptr<Var> &v = createVar(lhs->op, lhs->pc);

  // TODO: メンバー変数の検索

  // 右辺
  v = evaluate(c->pc.at(1));
  return v;
}

