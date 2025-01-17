#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <chrono>
#include <thread>
#include <iomanip>
#include <conio.h> // Windows 平台非阻塞输入

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// 棋盘大小和雷数
const int ROWS = 10;
const int COLS = 10;
const int MINES = 15;

// 单元格状态
enum CellStatus { HIDDEN, REVEALED, FLAGGED };

// 颜色定义 (ANSI 转义序列)
const string COLOR_RESET = "\x1b[0m";
const string COLOR_HIDDEN = "\x1b[37m";   // 白色
const string COLOR_REVEALED = "\x1b[32m"; // 绿色
const string COLOR_FLAGGED = "\x1b[31m";  // 红色
const string COLOR_MINE = "\x1b[91m";     // 亮红色
const string COLOR_TIME = "\x1b[33m";   // 黄色

// 棋盘和状态
vector<vector<int>> board(ROWS, vector<int>(COLS, 0));
vector<vector<CellStatus>> status(ROWS, vector<CellStatus>(COLS, HIDDEN));

// 启用 ANSI 转义序列 (Windows)
void enableVirtualTerminalProcessing() {
#ifdef _WIN32
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hConsole, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hConsole, dwMode);
        }
    }
#endif
}

// 初始化棋盘
void initBoard() {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<> distrib(0, ROWS * COLS - 1);

    for (int i = 0; i < MINES; ++i) {
        int pos;
        do {
            pos = distrib(gen);
        } while (board[pos / COLS][pos % COLS] == -1);

        board[pos / COLS][pos % COLS] = -1;
    }

    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            if (board[i][j] != -1) {
                int count = 0;
                for (int x = -1; x <= 1; ++x) {
                    for (int y = -1; y <= 1; ++y) {
                        int ni = i + x;
                        int nj = j + y;
                        if (ni >= 0 && ni < ROWS && nj >= 0 && nj < COLS && board[ni][nj] == -1) {
                            count++;
                        }
                    }
                }
                board[i][j] = count;
            }
        }
    }
}

// 打印棋盘 (带颜色和时间)
void printBoard(bool showMines = false, double elapsedTime = 0.0) {
    cout << "\x1b[2J\x1b[H"; // 清屏并移动光标到左上角
    cout << "\x1b[" << 1 << ";" << COLS * 2 + 5 << "H" << COLOR_TIME << "Time: " << fixed << setprecision(1) << elapsedTime << "s" << COLOR_RESET << endl;

    cout << "  ";
    for (int j = 0; j < COLS; ++j) {
        cout << j << " ";
    }
    cout << endl;

    for (int i = 0; i < ROWS; ++i) {
        cout << i << " ";
        for (int j = 0; j < COLS; ++j) {
            string color = COLOR_HIDDEN;
            char displayChar = '.';

            if (status[i][j] == FLAGGED) {
                color = COLOR_FLAGGED;
                displayChar = 'F';
            } else if (status[i][j] == REVEALED || showMines) {
                color = (board[i][j] == -1) ? COLOR_MINE : COLOR_REVEALED;
                displayChar = (board[i][j] == -1) ? '*' : (board[i][j] == 0 ? ' ' : (char)('0' + board[i][j]));
            }
            cout << color << displayChar << " " << COLOR_RESET;
        }
        cout << endl;
    }
}

// 揭示单元格
bool revealCell(int row, int col) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS || status[row][col] == REVEALED) {
        return true;
    }

    status[row][col] = REVEALED;

    if (board[row][col] == -1) {
        return false; // 踩到雷
    }

    if (board[row][col] == 0) {
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                int ni = row + x;
                int nj = col + y;
                if (ni >= 0 && ni < ROWS && nj >= 0 && nj < COLS && status[ni][nj] == HIDDEN) {
                    revealCell(ni, nj);
                }
            }
        }
    }

    return true;
}

// 切换标记状态
void toggleFlag(int row, int col) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS || status[row][col] == REVEALED) {
        return;
    }

    status[row][col] = (status[row][col] == HIDDEN) ? FLAGGED : HIDDEN;
}

// 检查是否获胜
bool checkWin() {
    int correctFlags = 0;
    int revealedCells = 0;
    int nonMineCells = 0;

    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            if (board[i][j] == -1) {
                if (status[i][j] == FLAGGED) {
                    correctFlags++;
                }
            } else {
                nonMineCells++;
                if (status[i][j] == REVEALED) {
                    revealedCells++;
                }
            }
        }
    }

    return (correctFlags == MINES || revealedCells == nonMineCells);
}



int main() {
    enableVirtualTerminalProcessing();
    srand(time(0));
    initBoard();

    auto startTime = chrono::high_resolution_clock::now();
    bool firstMove = true;
    printBoard();

    int row, col;
    char action;

    while (true) {
        if (_kbhit()) {
            cin >> action;

            if (action == 'q' || action == 'Q') { // 退出游戏
                cout << "退出游戏。" << endl;
                break;
            } else if (action == 'c' || action == 'C') { // 取消当前输入
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 清除输入缓冲区
                cout << "取消输入。" << endl;
                printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
                continue; // 继续下一次循环，不执行后续的输入处理
            } else if (action == 'r' || action == 'R') {
                cout << "输入要揭示的单元格 (行 列): ";

                if (!(cin >> row >> col)) { // 检查输入是否为数字
                    cout << "无效的输入，请输入数字！" << endl;
                    cin.clear(); // 清除错误标志
                    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 清除输入缓冲区
                    printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
                    continue;
                }

                if (row < 0 || row >= ROWS || col < 0 || col >= COLS) { // 检查是否超出边界
                    cout << "输入的行或列超出棋盘边界！" << endl;
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
                    continue;
                }
                if(firstMove){
                    startTime = chrono::high_resolution_clock::now();
                    firstMove = false;
                }
                if (!revealCell(row, col)) {
                    auto endTime = chrono::high_resolution_clock::now();
                    auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
                    printBoard(true, duration / 1000.0);
                    cout << "你踩到雷了！游戏结束。" << endl;
                    break;
                }
            } else if (action == 'f' || action == 'F') {
                cout << "输入要标记/取消标记的单元格 (行 列): ";
                if (!(cin >> row >> col)) { // 检查输入是否为数字
                    cout << "无效的输入，请输入数字！" << endl;
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
                    continue;
                }
                if (row < 0 || row >= ROWS || col < 0 || col >= COLS) { // 检查是否超出边界
                    cout << "输入的行或列超出棋盘边界！" << endl;
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
                    continue;
                }
                if(firstMove){
                    startTime = chrono::high_resolution_clock::now();
                    firstMove = false;
                }
                toggleFlag(row, col);
            } else {
                cout << "无效的操作！" << endl;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
                continue;
            }
        }
        if(!firstMove){
            auto currentTime = chrono::high_resolution_clock::now();
            auto elapsedTime = chrono::duration_cast<chrono::milliseconds>(currentTime - startTime).count();
            printBoard(false, elapsedTime / 1000.0);
        }
        if (checkWin()) {
            auto endTime = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
            printBoard(true, duration / 1000.0);
            cout << "恭喜你，获胜！" << endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 每0.1秒更新一次
    }
	
	system("PAUSE");
    return 0;
}