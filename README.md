# CardFlow Othello — Complete C++ Implementation
#
# This project implements a full-featured "CardFlow Othello" game engine,
# combining elements of Othello and card-based strategy games.
#
# Features:
#   - 8×8 board with alternating black/white starter stones
#   - Card system: Place(2), Move(3), Cover(2), Burst(4), Swap(3),
#                  Block(3), Shield(2), Reversal(4), Reserve(1), Restore(1)
#   - EP (Energy Point) system: +3 per turn, max 6
#   - Turn-based gameplay with immediate win/draw detection
#   - Category-balanced deck: Green≤2, Red≤2, Blue≤2, Violet≤1, Orange≤1, Yellow≤1
#
# Build instructions (C++ version):
#   cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
#   cmake --build . --config Debug -j4
#   ctest -j4
#
# Alternative: Python implementation for quick validation and testing.
