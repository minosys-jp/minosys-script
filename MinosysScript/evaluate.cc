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
  if (c->op == "?") {
    return eval_op_3term(c);
  }
  if (c->op == "=") {
    return eval_op_assign(c);
  }
  if (c->op == "+=") {
    return eval_op_assignplus(c);
  }
  if (c->op == "*=") {
    return eval_op_assignmultiply(c);
  }
  if (c->op == "-=") {
    return eval_op_assignminus(c);
  }
  if (c->op == "/=") {
    return eval_op_assigndiv(c);
  }
  if (c->op == "%=") {
    return eval_op_assignmod(c);
  }
  if (c->op == "&=") {
    return eval_op_assignand(c);
  }
  if (c->op == "|=") {
    return eval_op_assignor(c);
  }
  if (c->op == "^=") {
    return eval_op_assignxor(c);
  }
  if (c->op == "<<=") {
    return eval_op_assignlsh(c);
  }
  if (c->op == ">>=") {
    return eval_op_assignrsh(c);
  }
  if (c->op == "!") {
    return eval_op_monoNot(c);
  }
  if (c->op == "~") {
    return eval_op_negate(c);
  }
  if (c->op == "-m") {
    return eval_op_monoMinus(c);
  }
  if (c->op == "++x") {
    return eval_op_preIncr(c);
  }
  if (c->op == "x++") {
    return eval_op_postIncr(c);
  }
  if (c->op == "--x") {
    return eval_op_preDecr(c);
  }
  if (c->op == "x--") {
    return eval_op_postDecr(c);
  }
  if (c->op == "<") {
    return eval_op_lt(c);
  }
  if (c->op == "<=") {
    return eval_op_lteq(c);
  }
  if (c->op == ">") {
    return eval_op_gt(c);
  }
  if (c->op == ">=") {
    return eval_op_gteq(c);
  }
  if (c->op == "!=") {
    return eval_op_neq(c);
  }
  if (c->op == "==") {
    return eval_op_eq(c);
  }
  if (c->op == "+") {
    return eval_op_plus(c);
  }
  if (c->op == "-") {
    return eval_op_minus(c);
  }
  if (c->op == "*") {
    return eval_op_multiply(c);
  }
  if (c->op == "/") {
    return eval_op_div(c);
  }
  if (c->op == "%") {
    return eval_op_mod(c);
  }
  if (c->op == "&") {
    return eval_op_and(c);
  }
  if (c->op == "|") {
    return eval_op_or(c);
  }
  if (c->op == "^") {
    return eval_op_xor(c);
  }
  if (c->op == "&&") {
    return eval_op_logand(c);
  }
  if (c->op == "||") {
    return eval_op_logor(c);
  }
  if (c->op == "<<") {
    return eval_op_lsh(c);
  }
  if (c->op == ">>") {
    return eval_op_rsh(c);
  }
  throw RuntimeException(1002, string("operator not defined:") + c->op);
}

// 3項演算子
shared_ptr<Var> PackageMinosys::eval_op_3term(Content *c) {
  shared_ptr<Var> v0 = evaluate(c->pc.at(0));
  if (v0->isTrue()) {
    return evaluate(c->pc.at(1));
  } else {
    return evaluate(c->pc.at(2));
  }
}

// 単項 ! 演算子
shared_ptr<Var> PackageMinosys::eval_op_monoNot(Content *c) {
  shared_ptr<Var> v(evaluate(c->pc.at(0)));
  shared_ptr<Var> r = make_shared<Var>((int)(v->isTrue() ? 0 : 1));
  return r;
}

// 単項 ~ 演算子
shared_ptr<Var> PackageMinosys::eval_op_negate(Content *c) {
  shared_ptr<Var> v(evaluate(c->pc.at(0))->clone());
  if (v->vtype == VT_INT) {
    v->inum = ~v->inum;
  }
  return v;
}

// 単項 - 演算子
shared_ptr<Var> PackageMinosys::eval_op_monoMinus(Content *c) {
  // 値を評価する
  shared_ptr<Var> v(evaluate(c->pc.at(0))->clone());

  switch (v->vtype) {
  case VT_INT:
    v->inum = -v->inum;
    break;

  case VT_DNUM:
    v->dnum = -v->dnum;
    break;
  }

  return v;
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

// 代入演算子: += の評価
shared_ptr<Var> PackageMinosys::eval_op_assignplus(Content *c) {
  // 左辺
  Content *lhs = c->pc.at(0);

  // 変数を探す; なければ作成する
  shared_ptr<Var> &v1 = createVar(lhs->op, lhs->pc);

  // TODO: メンバー変数の検索

  // 右辺
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_NULL:
    v1 = v2;
    return v1;

  case VT_INT:
    switch (v2->vtype) {
    case VT_NULL:
      return v1;

    case VT_INT:
      v1->inum += v2->inum;
      return v1;

    case VT_DNUM:
      v1->vtype = VT_DNUM;
      v1->dnum = v1->inum + v2->dnum;
      return v1;

    case VT_STRING:
      v1->vtype = VT_STRING;
      v1->str = to_string(v1->inum) + v2->str;
      return v1;
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_NULL:
      return v1;

    case VT_INT:
      v1->dnum += v2->inum;
      return v1;

    case VT_DNUM:
      v1->dnum += v2->dnum;
      return v1;

    case VT_STRING:
      v1->vtype = VT_STRING;
      v1->str = to_string(v1->dnum) + v2->str;
      return v1;
    }
    break;

  case VT_STRING:
    switch (v2->vtype) {
    case VT_NULL:
      return v1;

    case VT_INT:
      v1->str += to_string(v2->inum);
      return v1;

    case VT_DNUM:
      v1->str += to_string(v2->dnum);
      return v1;

    case VT_STRING:
      v1->str += v2->str;
      return v1;
    }
  }

  // 評価できない場合は左辺の値を返す
  return v1;
}

// 代入演算子: -=
shared_ptr<Var> PackageMinosys::eval_op_assignminus(Content *c) {
  // 左辺
  Content *lhs = c->pc.at(0);

  // 変数を探す; なければ作成する
  shared_ptr<Var> &v1 = createVar(lhs->op, lhs->pc);

  // TODO: メンバー変数の検索

  // 右辺
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_NULL:
    switch (v2->vtype) {
    case VT_INT:
      v1->vtype = VT_INT;
      v1->inum = -v2->inum;
      return v1;

    case VT_DNUM:
      v1->vtype = VT_DNUM;
      v1->inum = (int)-v2->dnum;
      return v1;
    }
    break;

  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      v1->inum -= v2->inum;
      return v1;

    case VT_DNUM:
      v1->vtype = VT_DNUM;
      v1->dnum = v1->inum - v2->dnum;
      return v1;
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      v1->dnum -= v2->inum;
      return v1;

    case VT_DNUM:
      v1->dnum -= v2->dnum;
      return v1;
    }

  }

  // 評価できない場合は左辺の値を返す
  return v1;
}

// 代入演算子: *=
shared_ptr<Var> PackageMinosys::eval_op_assignmultiply(Content *c) {
  // 左辺
  Content *lhs = c->pc.at(0);

  // 変数を探す; なければ作成する
  shared_ptr<Var> &v1 = createVar(lhs->op, lhs->pc);

  // TODO: メンバー変数の検索

  // 右辺
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      v1->inum *= v2->inum;
      return v1;

    case VT_DNUM:
      v1->vtype = VT_DNUM;
      v1->dnum = v1->inum * v2->dnum;
      return v1;

    case VT_STRING:
      v1->vtype = VT_STRING;
      v1->str = createMulString(v1->inum, v2->str);
      return v1;
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      v1->vtype = VT_DNUM;
      v1->dnum = v1->inum * v2->dnum;
      return v1;

    case VT_DNUM:
      v1->dnum *= v2->dnum;
      return v1;

    case VT_STRING:
      v1->vtype = VT_STRING;
      v1->str = createMulString((int)v1->dnum, v2->str);
      return v1;
    }
    break;

  case VT_STRING:
    switch (v2->vtype) {
    case VT_INT:
      v1->str = createMulString(v2->inum, v1->str);
      return v1;

    case VT_DNUM:
      v1->str = createMulString((int)v2->dnum, v1->str);
      return v1;
    }
    break;
  }

  // 評価できない場合は左辺の値を返す
  return v1;
}

// 代入演算子: /=
shared_ptr<Var> PackageMinosys::eval_op_assigndiv(Content *c) {
  // 左辺
  Content *lhs = c->pc.at(0);

  // 変数を探す; なければ作成する
  shared_ptr<Var> &v1 = createVar(lhs->op, lhs->pc);

  // TODO: メンバー変数の検索

  // 右辺
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));
  
  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      v1->inum /= v2->inum;
      return v1;

    case VT_DNUM:
      v1->vtype = VT_DNUM;
      v1->dnum = v1->inum / v2->dnum;
      return v1;
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      v1->dnum /= v2->inum;
      return v1;

    case VT_DNUM:
      v1->dnum /= v2->dnum;
      return v1;
    }
    break;
  }

  // 評価できない場合は左辺の値を返す
  return v1;
}

// 代入演算子: %=
shared_ptr<Var> PackageMinosys::eval_op_assignmod(Content *c) {
  // 左辺
  Content *lhs = c->pc.at(0);

  // 変数を探す; なければ作成する
  shared_ptr<Var> &v1 = createVar(lhs->op, lhs->pc);

  // TODO: メンバー変数の検索

  // 右辺
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      v1->inum %= v2->inum;
      return v1;

    case VT_DNUM:
      v1->inum %= (int)v2->dnum;
      return v1;
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      v1->vtype = VT_INT;
      v1->inum = ((int)v1->dnum) % v2->inum;
      return v1;

    case VT_DNUM:
      v1->vtype = VT_INT;
      v1->inum = ((int)v1->dnum) % (int)v2->dnum;
      return v1;
    }
    break;
  }

  // 評価できない場合は左辺の値を返す
  return v1;
}

// 代入演算子: &=
shared_ptr<Var> PackageMinosys::eval_op_assignand(Content *c) {
  // 左辺
  Content *lhs = c->pc.at(0);

  // 変数を探す; なければ作成する
  shared_ptr<Var> &v1 = createVar(lhs->op, lhs->pc);

  // TODO: メンバー変数の検索
 
  // 右辺
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      v1->inum &= v2->inum;
      return v1;

    case VT_DNUM:
      v1->inum &= (int)v2->dnum;
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      v1->vtype = VT_INT;
      v1->inum = (int)v1->dnum & v2->inum;
      return v1;

    case VT_DNUM:
      v1->vtype = VT_INT;
      v1->inum = (int)v1->dnum & (int)v2->dnum;
      return v1;
    }
    break;
  }

  // 評価できない場合は左辺の値を返す
  return v1;
}

// 代入演算子: |=
shared_ptr<Var> PackageMinosys::eval_op_assignor(Content *c) {
  // 左辺
  Content *lhs = c->pc.at(0);

  // 変数を探す; なければ作成する
  shared_ptr<Var> &v1 = createVar(lhs->op, lhs->pc);

  // TODO: メンバー変数の検索

  // 右辺
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      v1->inum |= v2->inum;
      return v1;

    case VT_DNUM:
      v1->inum |= (int)v2->inum;
      return v1;
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      v1->vtype = VT_INT;
      v1->inum = (int)v1->dnum | v2->inum;
      return v1;

    case VT_DNUM:
      v1->vtype = VT_INT;
      v1->inum = (int)v1->dnum | (int)v2->dnum;
      return v1;
    }
    break;
  }

  // 評価できない場合は左辺の値を返す
  return v1;
}

// 代入演算子: ^=
shared_ptr<Var> PackageMinosys::eval_op_assignxor(Content *c) {
  // 左辺
  Content *lhs = c->pc.at(0);

  // 変数を探す; なければ作成する
  shared_ptr<Var> &v1 = createVar(lhs->op, lhs->pc);

  // TODO: メンバー変数の検索

  // 右辺
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      v1->inum ^= v2->inum;
      return v1;

    case VT_DNUM:
      v1->inum ^= (int)v2->inum;
      return v1;
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      v1->vtype = VT_INT;
      v1->inum = (int)v1->dnum ^ v2->inum;
      return v1;

    case VT_DNUM:
      v1->vtype = VT_INT;
      v1->inum = (int)v1->dnum ^ (int)v2->dnum;
      return v1;
    }
    break;
  }

  // 評価できない場合は左辺の値を返す
  return v1;
}

// 代入演算子: <<=
shared_ptr<Var> PackageMinosys::eval_op_assignlsh(Content *c) {
  // 左辺
  Content *lhs = c->pc.at(0);

  // 変数を探す; なければ作成する
  shared_ptr<Var> &v1 = createVar(lhs->op, lhs->pc);

  // TODO: メンバー変数の検索

  // 右辺
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      v1->inum <<= v2->inum;
      return v1;

    case VT_DNUM:
      v1->inum <<= (int)v2->dnum;
      return v1;
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      v1->vtype = VT_INT;
      v1->inum = (int)v1->dnum << v2->inum;
      return v1;

    case VT_DNUM:
      v1->vtype = VT_INT;
      v1->inum = (int)v1->dnum << (int)v2->dnum;
      return v1;
    }
    break;

  case VT_STRING:
    switch (v2->vtype) {
    case VT_INT:
      v1->str += createMulString(v2->inum, " ");
      return v1;

    case VT_DNUM:
      v1->str += createMulString((int)v2->dnum, " ");
      return v1;
    }
    break;
  }

  // 評価できない場合は左辺の値を返す
  return v1;
}

// 代入演算子: >>=
shared_ptr<Var> PackageMinosys::eval_op_assignrsh(Content *c) {
  // 左辺
  Content *lhs = c->pc.at(0);

  // 変数を探す; なければ作成する
  shared_ptr<Var> &v1 = createVar(lhs->op, lhs->pc);

  // TODO: メンバー変数の検索

  // 右辺
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      v1->inum >>= v2->inum;
      return v1;

    case VT_DNUM:
      v1->inum >>= (int)v2->dnum;
      return v1;
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      v1->vtype = VT_INT;
      v1->inum = (int)v1->dnum >> v2->inum;
      return v1;

    case VT_DNUM:
      v1->vtype = VT_INT;
      v1->inum = (int)v1->dnum >> (int)v2->dnum;
      return v1;
    }
    break;

  case VT_STRING:
    switch (v2->vtype) {
    case VT_INT:
      v1->str = createMulString(v2->inum, " ") + v1->str;
      return v1;

    case VT_DNUM:
      v1->str = createMulString((int)v2->dnum, " ") + v1->str;
      return v1;
    }
    break;
  }

  // 評価できない場合は左辺の値を返す
  return v1;
}

// 前置 +1 演算子の評価
shared_ptr<Var> PackageMinosys::eval_op_preIncr(Content *c) {
  // 変数を探す; なければ作成する
  Content *lhs = c->pc.at(0);
  shared_ptr<Var> &v = createVar(lhs->op, lhs->pc);

  switch (v->vtype) {
  case VT_NULL:
    v->vtype = VT_INT;
    v->inum = 1;
    break;

  case VT_INT:
    v->inum++;
    break;

  case VT_DNUM:
    v->dnum += 1;
    break;
  }

  return v;
}

// 後置 +1 演算子の評価
shared_ptr<Var> PackageMinosys::eval_op_postIncr(Content *c) {
  // 変数を探す
  Content *lhs = c->pc.at(0);
  shared_ptr<Var> &v = createVar(lhs->op, lhs->pc);
  shared_ptr<Var> vclone(v->clone());

  if (vclone->vtype == VT_NULL) {
    vclone->vtype = VT_INT;
    vclone->inum = 0;
  }

  switch (v->vtype) {
  case VT_NULL:
    v->vtype = VT_INT;
    v->inum = 1;
    break;

  case VT_INT:
    v->inum++;
    break;

  case VT_DNUM:
    v->dnum += 1;
    break;
  }

  return vclone;
}

// 前置 -1 演算子の評価
shared_ptr<Var> PackageMinosys::eval_op_preDecr(Content *c) {
  // 変数を探す
  Content *lhs = c->pc.at(0);
  shared_ptr<Var> &v = createVar(lhs->op, lhs->pc);

  switch (v->vtype) {
  case VT_NULL:
    v->vtype = VT_INT;
    v->inum = -1;
    break;

  case VT_INT:
    v->inum--;
    break;

  case VT_DNUM:
    v->dnum -= 1;
    break;
  }

  return v;
}

// 後置 -1 演算子の評価
shared_ptr<Var> PackageMinosys::eval_op_postDecr(Content *c) {
  // 変数を探す
  Content *lhs = c->pc.at(0);
  shared_ptr<Var> &v = createVar(lhs->op, lhs->pc);
  shared_ptr<Var> vclone(v->clone());

  switch (v->vtype) {
  case VT_NULL:
    v->vtype = VT_INT;
    v->inum = -1;
    break;

  case VT_INT:
    v->inum--;
    break;

  case VT_DNUM:
    v->dnum -= 1;
    break;
  }

  return vclone;
}

// 比較演算子: <
shared_ptr<Var> PackageMinosys::eval_op_lt(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_NULL:
    switch (v2->vtype) {
    case VT_INT:
    case VT_DNUM:
      return make_shared<Var>(1);

    case VT_STRING:
      return make_shared<Var>((int)(v2->str.empty() ? 0 : 1));
    }
    break;

  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>((int)(v1->inum < v2->inum ? 1 : 0));

    case VT_DNUM:
      return make_shared<Var>((int)(v1->inum < v2->dnum ? 1: 0));
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>((int)(v1->dnum < v2->inum ? 1 : 0));

    case VT_DNUM:
      return make_shared<Var>((int)(v1->dnum < v2->dnum ? 1: 0));
    }
    break;

  case VT_STRING:
    if (v2->vtype == VT_STRING) {
      return make_shared<Var>((int)(v1->str < v2->str ? 1 : 0));
    }
  }

  // 判定できない場合は[偽]を返す
  return make_shared<Var>((int)0);
}

// 比較演算子: <=
shared_ptr<Var> PackageMinosys::eval_op_lteq(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_NULL:
    switch (v2->vtype) {
    case VT_NULL:
    case VT_INT:
    case VT_DNUM:
    case VT_STRING:
      return make_shared<Var>(1);
    }
    break;

  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>((int)(v1->inum <= v2->inum ? 1 : 0));

    case VT_DNUM:
      return make_shared<Var>((int)(v1->inum <= v2->dnum ? 1: 0));
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>((int)(v1->dnum <= v2->inum ? 1 : 0));

    case VT_DNUM:
      return make_shared<Var>((int)(v1->dnum <= v2->dnum ? 1: 0));
    }
    break;

  case VT_STRING:
    if (v2->vtype == VT_STRING) {
      return make_shared<Var>((int)(v1->str <= v2->str ? 1 : 0));
    }
  }

  // 判定できない場合は[偽]を返す
  return make_shared<Var>((int)0);
}

// 比較演算子: >
shared_ptr<Var> PackageMinosys::eval_op_gt(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v2->vtype) {
  case VT_NULL:
    switch (v1->vtype) {
    case VT_INT:
    case VT_DNUM:
      return make_shared<Var>(1);

    case VT_STRING:
      return make_shared<Var>((int)(v1->str.empty() ? 0 : 1));
    }
    break;

  case VT_INT:
    switch (v1->vtype) {
    case VT_INT:
      return make_shared<Var>((int)(v1->inum > v2->inum ? 1 : 0));

    case VT_DNUM:
      return make_shared<Var>((int)(v1->dnum > v2->inum ? 1: 0));
    }
    break;

  case VT_DNUM:
    switch (v1->vtype) {
    case VT_INT:
      return make_shared<Var>((int)(v1->inum > v2->dnum ? 1 : 0));

    case VT_DNUM:
      return make_shared<Var>((int)(v1->dnum > v2->dnum ? 1: 0));
    }
    break;

  case VT_STRING:
    if (v2->vtype == VT_STRING) {
      return make_shared<Var>((int)(v1->str > v2->str ? 1 : 0));
    }
  }

  // 判定できない場合は[偽]を返す
  return make_shared<Var>((int)0);
}

// 比較演算子: >=
shared_ptr<Var> PackageMinosys::eval_op_gteq(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v2->vtype) {
  case VT_NULL:
    switch (v1->vtype) {
    case VT_NULL:
    case VT_INT:
    case VT_DNUM:
    case VT_STRING:
      return make_shared<Var>(1);
    }
    break;

  case VT_INT:
    switch (v1->vtype) {
    case VT_INT:
      return make_shared<Var>((int)(v1->inum >= v2->inum ? 1 : 0));

    case VT_DNUM:
      return make_shared<Var>((int)(v1->dnum >= v2->inum ? 1: 0));
    }
    break;

  case VT_DNUM:
    switch (v1->vtype) {
    case VT_INT:
      return make_shared<Var>((int)(v1->inum >= v2->dnum ? 1 : 0));

    case VT_DNUM:
      return make_shared<Var>((int)(v1->dnum >= v2->dnum ? 1: 0));
    }
    break;

  case VT_STRING:
    if (v2->vtype == VT_STRING) {
      return make_shared<Var>((int)(v1->str >= v2->str ? 1 : 0));
    }
  }

  // 判定できない場合は[偽]を返す
  return make_shared<Var>((int)0);
}

// 比較演算子: !=
shared_ptr<Var> PackageMinosys::eval_op_neq(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  return make_shared<Var> ((int)(*v1 == *v2 ? 0 : 1));
}

// 比較演算子: ==
shared_ptr<Var> PackageMinosys::eval_op_eq(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  return make_shared<Var> ((int)(*v1 == *v2 ? 1 : 0));
}

// 二項演算子: +
shared_ptr<Var> PackageMinosys::eval_op_plus(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch(v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->inum + v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->inum + v2->dnum);

    case VT_STRING:
      return make_shared<Var>(to_string(v1->inum) + v2->str);
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->dnum + v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->dnum + v2->dnum);

    case VT_STRING:
      return make_shared<Var>(to_string(v1->dnum) + v2->str);
    }
    break;

  case VT_STRING:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->str + to_string(v2->inum));

    case VT_DNUM:
      return make_shared<Var>(v1->str + to_string(v2->dnum));

    case VT_STRING:
      return make_shared<Var>(v1->str + v2->str);
    }
    break;
  }

  // 無効な演算
  return make_shared<Var>();
}

// 二項演算子: -
shared_ptr<Var> PackageMinosys::eval_op_minus(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->inum - v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->inum - v2->dnum);
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->dnum - v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->dnum - v2->dnum);
    }
    break;
  }

  // 評価できない場合は NULL を返す
  return make_shared<Var>();
}

// 二項演算子: *
shared_ptr<Var> PackageMinosys::eval_op_multiply(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->inum * v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->inum * v2->dnum);

    case VT_STRING:
      return make_shared<Var>(createMulString(v1->inum, v2->str));
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->dnum * v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->dnum * v2->dnum);

    case VT_STRING:
      return make_shared<Var>(createMulString((int)v1->dnum, v2->str));
    }
    break;

  case VT_STRING:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(createMulString(v2->inum, v1->str));

    case VT_DNUM:
      return make_shared<Var>(createMulString((int)v2->dnum, v1->str));
    }
    break;
  }

  // 無効な演算
  return make_shared<Var>();
}

// 二項演算子: /
shared_ptr<Var> PackageMinosys::eval_op_div(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->inum / v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->inum / v2->dnum);
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->dnum / v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->dnum / v2->dnum);
    }
    break;
  }

  // 無効な演算
  return make_shared<Var>();
}

// 二項演算子: %
shared_ptr<Var> PackageMinosys::eval_op_mod(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->inum % v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->inum % (int)v2->dnum);
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>((int)v1->dnum % v2->inum);

    case VT_DNUM:
      return make_shared<Var>((int)v1->dnum % (int)v1->dnum);
    }
    break;
  }

  // 無効な演算の場合は NULL を返す
  return make_shared<Var>();
}

// 二項演算子: &
shared_ptr<Var> PackageMinosys::eval_op_and(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->inum & v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->inum & (int)v2->dnum);
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>((int)v1->dnum & v2->inum);

    case VT_DNUM:
      return make_shared<Var>((int)v1->dnum & (int)v2->dnum);
    }
    break;
  }

  // 無効な演算の場合は NULL を返す
  return make_shared<Var>();
}

// 二項演算子: |
shared_ptr<Var> PackageMinosys::eval_op_or(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->inum | v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->inum | (int)v2->dnum);
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>((int)v1->dnum | v2->inum);

    case VT_DNUM:
      return make_shared<Var>((int)v1->dnum | (int)v2->dnum);
    }
  }

  // 無効な演算の場合は NULL を返す
  return make_shared<Var>();
}

// 二項演算子: ^
shared_ptr<Var> PackageMinosys::eval_op_xor(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->inum ^ v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->inum ^ (int)v2->dnum);
    }
    break;

  case VT_DNUM:
    switch(v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->inum ^ (int)v2->dnum);

    case VT_DNUM:
      return make_shared<Var>((int)v1->dnum ^ (int)v2->dnum);
    }
    break;
  }

  // 無効な演算の場合は NULL を返す
  return make_shared<Var>();
}

// 二項演算子: &&
shared_ptr<Var> PackageMinosys::eval_op_logand(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));

  if (!v1->isTrue()) {
    return make_shared<Var>((int)0);
  }
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));
  return make_shared<Var>(v2->isTrue() ? 1: (int)0);
}

// 二項演算子: ||
shared_ptr<Var> PackageMinosys::eval_op_logor(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));

  if (v1->isTrue()) {
    return make_shared<Var>(1);
  }
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));
  return make_shared<Var>(v2->isTrue() ? 1 : (int)0);
}

// 二項演算子: <<
shared_ptr<Var> PackageMinosys::eval_op_lsh(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->inum << v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->inum << (int)v2->inum);
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>((int)v1->dnum << v2->inum);

    case VT_DNUM:
      return make_shared<Var>((int)v1->dnum << (int)v2->dnum);
    }
    break;

  case VT_STRING:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->str + createMulString(v2->inum, " "));

    case VT_DNUM:
      return make_shared<Var>(v1->str + createMulString((int)v2->dnum, " "));
    }
    break;
  }

  // 無効な演算の場合は NULL を返す
  return make_shared<Var>();
}

// 二項演算子: >>
shared_ptr<Var> PackageMinosys::eval_op_rsh(Content *c) {
  shared_ptr<Var> v1 = evaluate(c->pc.at(0));
  shared_ptr<Var> v2 = evaluate(c->pc.at(1));

  switch (v1->vtype) {
  case VT_INT:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(v1->inum >> v2->inum);

    case VT_DNUM:
      return make_shared<Var>(v1->inum >> (int)v2->dnum);
    }
    break;

  case VT_DNUM:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>((int)v1->dnum >> v2->inum);

    case VT_DNUM:
      return make_shared<Var>((int)v1->dnum >> (int)v2->dnum);
    }
    break;

  case VT_STRING:
    switch (v2->vtype) {
    case VT_INT:
      return make_shared<Var>(createMulString(v2->inum, " ") + v1->str);

    case VT_DNUM:
      return make_shared<Var>(createMulString((int)v2->dnum, " ") + v1->str);
    }
    break;
  }

  // 無効な演算の場合は NULL を返す
  return make_shared<Var>();
}

// 文字列 s を count 回繰り返した文字列を返す
string PackageMinosys::createMulString(int count, const string &s) {
  string sr;
  for (int i = 0; i < count; i++) {
    sr += s;
  }
  return sr;
}

