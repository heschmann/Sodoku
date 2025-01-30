#include <iostream>
#include <bitset>
#include <array>
#include <set>
#include <chrono>
#include <fstream>
#include <sstream>

#define VERBOSE 0           // 0 = no 1 = Guesses 2 = setting 3 = removing
#define ADVANCED_COLLAPSE 0 // 0 = normal collapsing, 1 = check for unique options
#define BEST_GUESS 1        // 0 = guess in first unsolved cell, 1 = guess in cell with the least amount of options
// this avoids a lot of branching in the RBT tree early on
// note that the speedup is not deterministic, by chance using the first cell might be faster sometimes

#if VERBOSE == 1
#define D(x) x
#else
#define D(x)
#endif

void printBoard(std::array<int, 9 * 9> &board)
{
    int i = 0;
    for (int digit : board)
    {
        if (i % 9 == 0)
        {
            std::cout << std::endl;
        }
        std::cout << digit << " ";
        i++;
    }
    std::cout << std::endl;
}

std::array<int, 9 * 9> loadBoardFromFile(std::string fileName)
{
    std::array<int, 9 * 9> board;
    board.fill(0);

    std::ifstream file(fileName);

    std::string line;
    int oneValue;
    int i = 0;
    for (size_t j = 0; j < 9; j++)
    {
        std::getline(file, line);
        std::stringstream ss(line);
        while (ss >> oneValue)
        {
            board[i] = oneValue;
            i++;
        }
    }
    return board;
}

std::array<std::array<int, 9 * 9>, 50> loadBoardsFromFile(std::string fileName)
{
    std::array<std::array<int, 9 * 9>, 50> boards;
    for (auto &board : boards)
    {
        board.fill(0);
    }

    std::ifstream file(fileName);

    std::string line;
    int oneValue;
    for (size_t numBoard = 0; numBoard < 50; numBoard++)
    {
        std::getline(file, line);
        int i = 0;
        for (size_t j = 0; j < 9; j++)
        {
            std::getline(file, line);
            std::stringstream ss(line);
            while (ss >> oneValue)
            {
                boards[numBoard][i] = oneValue;
                i++;
            }
        }
    }
    return boards;
}

int getNumberFromBitset(std::bitset<9> bits)
{
    for (int i = 0; i < 9; i++)
    {
        if (bits[i])
        {
            return i + 1;
        }
    }
    std::cout << "Something went wrong (fetching number from empty options list)" << std::endl;
    return -1;
}

int getNumber(int row, int col)
{
    return 9 * row + col;
}

std::array<int, 9 + 6 + 6> getToCheck(int idx)
{
    std::set<int> candidate;
    int row = idx / 9;
    int col = idx % 9;
    for (int i = 0; i < 9; i++)
    {
        candidate.insert(getNumber(row, i));
        candidate.insert(getNumber(i, col));
    }
    int startRow = row / 3 * 3;
    int startCol = col / 3 * 3;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            candidate.insert(getNumber(startRow + i, startCol + j));
        }
    }
    std::array<int, 9 + 6 + 6> toCheck;
    int i = 0;
    for (int c : candidate)
    {
        toCheck[i] = c;
        i++;
    }
    return toCheck;
}

std::array<int, 9 + 9 + 9> getToCheck2(int idx)
{
    // redundant checks are faster than using set?
    std::array<int, 9 + 9 + 9> toCheck;
    int iter = 0;
    int row = idx / 9;
    int col = idx % 9;
    for (int i = 0; i < 9; i++)
    {
        toCheck[iter] = getNumber(row, i);
        iter++;
        toCheck[iter] = getNumber(i, col);
        iter++;
    }
    int startRow = row / 3 * 3;
    int startCol = col / 3 * 3;
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            toCheck[iter] = getNumber(startRow + i, startCol + j);
            iter++;
        }
    }
    return toCheck;
}

bool collapseAdvanced(std::array<int, 9 * 9> &board, std::array<std::bitset<9>, 9 * 9> &options, int idx);
bool collapse(std::array<int, 9 * 9> &board, std::array<std::bitset<9>, 9 * 9> &options, int idx)
{
    board[idx] = getNumberFromBitset(options[idx]); // set cell
    D(std::cout << "Setting " << idx << " to " << board[idx] << std::endl;)
    // std::array<int, 9 + 6 + 6> toCheck = getToCheck(idx);
    std::array<int, 9 + 9 + 9> toCheck = getToCheck2(idx);
    for (size_t j = 0; j < toCheck.size(); j++) // check all cells
    {
        int i = toCheck[j];
        int cell = board[i];
        if (!cell) // if cell not solved
        {
            options[i] &= ~options[idx]; // remove option from cell
            D(std::cout << "Removed " << board[idx] << " from " << i << std::endl;)
        }
    }
    bool flag = true;
    for (size_t j = 0; j < toCheck.size(); j++) // check all cells
    {
        int i = toCheck[j];
        if (options[i].none())
        {
            D(std::cout << "Board became infeasible." << std::endl;)
            return false; // no options = infeasible
        }
        else if (!board[i] && options[i].count() == 1) // one option and not solved
        {
            flag = collapse(board, options, i); // only one option and not solved
            if (!flag)
            {
                return false;
            }
        }
    }

// Advanced checking
#if ADVANCED_COLLAPSE == 1
    for (size_t j = 0; j < toCheck.size(); j++) // check all cells
    {
        int i = toCheck[j];
        if (!board[i]) // multiple options, not solved
        {
            flag = collapseAdvanced(board, options, i); // only one option and not solved
            if (!flag)
            {
                return false;
            }
        }
    }
#endif

    return true;
}

bool collapseAdvanced(std::array<int, 9 * 9> &board, std::array<std::bitset<9>, 9 * 9> &options, int idx)
{
    // if for any row, col, block there is only one cell left with a given option, that cell must be that option
    // compare current cell to all relevant cells (except solved cells and itself)
    // row
    int row = idx / 9;
    std::bitset<9> opt = options[idx];
    for (size_t i = 0; i < 9; i++)
    {
        int j = getNumber(row, i);
        if (j != idx)
        {
            opt &= ~options[j];
        }
    }
    if (opt.count() == 1)
    {
        D(std::cout << "Only one option for cell " << idx << " (row-wise)" << std::endl;)
        options[idx] = opt;
        return collapse(board, options, idx);
    }

    // col
    int col = idx % 9;
    opt = options[idx];
    for (size_t i = 0; i < 9; i++)
    {
        int j = getNumber(i, col);
        if (j != idx)
        {
            opt &= ~options[j];
        }
    }
    if (opt.count() == 1)
    {
        D(std::cout << "Only one option for cell " << idx << " (col-wise)" << std::endl;)
        options[idx] = opt;
        return collapse(board, options, idx);
    }

    // block
    int startRow = row / 3 * 3;
    int startCol = col / 3 * 3;
    opt = options[idx];
    for (int i = 0; i < 3; i++)
    {
        for (int k = 0; k < 3; k++)
        {
            int j = getNumber(startRow + i, startCol + k);
            if (j != idx)
            {
                opt &= ~options[j];
            }
        }
    }
    if (opt.count() == 1)
    {
        D(std::cout << "Only one option for cell " << idx << " (block-wise)" << std::endl;)
        options[idx] = opt;
        return collapse(board, options, idx);
    }

    return true;
}

#if BEST_GUESS == 0
int getBestGuess(std::array<int, 9 * 9> &board, std::array<std::bitset<9>, 9 * 9> &options)
{
    int idx = 0;
    while (board[idx])
    {
        idx++;
        if (idx == board.size())
        {
            return idx;
        }
    };
    return idx;
}
#else
int getBestGuess(std::array<int, 9 * 9> &board, std::array<std::bitset<9>, 9 * 9> &options)
{
    int bestIdx = board.size();
    int size = 10;
    for (size_t i = 0; i < board.size(); i++)
    {
        if (!board[i]) // not solved
        {
            if (options[i].count() == 2)
            {
                return i; // if only two options check this cell immediately
            }
            else if (options[i].count() < size)
            {
                // record best visited cell
                bestIdx = i;
                size = options[i].count();
            }
        }
    }
    return bestIdx;
}
#endif

bool recursiveSolve(std::array<int, 9 * 9> &board, std::array<std::bitset<9>, 9 * 9> &options)
{
    std::array<int, 9 * 9> boardCopy;
    std::array<std::bitset<9>, 9 * 9> optionsCopy;
    // find first unsolved cell
    int idx = getBestGuess(board, options);
    if (idx == board.size())
    {
        std::cout << "Board is solved!" << std::endl;
        return true;
    }
    bool solved = false;
    for (size_t i = 0; i < 9; i++)
    {
        if (options[idx][i])
        {
            // copy original
            // std::copy(std::begin(board), std::end(board), std::begin(boardCopy));
            // std::copy(std::begin(options), std::end(options), std::begin(optionsCopy));
            boardCopy = board;
            optionsCopy = options;
            optionsCopy[idx].reset();
            optionsCopy[idx].set(i);
            D(std::cout << "Guessing " << getNumberFromBitset(optionsCopy[idx]) << " for cell " << idx << std::endl;)
            solved = collapse(boardCopy, optionsCopy, idx);
            if (solved)
            {
                // valid board, start RBT
                solved = recursiveSolve(boardCopy, optionsCopy);
            }
            if (solved)
            {
                // copy solution into board and terminate
                std::copy(std::begin(boardCopy), std::end(boardCopy), std::begin(board));
                std::copy(std::begin(optionsCopy), std::end(optionsCopy), std::begin(options));
                return true;
            }
        }
    }
    D(std::cout << "End of options reached!" << std::endl;)
    return false;
}

void solve(std::array<int, 9 * 9> &board)
{
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::array<std::bitset<9>, 9 * 9> options;
    for (int i = 0; i < board.size(); i++)
    {
        options[i].set(); // initialize options
    }

    // first solving pass
    for (int i = 0; i < board.size(); i++)
    {
        if (board[i] > 0)
        {
            options[i].reset();
            options[i].set((board[i] - 1));
            collapse(board, options, i);
        }
        // std::cout << options[i] << std::endl;
    }

    recursiveSolve(board, options);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "solving time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << " µs" << std::endl;
}
int main()
{
    /*
    // auto board = loadBoardFromFile("boards/easy.txt");
    // auto board = loadBoardFromFile("boards/medium.txt");
    auto board = loadBoardFromFile("boards/hard.txt");
    // auto board = loadBoardFromFile("boards/extreme.txt");

    printBoard(board);
    solve(board);
    printBoard(board);
    */

    auto boards = loadBoardsFromFile("boards/multi.txt");
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for (auto &board : boards)
    {
        solve(board);
        // printBoard(board);
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "\nAvarage solving time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / boards.size() << " µs" << std::endl;

    return EXIT_SUCCESS;
}