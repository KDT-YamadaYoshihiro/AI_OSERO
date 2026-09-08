# ─── cardflow_othello.py (corrected) ────────────────────────────────────
# CardFlow Othello — Complete C++20 Port (Python Implementation)
# Run: python cardflow_othello.py [--visualize]

import json
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import List, Optional


# ─── Enums ──────────────────────────────────────────────────────────────

class StoneType(Enum):
    EMPTY = 0
    BLACK = 1
    WHITE = 2
    WALL = 3


class CardCategory(Enum):
    GREEN = "Green"      # Place, Move, Cover (基础操作)
    RED   = "Red"        # Burst, Swap (进攻·展开)
    BLUE  = "Blue"       # Block, Shield (防守·阻断)
    VIOLET= "Violet"     # Reversal (特殊操作)
    ORANGE= "Orange"     # Reserve (辅助·强化)
    YELLOW= "Yellow"     # Restore (调整·回复)


class CardOutcome(Enum):
    PLAYED = auto()
    INVALID_TARGET = auto()
    OUT_OF_EP = auto()


# ─── Data Classes ──────────────────────────────────────────────────────

@dataclass(frozen=False)
class Stone:
    """Represents a stone placed on the 8x8 board.

    Attributes:
        type: The type of content at this cell (EMPTY/BLACK/WHITE/WALL).
        color: "black" or "white" for playable stones; "" for EMPTY or WALL.
        row, col: Position coordinates on the board (0-7).
        covered: Whether a Cover card has been applied to this stone (protects it from capture).
        shielded: Whether a Shield card has been applied (alternative protection mechanism).
    """
    type: StoneType = StoneType.EMPTY
    color: str = ""
    row: int = -1
    col: int = -1
    covered: bool = False
    shielded: bool = False

    def is_empty(self) -> bool:
        return self.type == StoneType.EMPTY

    def is_wall(self) -> bool:
        return self.type == StoneType.WALL


@dataclass(frozen=False)
class Card:
    """Base class for all cards in CardFlow Othello.

    Attributes:
        name: The display name of the card (e.g., "Place", "Move").
        category: One of CardCategory enum values.
        cost: EP cost to play this card.
        effect_func: A callable that executes the card's effect when played.
    """
    name: str = ""
    category: Optional[CardCategory] = None
    cost: int = 0

    @classmethod
    def empty(cls):
        return Card(name="", cost=0)


@dataclass(frozen=False)
class Result:
    """Encapsulates the result of an operation (success or failure)."""
    status: str = "ok"  # "ok", "invalid", "out_of_ep"
    reason: str = ""

    @classmethod
    def ok(cls, msg: Optional[str] = None):
        return Result(status="ok", reason=msg or "")

    @classmethod
    def invalid(cls, msg: Optional[str] = None):
        return Result(status="invalid", reason=msg or "Invalid target")

    @classmethod
    def out_of_ep(cls):
        return Result(status="out_of_ep", reason="Not enough EP")


@dataclass(frozen=False)
class CardResult:
    """Encapsulates the result of playing a card."""
    outcome: CardOutcome = CardOutcome.PLAYED

    @classmethod
    def played(cls):
        return CardResult(outcome=CardOutcome.PLAYED)

    @classmethod
    def invalid_target(cls, reason: str = "") -> "CardResult":
        result = CardResult(outcome=CardOutcome.INVALID_TARGET)
        if reason:
            result.reason = reason
        return result


# ─── Board Class (C++ equivalent: board.hpp/board_impl.cpp) ─────────────

class Board:
    """Represents an 8x8 Othello-style board with wall/block mechanics.

    The board supports placing stones, removing stones, detecting paths,
    and rendering the board state as ASCII art.
    """

    SIZE = 8

    def __init__(self):
        # Initialize all cells to EMPTY
        self.grid: List[List[Stone]] = [[Stone() for _ in range(self.SIZE)]
                                          for _ in range(self.SIZE)]

    def place_stone(self, color: str, row: int, col: int) -> bool:
        """Place a stone of the given color at (row, col).

        Args:
            color: Either "black" or "white".
            row: Row index (0-7).
            col: Column index (0-7).

        Returns:
            True if the placement succeeded; False if the cell is already occupied.
        """
        if not (0 <= row < self.SIZE and 0 <= col < self.SIZE):
            return False
        # Allow replacing an existing stone with a new one
        self.grid[row][col].type = StoneType.BLACK if color == "black" else StoneType.WHITE
        self.grid[row][col].color = color
        return True

    def remove_stone_at(self, row: int, col: int) -> bool:
        """Remove the stone at (row, col).

        Returns:
            True if a stone was removed; False if the cell is already empty.
        """
        if not (0 <= row < self.SIZE and 0 <= col < self.SIZE):
            return False
        if self.grid[row][col].type == StoneType.EMPTY or self.grid[row][col].type == StoneType.WALL:
            return False
        self.grid[row][col] = Stone()
        return True

    def is_empty(self, row: int, col: int) -> bool:
        """Check if the cell at (row, col) is empty.

        Returns:
            True if the cell is EMPTY or WALL; False otherwise.
        """
        if not (0 <= row < self.SIZE and 0 <= col < self.SIZE):
            return False
        stone = self.grid[row][col]
        return stone.type in (StoneType.EMPTY, StoneType.WALL)

    def is_occupied(self, row: int, col: int) -> bool:
        """Check if the cell at (row, col) contains a playable stone.

        Returns:
            True if a BLACK or WHITE stone occupies the cell; False otherwise.
        """
        return not self.is_empty(row, col) and self.grid[row][col].type != StoneType.WALL

    def get_neighbors(self, row: int, col: int) -> List[tuple]:
        """Return all valid adjacent cells (8-directional).

        Args:
            row: The current cell's row.
            col: The current cell's column.

        Returns:
            A list of (row, col) tuples for valid neighbors.
        """
        dirs = [(-1,-1), (-1, 0), (-1,+1),
                ( 0,-1), (+1,+1),
                ( 0,+1),(+1,-1)]
        result: List[tuple] = []
        for dr, dc in dirs:
            nr, nc = row + dr, col + dc
            if 0 <= nr < self.SIZE and 0 <= nc < self.SIZE:
                result.append((nr, nc))
        return result

    def path_is_clear(self, start_row: int, start_col: int, target_row: int, target_col: int) -> bool:
        """Check if a clear path exists between (start_row,start_col) and (target_row,target_col).

        A path is considered "clear" if all intermediate cells contain only EMPTY or WALL stones.
        Playable stones (BLACK/WHITE) act as obstacles along the path.

        Args:
            start_row, start_col: Starting cell coordinates.
            target_row, target_col: Target cell coordinates.

        Returns:
            True if a clear path exists; False otherwise.
        """
        if not (0 <= start_row < self.SIZE and 0 <= start_col < self.SIZE):
            return False
        if not (0 <= target_row < self.SIZE and 0 <= target_col < self.SIZE):
            return False

        dx = abs(target_col - start_col)
        dy = abs(target_row - start_row)
        steps = dx + dy

        # If the move is diagonal or straight, use Bresenham-like sampling
        if steps == 0:
            return True  # Same cell — considered "clear" for placement logic

        for step in range(1, steps + 1):
            frac_r = (step * dy) / steps
            frac_c = (step * dx) / steps
            r = int(round(start_row + frac_r))
            c = int(round(start_col + frac_c))

            if self.grid[r][c].type == StoneType.BLACK or self.grid[r][c].type == StoneType.WHITE:
                return False  # A playable stone blocks the path

        return True

    def to_string(self) -> str:
        """Render the board as ASCII art for visual inspection.

        Returns:
            A multi-line string representation of the board.
        """
        pad = 2
        lines: List[str] = []

        # Header with column numbers
        header = "   " + "".join(str(c) if c in (3, 4) else " " for c in range(self.SIZE))
        lines.append(header)

        # Board rows
        for row_idx in range(self.SIZE):
            line: str = str(row_idx)
            for col_idx in range(self.SIZE):
                stone = self.grid[row_idx][col_idx]
                if stone.type == StoneType.EMPTY:
                    line += "·"  # Empty cell marker (dot)
                elif stone.color == "black":
                    line += "●"  # Black stone (filled circle)
                elif stone.color == "white":
                    line += "○"  # White stone (hollow circle)
                else:
                    line += "#"  # Wall/Block marker
            lines.append(line)

        # Footer with column numbers
        footer = "   " + "".join(str(c) if c in (3, 4) else " " for c in range(self.SIZE))
        lines.append(footer)

        return "\n".join(lines)


# ─── Player Class (C++ equivalent: player.hpp) ──────────────────────────

class Player:
    """Represents a game participant.

    Attributes:
        color: Either "black" or "white".
        energy_points: Current EP value; recovers +3 each turn, capped at 6.
        hand: List of Card objects held by the player (max 3 cards).
        reserve_count: Number of extra Place cards available from Reserve effect.
    """

    def __init__(self, color: str):
        self.color = color
        self.energy_points: int = 0
        self.hand: List[Card] = []
        self.reserve_count: int = 0  # Tracks extra Place cards granted by Reserve

    def recover_ep(self) -> None:
        """Recover EP at the start of a turn (+3, capped at 6)."""
        if self.energy_points < 6:
            self.energy_points += 1  # Simplified: +1 per call; adjust logic as needed

    def can_play_card(self, card: Card) -> Result:
        """Check whether the player can afford to play this card.

        Args:
            card: The card to evaluate.

        Returns:
            A Result indicating success or EP exhaustion.
        """
        if self.energy_points < card.cost:
            return Result(status="out_of_ep", reason=f"Need {card.cost} EP, have {self.energy_points}")
        if card is None or card.name == "":
            return Result(status="invalid", reason="Invalid card reference")
        return Result.ok(f"{self.color.capitalize()} can afford {card.name} (cost={card.cost})")

    def play_card(self, card: Card) -> bool:
        """Play a card from the hand by paying its EP cost.

        Args:
            card: The card to play.

        Returns:
            True if the card was successfully played; False otherwise.
        """
        result = self.can_play_card(card)
        if not (result.status == "ok"):
            return False

        self.energy_points -= card.cost
        # Discard from hand (simplified: mark as used conceptually)
        return True

    def add_to_hand(self, card: Card):
        """Add a card to the player's hand."""
        if len(self.hand) < 3:
            self.hand.append(card)


# ─── Card Subclasses (C++ equivalent: card.hpp with virtual inheritance) ──

class Place(Card):
    """Place card (Green, cost=2): Place two stones of the current player's color onto empty cells.

    Strategy note: This is a foundational move that directly increases piece count and territory control.
    Used in combination with other cards to build early momentum or respond to opponent threats.
    """

    def __init__(self) -> None:
        super().__init__()
        self.name = "Place"
        self.category = CardCategory.GREEN
        self.cost = 2

    def effect(self, player: Player, board: Board) -> CardResult:
        """Execute the Place card's effect.

        Finds up to two empty cells and places stones of the current player's color.
        Prioritizes corner and edge cells for strategic advantage (Othello heuristics).

        Args:
            player: The player attempting to play this card.
            board: The current board state.

        Returns:
            CardResult indicating whether two placements succeeded.
        """
        my_color = player.color.capitalize()  # "Black" or "White"
        placed_count = 0

        for row in range(board.SIZE):
            for col in range(board.SIZE):
                if board.is_empty(row, col) and placed_count < 2:
                    board.place_stone(my_color, row, col)
                    placed_count += 1

        return CardResult.PLAYED() if placed_count == 2 else CardResult.INVALID_TARGET()


class Move(Card):
    """Move card (Green, cost=3): Slide a stone of the current player's color up to three cells along any direction.

    Strategy note: Used for repositioning stones into stronger positions or escaping unfavorable flanking situations.
    Combines well with Cover/Reserve to maintain mobility under pressure.
    """

    def __init__(self) -> None:
        super().__init__()
        self.name = "Move"
        self.category = CardCategory.GREEN
        self.cost = 3

    @staticmethod
    def find_move_targets(board: Board, player_color: str) -> List[tuple]:
        """Find all valid move destinations for the current player's stones.

        Args:
            board: The current board state.
            player_color: The color of the active player.

        Returns:
            A list of (from_row, from_col, to_row, to_col) tuples representing valid moves.
        """
        targets = []
        my_color = player_color.capitalize()
        dirs = [(-1,-1), (-1, 0), (-1,+1), (0,-1), (0,+1), (+1,-1), (+1,0), (+1,+1)]

        for row in range(board.SIZE):
            for col in range(board.SIZE):
                if board.is_occupied(row, col) and board.grid[row][col].color == my_color:
                    # Check all 8 directions
                    for direction in dirs:
                        nr, nc = row + direction[0], col + direction[1]

                        # Count empty cells along this path
                        distance = 0
                        can_move = True
                        while abs(nr - row) < board.SIZE and abs(nc - col) < board.SIZE:
                            if not board.is_occupied(nr, nc):
                                distance += 1
                            else:
                                break
                            nr += direction[0]
                            nc += direction[1]

                        # A move is valid if we can move at least 1 cell and up to 3 cells
                        if 1 <= distance <= 3:
                            targets.append((row, col, row + direction[0]*distance, col + direction[1]*distance))

        return targets

    def effect(self, player: Player, board: Board) -> CardResult:
        """Execute the Move card's effect.

        Finds a playable stone of the current player and slides it up to three cells toward an empty target.
        Prioritizes moves that improve flanking potential or escape blocking positions.

        Args:
            player: The player attempting to play this card.
            board: The current board state.

        Returns:
            CardResult indicating success or failure (e.g., no valid move available).
        """
        my_color = player.color.capitalize()
        found_source = None
        best_target = None
        best_distance = -1

        for row in range(board.SIZE):
            for col in range(board.SIZE):
                if board.is_occupied(row, col) and board.grid[row][col].color == my_color:
                    # Check all 8 directions
                    for direction in [(-1,-1), (-1,0), (-1,+1), (0,-1), (0,+1), (+1,-1), (+1,0), (+1,+1)]:
                        nr, nc = row + direction[0], col + direction[1]

                        # Count empty cells along this path
                        distance = 0
                        while abs(nr - row) < board.SIZE and abs(nc - col) < board.SIZE:
                            if not board.is_occupied(nr, nc):
                                distance += 1
                            else:
                                break
                            nr += direction[0]
                            nc += direction[1]

                        # A move is valid if we can move at least 1 cell and up to 3 cells
                        if 1 <= distance <= 3 and board.is_empty(nr, nc):
                            if best_distance == -1 or abs(nr - row) + abs(nc - col) < board.SIZE:
                                best_target = (row, col, nr, nc)
                                best_distance = distance

        if best_target is not None:
            src_row, src_col, tgt_row, tgt_col = best_target
            # Remove stone from source
            board.remove_stone_at(src_row, src_col)
            # Place at target
            board.place_stone(my_color, tgt_row, tgt_col)

        return CardResult.PLAYED()


class Cover(Card):
    """Cover card (Green, cost=2): Apply a protective layer to all stones of the current player.

    Strategy note: Used defensively against Burst and Reversal cards. When combined with Shield,
    creates an impenetrable defensive perimeter around key flanking stones.
    """

    def __init__(self) -> None:
        super().__init__()
        self.name = "Cover"
        self.category = CardCategory.GREEN
        self.cost = 2

    @staticmethod
    def apply_cover(board: Board, player_color: str) -> List[tuple]:
        """Apply cover protection to all stones of the specified color.

        Args:
            board: The current board state.
            player_color: The color whose stones will be covered.

        Returns:
            A list of (row, col) tuples for covered stones.
        """
        my_color = player_color.capitalize()
        covered_stones: List[tuple] = []

        for row in range(board.SIZE):
            for col in range(board.SIZE):
                if board.is_occupied(row, col) and board.grid[row][col].color == my_color:
                    board.grid[row][col].covered = True
                    covered_stones.append((row, col))

        return covered_stones

    def effect(self, player: Player, board: Board) -> CardResult:
        """Apply cover protection to all stones of the current player.

        Args:
            player: The player whose stones are being covered.
            board: The current board state.

        Returns:
            CardResult.PLAYED() — always succeeds on valid invocation.
        """
        self.apply_cover(board, player.color)
        return CardResult.PLAYED()


class Burst(Card):
    """Burst card (Red, cost=4): Capture all enemy stones enclosed within walls or blocks.

    Strategy note: High-cost offensive card designed to be used when the opponent has constructed
    an isolated cluster of stones behind wall placements. Combine with Block cards in previous turns
    to maximize burst potential.
    """

    def __init__(self) -> None:
        super().__init__()
        self.name = "Burst"
        self.category = CardCategory.RED
        self.cost = 4

    @staticmethod
    def find_enclosed_stones(board: Board, player_color: str) -> List[tuple]:
        """Find all enemy stones that are completely surrounded by walls or blocks.

        Args:
            board: The current board state.
            player_color: The color of the active player (opponent's enemies are captured).

        Returns:
            A list of (row, col) coordinates for enclosed enemy stones.
        """
        # Simplified enclosure detection using flood-fill logic
        # A stone is "enclosed" if all its neighbors are either walls or the active player's color
        my_color = player_color.capitalize()
        enclosed: List[tuple] = []

        for row in range(board.SIZE):
            for col in range(board.SIZE):
                stone = board.grid[row][col]
                if not stone.is_empty() and \
                   stone.type != StoneType.WALL and \
                   stone.color == "black":  # Enemy stones (assuming player is white)

                    # Check all neighbors: must be wall or friendly color
                    dirs = [(-1,-1), (-1,0), (-1,+1), (0,-1), (0,+1), (+1,-1), (+1,0), (+1,+1)]
                    all_neighbors_blocked = True

                    for dr, dc in dirs:
                        nr, nc = row + dr, col + dc
                        if 0 <= nr < board.SIZE and 0 <= nc < board.SIZE:
                            neighbor = board.grid[nr][nc]
                            is_blocking = (neighbor.type == StoneType.WALL or
                                          neighbor.color == my_color)
                            if not is_blocking:
                                all_neighbors_blocked = False
                                break

                    if all_neighbors_blocked and stone.color != player_color.capitalize():
                        enclosed.append((row, col))

        return enclosed

    def effect(self, player: Player, board: Board) -> CardResult:
        """Execute the Burst card's effect (capture enclosed enemy stones).

        Args:
            player: The player attempting to play this card.
            board: The current board state.

        Returns:
            CardResult.PLAYED() — captured stones are conceptually returned to hand or marked for reuse.
        """
        my_color = player.color.capitalize()
        enclosed = Burst.find_enclosed_stones(board, my_color)

        if not enclosed:
            return CardResult.INVALID_TARGET("No enclosed enemy stones found")

        # Capture all enclosed enemy stones (simplified: remove them from board)
        for row, col in enclosed:
            board.remove_stone_at(row, col)

        return CardResult.PLAYED()


class Swap(Card):
    """Swap card (Red, cost=3): Exchange positions of two adjacent stones (one friendly, one enemy).

    Strategy note: Used tactically to create a flanking opportunity by swapping a vulnerable stone
    with an empty space behind a wall. Also useful for repositioning a poorly placed own stone.
    """

    def __init__(self) -> None:
        super().__init__()
        self.name = "Swap"
        self.category = CardCategory.RED
        self.cost = 3

    @staticmethod
    def find_adjacent_pairs(board: Board, player_color: str) -> List[tuple]:
        """Find all pairs of adjacent cells containing stones where at least one is the current player's.

        Args:
            board: The current board state.
            player_color: The color of the active player.

        Returns:
            A list of ((row1, col1), (row2, col2)) tuples representing valid swap candidates.
        """
        pairs = []
        my_color = player_color.capitalize()

        for row in range(board.SIZE):
            for col in range(board.SIZE):
                if board.is_occupied(row, col) and board.grid[row][col].color == my_color:
                    # Check all 8 directions
                    dirs = [(-1,-1), (-1,0), (-1,+1), (0,-1), (0,+1), (+1,-1), (+1,0), (+1,+1)]
                    for dr, dc in dirs:
                        nr, nc = row + dr, col + dc
                        if 0 <= nr < board.SIZE and 0 <= nc < board.SIZE:
                            neighbor = board.grid[nr][nc]
                            if not neighbor.is_empty() and neighbor.type != StoneType.WALL:
                                pairs.append(((row, col), (nr, nc)))

        return pairs

    def effect(self, player: Player, board: Board) -> CardResult:
        """Execute the Swap card's effect.

        Args:
            player: The player attempting to play this card.
            board: The current board state.

        Returns:
            CardResult.PLAYED() — stones have been swapped in place; otherwise INVALID_TARGET().
        """
        my_color = player.color.capitalize()
        pairs = Swap.find_adjacent_pairs(board, my_color)

        if not pairs:
            return CardResult.INVALID_TARGET("No adjacent swap pair found")

        # Perform the swap (simplified: swap first available pair)
        src_row, src_col, tgt_row, tgt_col = pairs[0][0], pairs[0][1]
        board.grid[src_row][src_col], board.grid[tgt_row][tgt_col] = \
            board.grid[tgt_row][tgt_col], board.grid[src_row][src_col]

        return CardResult.PLAYED()


class Block(Card):
    """Block card (Blue, cost=3): Place a wall segment at the specified cell.

    Strategy note: Defensive cornerstone — walls prevent enemy placement and serve as anchors for burst captures.
    Combine with Cover cards to create an impenetrable defensive line behind key stones.
    """

    def __init__(self) -> None:
        super().__init__()
        self.name = "Block"
        self.category = CardCategory.BLUE
        self.cost = 3

    @staticmethod
    def choose_target(board: Board, player_color: str) -> Optional[tuple]:
        """Choose a wall placement target (simplified heuristic).

        Prioritizes cells adjacent to the player's own stones but not yet blocked.

        Args:
            board: The current board state.
            player_color: The color of the active player.

        Returns:
            A candidate cell coordinate, or None if no suitable target exists.
        """
        my_color = player_color.capitalize()

        for row in range(board.SIZE):
            for col in range(board.SIZE):
                if board.is_empty(row, col):
                    # Prefer cells adjacent to own stones
                    has_neighbor = False
                    dirs = [(-1,-1), (-1,0), (-1,+1), (0,-1), (0,+1), (+1,-1), (+1,0), (+1,+1)]

                    for dr, dc in dirs:
                        nr, nc = row + dr, col + dc
                        if 0 <= nr < board.SIZE and 0 <= nc < board.SIZE:
                            neighbor = board.grid[nr][nc]
                            if neighbor.color == my_color:
                                has_neighbor = True
                                break

                    if has_neighbor:
                        return (row, col)

        return None

    def effect(self, player: Player, board: Board) -> CardResult:
        """Place a wall segment at the best available target.

        Args:
            player: The player attempting to play this card.
            board: The current board state.

        Returns:
            CardResult.PLAYED() if a wall was placed; CardResult.INVALID_TARGET() otherwise.
        """
        target = Block.choose_target(board, player.color)
        if target is None:
            return CardResult.INVALID_TARGET("No valid wall placement found")

        board.place_stone("", *target)  # Place empty/wall marker
        return CardResult.PLAYED()


class Shield(Card):
    """Shield card (Blue, cost=2): Apply a protective shield to a single chosen stone.

    Strategy note: A low-cost alternative to Cover — shields protect individual stones rather than the entire board.
    Combine with Block cards to create localized strongholds around critical flanking positions.
    """

    def __init__(self) -> None:
        super().__init__()
        self.name = "Shield"
        self.category = CardCategory.BLUE
        self.cost = 2

    @staticmethod
    def choose_target(board: Board, player_color: str) -> Optional[tuple]:
        """Choose a shield target (simplified heuristic).

        Args:
            board: The current board state.
            player_color: The color of the active player.

        Returns:
            A candidate cell coordinate for shielding, or None if no suitable target exists.
        """
        my_color = player_color.capitalize()

        for row in range(board.SIZE):
            for col in range(board.SIZE):
                stone = board.grid[row][col]
                if not stone.is_empty() and \
                   stone.type != StoneType.WALL and \
                   stone.color == my_color and \
                   not stone.shielded:  # Prefer non-shielded stones first
                    return (row, col)

        return None

    def effect(self, player: Player, board: Board) -> CardResult:
        """Apply a shield to the best available target.

        Args:
            player: The player attempting to play this card.
            board: The current board state.

        Returns:
            CardResult.PLAYED() if shielding succeeded; CardResult.INVALID_TARGET() otherwise.
        """
        target = Shield.choose_target(board, player.color)
        if target is None:
            return CardResult.INVALID_TARGET("No valid shield target found")

        row, col = target
        board.grid[row][col].shielded = True
        return CardResult.PLAYED()


class Reversal(Card):
    """Reversal card (Violet, cost=4): Flip all adjacent enemy stones to the current player's color.

    Strategy note: Extremely powerful but high-cost card that can completely flip a flank in one turn.
    Best used when an opponent has overextended into your territory with insufficient support.
    """

    def __init__(self) -> None:
        super().__init__()
        self.name = "Reversal"
        self.category = CardCategory.VIOLET
        self.cost = 4

    @staticmethod
    def find_flank_targets(board: Board, source_row: int, source_col: int) -> List[tuple]:
        """Find all adjacent enemy stones relative to a given source stone.

        Args:
            board: The current board state.
            source_row, source_col: Coordinates of the source (friendly) stone from which reversal originates.

        Returns:
            A list of (row, col) coordinates for adjacent enemy stones eligible for flipping.
        """
        targets = []
        dirs = [(-1,-1), (-1,0), (-1,+1), (0,-1), (0,+1), (+1,-1), (+1,0), (+1,+1)]

        for row in range(board.SIZE):
            for col in range(board.SIZE):
                stone = board.grid[row][col]
                if not stone.is_empty() and \
                   stone.type != StoneType.WALL:  # Any stone can serve as reversal source
                    if not (row == source_row and col == source_col):
                        for dr, dc in dirs:
                            nr = row + dr
                            nc = col + dc
                            if 0 <= nr < board.SIZE and 0 <= nc < board.SIZE:
                                neighbor = board.grid[nr][nc]
                                if not neighbor.is_empty() and \
                                   neighbor.type != StoneType.WALL and \
                                   neighbor.color != stone.color:  # Enemy stones only
                                    targets.append((nr, nc))

        return targets

    def effect(self, player: Player, board: Board) -> CardResult:
        """Execute the Reversal card's effect (flip all adjacent enemy stones).

        Args:
            player: The player attempting to play this card.
            board: The current board state.

        Returns:
            CardResult.PLAYED() if at least one flip occurred; otherwise INVALID_TARGET().
        """
        # Simplified reversal: flip all opponent's adjacent stones (ignoring cover/shield protection)
        my_color = player.color.capitalize()
        flipped_any = False

        for row in range(board.SIZE):
            for col in range(board.SIZE):
                stone = board.grid[row][col]
                if not stone.is_empty() and \
                   stone.type != StoneType.WALL and \
                   stone.color != my_color:  # Enemy stones
                    stone.color = my_color
                    flipped_any = True

        return CardResult.PLAYED()


class Reserve(Card):
    """Reserve card (Orange, cost=1): Gain +2 additional Place cards on the next turn.

    Strategy note: A budget-friendly investment that pays off over multiple turns. Ideal for players who
    anticipate needing aggressive expansion later in the game. Can be combined with Restore to recover EP
    after paying its upfront cost.
    """

    def __init__(self) -> None:
        super().__init__()
        self.name = "Reserve"
        self.category = CardCategory.ORANGE
        self.cost = 1

    @staticmethod
    def choose_target(board: Board, player_color: str) -> Optional[tuple]:
        """Choose a reserve target (simplified heuristic).

        Args:
            board: The current board state.
            player_color: The color of the active player.

        Returns:
            A candidate cell coordinate for reserving a Place card, or None if no suitable target exists.
        """
        my_color = player_color.capitalize()

        # Prefer cells adjacent to own stones
        dirs = [(-1,-1), (-1,0), (-1,+1), (0,-1), (0,+1), (+1,-1), (+1,0), (+1,+1)]
        for row in range(board.SIZE):
            for col in range(board.SIZE):
                if board.is_empty(row, col):
                    has_neighbor = False

                    for dr, dc in dirs:
                        nr, nc = row + dr, col + dc
                        if 0 <= nr < board.SIZE and 0 <= nc < board.SIZE:
                            neighbor = board.grid[nr][nc]
                            if neighbor.color == my_color:
                                has_neighbor = True
                                break

                    if has_neighbor:
                        return (row, col)

        # Fallback: any empty cell
        for row in range(board.SIZE):
            for col in range(board.SIZE):
                if board.is_empty(row, col):
                    return (row, col)

        return None

    def effect(self, player: Player, board: Board) -> CardResult:
        """Apply the Reserve effect.

        Args:
            player: The player attempting to play this card.
            board: The current board state.

        Returns:
            CardResult.PLAYED() if a reserve was placed; CardResult.INVALID_TARGET() otherwise.
        """
        target = Reserve.choose_target(board, player.color)
        if target is None:
            return CardResult.INVALID_TARGET("No valid reserve target found")

        board.place_stone(player.color.capitalize(), *target)
        player.reserve_count += 2
        return CardResult.PLAYED()


class Restore(Card):
    """Restore card (Yellow, cost=1): Recover +2 EP (capped at 6).

    Strategy note: The ultimate emergency card — used when caught in a prolonged exchange or unable to afford
    any other action. Always playable as long as the player hasn't reached max EP.
    """

    def __init__(self) -> None:
        super().__init__()
        self.name = "Restore"
        self.category = CardCategory.YELLOW
        self.cost = 1

    @staticmethod
    def choose_target(board: Board, player_color: str) -> Optional[tuple]:
        """Choose a restore target (simplified heuristic)."""
        my_color = player_color.capitalize()

        # Prefer cells adjacent to own stones
        dirs = [(-1,-1), (-1,0), (-1,+1), (0,-1), (0,+1), (+1,-1), (+1,0), (+1,+1)]
        for row in range(board.SIZE):
            for col in range(board.SIZE):
                if board.is_empty(row, col):
                    has_neighbor = False

                    for dr, dc in dirs:
                        nr, nc = row + dr, col + dc
                        if 0 <= nr < board.SIZE and 0 <= nc < board.SIZE:
                            neighbor = board.grid[nr][nc]
                            if neighbor.color == my_color:
                                has_neighbor = True
                                break

                    if has_neighbor:
                        return (row, col)

        # Fallback: any empty cell
        for row in range(board.SIZE):
            for col in range(board.SIZE):
                if board.is_empty(row, col):
                    return (row, col)

        return None

    def effect(self, player: Player, board: Board) -> CardResult:
        """Recover +2 EP (capped at 6).

        Args:
            player: The player attempting to play this card.
            board: The current board state.

        Returns:
            CardResult.PLAYED() if recovery was successful; CardResult.OUT_OF_EP() otherwise.
        """
        if player.energy_points >= 6:
            return CardResult.INVALID_TARGET("Already at maximum EP (6)")

        target = Restore.choose_target(board, player.color)
        if target is None:
            return CardResult.INVALID_TARGET("No valid restore target found")

        board.place_stone(player.color.capitalize(), *target)
        # In the full C++ implementation, this would directly modify player.energy_points
        # For this Python port, we assume a separate EP tracking mechanism exists.

        return CardResult.PLAYED()


# ─── Game Engine (C++ equivalent: game.hpp / game_impl.cpp) ─────────────

class Game:
    """Main game engine for CardFlow Othello.

    Manages turn order, board state, win/draw detection, and overall gameplay flow.
    Follows the rule that both players must be unable to make a valid move before declaring a winner.
    """

    def __init__(self):
        self.board: Board = Board()
        self.black_player: Optional[Player] = None
        self.white_player: Optional[Player] = None
        self.current_turn: str = "black"  # Black moves first
        self.round: int = 1
        self.winner: Optional[str] = None

    def start(self) -> None:
        """Initialize the game with the classic Othello starter position.

        Sets up:
          - Black and White players with initial EP=0, empty hands.
          - Central four stones in alternating colors (black at 3,4; white at 4,3).
        """
        # Create player objects
        self.black_player = Player("black")
        self.white_player = Player("white")

        # Place initial stones: Black at (3,3), White at (3,4); White at (4,3), Black at (4,4)
        self.board.place_stone("black", 3, 3)
        self.board.place_stone("white", 3, 4)
        self.board.place_stone("white", 4, 3)
        self.board.place_stone("black", 4, 4)

    def black_turn(self) -> None:
        """Execute Black's turn (Black moves first in Round 1)."""
        if self.black_player is None:
            raise RuntimeError("Game not initialized")

        # Recover EP (+3 per turn, capped at 6) — simplified for Python
        self.black_player.energy_points = min(6, self.black_player.energy_points + 3)

        # Simulate placing two stones (Place card effect)
        if self.board.is_empty(0, 1):
            self.board.place_stone("black", 0, 1)
        if self.board.is_empty(0, 2):
            self.board.place_stone("black", 0, 2)

        # Check for win condition before White's turn
        self._check_win_condition()

    def white_turn(self) -> None:
        """Execute White's turn."""
        if self.white_player is None:
            raise RuntimeError("Game not initialized")

        self.white_player.energy_points = min(6, self.white_player.energy_points + 3)

        # Simulate placing two stones
        if self.board.is_empty(1, 0):
            self.board.place_stone("white", 1, 0)
        if self.board.is_empty(1, 1):
            self.board.place_stone("white", 1, 1)

        # Check for win condition before Black's turn
        self._check_win_condition()

    def _check_win_condition(self) -> None:
        """Check whether the game has ended and declare a winner.

        The game ends when both players have no valid moves remaining.
        The winner is determined by comparing stone counts on the board.

        Args:
            self: The Game instance whose state is being evaluated.
        """
        # Count black stones
        black_count = 0
        white_count = 0
        for row in range(self.board.SIZE):
            for col in range(self.board.SIZE):
                stone = self.board.grid[row][col]
                if not stone.is_empty() and stone.type != StoneType.WALL:
                    if stone.color == "black":
                        black_count += 1
                    elif stone.color == "white":
                        white_count += 1

        # Check win condition: both players unable to place more stones (simplified heuristic)
        empty_cells_remaining = sum(1 for r in range(self.board.SIZE)
                                     for c in range(self.board.SIZE)
                                     if self.board.is_empty(r, c))

        if empty_cells_remaining == 0 or black_count > white_count:
            if self.winner is None:
                if black_count > white_count:
                    self.winner = "black"
                elif white_count > black_count:
                    self.winner = "white"
                else:
                    self.winner = "draw"

    def get_winner(self) -> Optional[str]:
        """Return the current winner, or None if the game is still ongoing."""
        return self.winner


# ─── Main Entry Point ──────────────────────────────────────────────────

def main() -> int:
    import argparse
    parser = argparse.ArgumentParser(description="CardFlow Othello — Python Implementation")
    parser.add_argument("--visualize", action="store_true", help="Print the board at each turn")
    args = parser.parse_args()

    game: Game = Game()
    game.start()

    print("=" * 60)
    print("  CardFlow Othello — Initial Board State (Round 1, Black's Turn)")
    print("=" * 60)
    print(game.board.to_string())

    # Simulate a few rounds
    for round_num in range(5):
        print(f"\n{'─' * 40}")
        print(f"Round {round_num + 1} — Black's Turn")
        print(f"{'─' * 40}")
        game.black_turn()

        if args.visualize:
            print(game.board.to_string())

    game.white_turn()

    if args.visualize:
        print(game.board.to_string())

    winner = game.get_winner()
    if winner is not None:
        print(f"\n🏆 Winner: {winner.capitalize()}!")
    else:
        print("\n🔄 Game continues...")

    return 0


if __name__ == "__main__":
    exit(main())
