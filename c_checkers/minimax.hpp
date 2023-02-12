#ifndef MINIMAX_H
#define MINIMAX_H

#include <unordered_map>
#include <chrono>

#include "board.hpp"

typedef struct transpositionKey {
    std::size_t boardHash;
    int player;

    bool operator==(const transpositionKey& other) const {
        return boardHash == other.boardHash && player == other.player;
    }
} transpositionKey;

typedef struct transpositionValue {
    int depth;
    float value;
} transpositionValue;

typedef struct strategyKey {
    std::size_t boardHash;
    int player;

    bool operator==(const strategyKey& other) const {
        return boardHash == other.boardHash && player == other.player;
    }
} strategyKey;

typedef struct strategyValue {
    Board* board;
    location piece;
    move move;
    float value;
} strategyValue;

namespace std {
    template <> struct hash<transpositionKey> {
        std::size_t operator()(const transpositionKey& key) const {
            return key.boardHash ^ key.player;
        }
    };
    template <> struct hash<strategyKey> {
        std::size_t operator()(const strategyKey& key) const {
            return key.boardHash ^ key.player;
        }
    };
}

class GameObserver {
private:
    std::unordered_map<std::size_t, float> evaluationCache;
    std::unordered_map<std::size_t, float> utilityCache;
    std::unordered_map<std::size_t, int> terminalCache;
    std::unordered_map<transpositionKey, transpositionValue> transpositionTable;
    std::unordered_map<strategyKey, strategyValue> strategy;  // Maps board hashes to the optimal continuation.
    std::chrono::time_point<std::chrono::high_resolution_clock> endTime;
public:
    int nodesExpanded;
    int evaluationCacheHits;
    int utilityCacheHits;
    int terminalCacheHits;
    int transpositionTableHits;
    int pruneEvents;

    GameObserver();
    // If the observer is passed an end time, shouldExit() will return true if the current time is greater than the end time.
    GameObserver(std::chrono::time_point<std::chrono::high_resolution_clock> endTime);
    ~GameObserver();

    void setEndTime(std::chrono::time_point<std::chrono::high_resolution_clock> endTime);
    bool shouldExit();  // Returns true if the observer has been instructed to exit. This would usually be due to a timeout.

    float evaluate(Board* board);  // Returns the computed evaluation. Stores the result in the evaluation cache.
    float utility(Board* board);  // Returns the computed utility. Stores the result in the utility cache.
    int getTerminalValue(Board* board);  // Returns 0 if the board is not terminal, 1 if red wins, -1 if black wins, and 2 if the game is a draw. Stores the result in the terminal cache.
    float getTranspositionValue(Board* board, int depth, int player);  // Returns the stored value if the board is in the transposition table and the depth is greater than or equal to the stored depth. Otherwise returns NAN.
    void storeTranspositionValue(Board* board, int depth, int player, float value);  // Stores the value in the transposition table.

    void updateStrategy(Board* board, Board* nextBoard, int player, location piece, move move, float value);  // Updates the strategy map with the optimal continuation.
    std::vector<Board*> recoverStrategy(Board* board);  // Recovers the optimal sequence of moves from the strategy map in the observer.

    void clearCaches();
    void clearTranspositionTable();
    void clearStrategy();
};

typedef struct minimaxResult {
    float value;
    Board* move;
    GameObserver* observer;
} minimaxResult;

minimaxResult iterativeMinimax(Board* board, int depth, int maxTime, GameObserver* observer);
minimaxResult minimax(Board* board, int depth, GameObserver* observer);

float minimax_step(Board* board, int depth, int player, float alpha, float beta, GameObserver* observer);

void getOptimalContinuation(std::string boardFile, std::string outFile, int maxDepth, int maxTime);
std::string getOptimalContinuationFromString(std::string inBoard, int maxDepth, int maxTime);

int getOptimalWinLength(std::string boardFile, int maxDepth, int maxTime);
int getOptimalWinLengthFromString(std::string inBoard, int maxDepth, int maxTime);

bool isWinning(std::string boardFile, int maxDepth, int maxTime);
bool isWinningFromString(std::string inBoard, int maxDepth, int maxTime);

#endif