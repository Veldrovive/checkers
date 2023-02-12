#include "board.hpp"

#include <iostream>
#include <fstream>
#include <functional>
#include <algorithm>

move redManMoves[2] = {{-1, -1}, {1, -1}};
move blackManMoves[2] = {{-1, 1}, {1, 1}};
move kingMoves[4] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};

Board::Board(int width, int height) {  // Main Constructor
    this->width = width;
    this->height = height;
    this->strRep = "";
}

Board::Board(const Board& other) {  // Copy Constructor
    this->width = other.width;
    this->height = other.height;
    this->sparseBoard = other.sparseBoard;
    this->strRep = "";
}

Board::~Board() {
    // Destructor
}

void Board::setRep() {
    // strRep = toString();
    // hashVal = std::hash<std::string>()(strRep);
    // Create a hash builder
    strRep = "";
    std::hash<std::string> hashBuilder;
    // Sort the sparseBoard by location
    std::vector<std::pair<location, int>> sortedBoard(sparseBoard.begin(), sparseBoard.end());
    std::sort(sortedBoard.begin(), sortedBoard.end(), [](const std::pair<location, int>& a, const std::pair<location, int>& b) {
        return a.first.x < b.first.x || (a.first.x == b.first.x && a.first.y < b.first.y);
    });
    // Iterate over the sparseBoard and hash each value
    for (auto it = sortedBoard.begin(); it != sortedBoard.end(); it++) {
        location loc = it->first;
        int val = it->second;
        // int p = ((val + 3) << 16) | (loc.x) | (loc.y << 8);
        // hashVal ^= loc.x ^ (loc.x << loc.y);
        // hashVal ^= loc.y ^ (loc.y << loc.x);
        // hashVal ^= val ^ (val << 16);
        // Create a string representation
        strRep += std::to_string(val) + std::to_string(loc.x) + std::to_string(loc.y);
        // Hash the string representation
    }
    hashVal = hashBuilder(strRep);
}

std::size_t Board::hash() {
    if (strRep == "") {
        setRep();
    }
    return hashVal;
}

Board* Board::invert() {
    // Returns a new board with the pieces inverted and mirror across the x-axis
    Board* newBoard = new Board(width, height);
    for (auto it = sparseBoard.begin(); it != sparseBoard.end(); it++) {
        location loc = it->first;
        int val = it->second;
        location newLoc = {loc.x, height - 1 - loc.y};
        newBoard->sparseBoard[newLoc] = -val;
    }
    return newBoard;
}

Board* Board::performMove(int player, location loc, move m) {
    // Returns a new board with the move performed
    // Moves involve simply removing the original position from the sparseBoard and adding the new position with the same value
    Board* newBoard = new Board(*this);
    int val = newBoard->sparseBoard[loc];
    int newRow = loc.y + m.dy;
    if (newRow == 0 && val == 1) {
        val = 2;
    } else if (newRow == height - 1 && val == -1) {
        val = -2;
    }
    newBoard->sparseBoard.erase(loc);
    location newLoc = {loc.x + m.dx, loc.y + m.dy};
    if (val == 0) {
        throw "Error: Attempted to move an empty space";
    }
    newBoard->sparseBoard[newLoc] = val;
    return newBoard;
}

bool Board::isMoveValid(int player, location loc, move m) {
    // Returns true if the piece belongs to the player, loc + m is empty, and loc + m is in bounds
    // Is loc in the sparseBoard?
    if (sparseBoard.find(loc) == sparseBoard.end()) {
        return false;
    }
    // Does loc belong to the player?
    int ownVal = sparseBoard[loc];
    if (ownVal * player <= 0) {
        return false;
    }
    location newLoc = {loc.x + m.dx, loc.y + m.dy};
    // newLoc must be in bounds and must not be a key in the sparseBoard
    if (newLoc.x < 0 || newLoc.x >= width || newLoc.y < 0 || newLoc.y >= height) {
        return false;
    }
    if (sparseBoard.find(newLoc) != sparseBoard.end()) {
        return false;
    }
    return true;
}

Board* Board::performJump(int player, location loc, move m) {
    // Returns a new board with the jump performed
    // Jumps are slightly more complicated. We assume there is a piece at {loc.x + m.dx, loc.y + m.dy} that will be remove.
    // We then remove the original position from the sparseBoard and add {loc.x + 2*m.dx, loc.y + 2*m.dy} with the same value
    Board* newBoard = new Board(*this);
    int val = newBoard->sparseBoard[loc];
    int newRow = loc.y + 2*m.dy;
    if (newRow == 0 && val == 1) {
        val = 2;
    } else if (newRow == height - 1 && val == -1) {
        val = -2;
    }
    if (val == 0) {
        throw "Error: Attempted to jump with an empty space";
    }
    newBoard->sparseBoard.erase(loc);
    location newLoc = {loc.x + 2*m.dx, loc.y + 2*m.dy};
    newBoard->sparseBoard[newLoc] = val;
    location removeLoc = {loc.x + m.dx, loc.y + m.dy};
    newBoard->sparseBoard.erase(removeLoc);
    return newBoard;
    // TODO: Check if the piece has reached the end of the board and should be promoted to a king
}

bool Board::isJumpValid(int player, location loc, move m) {
    // Returns true if the piece belongs to the player, loc + m is an opposing piece (val * player < 0), and loc + 2*m is empty
    // Is loc in the sparseBoard?
    if (sparseBoard.find(loc) == sparseBoard.end()) {
        return false;
    }
    // Is the piece at loc owned by the player?
    int ownVal = sparseBoard[loc];
    if (ownVal * player <= 0) {
        return false;
    }
    location jumpLoc = {loc.x + m.dx, loc.y + m.dy};
    // Is there a piece at jumpLoc?
    if (sparseBoard.find(jumpLoc) == sparseBoard.end()) {
        return false;
    }
    // Is the piece at jumpLoc owned by the opponent?
    int jumpVal = sparseBoard[jumpLoc];
    if (jumpVal * player >= 0) {
        return false;
    }
    location newLoc = {loc.x + 2*m.dx, loc.y + 2*m.dy};
    // newLoc must be in bounds and must not be a key in the sparseBoard
    if (newLoc.x < 0 || newLoc.x >= width || newLoc.y < 0 || newLoc.y >= height) {
        return false;
    }
    if (sparseBoard.find(newLoc) != sparseBoard.end()) {
        return false;
    }
    return true;
}

void Board::followMultiJump(std::vector<successor>& successors, int player, location loc, move m) {
    // Recursive function that appends all possible end states to the successors vector
    // In a single step, performs the jump and then calls itself with all valid jumps from the new board
    int pieceVal = sparseBoard[loc];  // This is important to have this from the old board because it prevents newly promoted pieces from jumping again
    Board* newBoard = performJump(player, loc, m);
    location newLoc = {loc.x + 2*m.dx, loc.y + 2*m.dy};
    bool isKing = pieceVal == 2 || pieceVal == -2;
    // Now we need to check if there are any more jumps we can make
    // To do this, we first select the correct move set.
    move* moves;
    int numMoves;
    if (isKing) {
        moves = kingMoves;
        numMoves = 4;
    } else {
        if (player == 1) {
            moves = redManMoves;
            numMoves = 2;
        } else {
            moves = blackManMoves;
            numMoves = 2;
        }
    }
    // Now we check each move to see if it is valid. If it is then we call followMultiJump on it
    bool foundJump = false;
    for (int i = 0; i < numMoves; i++) {
        move newMove = moves[i];
        if (newBoard->isJumpValid(player, newLoc, newMove)) {
            // TODO: Make sure the multi-jump ends if the piece is promoted to a king instead of continuing
            newBoard->followMultiJump(successors, player, newLoc, newMove);
            foundJump = true;
        }
    }
    // If we get here and haven't found a jump, we are in a terminal state and we can add the board to the successors vector
    if (!foundJump) {
        successors.push_back({ newBoard, loc, m });
    }
}

std::vector<successor> Board::getSuccessors(int player) {
    // Returns a vector of all possible successor boards
    // To do this, we first check for any jumps. If there are any, we can skip the non-jump moves
    std::vector<successor> successors;
    // Check for jumps
    bool foundJump = false;
    for (auto it = sparseBoard.begin(); it != sparseBoard.end(); it++) {
        location loc = it->first;
        int pieceVal = it->second;
        // Is the piece owned by the player?
        if (pieceVal * player <= 0) {
            continue;
        }
        bool isKing = pieceVal == 2 || pieceVal == -2;
        // Now we need to check if there are any more jumps we can make
        // To do this, we first select the correct move set.
        move* moves;
        int numMoves;
        if (isKing) {
            moves = kingMoves;
            numMoves = 4;
        } else {
            if (player == 1) {
                moves = redManMoves;
                numMoves = 2;
            } else {
                moves = blackManMoves;
                numMoves = 2;
            }
        }
        // Now we check each move to see if it is valid. If it is then we call followMultiJump on it
        for (int i = 0; i < numMoves; i++) {
            move m = moves[i];
            if (isJumpValid(player, loc, m)) {
                followMultiJump(successors, player, loc, m);
                foundJump = true;
            }
        }
    }
    // If we found a jump, we can return the successors vector
    if (foundJump) {
        return successors;
    }
    // If we get here, there are no jumps, so we need to check for non-jump moves
    for (auto it = sparseBoard.begin(); it != sparseBoard.end(); it++) {
        location loc = it->first;
        int pieceVal = it->second;
        // Is the piece owned by the player?
        if (pieceVal * player <= 0) {
            continue;
        }
        bool isKing = pieceVal == 2 || pieceVal == -2;
        // Now we need to check if there are any more jumps we can make
        // To do this, we first select the correct move set.
        move* moves;
        int numMoves;
        if (isKing) {
            moves = kingMoves;
            numMoves = 4;
        } else {
            if (player == 1) {
                moves = redManMoves;
                numMoves = 2;
            } else {
                moves = blackManMoves;
                numMoves = 2;
            }
        }
        // Now we check each move to see if it is valid. If it is then we call followMultiJump on it
        for (int i = 0; i < numMoves; i++) {
            move m = moves[i];
            if (isMoveValid(player, loc, m)) {
                Board* newBoard = performMove(player, loc, m);
                successors.push_back({ newBoard, loc, m });
            }
        }
    }
    return successors;
}

std::vector<successor> Board::getUniqueSuccessors(int player) {
    // Gets the successors and then removes any duplicates
    std::vector<successor> successors = getSuccessors(player);
    // Sort by hash to make duplicates adjacent
    std::sort(successors.begin(), successors.end(), [](successor a, successor b) {
        return a.board->hash() < b.board->hash();
    });
    // Remove duplicates
    successors.erase(std::unique(successors.begin(), successors.end(), [](successor a, successor b) {
        return a.board->hash() == b.board->hash();
    }), successors.end());
    return successors;
}

Board* Board::readFromFile(std::string filename) {
    // Create a new board
    Board* board = new Board(8, 8);
    // Open the file
    std::ifstream file(filename);
    // Read the file
    std::string line;
    int y = 0;
    while (getline(file, line)) {
        for (int x = 0; x < line.length(); x++) {
            char c = line[x];
            if (c != '.') {
                int value = 0;
                if (c == 'r') {
                    value = 1;
                } else if (c == 'R') {
                    value = 2;
                } else if (c == 'b') {
                    value = -1;
                } else if (c == 'B') {
                    value = -2;
                }
                location loc = {x, y};
                if (value == 0) {
                    std::cout << "Error: Invalid character in file" << std::endl;
                    exit(1);
                }
                board->sparseBoard[loc] = value;
            }
        }
        y++;
    }
    file.close();
    return board;
}

Board* Board::fromString(std::string s) {
    Board* board = new Board(8, 8);
    int x = 0;
    int y = 0;
    for (char c : s) {
        if (c == '\n') {
            x = 0;
            y++;
        } else if (c == 'r') {
            location loc = {x, y};
            board->sparseBoard[loc] = 1;
            x++;
        } else if (c == 'R') {
            location loc = {x, y};
            board->sparseBoard[loc] = 2;
            x++;
        } else if (c == 'b') {
            location loc = {x, y};
            board->sparseBoard[loc] = -1;
            x++;
        } else if (c == 'B') {
            location loc = {x, y};
            board->sparseBoard[loc] = -2;
            x++;
        } else if (c == '.') {
            x++;
        } else {
            throw std::invalid_argument("Invalid character in string");
        }
    }
    return board;
}

void Board::display() {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            location loc = {x, y};
            if (sparseBoard.count(loc) > 0) {
                int value = sparseBoard[loc];
                if (value == 1) {
                    std::cout << "r";
                } else if (value == 2) {
                    std::cout << "R";
                } else if (value == -1) {
                    std::cout << "b";
                } else if (value == -2) {
                    std::cout << "B";
                }
            } else {
                std::cout << ".";
            }
        }
        std::cout << std::endl;
    }
}

int Board::getTerminalValue() {
    // Returns 1 if red wins, -1 if black wins, 0 if draw
    int redCount = 0;
    int blackCount = 0;
    for (auto it = sparseBoard.begin(); it != sparseBoard.end(); it++) {
        if (it->second > 0) {
            redCount++;
        } else if (it->second < 0) {
            blackCount++;
        }
    }
    if (redCount == 0) {
        return -1;
    } else if (blackCount == 0) {
        return 1;
    } else {
        return 0;
    }
}

float Board::evaluate() {
    // Returns a value from -1 to 1 that represents how good the board is for red
    // The value is calculated by subtracting the number of black pieces from the number of red pieces
    int redCount = 0;
    int blackCount = 0;
    for (auto it = sparseBoard.begin(); it != sparseBoard.end(); it++) {
        if (it->second > 0) {
            redCount++;
        } else if (it->second < 0) {
            blackCount++;
        }
    }
    return (float) (redCount - blackCount) / (redCount + blackCount);
}

float Board::utility() {
    // Returns a value from -1 to 1 that represents how good the board is for red
    // The value is calculated by subtracting the number of black pieces from the number of red
    // pieces, and then dividing by the total number of pieces
    int redCount = 0;
    int blackCount = 0;
    for (auto it = sparseBoard.begin(); it != sparseBoard.end(); it++) {
        if (it->second > 0) {
            redCount++;
        } else if (it->second < 0) {
            blackCount++;
        }
    }
    return (float) (redCount - blackCount) / (redCount + blackCount);
}

std::string Board::toString() const {
    std::string str = "";
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            location loc = {x, y};
            if (sparseBoard.count(loc) > 0) {
                int value = sparseBoard.at(loc);
                if (value == 1) {
                    str += "r";
                } else if (value == 2) {
                    str += "R";
                } else if (value == -1) {
                    str += "b";
                } else if (value == -2) {
                    str += "B";
                }
            } else {
                str += ".";
            }
        }
        str += "\n";
    }
    return str;
}