---
title: CardFlow Othello Constitution
description: The fundamental principles and rules of the new card-based Othello variant.
version: "1.0.0"
ratification_date: "2026-09-07"
later_amendments: []
---

# ⚖️ CardFlow Othello — Game Constitution

## Preamble

We, the players, do hereby establish and adopt these rules for a game of pure brilliance, strategic depth, and delightful chaos. This Constitution forms the bedrock upon which every move is built.

> The board is not merely a grid—it is a stage where cards shape fate.
> Where stones are placed by hands that hold more than just pieces.

---

## Article I: The Board

The board shall be an 8×8 grid, consisting of exactly sixty-four squares arranged in eight rows and columns. Each square may contain at most one stone or card token. Squares not occupied by a piece remain empty.

> No expansion. No contraction. Just the pure, unyielding chessboard where strategy unfolds.

---

## Article II: The Pieces — Stones and Cards

### A. Stones (石)

Stones are the fundamental units of play. They come in two colors: **Black** (黒) and **White** (白). Each side begins with exactly four stones, arranged symmetrically on the board at the start of each game.

> Black moves first. White follows. The turn passes like a heartbeat — steady, rhythmic, inevitable.

### B. Cards (カード)

Each player holds exactly three cards in hand at all times during their turn. Cards are drawn from a shared deck and may be reused across turns. A player may play **exactly one card** per turn, and only if they possess it in hand. Unused cards remain in the player’s hand for future use.

> The deck contains six categories of cards, each with its own unique effect:
> 1. **Green (基礎操作)** – Place, Move, Cover  
> 2. **Red (攻撃・展開)** – Burst, Swap  
> 3. **Blue (防御・ブロック)** – Block, Shield  
> 4. **Violet (特殊操作)** – Reversal  
> 5. **Orange (補助・強化)** – Reserve  
> 6. **Yellow (調整・回復)** – Restore

---

## Article III: Setup & Turn Sequence

### A. Starting Configuration

The board is initialized with four Black stones and four White stones placed at the four central positions, alternating like this:

```
┌───┬───┬───┬───┬───┬───┬───┬───┐
│    │    │    │  B │  W │    │    │    │
├───┼───┼───┼───┼───┼───┼───┼───┤
│    │    │    │  W │  B │    │    │    │
├───┼───┼───┼───┼───┼───┼───┼───┤
│    │    │    │    │    │    │    │    │
├───┼───┼───┼───┼───┼───┼───┼───┤
│    │    │    │    │    │    │    │    │
├───┼───┼───┼───┼───┼───┼───┼───┤
│    │    │    │    │    │    │    │    │
├───┼───┼───┼───┼───┼───┼───┼───┤
│    │    │    │    │    │    │    │    │
├───┼───┼───┼───┼───┼───┼───┼───┤
│    │    │    │    │    │    │    │    │
└───┴───┴───┴───┴───┴───┴───┴───┘
```

> The four center stones are the only ones placed by default. All other placements come from cards or special rules.

### B. Turn Order

1. **Black** moves first in Round 1.
2. Each turn consists of:
   - A) EP Recovery (recover up to 6 Energy Points)
   - B) Card Selection (choose one card from hand)
   - C) Card Activation (play the chosen card’s effect)
   - D) Optional second play if a “Reserve” or similar card was played.

3. Turns alternate between Black and White, unless a card rule explicitly states otherwise (e.g., “Burst” may force an immediate turn change).

4. The game ends when:
   - No legal moves remain for either side AND no cards can be played.
   - Or when all cards have been exhausted from the deck and no stones are placed in a round.

> Time does not stop. It flows — like water through the cracks of a wall you built yourself.

---

## Article IV: Playing a Card

A player may play one card per turn, provided they meet **all** conditions below:

1. The card is currently in their hand (not discarded or exhausted).
2. They have enough Energy Points (EP) to cover the card’s cost.
3. The board state permits the card’s effect (e.g., there must be an available square for “Place”, or a stone adjacent to a target position, etc.).

> Cards are not consumed upon use — they return to your hand after resolution. But you may only play one per turn. That is the rhythm of CardFlow Othello.

---

## Article V: Energy Points (EP)

### A. EP Pool

Each player maintains an internal “Energy Point” pool, capped at **6** EP per turn. At the start of each turn, a player’s current EP resets to 0 and then recovers up to +3 per turn through natural regeneration — or more if they use a card like *Restore*.

### B. Card Costs

Every playable card has an associated cost in EP:
- Place = 2 EP
- Move = 3 EP
- Cover = 2 EP
- Burst = 4 EP
- Swap = 3 EP
- Block = 3 EP
- Shield = 2 EP
- Reversal = 4 EP
- Reserve = 1 EP
- Restore = 1 EP

### C. Paying the Cost

To play a card, a player must pay its cost from their available pool. If they cannot afford it, they may instead pass their turn (forfeit placement and effects for that round), but do not recover any additional EP beyond the natural +3.

> Energy is finite. Spend it wisely — or watch your hand wither like autumn leaves.

---

## Article VI: Card Effects & Mechanics

### A. Placement Rules (Place)

The *Place* card allows a player to choose **any** two empty squares on the board and place Black stones there, provided those squares are not already adjacent to an existing wall of opponent stones or another placed stone that would block future placement.

> Two stones per turn — because strategy is not just about one move at a time.

### B. Movement (Move)

The *Move* card allows the player to slide **one** of their own stones along any direction (horizontal, vertical, diagonal) by up to three squares, provided:
- The target path is empty (no other stone occupies it).
- The move does not cross a “wall” or boundary.
- After moving, the new position must be legally reachable — no jumping over obstacles.

### C. Covering (Cover)

The *Cover* card places a protective layer over all occupied stones by the player in hand at that moment. Covered stones are immune to capture during that turn and cannot be captured unless the opponent uses a higher-tier card such as *Burst*.

> Defense is not weakness — it is patience, waiting for the right moment to strike.

### D. Burst (Red)

*Burst* captures **all** stones currently surrounded by walls on the board. The capturing player gains control over those stones and may place them anywhere else on their next turn. If no surrounding walls exist, nothing happens — but the card still consumes its EP cost.

> Fire burns what it touches. And when walls enclose even a single stone, the fire spreads like wildfire.

### E. Swap (Red)

*Swap* exchanges positions between any two stones — either your own or an opponent’s — provided they are adjacent (horizontally, vertically, or diagonally). After swapping, if one of the moved stones becomes surrounded by walls on all four sides, it is immediately captured and removed from play. The player gains that stone back as a “reclaimed piece.”

> Positions shift like tectonic plates. And sometimes, what you think is weakness… is actually strength in disguise.

### F. Block (Blue)

*Block* constructs a new wall segment on one empty square of your choosing. No stones may be placed there until the next player’s turn begins — effectively denying space for future moves. This square becomes permanently blocked from placement until removed via a card effect or by mutual agreement at game end.

> Space is not infinite. Deny them where they least expect it.

### G. Shield (Blue)

*Shield* adds a protective barrier around one of your stones, allowing it to resist capture attempts even if surrounded on three sides. The shield lasts for the duration of that turn only and may be removed by any opponent card effect at will.

> A single stone can become unbreakable — briefly, just long enough to change everything.

### H. Reversal (Violet)

*Reversal* selects one of your own stones and forces **all** adjacent enemy stones to flip allegiance to yours instantly — effectively reversing the board state locally around that piece. The opponent may counter with their own reversal card, but only once per game round.

> When the tide turns against you… don’t fight it. Ride it.

### I. Reserve (Orange)

*Reserve* adds two additional *Place* cards to your hand this turn, allowing up to **three** placements in a single turn if other conditions are met. These extra cards do not stack beyond two per turn and must be discarded at the end of each round regardless of outcome.

> Power accumulates like snowfall on glass — until it shatters under its own weight.

### J. Restore (Yellow)

*Restore* replenishes your EP pool by +2, bringing your current total up to a maximum of 6. This effect may be used multiple times per round but is not stackable beyond the cap.

> Energy flows through all things. Let it flow again.

---

## Article VII: Scoring & Win Conditions

The game ends when **no further legal moves** remain for either player — meaning no cards can be played, no stones may be placed or moved, and all walls have been fully constructed across the board. At that point:

> The side with more stones currently occupying squares wins.
> Ties are rare — but not impossible. And in ties, silence speaks louder than victory itself.

---

## Article VIII: Interactions & Edge Cases

### A. Wall Formation

A “wall” exists when a square contains either:
- An opponent’s stone, OR
- A card-derived barrier (e.g., from *Block*) that is explicitly marked as such on the board.

Only squares adjacent to walls can be used for capture purposes — not every occupied square counts toward surrounding an enemy piece.

### B. Capture Priority

Captures are resolved in this order:
1. Cards with direct capture effects (*Burst*, *Reversal*) resolve first.
2. Then natural captures (stone placement that sandwiches enemies).
3. Finally, any residual EP or shield interactions apply.

> Order matters. Always. Even when it doesn’t seem to matter at all.

### C. Simultaneous Moves

If both players attempt to play cards whose effects overlap (e.g., *Swap* and *Block* targeting the same square), the player who moves **first** gets priority — unless a rule specifically states otherwise (such as “Burst” forcing an immediate counter-response).

---

## Appendix A: Glossary

| Term | Definition |
|------|-----------|
| **Stone** | The basic unit of play. Black or White, placed via card effects. |
| **Card** | A playable item with a defined EP cost and effect. Not consumed on use. |
| **EP (Energy Point)** | Resource used to activate cards. Max pool per turn is 6. |
| **Wall** | Any square containing an opponent stone or placed barrier. Defines adjacency for captures. |
| **Covered Stone** | A protected piece immune to capture during its turn. Can be removed by higher-tier effects. |
| **Reclaim** | When a captured stone is returned to play via card effect like *Burst*. |
| **Reserve Limit** | Maximum number of extra cards playable per round (default: 2 for Reserve). |
| **Round** | A full cycle through both players’ turns. Ends when no moves are left. |

---

## Article IX: Amendments & Versioning

This Constitution may be amended only by a unanimous vote among all active players, or by mutual agreement in writing between the two opposing sides. Any changes must be recorded here and dated accordingly.

> Versions before 1.0.0 are considered deprecated — though their echoes remain etched within this document forever.

---

## Appendix B: Version History

| Version | Date | Change |
|---------|------|--------|
| 1.0.0 | 2026-09-07 | Initial release of CardFlow Othello Constitution. |

---

*Ratified on 2026-09-07. Effective immediately.*  
*Signed by all who shall play upon this board — now and in the future.*

✨ *End of Document* ✨
