#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <chrono>
#include <thread>
#include <iomanip>
#include <conio.h>
#include <limits>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// 棋盘大小和雷数（可变，由难度选择决定）
int ROWS;
int COLS;
int MINES;

// 单元格状态
enum CellStatus { HIDDEN, REVEALED, FLAGGED };

// 颜色定义 (ANSI 转义序列)
const string COLOR_RESET = "\x1b[0m";
const string COLOR_HIDDEN = "\x1b[37m";
const string COLOR_REVEALED = "\x1b[32m";
const string COLOR_FLAGGED = "\x1b[31m";
const string COLOR_MINE = "\x1b[91m";
const string COLOR_TIME = "\x1b[33m";

// 棋盘和状态（动态分配）
vector<vector<int>> board;
vector<vector<CellStatus>> status;
vector<pair<int, int>> minePositions;



#ifdef _WIN32
void enableVirtualTerminalProcessing() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
}
#else
void enableVirtualTerminalProcessing() {}
#endif

bool saveBoardToFile(const string& filename, int ROWS, int COLS, int MINES, const std::vector<std::pair<int, int>>& minePositions) {
    ofstream outfile(filename + ".sl"); // 添加 .sl 后缀
    if (!outfile.is_open()) {
        cerr << "无法创建文件 " << filename << ".sl" << endl;
        return false;
    }

    outfile << ROWS << " " << COLS << " " << MINES << endl; // 写入棋盘信息
    for (const auto& pos : minePositions) {
        outfile << pos.first << " " << pos.second << endl; // 写入雷的位置
    }

    outfile.close();
    cout << "棋盘已保存到 " << filename << ".sl" << endl;
    system("PAUSE");
    return true;
}


// 加载棋盘布局从文件
bool loadBoardFromFile(const string& filename, int& ROWS, int& COLS, int& MINES) {
    // 检查文件后缀
    string lowerFilename = filename;
    transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);
    if (lowerFilename.length() < 3 || lowerFilename.substr(lowerFilename.length() - 3) != ".sl") {
        cerr << "错误：文件后缀名必须为 .sl" << endl;
        return false;
    }

    ifstream infile(filename);
    if (!infile.is_open()) {
        cerr << "无法打开文件 " << filename << endl;
        return false;
    }

    if (!(infile >> ROWS >> COLS >> MINES)) {
        cerr << "文件格式错误！" << endl;
        return false;
    }

    board.assign(ROWS, vector<int>(COLS, 0));
    status.assign(ROWS, vector<CellStatus>(COLS, HIDDEN));
    minePositions.clear();

    for (int i = 0; i < MINES; ++i) {
        int row, col;
        if (!(infile >> row >> col) || row < 0 || row >= ROWS || col < 0 || col >= COLS) {
            cerr << "文件格式错误或雷的位置超出范围！" << endl;
            return false;
        }
        board[row][col] = -1;
        minePositions.push_back({row, col});
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

    infile.close();
    return true;
}

// 初始化棋盘
void initBoard(int ROWS, int COLS, int MINES) {
    minePositions.clear();
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distribRow(0, ROWS - 1);
    uniform_int_distribution<> distribCol(0, COLS - 1);

    for (int i = 0; i < MINES; ++i) {
        while (true) {
            int row = distribRow(gen);
            int col = distribCol(gen);
            if (board[row][col] != -1) {
                board[row][col] = -1;
                minePositions.push_back({row, col});
                break;
            }
        }
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


void printBoard(bool showMines, double elapsedTime, int cursorRow, int cursorCol, int ROWS/*列*/, int COLS/*行*/) {
    system("cls"); // 清屏
    cout << COLOR_TIME << "用时：" << fixed << setprecision(2) << elapsedTime << " 秒" << COLOR_RESET << endl;

    // 计算最大列号的宽度（位数）
    int colWidth = to_string(COLS - 1).length() + 1;

    // 输出列号
    cout << "   "; // 三个空格用于对齐行号
    for (int j = 0; j < COLS; ++j) {
        cout << setw(colWidth) << j;
    }
    cout << endl;

    // 输出上边框
    cout << " +";
    cout << setfill('-') << setw(colWidth * COLS) << "+" << setfill(' ') << endl; // 一次性输出整个上边框

    for (int i = 0; i < ROWS; ++i) {
        cout << setw(2) << i << "|"; // 输出行号和左边框
        for (int j = 0; j < COLS; ++j) {
            if (i == cursorRow && j == cursorCol) {
                cout << "["; // 光标左括号
            } else {
                cout << " ";
            }

            if (status[i][j] == REVEALED || showMines) {
                if (board[i][j] == -1) {
                    cout << COLOR_MINE << "*" << COLOR_RESET; // 显示雷
                } else if (board[i][j] == 0) {
                    cout << COLOR_REVEALED << " " << COLOR_RESET; // 显示空格
                } else {
                    cout << COLOR_REVEALED << board[i][j] << COLOR_RESET; // 显示数字
                }
            } else if (status[i][j] == FLAGGED) {
                cout << COLOR_FLAGGED << "F" << COLOR_RESET; // 显示旗帜
            } else {
                cout << COLOR_HIDDEN << "." << COLOR_RESET; // 显示隐藏
            }

            if (i == cursorRow && j == cursorCol) {
                cout << "]"; // 光标右括号
            } else {
                cout << " ";
            }
        }
        cout << "|" << endl; // 输出右边框
    }

    // 输出下边框
    cout << " +";
    cout << setfill('-') << setw(colWidth * COLS) << "+" << setfill(' ') << endl; // 一次性输出整个下边框
}


// 揭示单元格
bool revealCell(int row, int col, int ROWS, int COLS) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS || status[row][col] != HIDDEN) {
        return true; // 无效的单元格或已揭示
    }

    status[row][col] = REVEALED;
    if (board[row][col] == -1) {
        return false; // 踩到雷
    }

    if (board[row][col] == 0) {
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                if (x == 0 && y == 0) continue;
                revealCell(row + x, col + y, ROWS, COLS); // 递归揭示相邻单元格
            }
        }
    }
    return true; // 成功揭示
}

// 切换标记状态
void toggleFlag(int row, int col, int ROWS, int COLS) {
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS || status[row][col] == REVEALED) {
        return; // 无效的单元格或已揭示
    }

    if (status[row][col] == FLAGGED) {
        status[row][col] = HIDDEN;
    } else {
        status[row][col] = FLAGGED;
    }
}

// 检查是否获胜
bool checkWin(int ROWS, int COLS) {
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            if (board[i][j] != -1 && status[i][j] != REVEALED) {
                return false; // 还有未揭示的非雷单元格
            }
        }
    }
    return true; // 所有非雷单元格都已揭示
}




int handleInput(bool& firstMove, int& cursorRow, int& cursorCol, ofstream& logFile, const chrono::high_resolution_clock::time_point& startTime, int ROWS, int COLS, double& elapsedTime) {
    if (_kbhit()) {
        int ch = _getch();
        logFile << "Input: ";

        if (ch == 0xE0) { // 扩展按键（方向键）
            ch = _getch();
            switch (ch) {
                case 72: // Up
                    cursorRow = (cursorRow - 1 + ROWS) % ROWS;
                    logFile << "Up" << endl;
                    printBoard(false, elapsedTime, cursorRow, cursorCol, ROWS, COLS);
                    break;
                case 80: // Down
                    cursorRow = (cursorRow + 1) % ROWS;
                    logFile << "Down" << endl;
                    printBoard(false, elapsedTime, cursorRow, cursorCol, ROWS, COLS);
                    break;
                case 75: // Left
                    cursorCol = (cursorCol - 1 + COLS) % COLS;
                    logFile << "Left" << endl;
                    printBoard(false, elapsedTime, cursorRow, cursorCol, ROWS, COLS);
                    break;
                case 77: // Right
                    cursorCol = (cursorCol + 1) % COLS;
                    logFile << "Right" << endl;
                    printBoard(false, elapsedTime, cursorRow, cursorCol, ROWS, COLS);
                    break;
                default:
                    logFile << "Unknown extended key: " << ch << endl;
                    break;
            }
            return 0; // 防止在方向键操作后还执行其他操作
        } else {
            switch (ch) {
                case ' ': // 空格键，标记/取消标记
                    if (firstMove) {
                        firstMove = false;
                    }
                    toggleFlag(cursorRow, cursorCol, ROWS, COLS);
                    logFile << "Flag/Unflag at: " << cursorRow << " " << cursorCol << endl;
                    printBoard(false, elapsedTime, cursorRow, cursorCol, ROWS, COLS);
                    break;
                case 13: { // 回车键，翻开
                    if (firstMove) {
                        firstMove = false;
                    }
                    if (!revealCell(cursorRow, cursorCol, ROWS, COLS)) {
                        return 2; // 踩到雷，游戏结束
                    }
                    logFile << "Reveal at: " << cursorRow << " " << cursorCol << endl;
                    printBoard(false, elapsedTime, cursorRow, cursorCol, ROWS, COLS);
                    break;
                }
                case 27: // Esc 键，退出
                    cout << "退出游戏。" << endl;
                    logFile << "Game Ended by User." << endl;
                    logFile.close();
                    return 1; // 返回 1 表示退出
                default:
                    logFile << (char)ch << " is invalid." << endl;
                    break;
            }
        }
    }
    return 0; // 没有按键按下，游戏继续
}



// 处理游戏结束
bool gameOver(ofstream& logFile, double duration, bool win, bool& playAgain, bool& sameSeed, int ROWS, int COLS, int MINES) {
    if (win) {
        cout << COLOR_REVEALED << "恭喜你，获胜！" << COLOR_RESET << endl;
        logFile << "Game Over (Won). Time: " << duration / 1000.0 << "s" << endl;
    } else {
        cout << COLOR_MINE << "你踩到雷了！游戏结束。" << COLOR_RESET << endl;
        logFile << "Game Over (Lost). Time: " << duration / 1000.0 << "s" << endl;
        _getch(); // 暂停，按任意键继续
    }


    logFile.close();

    cout << "游戏结束！再来一局？(y/n/s - 相同种子,l - 加载游戏): ";
    char playAgainChoice;
    cin >> playAgainChoice;

    if (playAgainChoice == 'l' || playAgainChoice == 'L') {
        string filename;
        cout << "输入文件名：";
        cin >> filename;
        int loadedRows, loadedCols, loadedMines;//用于接收加载文件中的行列数和雷数
        if (!loadBoardFromFile(filename, loadedRows, loadedCols, loadedMines)) {
            cout << "加载失败，返回难度选择。" << endl;
            return true; // 加载失败，需要重新初始化棋盘
        }
        ROWS = loadedRows;
        COLS = loadedCols;
        MINES = loadedMines;
        return false; // 加载成功，不需要重新初始化棋盘
    } else if (playAgainChoice != 'y' && playAgainChoice != 'Y') {
        if (playAgainChoice == 's' || playAgainChoice == 'S') {
            sameSeed = true;
        } else {
            playAgain = false;
        }
        return false; // 不需要重新初始化棋盘，但需要重新开始游戏循环(相同种子)
    }
    return false; // 用户选择重新开始新游戏，不需要重新初始化棋盘
}




void chooseDifficulty(int& ROWS, int& COLS, int& MINES, bool& sameSeed) {
    if (sameSeed) return;

    int choice;
    cout << "请选择游戏难度：" << endl;
    cout << "1. 初级 (10x10, 15 雷)" << endl;
    cout << "2. 中级 (15x15, 25 雷)" << endl;
    cout << "3. 高级 (20x20, 35 雷)" << endl;
    cout << "4. 自定义" << endl;
    cout << "5. 加载文件" << endl;
    cin >> choice;

    switch (choice) {
        case 1:
            ROWS = 10;
            COLS = 10;
            MINES = 15;
            break;
        case 2:
            ROWS = 15;
            COLS = 15;
            MINES = 25;
            break;
        case 3:
            ROWS = 20;
            COLS = 20;
            MINES = 35;
            break;
        case 4: {
            cout << "请输入棋盘行数（ROWS）：";
            cin >> ROWS;
            cout << "请输入棋盘列数（COLS）：";
            cin >> COLS;
            do {
                cout << "请输入雷数（MINES）：";
                cin >> MINES;
                if (MINES < ROWS * COLS * 0.15 || MINES > ROWS * COLS * 0.85) {
                    cout << "雷数不合法，请重新输入（雷数应在棋盘面积的15%到85%之间）。" << endl;
                }
            } while (MINES < ROWS * COLS * 0.15 || MINES > ROWS * COLS * 0.85);
            break;
        }
        case 5: {
            string filename;
            cout << "输入文件名：";
            cin >> filename;
            int loadedRows, loadedCols, loadedMines;
            if (!loadBoardFromFile(filename, loadedRows, loadedCols, loadedMines)) {
                cout << "加载失败，返回难度选择。" << endl;
                chooseDifficulty(ROWS, COLS, MINES, sameSeed); // 递归调用，返回难度选择
            } else {
                ROWS = loadedRows;
                COLS = loadedCols;
                MINES = loadedMines;
            }
            break;
        }
        default:
            cout << "无效的选择，使用默认设置 (10x10, 15 雷)。" << endl;
            break;
    }
}


bool checkWinByFlags(int ROWS, int COLS) {
    int flaggedCount = 0;
    for (int i = 0; i < ROWS; ++i) {
        for (int j = 0; j < COLS; ++j) {
            if (status[i][j] == FLAGGED && board[i][j] == -1) {
                flaggedCount++;
            }
        }
    }
    return flaggedCount == MINES;
}

int gameLoop(bool& firstMove, int& cursorRow, int& cursorCol, ofstream& logFile, const chrono::high_resolution_clock::time_point& startTime, bool& playAgain, bool& sameSeed, int ROWS, int COLS, int MINES) {
    double elapsedTime = 0;
    while (true) {
        if (!firstMove) {
            auto currentTime = chrono::high_resolution_clock::now();
            elapsedTime = chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0;
        }
        int result = handleInput(firstMove, cursorRow, cursorCol, logFile, startTime, ROWS, COLS, elapsedTime);
        if (result == 1) {
            return 1; // 用户选择退出游戏
        }
        if (result == 2) { // 踩到雷！
            auto endTime = chrono::high_resolution_clock::now();
            elapsedTime = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count() / 1000.0;
            printBoard(true, elapsedTime, cursorRow, cursorCol, ROWS, COLS);
            if (gameOver(logFile, elapsedTime, false, playAgain, sameSeed, ROWS, COLS, MINES)) {
                return 0;
            }
            return 0;
        }

        if (!firstMove) {
            printBoard(false, elapsedTime, cursorRow, cursorCol, ROWS, COLS);
        }

        if (checkWin(ROWS, COLS) || checkWinByFlags(ROWS, COLS)) { // 同时检查两种获胜条件
            auto endTime = std::chrono::high_resolution_clock::now();
            elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() / 1000.0;
            printBoard(true, elapsedTime, cursorRow, cursorCol, ROWS, COLS);
            if (gameOver(logFile, elapsedTime, true, playAgain, sameSeed, ROWS, COLS, MINES)) {
                return 0;
            }
            return 0;
        }
        this_thread::sleep_for(chrono::milliseconds(100)); // 适当的延时
    }
}

// 新增的 rebuildBoard 函数
void rebuildBoard(int ROWS, int COLS, const std::vector<std::pair<int, int>>& minePositions) {
    board.assign(ROWS, std::vector<int>(COLS, 0)); // 清空 board
    for (const auto& pos : minePositions) {
        board[pos.first][pos.second] = -1; // 设置雷
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



int main() {
    enableVirtualTerminalProcessing();
    srand(time(0));

    ROWS = 10;
    COLS = 10;
    MINES = 15;

    bool playAgain = true;
    bool sameSeed = false;
    double elapsedTime = 0;

    while (playAgain) {
        chooseDifficulty(ROWS, COLS, MINES, sameSeed);

        ofstream logFile("minesweeper_log.txt", ios::app);
        if (!logFile.is_open()) {
            cerr << "无法打开日志文件！" << endl;
            return 1;
        }
        logFile << "New Game Started at: " << std::time(0) << std::endl;

        int cursorRow = 0;
        int cursorCol = 0;
        bool firstMove = true;
        elapsedTime = 0;

        cout << "按任意键开始游戏..." << endl;
        _getch();

        board.assign(ROWS, vector<int>(COLS, 0));
        status.assign(ROWS, vector<CellStatus>(COLS, HIDDEN));

        if (!sameSeed) {
            minePositions.clear();
            initBoard(ROWS, COLS, MINES);

            cout << "是否保存当前设置？(y/n): ";
            char saveChoice;
            cin >> saveChoice;
            if (saveChoice == 'y' || saveChoice == 'Y') {
                time_t rawtime;
                struct tm* timeinfo;
                char buffer[80];

                time(&rawtime);
                timeinfo = localtime(&rawtime);

                strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", timeinfo);
                string str(buffer);

                if (!saveBoardToFile(str, ROWS, COLS, MINES, minePositions)) {
                    cerr << "保存文件失败" << endl;
                }
            }
        }

        rebuildBoard(ROWS, COLS, minePositions);

        printBoard(false, elapsedTime, cursorRow, cursorCol, ROWS, COLS);
        int gameResult = gameLoop(firstMove, cursorRow, cursorCol, logFile, chrono::high_resolution_clock::now(), playAgain, sameSeed, ROWS, COLS, MINES);
        if (gameResult == 1) {
            playAgain = false;
            break;
        }

        if (!playAgain) {
            continue;
        }

        if (sameSeed) {
            sameSeed = true;
        } else {
            sameSeed = false;
        }
        logFile.close();
    }
    return 0;
}