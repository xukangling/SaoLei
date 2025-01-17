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
const string COLOR_HIDDEN = "\x1b[37m";
const string COLOR_REVEALED = "\x1b[32m";
const string COLOR_FLAGGED = "\x1b[31m";
const string COLOR_MINE = "\x1b[91m";
const string COLOR_TIME = "\x1b[33m";

// 棋盘和状态
vector<vector<int>> board(ROWS, vector<int>(COLS, 0));
vector<vector<CellStatus>> status(ROWS, vector<CellStatus>(COLS, HIDDEN));
vector<pair<int, int>> minePositions;

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

// 初始化棋盘 (保存地雷位置)
void initBoard() {
	board.assign(ROWS, vector<int>(COLS, 0));
	status.assign(ROWS, vector<CellStatus>(COLS, HIDDEN));
	minePositions.clear();

	static random_device rd;
	static mt19937 gen(rd());
	uniform_int_distribution<> distrib(0, ROWS * COLS - 1);

	for (int i = 0; i < MINES; ++i) {
		int pos;
		do {
			pos = distrib(gen);
		} while (board[pos / COLS][pos % COLS] == -1);

		int row = pos / COLS;
		int col = pos % COLS;
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
}

// 打印棋盘 (带颜色、时间和剩余雷数)
void printBoard(bool showMines = false, double elapsedTime = 0.0) {
	cout << "\x1b[2J\x1b[H";
	int flaggedCount = 0;
	for (int i = 0; i < ROWS; ++i) {
		for (int j = 0; j < COLS; ++j) {
			if (status[i][j] == FLAGGED) {
				flaggedCount++;
			}
		}
	}

	cout << "\x1b[" << 1 << ";" << COLS * 2 + 5 << "H" << COLOR_TIME << "Time: " << fixed << setprecision(1) << elapsedTime << "s  Mines Left: " << MINES - flaggedCount << COLOR_RESET << endl;

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
	enableVirtualTerminalProcessing(); // 启用 ANSI 转义序列
	srand(time(0)); // 设置随机数种子
	
	//显示版权信息
	cout << "Minesweeper 0.0.2 \n+++++++++\n+Welcome+\n+++++++++\nCopyright (c) 2025 by Xu Kangling. All rights reserved.\n\tHow to play it?\n\t\t常用命令: Q/q=退出程序; R/r+空格+行数(0~9)+列数(0-9); R/r+空格+行数(0~9)+列数(0-9); \n\t\t再来一局命令:Y/y=是; N/n=否; S/s=从上局踩雷初重玩,地雷位置相同,被踩的地雷将用*表示.\n";
	system("PAUSE");
	cin.clear();
	
	bool playAgain = true; // 控制是否继续游戏
	bool sameSeed = false; // 控制是否使用相同的地雷分布

	while (playAgain) { // 游戏主循环
		if (!sameSeed) {
			initBoard(); // 初始化棋盘（新游戏）
		}
		sameSeed = false; // 重置 sameSeed 标志

		ofstream logFile("minesweeper_log.txt", ios::app); // 打开日志文件
		if (!logFile.is_open()) {
			cerr << "无法打开日志文件！" << endl;
			return 1; // 错误处理
		}
		logFile << "\n=========================\n" << "New Game Started at: " << time(0);

		auto startTime = chrono::high_resolution_clock::now(); // 记录游戏开始时间
		bool firstMove = true; // 标记是否为第一次操作
		printBoard(); // 打印初始棋盘

		int row, col;
		char action;
		while (true) { // 游戏进行中循环
			if (_kbhit()) { // 检测是否有按键输入
				cin >> action;
				logFile << "\nInput : " << action; // 记录输入到日志

				if (action == 'r' || action == 'R') { // 揭示操作
					cout << "输入要揭示的单元格 (行 列): ";

					if (!(cin >> row >> col)) { // 检查输入是否为数字
						cout << "无效的输入，请输入数字！" << endl;
						system("PAUSE");
						cin.clear(); // 清除错误标志
						cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 清除输入缓冲区
						printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
						continue;
					}

					if (row < 0 || row >= ROWS || col < 0 || col >= COLS) { // 检查是否超出边界
						cout << "输入的行或列超出棋盘边界！" << endl;
						system("PAUSE");
						cin.clear();
						cin.ignore(numeric_limits<streamsize>::max(), '\n');
						printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
						continue;
					}
					logFile << endl << row << " " << col;
					if (firstMove) { // 第一次操作开始计时
						startTime = chrono::high_resolution_clock::now();
						firstMove = false;
					}

					if (!revealCell(row, col)) { // 踩到雷
						auto endTime = chrono::high_resolution_clock::now();
						auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
						printBoard(true, duration / 1000.0);
						cout << "你踩到雷了！游戏结束。" << endl;
						logFile << endl << "Game Over (Lost). Time: " << duration / 1000.0 << "s";
						break; // 结束游戏进行中循环
					}
				} else if (action == 'f' || action == 'F') { // 标记/取消标记操作
					cout << "输入要标记/取消标记的单元格 (行 列): ";
					if (!(cin >> row >> col)) { // 检查输入是否为数字
						cout << "无效的输入，请输入数字！" << endl;
						system("PAUSE");
						cin.clear();
						cin.ignore(numeric_limits<streamsize>::max(), '\n');
						printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
						continue;
					}

					if (row < 0 || row >= ROWS || col < 0 || col >= COLS) { // 检查是否超出边界
						cout << "输入的行或列超出棋盘边界！" << endl;
						system("PAUSE");
						cin.clear();
						cin.ignore(numeric_limits<streamsize>::max(), '\n');
						printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
						continue;
					}
					logFile << row << " " << col << endl;
					if (firstMove) { // 第一次操作开始计时
						startTime = chrono::high_resolution_clock::now();
						firstMove = false;
					}
					toggleFlag(row, col);
				} else if (action == 'q' || action == 'Q') { // 退出游戏
					cout << "退出游戏。" << endl;
					logFile << endl << "Game Ended by User." ;
					logFile.close();
					system("PAUSE");
					return 0;
				} else { // 无效输入
					cout << "无效的操作！" << endl;
					system("PAUSE");
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
					continue;
				}
			}

			if (!firstMove) { // 游戏开始后才更新时间并重绘
				auto currentTime = chrono::high_resolution_clock::now();
				auto elapsedTime = chrono::duration_cast<chrono::milliseconds>(currentTime - startTime).count();
				printBoard(false, elapsedTime / 1000.0);
			}

			if (checkWin()) { // 检查是否获胜
				auto endTime = chrono::high_resolution_clock::now();
				auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
				printBoard(true, duration / 1000.0);
				cout << "恭喜你，获胜！" << endl;
				logFile << endl << "Game Over (Won). Time: " << duration / 1000.0 << "s";
				break; // 结束游戏进行中循环
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 控制更新频率
		}

		logFile.close(); // 关闭日志文件

		cout << "再来一局？"; // 询问是否重玩
		char playAgainChoice;
		cin >> playAgainChoice;
		if (playAgainChoice != 'y' && playAgainChoice != 'Y') {
			if(playAgainChoice == 's' || playAgainChoice == 'S') {
				sameSeed = true;
			} else {
				playAgain = false;
			}
		}
	}
	system("PAUSE");
	return 0;
}
