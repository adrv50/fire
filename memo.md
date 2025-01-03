# structure map of union-data in struct Node

子ノードの管理に union と `#define` を使用しており、曖昧さを回避するため、
メモリ割り当てと使用方法について記載します

## `nd_(name)`
=> `nd.(var-name)`
- (the kinds used in)

## `nd_value`
=> `nd.obj`
- ND_Value

## `nd_id_target`
=> `nd.nb` <br>
--> `->kind = ND_{ Struct, Function, Enum, ... }` <br>
識別子、もしくはスコープ解決演算子を使用されたそれである場合、何の名前を表しているかの対象を保持する。
- ND_Identifier
- ND_ScopeResol

## `nd_id_enumerator_index`
=> `nd.size`
- ND_Identifier
- ND_ScopeResol

## `nd_scope_resol_first`
=> `nd.na`
- ND_ScopeResol

## `nd_scope_resol_idlist`
=> `nd.list`
- ND_ScopeResol
