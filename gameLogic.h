// gameLogic.h
#ifndef GAMELOGIC_H
#define GAMELOGIC_H

// Fix Windows byte conflict BEFORE including anything
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <iostream>
#include <deque>
#include <vector>
#include <random>
#include <atomic>
#include <memory>
#include <chrono>

using namespace std;

// Immutable game state snapshot
struct GameState {
    vector<vector<int>> board;  
    int rows, cols;
    int score;
    bool gameOver;
    pair<int, int> food;
    bool foodExists;
    deque<pair<int, int>> snake;
    int snakeLength;
};

class SnakeGameLogic {
private:
    enum Direction { UP = 0, DOWN = 1, LEFT = 2, RIGHT = 3, NONE = 4 };
    enum CellType { EMPTY = 0, SNAKE = 1, FOOD = 2, WALL = 3 };
    
    // ============================================
    // THREAD-SAFE: Atomic pointer to current state
    // ============================================
    atomic<shared_ptr<const GameState>> currentState;
    
    // Double buffers (game thread only modifies these)
    shared_ptr<GameState> writeBuffer;
    shared_ptr<GameState> readBuffer;
    
    // Game logic state (game thread exclusive)
    deque<pair<int, int>> snake;
    vector<vector<int>> board;
    int rows, cols;
    int score, pointsPerFood;
    bool gameOver;
    pair<int, int> food;
    bool foodExists;
    int snakeGrowth, snakeStartingLength;
    
    Direction currentDirection, nextDirection;
    atomic<int> atomicDirection;
    
    mt19937 rng;  // Game thread exclusive - no synchronization needed
    
    // ============================================
    // Helper Methods (Game Thread Only)
    // ============================================
    
    bool isValidDirectionChange(Direction newDir) const {
        if (currentDirection == UP && newDir == DOWN) return false;
        if (currentDirection == DOWN && newDir == UP) return false;
        if (currentDirection == LEFT && newDir == RIGHT) return false;
        if (currentDirection == RIGHT && newDir == LEFT) return false;
        return true;
    }
    
    pair<int, int> getNextHeadPosition() const {
        pair<int, int> head = snake.front();
        int newRow = head.first;
        int newCol = head.second;
        
        switch (currentDirection) {
            case UP:    newRow--; break;
            case DOWN:  newRow++; break;
            case LEFT:  newCol--; break;
            case RIGHT: newCol++; break;
            case NONE:  break;
        }
        return {newRow, newCol};
    }
    
    bool isInBounds(int r, int c) const {
        return r >= 0 && r < rows && c >= 0 && c < cols;
    }
    
    bool checkSelfCollision(pair<int, int> pos) const {
        for (size_t i = 1; i < snake.size(); i++) {
            if (snake[i] == pos) return true;
        }
        return false;
    }
    
    void moveSnake(pair<int, int> newHead) {
        snake.push_front(newHead);
        board[newHead.first][newHead.second] = SNAKE;
        
        if (snakeGrowth > 0) {
            snakeGrowth--;
        } else {
            pair<int, int> tail = snake.back();
            snake.pop_back();
            board[tail.first][tail.second] = EMPTY;
        }
    }
    
    void placeRandomFood() {
        vector<pair<int, int>> emptyCells;
        
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                if (board[r][c] == EMPTY) {
                    emptyCells.push_back({r, c});
                }
            }
        }
        
        if (emptyCells.empty()) {
            foodExists = false;
            return;
        }
        
        uniform_int_distribution<int> dist(0, emptyCells.size() - 1);
        int idx = dist(rng);
        food = emptyCells[idx];
        board[food.first][food.second] = FOOD;
        foodExists = true;
    }
    
    void placeWalls(int wallCount = 0) {
        for (int i = 0; i < wallCount; i++) {
            vector<pair<int, int>> emptyCells;
            for (int r = 0; r < rows; r++) {
                for (int c = 0; c < cols; c++) {
                    if (board[r][c] == EMPTY) {
                        emptyCells.push_back({r, c});
                    }
                }
            }
            
            if (!emptyCells.empty()) {
                uniform_int_distribution<int> dist(0, emptyCells.size() - 1);
                int idx = dist(rng);
                board[emptyCells[idx].first][emptyCells[idx].second] = WALL;
            }
        }
    }
    
    // ============================================
    // STATE PUBLISHING (Game Thread Only)
    // ============================================
    void publishState() {
        // Copy current game state to write buffer
        writeBuffer->rows = rows;
        writeBuffer->cols = cols;
        writeBuffer->score = score;
        writeBuffer->gameOver = gameOver;
        writeBuffer->food = food;
        writeBuffer->foodExists = foodExists;
        writeBuffer->snake = snake;
        writeBuffer->snakeLength = snake.size();
        writeBuffer->board = board;
        
        // Atomic swap: publish to render thread
        // Using memory_order_release ensures all writes are visible
        currentState.store(writeBuffer, memory_order_release);
        
        // Swap buffers for next update
        swap(writeBuffer, readBuffer);
    }

public:
    SnakeGameLogic() {
        auto seed = chrono::high_resolution_clock::now().time_since_epoch().count();
        rng.seed(static_cast<unsigned int>(seed));
        atomicDirection.store(static_cast<int>(NONE), memory_order_relaxed);
        
        // Allocate double buffers
        writeBuffer = make_shared<GameState>();
        readBuffer = make_shared<GameState>();
        currentState.store(writeBuffer, memory_order_relaxed);
    }
    
    void initializeBoard(
        int rows, 
        int cols, 
        int startingLength, 
        int pointsPerFood, 
        Direction initialDirection
    ) {
        this->rows = rows;
        this->cols = cols;
        this->snakeStartingLength = startingLength;
        this->pointsPerFood = pointsPerFood;
        this->currentDirection = initialDirection;
        this->nextDirection = initialDirection;
        
        score = 0;
        gameOver = false;
        snakeGrowth = 0;
        foodExists = false;
        snake.clear();
        
        board = vector<vector<int>>(rows, vector<int>(cols, EMPTY));
        
        int startRow = rows / 2;
        int startCol = cols / 2;
        
        for (int i = 0; i < snakeStartingLength; i++) {
            int r = startRow;
            int c = startCol;
            
            switch (initialDirection) {
                case RIGHT: c -= i; break;
                case LEFT:  c += i; break;
                case UP:    r += i; break;
                case DOWN:  r -= i; break;
                case NONE:  break;
            }
            
            snake.push_back({r, c});
            board[r][c] = SNAKE;
        }
        
        placeRandomFood();
        publishState();
    }
    
    // LOCK-FREE: Can be called from input thread
    void setDirection(Direction newDir) {
        atomicDirection.store(static_cast<int>(newDir), memory_order_release);
    }
    
    // GAME THREAD ONLY: Call this from game update thread
    bool update() {
        if (gameOver) {
            return false;
        }
        
        // Read atomic direction (lock-free)
        int dirValue = atomicDirection.exchange(static_cast<int>(NONE), memory_order_acquire);
        Direction inputDir = static_cast<Direction>(dirValue);
        
        // Apply direction if valid
        if (inputDir != NONE && isValidDirectionChange(inputDir)) {
            nextDirection = inputDir;
        }
        
        currentDirection = nextDirection;
        pair<int, int> newHead = getNextHeadPosition();
        
        // Collision checks
        if (!isInBounds(newHead.first, newHead.second)) {
            gameOver = true;
            publishState();
            return false;
        }
        
        if (board[newHead.first][newHead.second] == WALL) {
            gameOver = true;
            publishState();
            return false;
        }
        
        if (checkSelfCollision(newHead)) {
            gameOver = true;
            publishState();
            return false;
        }
        
        // Food collision
        if (foodExists && newHead == food) {
            snakeGrowth++;
            score += pointsPerFood;
            foodExists = false;
        }
        
        moveSnake(newHead);
        
        if (!foodExists) {
            placeRandomFood();
        }
        
        if (!foodExists && snakeGrowth == 0) {
            gameOver = true;
            publishState();
            return false;
        }
        
        // Publish state to render thread (atomic)
        publishState();
        return true;
    }
    
    // ============================================
    // RENDER THREAD: Read-only snapshots (lock-free)
    // ============================================
    
    shared_ptr<const GameState> getGameState() const {
        // memory_order_acquire ensures we see all writes from game thread
        return currentState.load(memory_order_acquire);
    }
    
    int getRows() const {
        auto state = currentState.load(memory_order_acquire);
        return state->rows;
    }
    
    int getCols() const {
        auto state = currentState.load(memory_order_acquire);
        return state->cols;
    }
    
    int getScore() const {
        auto state = currentState.load(memory_order_acquire);
        return state->score;
    }
    
    bool isGameOver() const {
        auto state = currentState.load(memory_order_acquire);
        return state->gameOver;
    }
    
    int getCellType(int r, int c) const {
        auto state = currentState.load(memory_order_acquire);
        if (r >= 0 && r < state->rows && c >= 0 && c < state->cols) {
            return state->board[r][c];
        }
        return WALL;
    }
    
    char renderSymbol(int cellType) const {
        switch(cellType) {
            case EMPTY: return ' ';
            case SNAKE: return 'O';
            case FOOD:  return '*';
            case WALL:  return '#';
        }
        return ' ';
    }
    
    // Static direction accessors
    static Direction getDirectionUp() { return UP; }
    static Direction getDirectionDown() { return DOWN; }
    static Direction getDirectionLeft() { return LEFT; }
    static Direction getDirectionRight() { return RIGHT; }
};
#endif // GAMELOGIC_H
