#include<iostream>
#include<deque>
#include<vector>
using namespace std;
class SnakeGameLogic {
    private:
        int rows, cols;                  
        int score;                       
        int pointsPerFood;               
        bool gameOver;   

        deque<pair<int, int>> snake; 
        int snakeGrowth;                 
        enum Direction { UP, DOWN, LEFT, RIGHT };
        Direction currentDirection;
        int snakeStartingLength;

        enum CellType { EMPTY, SNAKE, FOOD, WALL };
        vector<vector<CellType>> board;
        pair<int, int> food;    

    public:
        char renderSymbol(CellType cell) {
            switch(cell) {
                case EMPTY: return ' ';
                case SNAKE: return 'O';
                case FOOD:  return '*';
                case WALL:  return '#';
            }
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

            score = 0;
            gameOver = false;
            snakeGrowth = 0;
            snake.clear(); 

            board = vector<vector<CellType>>(rows, vector<CellType>(cols, CellType::EMPTY));

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
                }

                snake.push_back({r, c});
                board[r][c] = CellType::SNAKE;
            }
   
        }
};
