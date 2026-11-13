---
title: CardFlow Othello — Technical Specification v0.1
source_constitution: .specify/memory/constitution.md
version: "0.1.0"
created_date: "{CREATED_DATE}"
status: draft
next_command: /speckit-specify-implement
---

# 📄 CardFlow Othello — Technical Specification v0.1

## 概要

本ドキュメントは、`CardFlow Othello` という名前の新規オセロ派生ゲームの完全な仕様に係るものです。  
これは「手札＋カード」を駆使して石を配置・移動・ブロックできる、戦略と偶然性のバランスが楽しめるボードゲームです。

---

## 目次

1. [用語定義](#1-用語定義)
2. [ボード構成](#2-ボード構成)
3. [プレイヤー・ターン管理](#3-プレイヤーターン管理)
4. [カードシステム](#4-カードシステム)
5. [EP システム（エネルギーポイント）](#5-ep-システム)
6. [石の配置・移動・ブロック・捕獲](#6-石の配置移動ブロック捕獲)
7. [ゲーム終了条件とスコアリング](#7-ゲーム終了条件とスコアリング)
8. [特殊な状況と例外処理](#8-特殊な状況と例外処理)
9. [データ構造・内部表現](#9-データ構造内部表現)
10. [検証・テスト戦略](#10-検証テスト戦略)

---

## 1. 用語定義

| 用語 | 説明 |
|------|------|
| **ボード** | 8x8 のグリッド。各マスは空か石か壁で状態を持つ。 |
| **石** | プレイヤー（黒・白）が持つ基本的な単位。`Stone` クラスで管理される。 |
| **カード** | 1 ターンに 1 枚しかプレイできない、EP を消費して発動する効果を持つアイテム。 |
| **手札** | プレイヤーが保持しているカードのリスト。最大 3 枚。使い切ったら元に戻る（リサイクル）。 |
| **EP（エネルギーポイント）** | カードを発動するための通貨。毎ターン +3 で回復し、上限は 6。 |
| **壁** | 石やブロックによって形成された「境界」。捕獲判定の基準となる。 |
| **カバー** | カバーカードで付与される保護状態。そのターンの捕獲を回避する。 |
| **Round** | 両プレイヤーが各 1 回ずつターンを取ったサイクル。ゲームは Round が複数回繰り返される。 |
| **Turn** | 1 プレイヤーの行動単位（EP 回復→カード選択→発動）。 |

---

## 2. ボード構成

ボードは `Board` クラスで管理され、以下のようなデータ構造を持つ：

```python
class Board:
    def __init__(self, size=8):
        self.size = size                # 8
        self.grid: List[List[Stone | Wall | None]] = [[None]*size for _ in range(size)]

    def place_stone(self, r: int, c: int, color: str) -> Stone:
        ...

    def remove_stone_at(self, r: int, c: int) -> Stone:
        ...

    def add_wall_segment(self, r: int, c: int) -> WallSegment:
        ...

    def is_occupied(self, r: int, c: int) -> bool:
        ...

    def get_neighbors(self, r: int, c: int) -> List[Tuple[int,int]]:
        # 4 方向＋斜め（8 方）の隣接マスを返す
        ...
```

---

## 3. プレイヤー・ターン管理

- `Player` クラス：色 (`color`)、現在の EP (`energy_points`)、手札 (`hand: List[Card]`) を保持。
- ゲームループは「Black のターンのみ」＋「White のターンのみ」を交互に実行する。
- Black は Round 1 で先攻し、各ターンで +3EP を回復（ただし上限 6）。

---

## 4. カードシステム

### 4.1 Card ベースクラスとサブクラス

```python
class Card:
    def __init__(self, name: str, category: str, cost: int, effect: Callable):
        self.name = name
        self.category = category
        self.cost = cost
        self.effect = effect     # 発動時に呼ばれる関数

    def play(self, player: Player, board: Board) -> Result:
        if not self.can_play(player, board):
            return Result(status="invalid", reason="Cannot afford or invalid target.")
        self.effect(board)       # ゲーム状態を変更
        return Result(status="ok")

class Place(Card): cost=2
    effect → 空マス 2 カ所を選択して石を置く（黒または白、プレイヤー色に合わせて）。

class Move(Card): cost=3
    effect → 自分の石を隣接マスへ最大 3 マス移動させる。壁や境界を超えられない。

class Cover(Card): cost=2
    effect → プレイヤーのすべての石に「Cover」フラグを立てる。

class Burst(Card): cost=4
    effect → 「壁」に囲まれた敵石をすべて捕獲し、手札に戻す（または再利用可能にする）。

class Swap(Card): cost=3
    effect → 隣接する石同士を入れ替える。入れ替え後、片方が完全な「壁」に挟まれば即座に捕獲される。

class Block(Card): cost=3
    effect → 指定マスを「WallSegment」として登録する（その後は放置不可）。

class Shield(Card): cost=2
    effect → 1 つの石に Shield を付与。ターンの残り時間はカバーと同じように保護。

class Reversal(Card): cost=4
    effect → 選択した自分の石から、隣接する敵石をすべて味方へ「裏返す」。

class Reserve(Card): cost=1
    effect → 次のターンに Place カードが +2 枚補充される（最大追加数は 2）。

class Restore(Card): cost=1
    effect → EP を +2 回復（上限 6）。
```

---

## 5. EP システム（エネルギーポイント）

- 各プレイヤーは `energy_points: int` を持つ。
- ターン開始時に `+3` が加算され、最大値は `6` でクリップされる。
- カード発動にはコストがかかる。支払えない場合は「パスターン」として処理する（ただし EP は回復しない）。

---

## 6. 石の配置・移動・ブロック・捕獲

| 動作 | 条件 | 結果 |
|------|------|------|
| **Place** | 空マスが少なくとも 2 つあること | 2 マスに石を置く（黒または白を選択）。 |
| **Move** | 目標マスのパスが連続して空であること、かつ壁や境界を超えないこと | 石が移動する。 |
| **Block** | 指定マスが空であること | そのマスは「ブロック済み」となる。その上には石を置けない。 |
| **Cover / Shield** | 対象の石が存在すること | 保護状態に付与される。 |
| **Burst / Reversal / Swap** | 対象となる敵石が壁で囲まれている、または隣接していること | 捕獲・入れ替え・裏返しが発生する。 |

---

## 7. ゲーム終了条件とスコアリング

- ゲームは「両プレイヤーともに有効な移動ができなくなった」時点で終了する。
- その時点での各プレイヤーの石数で勝敗が決まる：多ければ勝ち、同点なら引き分け。

```python
def game_over(board: Board) -> str | None:
    if not any_can_play(board):
        black_count = sum(1 for stone in board.stones if stone.color == "black")
        white_count = 32 - black_count
        return "black" if black_count > white_count else ("white" if white_count > black_count else "draw")
    return None
```

---

## 8. 特殊な状況と例外処理

| シナリオ | 振る舞い |
|----------|-----------|
| 壁に囲まれた石があるが、EP が不足している | カードをプレイできない → パスターン。 |
| Reserve を使った追加 Place 枚数が上限（2）を超える場合 | 超過分は無効化される。 |
| Swap で入れ替え後、両方の石が捕獲対象になる場合 | どちらも捕獲されず、そのままの状態で次のターンへ。 |
| Reversal が同時に複数回発動した場合 | 先後順で適用し、最後に出た効果で確定する（最後の勝利者が優先）。 |

---

## 9. データ構造・内部表現

```python
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

@dataclass(frozen=True)
class Stone:
    color: str          # "black" or "white"
    r: int              # 行（0～7）
    c: int              # 列（0～7）
    covered: bool = False
    shielded: bool = False

@dataclass(frozen=True)
class WallSegment:
    r: int
    c: int
    owner: str          # "black" or "white" or "neutral"

@dataclass
class Card:
    name: str
    category: str
    cost: int
    effect_func: Callable[[Board], None]   # 簡易型。実装では引数も加える。
```

---

## 10. 検証・テスト戦略

- **単体テスト**：各カードの `effect` が意図通りに動作するか確認（例：Place で正しいマスが埋まるか、Move で壁を超えないか）。
- **統合テスト**：複数のカードを組み合わせるときの競合挙動を確認（Reserve による複数 Place の同時実行など）。
- **境界値テスト**：EP がちょうどコストと等しい場合、オーバーフローするケースなど。

---

## 次へのステップ

本仕様書に基づき、Python で実装・動作確認を行います。  
次に `/speckit-specify-implement` を使ってコード生成の準備に入ります。



## Clarifications

### Session 2026-09-08

- **Q1（ゲーム終了・勝利宣言のタイミング）：**
  - **回答：C) 両方のターンで判定する**
  - Black と White のどちらかのターンの後、常に即時に勝敗判定を行う。ただし「有効な移動ができない」という条件は双方共通適用。Black が先手でも、White のターンの後にゲームが終了することはある（置ける場所がないため）。

- **Q2（カード配布ルール）：**
  - **回答：B) カードカテゴリバランス配り**
  - ゲーム開始時（Round 1 の Black のターン）、デッキからランダムに 3 枚引くが、以下の制約あり：
    - Green: 0-2 枚, Red: 0-2 枚, Blue: 0-2 枚, Violet: 0-1 枚, Orange: 0-1 枚, Yellow: 0-1 枚
    - 「Green が 3 枚」は禁止（強すぎるため）。
    - 結果として、Black と White は「カテゴリ偏りなしのランダム配分」で初期手札を得る。

### Session 2026-11-13（捕獲ルールの確定）

- **Q3（「挟み」の定義）：**
  - **回答：オセロ式の一直線の挟み**
  - 縦・横・斜めの一直線上で、連続する敵石の列の**両端**が「自分の石」または「壁」であるとき、その列は挟まれている。
  - 盤の端は挟む側として数えない。空マスが間にあれば挟みは成立しない。

- **Q4（Place による裏返し）：**
  - **回答：裏返る（置き場所の制約なし）**
  - Place で置いた石ごとに、その石を起点とする 8 方向で挟まれた敵石を裏返す（通常のオセロと同じ）。
  - ただし「1 枚以上裏返せるマスにしか置けない」という制約はなく、裏返しが 0 枚でも置ける。

- **Q5（捕獲された石の扱い）：**
  - **回答：盤から取り除かれ、空マスになる**
  - 裏返して味方にするのは Place と Reversal の役割、取り除くのが捕獲（Burst / Swap）の役割。

- **Q6（壁の扱い）：**
  - **回答：両者の壁**
  - Block で置いた壁は、設置者に関係なく両プレイヤーにとって挟む側として働く。

- **Q7（保護された石）：**
  - 保護された石も挟みの列の一部として数える。そのうえで裏返し・捕獲を受けたとき:
    - **Cover**: 効果を受けない。Cover は使ったプレイヤーの**次のターン開始時**に解除される（相手の 1 ターン分を守る）。
    - **Shield**: 効果を 1 回だけ防ぎ、その時点で Shield が外れる（期限なし）。
    - 両方を持つ石は Cover が優先され、Shield は消費されない。

### Session 2026-11-13（カード効果の確定）

| カード | 対象 | 効果 |
|---|---|---|
| Place (2) | 異なる空マス 2 つ | 順に置き、それぞれ Q4 の裏返しを行う |
| Move (3) | 自分の石、移動先 | 8 方向のいずれかへ直線で 1〜3 マス。途中と移動先は空マス（壁・石・盤外は越えられない）。移動先で Place と同じ裏返しを行う |
| Cover (2) | なし | 自分の全石に Cover を付与（自分の石が 1 個以上必要） |
| Burst (4) | なし | 挟まれた敵石をすべて捕獲（挟まれた敵石が 1 個以上必要） |
| Swap (3) | 自分の石、隣接する敵石 | 2 つを入れ替える。入れ替え後、片方だけが相手に挟まれていればその石を捕獲。両方が挟まれていればどちらも捕獲しない（自分の石が捕獲されることもある） |
| Block (3) | 空マス | 壁を置く（設置者を記録） |
| Shield (2) | Shield の付いていない自分の石 | Shield を付与 |
| Reversal (4) | 自分の石（隣接する敵石が 1 個以上） | 隣接 8 マスの敵石をすべて裏返す（保護は Q7 に従う） |
| Reserve (1) | なし | 次の自分のターン開始時に Place を 2 枚、手札上限とは別枠で加える。そのターン中に使わなかった分は消える。重ねて使っても予約は最大 2 枚 |
| Restore (1) | なし | コストを支払った後に EP +2（上限 6） |

- カードの発動手順: 手札（Reserve の追加分を含む）にある → EP が足りる → 対象が正しい、をすべて満たしたときだけ、EP を支払い、手札から 1 枚除き、効果を適用する。どれかを満たさなければ何も変化しない。
- ターン開始処理: 自分の石の Cover を解除 → EP +3（上限 6）→ Reserve の予約分を追加。
- 未確定（ゲームループ実装時に決める）: デッキの構成・配札、手札の補充（「使い切ったら元に戻る」）、1 ターン 1 枚の制限の扱い。