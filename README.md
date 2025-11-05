# Snake Game

A terminal-based Snake game built with C++ featuring cross-platform compatibility, thread-safe game logic, and persistent high score tracking.

---

## For Players

### Getting Started

### Game Screenshots

**Start Screen**
The welcome menu displays your high score and options to begin a new game or exit.
![Game Start Screen](https://github.com/maitry4/code_crafters/blob/main/Game%20Screenshots/game_start_screen.png)

**Initial Board Display**
The game board with instructions showing all control options before gameplay begins.
![Game Initial Board Display](https://github.com/maitry4/code_crafters/blob/main/Game%20Screenshots/game_initial_board_display.png)

**Gameplay in Action**
Real-time action showing the snake (displayed as `O` for head and `o` for body segments), food (`*`), and current score/length tracking at the top.

![Game Being Played](https://github.com/maitry4/code_crafters/blob/main/Game%20Screenshots/game_being_played.png)

**Game Over Screen**
Final score display with high score tracking and options to replay or quit. New high scores are highlighted.
![Game Over](https://github.com/maitry4/code_crafters/blob/main/Game%20Screenshots/game_over.png)

**Quit Confirmation**
Clean exit message displayed when you quit the game.

![Game Quit Message](https://github.com/maitry4/code_crafters/blob/main/Game%20Screenshots/game_quit_message.png)

#### Installation

**Windows:**
1. Download or compile the executable
2. Run `main.exe` from Command Prompt

**Linux/macOS:**
1. Ensure you have a C++ compiler installed (`g++`)
2. Compile: `g++ -std=c++20  main.cpp -o snake_game`
3. Run: `./snake_game`

### How to Play

Navigate your snake around the board, eat the food (marked with `*`), and grow longer. Avoid hitting walls, other snakes, or yourself!

**Goal:** Eat as much food as possible to increase your score and beat your personal high score.

**Controls:**
- **Move Up:** `W` or `↑ Arrow Key`
- **Move Down:** `S` or `↓ Arrow Key`
- **Move Left:** `A` or `← Arrow Key`
- **Move Right:** `D` or `→ Arrow Key`
- **Quit:** `Q`

### Gameplay Rules

- Each food eaten grants **10 points**
- The snake grows by one segment for each food consumed
- The game ends if the snake:
  - Hits the boundary wall
  - Collides with itself
- After game over, press `R` to replay or `Q` to quit

### Features

- **High Score Tracking:** Your best score is automatically saved to `game_highest.txt`
- **Real-Time Score Display:** Monitor your current score, snake length, and high score at the top of the screen
- **Smooth Controls:** Responsive arrow key and WASD input handling
- **Cross-Platform:** Works seamlessly on Windows, Linux, and macOS

---

## For Contributors

### Project Overview

This is a terminal-based Snake game implementing sophisticated game design patterns: **lock-free thread-safe architecture**, **double-buffering for rendering**, and **platform abstraction layers**.

### Architecture

The codebase is organized into three main components:

#### 1. **Game Logic (`gameLogic.h`)**
The core game engine using **immutable state snapshots** and **atomic operations** for thread safety.

**Key Concepts:**
- **GameState Struct:** Immutable snapshot of the game at any moment (board state, snake position, score, etc.)
- **Lock-Free Threading:** Uses `std::atomic<shared_ptr<const GameState>>` to safely publish state from the game thread to the render thread without locks
- **Double Buffering:** Maintains `writeBuffer` and `readBuffer` to prevent torn reads during state updates
- **Direction Validation:** Prevents invalid moves (e.g., reversing 180° into yourself)

**Critical Methods:**
- `initializeBoard()`: Sets up the game with specified dimensions and starting configuration
- `update()`: Game loop tick—reads input, moves snake, checks collisions, publishes state
- `getGameState()`: Lock-free read of current game state (safe for render thread)

#### 2. **Platform Abstraction (`main.cpp` - TerminalController)**
Handles all platform-specific terminal operations with unified API.

**Windows Support:**
- Uses `<conio.h>` for keyboard input (`_kbhit()`, `_getch()`)
- Uses Windows Console API for cursor positioning and screen clearing

**Linux/macOS Support:**
- Uses `termios` for raw input mode configuration
- Uses ANSI escape sequences for cursor control and screen manipulation
- Uses `fcntl` for non-blocking I/O

**Key Methods:**
- `clearScreen()`: Cross-platform screen clearing
- `setCursorPosition()`: Atomic cursor positioning
- `enableRawMode()` / `disableRawMode()`: Terminal configuration for Linux
- `kbhit()` / `getch()`: Cross-platform non-blocking keyboard input

#### 3. **Rendering & Input (`GameRenderer`, `InputHandler`)**
Handles visual output and user input processing.

**GameRenderer:**
- `drawFullScreen()`: Initial board layout with instructions
- `updateGameBoard()`: Efficient per-frame updates (only redraws game cells)
- `showGameOver()`: Game-over screen with score display

**InputHandler:**
- Handles arrow key sequences (different on Windows vs. Linux)
- Supports both arrow keys and WASD input
- `pollInput()`: Non-blocking input polling

### Tech Stack & Design Choices

| Technology | Rationale |
|---|---|
| **C++20** | Modern standard for atomic operations, smart pointers, and concurrency primitives |
| **std::atomic** | Lock-free thread safety without mutex overhead; minimal latency for input/render synchronization |
| **std::shared_ptr** | Automatic memory management for game state snapshots; safe concurrent access |
| **Double Buffering** | Prevents visual glitches (tearing) when updating display while game thread modifies state |
| **ANSI Escape Sequences** | Portable cursor control across Linux terminals; more reliable than system calls |
| **Terminal Raw Mode** | Non-blocking, responsive input without OS event loop overhead |

### Cross-Platform Compatibility

**Supported Platforms:**
- ✅ **Windows 7+** (using Windows Console API)
- ✅ **Linux** (tested on Ubuntu 20.04+, Fedora, Debian)
- ✅ **macOS** (using standard POSIX termios)

