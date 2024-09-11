# metro-parser

## 処理の流れ
多分ちゃんと名前ついてる解析法で別にフツーだと思いますが、名前わかんないです。アルゴリズムよりも実装処理をとにかく解説していきます。

### 1. ASTContext を作成
ソースコード全体を見て、
fn class struct enum などの
トークンとそれに紐づく構文を取得する。

例えば、以下の例
```
struct Ringo {
    ...
};

class RingoParty {
    ...
};

fn my_func(a: vector<Ringo>) -> RingoParty {
    ...
}
```

詳細を書く必要はないので省略しているけど、
パーサからまずこのように見ても、↑のように大部分から見たい。

そこで以下の構造体
```cpp
struct ASTContext {
    ASTKind         kind; // 最初のトークンで判断
    AST::Base*      ast;  // 自分自身の構文木 ("..." を問題なく解析できたあとに作成)

    TokenVector  tokens; // "..." にあるトークン列

    std::vector<ASTContext> list; // 

};
```

そしてさっきのソースコードから以下のデータを作る
```
// using ctx = ASTContext;

ctx "(program)" {
    .kind = Kind::Block,
    .list = [
        ctx "Ringo" { ... },
        ctx "RingoParty" { ... },
        ctx "my_func" { ... },
    ]
}
```


