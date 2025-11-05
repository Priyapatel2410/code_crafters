#include "gameLogic.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fstream>

#ifdef _WIN32
    #include <conio.h>
    #include <windows.h>
#else
    #include <unistd.h>
    #include <termios.h>
    #include <fcntl.h>
    #include <sys/ioctl.h>
    #include <cstring>
#endif

using namespace std;

// ============================================
// High Score Manager
// ============================================

class HighScoreManager {
private:
    const string filename = "game_highest.txt";
    int highScore;
    
public:
    HighScoreManager() : highScore(0) {
        loadHighScore();
    }
    
    void loadHighScore() {
        ifstream file(filename);
        if (file.is_open()) {
            file >> highScore;
            file.close();
        } else {
            highScore = 0;
        }
    }
    
    void saveHighScore(int score) {
        if (score > highScore) {
            highScore = score;
            ofstream file(filename);
            if (file.is_open()) {
                file << highScore;
                file.close();
            }
        }
    }
    
    int getHighScore() const {
        return highScore;
    }
    
    bool isNewHighScore(int score) const {
        return score > highScore;
    }
};

// ============================================
// Platform-Independent Terminal Control
// ============================================

class TerminalController {
private:
#ifndef _WIN32
    termios originalSettings;
    bool settingsChanged = false;
    int originalFlags = 0;
#endif

public:
    void clearScreen() {
#ifdef _WIN32
        system("cls");
#else
        // LINUX FIX: Use \033[H\033[J instead of \033[2J\033[1;1H for more reliable clearing
        // \033[H moves cursor to home, \033[J clears from cursor to end of screen
        cout << "\033[H\033[J";
        cout.flush();
        // LINUX FIX: Small delay to ensure terminal processes the escape sequence
        this_thread::sleep_for(chrono::milliseconds(10));
#endif
    }
    
    void setCursorPosition(int row, int col) {
#ifdef _WIN32
        COORD pos = {(SHORT)col, (SHORT)row};
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
#else
        cout << "\033[" << (row + 1) << ";" << (col + 1) << "H";
        // LINUX FIX: Always flush after cursor positioning to prevent artifacts
        cout.flush();
#endif
    }
    
    void hideCursor() {
#ifdef _WIN32
        HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO info;
        info.dwSize = 100;
        info.bVisible = FALSE;
        SetConsoleCursorInfo(consoleHandle, &info);
#else
        cout << "\033[?25l";
        // LINUX FIX: Flush to ensure cursor hide command is processed immediately
        cout.flush();
#endif
    }
    
    void showCursor() {
#ifdef _WIN32
        HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_CURSOR_INFO info;
        info.dwSize = 100;
        info.bVisible = TRUE;
        SetConsoleCursorInfo(consoleHandle, &info);
#else
        cout << "\033[?25h";
        // LINUX FIX: Flush to ensure cursor show command is processed immediately
        cout.flush();
#endif
    }
    
    void enableRawMode() {
#ifndef _WIN32
        tcgetattr(STDIN_FILENO, &originalSettings);
        settingsChanged = true;
        
        termios raw = originalSettings;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        
        originalFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, originalFlags | O_NONBLOCK);
#endif
    }
    
    void disableRawMode() {
#ifndef _WIN32
        if (settingsChanged) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalSettings);
            fcntl(STDIN_FILENO, F_SETFL, originalFlags);
            settingsChanged = false;
        }
#endif
    }
    
    bool kbhit() {
#ifdef _WIN32
        return _kbhit() != 0;
#else
        int bytesWaiting;
        ioctl(STDIN_FILENO, FIONREAD, &bytesWaiting);
        return bytesWaiting > 0;
#endif
    }
    
    char getch() {
#ifdef _WIN32
        return _getch();
#else
        char c = 0;
        read(STDIN_FILENO, &c, 1);
        return c;
#endif
    }
    
    ~TerminalController() {
        disableRawMode();
        showCursor();
    }
};

// ============================================
// Game Renderer
// ============================================

class GameRenderer {
private:
    TerminalController& terminal;
    HighScoreManager& highScoreManager;
    int headerRows = 6;
    int footerRows = 2;
    
public:
    GameRenderer(TerminalController& term, HighScoreManager& hsm) 
        : terminal(term), highScoreManager(hsm) {}
    
    void drawFullScreen(const SnakeGameLogic& game, bool showInstructions = false) {
    auto state = game.getGameState();
    
    ostringstream buffer;
    
    // Title
    buffer << "\n";
    buffer << "  +===============================+\n";
    buffer << "  |       SNAKE GAME              |\n";
    buffer << "  +===============================+\n\n";
    
    // REMOVED: Do NOT add score line here - it's handled separately by updateGameBoard()
    
    // Game board
    buffer << "+";
    for (int i = 0; i < state->cols; i++) buffer << "-";
    buffer << "+\n";
    
    for (int r = 0; r < state->rows; r++) {
        buffer << "|";
        for (int c = 0; c < state->cols; c++) {
            buffer << " ";
        }
        buffer << "|\n";
    }
    
    buffer << "+";
    for (int i = 0; i < state->cols; i++) buffer << "-";
    buffer << "+\n";
    
    // Controls section
    buffer << "\n";
    if (showInstructions) {
        buffer << "  +===================================+\n";
        buffer << "  |  CONTROLS:                        |\n";
        buffer << "  |                                   |\n";
        buffer << "  |  W or UP Arrow    - Move UP       |\n";
        buffer << "  |  S or DOWN Arrow  - Move DOWN     |\n";
        buffer << "  |  A or LEFT Arrow  - Move LEFT     |\n";
        buffer << "  |  D or RIGHT Arrow - Move RIGHT    |\n";
        buffer << "  |  Q                - Quit Game     |\n";
        buffer << "  |                                   |\n";
        buffer << "  |  Press any key to start...        |\n";
        buffer << "  +===================================+\n";
    } else {
        buffer << "  Controls: Arrow Keys or WASD  |  Q: Quit\n";
    }
    
    terminal.clearScreen();
    terminal.hideCursor();
    cout << buffer.str();
    cout.flush();
    
    // NOW output the score after the static board
    terminal.setCursorPosition(4, 0);
    ostringstream scoreBuffer;
    scoreBuffer << "  Score: " << setw(4) << state->score 
                << "  |  Length: " << setw(3) << state->snakeLength 
                << "  |  High Score: " << setw(4) << highScoreManager.getHighScore() << "  ";
    cout << scoreBuffer.str();
    cout.flush();
}

    
    void updateGameBoard(const SnakeGameLogic& game) {
        auto state = game.getGameState();
        
        // LINUX FIX: Build score line in buffer first, then output atomically
        ostringstream scoreBuffer;
        scoreBuffer << "  Score: " << setw(4) << state->score 
                    << "  |  Length: " << setw(3) << state->snakeLength 
                    << "  |  High Score: " << setw(4) << highScoreManager.getHighScore() << "  ";
        
        // Update score
        terminal.setCursorPosition(4, 0);
        cout << scoreBuffer.str();
        cout.flush();
        
        // LINUX FIX: Build each row in a buffer before outputting to reduce flicker
        for (int r = 0; r < state->rows; r++) {
            ostringstream rowBuffer;
            terminal.setCursorPosition(headerRows + r, 1);
            
            for (int c = 0; c < state->cols; c++) {
                int cellType = state->board[r][c];
                
                switch(cellType) {
                    case 0: // EMPTY
                        rowBuffer << " ";
                        break;
                    case 1: // SNAKE
                        if (r == state->snake.front().first && 
                            c == state->snake.front().second) {
                            rowBuffer << "O"; // Head
                        } else {
                            rowBuffer << "o"; // Body
                        }
                        break;
                    case 2: // FOOD
                        rowBuffer << "*";
                        break;
                    case 3: // WALL
                        rowBuffer << "#";
                        break;
                    default:
                        rowBuffer << " ";
                }
            }
            
            // LINUX FIX: Output entire row at once
            cout << rowBuffer.str();
        }
        
        // LINUX FIX: Single flush after all updates
        cout.flush();
    }
    
    void showGameOver(const SnakeGameLogic& game) {
        auto state = game.getGameState();
        highScoreManager.saveHighScore(state->score);
        
        // LINUX FIX: Build game over message in buffer for atomic output
        ostringstream buffer;
        buffer << "\n";
        buffer << "  +===============================+\n";
        buffer << "  |         GAME OVER!            |\n";
        buffer << "  |   Final Score: " << setw(4) << state->score << "          |\n";
        buffer << "  |   High Score:  " << setw(4) << highScoreManager.getHighScore() << "          |\n";
        
        if (highScoreManager.isNewHighScore(state->score) && state->score > 0) {
            buffer << "  |                               |\n";
            buffer << "  |   *** NEW HIGH SCORE! ***     |\n";
        }
        
        buffer << "  |                               |\n";
        buffer << "  |   Press R to Replay           |\n";
        buffer << "  |   Press Q to Quit             |\n";
        buffer << "  +===============================+\n";
        
        int messageRow = headerRows + state->rows + 3;
        terminal.setCursorPosition(messageRow, 0);
        cout << buffer.str();
        cout.flush();
    }
};

// ============================================
// Input Handler
// ============================================

class InputHandler {
private:
    TerminalController& terminal;
    SnakeGameLogic& game;
    char buffer[3];
    int bufferPos = 0;
    
public:
    InputHandler(TerminalController& term, SnakeGameLogic& g) 
        : terminal(term), game(g) {
        memset(buffer, 0, sizeof(buffer));
    }
    
    char getKey() {
        if (!terminal.kbhit()) return 0;
        return terminal.getch();
    }
    
    char pollInput() {
        char key = getKey();
        if (key == 0) return 0;
        
        // Handle arrow keys (platform-specific)
#ifdef _WIN32
        if (key == -32 || key == 0) { // Arrow key prefix on Windows
            key = terminal.getch();
            switch(key) {
                case 72: game.setDirection(SnakeGameLogic::getDirectionUp()); break;
                case 80: game.setDirection(SnakeGameLogic::getDirectionDown()); break;
                case 75: game.setDirection(SnakeGameLogic::getDirectionLeft()); break;
                case 77: game.setDirection(SnakeGameLogic::getDirectionRight()); break;
            }
            return 0;
        }
#else
        if (key == 27) { // Escape sequence start
            buffer[0] = key;
            bufferPos = 1;
            
            // LINUX FIX: Increased timeout to 20ms for more reliable arrow key detection
            auto startTime = chrono::steady_clock::now();
            while (bufferPos < 3 && chrono::duration_cast<chrono::milliseconds>(
                   chrono::steady_clock::now() - startTime).count() < 20) {
                if (terminal.kbhit()) {
                    buffer[bufferPos++] = terminal.getch();
                }
            }
            
            // Check if we have a complete arrow key sequence
            if (bufferPos >= 3 && buffer[0] == 27 && buffer[1] == '[') {
                switch(buffer[2]) {
                    case 'A': game.setDirection(SnakeGameLogic::getDirectionUp()); break;
                    case 'B': game.setDirection(SnakeGameLogic::getDirectionDown()); break;
                    case 'C': game.setDirection(SnakeGameLogic::getDirectionRight()); break;
                    case 'D': game.setDirection(SnakeGameLogic::getDirectionLeft()); break;
                }
            }
            
            // Reset buffer
            memset(buffer, 0, sizeof(buffer));
            bufferPos = 0;
            return 0;
        }
#endif
        
        // WASD and quit
        switch(key) {
            case 'w': case 'W':
                game.setDirection(SnakeGameLogic::getDirectionUp());
                return 0;
            case 's': case 'S':
                game.setDirection(SnakeGameLogic::getDirectionDown());
                return 0;
            case 'a': case 'A':
                game.setDirection(SnakeGameLogic::getDirectionLeft());
                return 0;
            case 'd': case 'D':
                game.setDirection(SnakeGameLogic::getDirectionRight());
                return 0;
            case 'q': case 'Q':
                return 'Q';
            default:
                return 0;
        }
    }
    
    void clearBuffer() {
        while (terminal.kbhit()) {
            terminal.getch();
        }
        memset(buffer, 0, sizeof(buffer));
        bufferPos = 0;
    }
};

// ============================================
// Main Menu
// ============================================

void showIntro(TerminalController& terminal, HighScoreManager& highScoreManager) {
    // LINUX FIX: Build intro screen in buffer for atomic output
    ostringstream buffer;
    buffer << "\n\n\n";
    buffer << "  #########################################\n";
    buffer << "  #                                       #\n";
    buffer << "  #          SNAKE GAME                   #\n";
    buffer << "  #                                       #\n";
    buffer << "  #########################################\n\n";
    buffer << "  High Score: " << highScoreManager.getHighScore() << "\n\n\n";
    buffer << "  Press ENTER to Start\n";
    buffer << "  Press Q to Quit\n\n";
    
    terminal.clearScreen();
    cout << buffer.str();
    cout.flush();
}

// ============================================
// Game Loop
// ============================================

bool runGame(TerminalController& terminal, HighScoreManager& highScoreManager) {
    SnakeGameLogic game;
    GameRenderer renderer(terminal, highScoreManager);
    
    // Game configuration (single difficulty)
    int rows = 20;
    int cols = 40;
    int updateDelay = 150;
    int startingLength = 3;
    int pointsPerFood = 10;
    
    game.initializeBoard(
        rows, 
        cols, 
        startingLength, 
        pointsPerFood,
        SnakeGameLogic::getDirectionRight()
    );
    
    InputHandler input(terminal, game);
    
    // Draw initial screen with instructions
    renderer.drawFullScreen(game, true);
    
    // Wait for any key to start
    bool keyPressed = false;
    while (!keyPressed) {
        if (terminal.kbhit()) {
            terminal.getch();
            keyPressed = true;
        }
        this_thread::sleep_for(chrono::milliseconds(50));
    }
    
    // Clear any remaining buffered input
    input.clearBuffer();
    
    // Redraw without instructions
    renderer.drawFullScreen(game, false);
    
    // LINUX FIX: Small delay after redraw to ensure terminal is ready
    this_thread::sleep_for(chrono::milliseconds(50));
    
    // Game loop
    auto lastUpdate = chrono::steady_clock::now();
    bool gameActive = true;
    
    while (gameActive) {
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - lastUpdate).count();
        
        // Poll input
        char key = input.pollInput();
        if (key == 'Q') {
            return false; // User wants to quit
        }
        
        // Update game at fixed interval
        if (elapsed >= updateDelay) {
            gameActive = game.update();
            renderer.updateGameBoard(game);
            lastUpdate = now;
        }
        
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    
    // Game over - show the game over screen
    renderer.showGameOver(game);
    
    // Wait for user input (R to replay, Q to quit)
    while (true) {
        if (terminal.kbhit()) {
            char key = terminal.getch();
            if (key == 'r' || key == 'R') {
                return true; // Replay
            } else if (key == 'q' || key == 'Q') {
                return false; // Quit
            }
        }
        this_thread::sleep_for(chrono::milliseconds(50));
    }
}

// ============================================
// Main
// ============================================

int main() {
    TerminalController terminal;
    HighScoreManager highScoreManager;
    terminal.enableRawMode();
    
    while (true) {
        showIntro(terminal, highScoreManager);
        
        // Wait for ENTER or Q
        bool startGame = false;
        bool quit = false;
        
        while (!startGame && !quit) {
            if (terminal.kbhit()) {
                char key = terminal.getch();
                if (key == '\n' || key == '\r' || key == ' ') {
                    startGame = true;
                } else if (key == 'q' || key == 'Q') {
                    quit = true;
                }
            }
            this_thread::sleep_for(chrono::milliseconds(50));
        }
        
        if (quit) {
            break;
        }
        
        if (startGame) {
            bool replay = runGame(terminal, highScoreManager);
            if (!replay) {
                break; // User chose to quit after game over
            }
        }
    }
    
    terminal.clearScreen();
    terminal.showCursor();
    // LINUX FIX: Build exit message in buffer
    ostringstream exitBuffer;
    exitBuffer << "\n  Thanks for playing!\n\n";
    cout << exitBuffer.str();
    cout.flush();
    
    return 0;
}

