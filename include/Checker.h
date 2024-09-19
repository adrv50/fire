#pragma once

#include "AST.h"

/*
・構文木を探索しているとき、class, enum,
  など型を定義する構文に遭遇したら
  = リストに名前を追加する
    --->
    追加した名前に、付属情報として（どこのスコープに属しているか）を付与。


・型名として評価される構文に遭遇
  = 型名の先頭につける、暗黙的に省略されたスコープ名を調べる。

    AA::BB に自分が位置するとき、C を見つけたとすると、
    BB の中で C を探し、なければ AA 、グローバルの順で探す。



A::B::C
  -->  C{ parent={B, A}, ... }



*/

namespace fire::checker {

class Checker {
public:
};

} // namespace fire::checker