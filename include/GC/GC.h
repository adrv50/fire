#pragma once

// clang-format off
/*

メモリ   =  std::vector<Object*>

1. metro 用の仮想メモリを作成 ( heap )

2. memptr を heap に当てる // memptr = 確保するとき使うメモリ

lifemem = 生存しているオブジェクトの移動先 (Collection)
temp    = Collect の処理をしている間、使用する


      [ heap ]

  [lifemem]  [temp]

###
オブジェクトを作りたい
v
memptr に空きがある  ->  作る
v
空きがない
  v
  Collect 処理が完了もしくは呼ばれていない
    Collection 起動
    memptr = temp に変更

  v
  処理中
    temp を拡張して、空きを作る

###
Collection()
|
処理が終わったら:
  1. heap = lifemem + temp + buffer
  2. memptr = heap
  3. lifemem.clear()
  4. temp.clear()

*/
// clang-format on

#include "Evaluator/Object.h"

#define GC_HEAP_INITIALIZE_SIZE 0x100

#define GC_HEAP_MAXIMUM_SIZE 0x10000

namespace metro::gc {

void InitGC();
void ExitGC();

void ForceCollection();

} // namespace metro::gc