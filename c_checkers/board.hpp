#ifndef BOARD_H
#define BOARD_H

#include <unordered_map>
#include <string>
#include <vector>
#include <functional>

typedef struct location {
    int x;
    int y;

    bool operator==(const location& other) const {
        return x == other.x && y == other.y;
    }
} location;

typedef struct move {
    int dx;
    int dy;
} move;

namespace std {
    template <> struct hash<location> {
        std::size_t operator()(const location &loc) const {
            return std::hash<int>()(loc.x) ^ std::hash<int>()(loc.y);
        }
    };
}

class Board;

typedef struct successor {
    Board* board;
    location piece;
    move move;
} successor;

class Board {
    // Represents a checkers board with a sparse representation
    // Red men are represented by 1, kings by 2
    // Black men are represented by -1, kings by -2
private:
    int width;
    int height;
    std::string strRep;
    std::size_t hashVal;
    std::unordered_map<location, int> sparseBoard;

    // Multi-jump moves require a recursive dfs that appends a list of all end states. Each step concatenates the current board to the end of the list and calls recursively with all valid moves.
    bool isJumpValid(int player, location loc, move m);  // Returns true if the piece belongs to the player, loc + m is an opposing piece (val * player < 0), and loc + 2*m is empty
    bool isMoveValid(int player, location loc, move m);  // Returns true if the piece belongs to the player, loc + m is empty, and loc + m is in bounds

    // std::vector<float> weights = {10.0, 2.0, 1.0, 15.0, 5.0};
    std::vector<float> redTurnRedWeights = {30.0, -30.0, 30.0, -30.0, -30.0};
    std::vector<float> redTurnBlackWeights = {30.0, 30.0, -21.953517783634453, 30.0, 30.0};
    std::vector<float> blackTurnRedWeights = {30.0, 11.227915419751112, 30.0, -30.0, 30.0};
    std::vector<float> blackTurnBlackWeights = {16.42124258905689, -30.0, -30.0, -30.0, 30.0};
public:
    void followMultiJump(std::vector<successor>& successors, int player, location loc, move m);
    Board* performJump(int player, location loc, move m);  // Returns a new board with the jump performed
    Board* performMove(int player, location loc, move m);  // Returns a new board with the move performed
    Board* performGeneralTurn(int player, location loc, move m);  // Returns a new board with the turn performed.

    Board(int width, int height);  // Main Constructor
    Board(const Board& other);  // Copy Constructor
    static Board* readFromFile(std::string filename);  // Assumes an 8x8 board and reads the sparseBoard from a file
    static Board* fromString(std::string str);  // Assumes an 8x8 board and reads the sparseBoard from a string
    Board* invert();  // Returns a new board with the pieces inverted and mirrored across the x-axis
    
    void display();

    // void setWeights(float w1, float w2, float w3, float w4, float w5);
    // void setWeights(std::vector<float> weights);
    void setWeights(std::vector<float> redWeights, std::vector<float> blackWeights, int player);
    int getTerminalValue();
    // float evaluate();
    float evaluate(int player);
    float utility();

    std::vector<successor> getSuccessors(int player);
    std::vector<successor> getUniqueSuccessors(int player);

    std::string toString() const;
    std::size_t hash();
    void setRep();

    operator std::string() const {
        return toString();
    }

    bool operator== (const Board& other) const {
        // return toString() == other.toString();
        return strRep == other.strRep;
    }

    ~Board();
};

#endif