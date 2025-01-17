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

const int ROWS = 10;
const int COLS = 10;
const int MINES = 15;

enum CellStatus { HIDDEN, REVEALED, FLAGGED };

const string COLOR_RESET = "\x1b[0m";
const string COLOR_HIDDEN = "\x1b[37m";
const string COLOR_REVEALED = "\x1b[32m";
const string COLOR_FLAGGED = "\x1b[31m";
const string COLOR_MINE = "\x1b[91m";
const string COLOR_TIME = "\x1b[33m";

vector<vector<int>> board(ROWS, vector<int>(COLS, 0));
vector<vector<CellStatus>> status(ROWS, vector<CellStatus>(COLS, HIDDEN));
vector<pair<int, int>> minePositions;

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

bool revealCell(int row, int col) {
	if (row < 0 || row >= ROWS || col < 0 || col >= COLS || status[row][col] == REVEALED) {
		return true;
	}

	status[row][col] = REVEALED;

	if (board[row][col] == -1) {
		return false; 
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

void toggleFlag(int row, int col) {
	if (row < 0 || row >= ROWS || col < 0 || col >= COLS || status[row][col] == REVEALED) {
		return;
	}

	status[row][col] = (status[row][col] == HIDDEN) ? FLAGGED : HIDDEN;
}

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
	
	cout << "Minesweeper 0.0.2 \n+++++++++\n+Welcome+\n+++++++++\nCopyright (c) 2025 by Xu Kangling. All rights reserved.\n\tHow to play it?\n\t\tCommon commands: Q/q=Exit program; R/r+Space+Row number (0~9)+Column number (0-9); R/r+Space+Row number (0~9)+Column number (0-9); \n\t\tCommand for another round: Y/y=Yes; N/n=No; S/s=Restart from the last round. The mines are in the same position, and the stepped mines will be indicated by *.\n";
	system("PAUSE");
	cin.clear();
	
	bool playAgain = true; 
	bool sameSeed = false; 

	while (playAgain) { 
		if (!sameSeed) {
			initBoard(); 
		}
		sameSeed = false; 

		ofstream logFile("minesweeper_log.txt", ios::app); 
		if (!logFile.is_open()) {
			cerr << "Unable to open log file!" << endl;
			return 1; 
		}
		logFile << "\n=========================\n" << "New Game Started at: " << time(0);

		auto startTime = chrono::high_resolution_clock::now(); 
		bool firstMove = true; 
		printBoard(); 

		int row, col;
		char action;
		while (true) { 
			if (_kbhit()) { 
				cin >> action;
				logFile << "\nInput : " << action; 

				if (action == 'r' || action == 'R') { 
					cout << "Enter the cell to open (row   column):";

					if (!(cin >> row >> col)) { 
						cout << "Invalid input, please enter a number!" << endl;
						system("PAUSE");
						cin.clear(); 
						cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
						printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
						continue;
					}

					if (row < 0 || row >= ROWS || col < 0 || col >= COLS) { 
						cout << "The input row or column exceeds the board boundary!" << endl;
						system("PAUSE");
						cin.clear();
						cin.ignore(numeric_limits<streamsize>::max(), '\n');
						printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
						continue;
					}
					logFile << endl << row << " " << col;
					if (firstMove) { 
						startTime = chrono::high_resolution_clock::now();
						firstMove = false;
					}

					if (!revealCell(row, col)) {
						auto endTime = chrono::high_resolution_clock::now();
						auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
						printBoard(true, duration / 1000.0);
						cout << "You stepped on a mine!\n GAME OVER!" << endl;
						logFile << endl << "Game Over (Lost). Time: " << duration / 1000.0 << "s";
						break; 
					}
				} else if (action == 'f' || action == 'F') { 
					cout << "Enter the cells to mark OR unmark (row   column):";
					if (!(cin >> row >> col)) { 
						cout << "Invalid input, please enter a number!" << endl;
						system("PAUSE");
						cin.clear();
						cin.ignore(numeric_limits<streamsize>::max(), '\n');
						printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
						continue;
					}

					if (row < 0 || row >= ROWS || col < 0 || col >= COLS) { 
						cout << "The input row or column exceeds the board boundary!" << endl;
						system("PAUSE");
						cin.clear();
						cin.ignore(numeric_limits<streamsize>::max(), '\n');
						printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
						continue;
					}
					logFile << row << " " << col << endl;
					if (firstMove) { 
						startTime = chrono::high_resolution_clock::now();
						firstMove = false;
					}
					toggleFlag(row, col);
				} else if (action == 'q' || action == 'Q') { 
					cout << "EXIT" << endl;
					logFile << endl << "Game Ended by User." ;
					logFile.close();
					system("PAUSE");
					return 0;
				} else { 
					cout << "Invalid operation!" << endl;
					system("PAUSE");
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					printBoard(false, (firstMove ? 0 : chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - startTime).count() / 1000.0));
					continue;
				}
			}

			if (!firstMove) { 
				auto currentTime = chrono::high_resolution_clock::now();
				auto elapsedTime = chrono::duration_cast<chrono::milliseconds>(currentTime - startTime).count();
				printBoard(false, elapsedTime / 1000.0);
			}

			if (checkWin()) { 
				auto endTime = chrono::high_resolution_clock::now();
				auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime).count();
				printBoard(true, duration / 1000.0);
				cout << "Congratulations, you win!" << endl;
				logFile << endl << "Game Over (Won). Time: " << duration / 1000.0 << "s";
				break; 
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		logFile.close(); 

		cout << "Another game? "; 
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
