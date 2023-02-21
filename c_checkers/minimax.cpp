#include "minimax.hpp"
#include "board.hpp"
#include <iostream>
#include <algorithm>
#include <functional>
#include <fstream>
#include <chrono>
#include <math.h>

// TODO: Optimizations
// Once a solution has been found, update the depth of the search to go no deeper than that first solution
// Evaluation & utility caches
// Transposition tables

GameObserver::GameObserver() {
    endTime = std::chrono::high_resolution_clock::now() + std::chrono::hours(1000000);
    evaluationCache = std::unordered_map<std::size_t, float>();
    utilityCache = std::unordered_map<std::size_t, float>();
    terminalCache = std::unordered_map<std::size_t, int>();
    transpositionTable = std::unordered_map<transpositionKey, transpositionValue>();
    strategy = std::unordered_map<strategyKey, strategyValue>();

    nodesExpanded = 0;
    evaluationCacheHits = 0;
    utilityCacheHits = 0;
    terminalCacheHits = 0;
    transpositionTableHits = 0;
    pruneEvents = 0;
}

GameObserver::GameObserver(std::chrono::time_point<std::chrono::high_resolution_clock> endTime) {
    this->endTime = endTime;
    evaluationCache = std::unordered_map<std::size_t, float>();
    utilityCache = std::unordered_map<std::size_t, float>();
    terminalCache = std::unordered_map<std::size_t, int>();
    transpositionTable = std::unordered_map<transpositionKey, transpositionValue>();
    strategy = std::unordered_map<strategyKey, strategyValue>();

    nodesExpanded = 0;
    evaluationCacheHits = 0;
    utilityCacheHits = 0;
    terminalCacheHits = 0;
    transpositionTableHits = 0;
    pruneEvents = 0;
}

GameObserver::~GameObserver() {
    // Clear the caches
    clearCaches();
    clearTranspositionTable();
    clearStrategy();
}

void GameObserver::setEndTime(std::chrono::time_point<std::chrono::high_resolution_clock> endTime) {
    this->endTime = endTime;
}

bool GameObserver::shouldExit() {
    return std::chrono::high_resolution_clock::now() >= endTime;
}

float GameObserver::evaluate(Board* board, int player) {
    std::size_t boardHash = board->hash();
    if (evaluationCache.find(boardHash) != evaluationCache.end()) {
        evaluationCacheHits++;
        return evaluationCache[boardHash];
    }
    float evaluation = board->evaluate(player);
    evaluationCache[boardHash] = evaluation;
    return evaluation;
}

float GameObserver::utility(Board* board) {
    std::size_t boardHash = board->hash();
    if (utilityCache.find(boardHash) != utilityCache.end()) {
        utilityCacheHits++;
        return utilityCache[boardHash];
    }
    float utility = board->utility();
    utilityCache[boardHash] = utility;
    return utility;
}

int GameObserver::getTerminalValue(Board* board) {
    std::size_t boardHash = board->hash();
    if (terminalCache.find(boardHash) != terminalCache.end()) {
        terminalCacheHits++;
        return terminalCache[boardHash];
    }
    int terminalValue = board->getTerminalValue();
    terminalCache[boardHash] = terminalValue;
    return terminalValue;
}

float GameObserver::getTranspositionValue(Board* board, int depth, int player) {
    transpositionKey key = { board->hash(), depth };
    // if (transpositionTable.find(key) != transpositionTable.end()) {
    //     transpositionValue value = transpositionTable[key];
    //     if (value.depth >= depth) {
    //         transpositionTableHits++;
    //         return value.value;
    //     }
    // }
    return NAN;
}

void GameObserver::storeTranspositionValue(Board* board, int depth, int player, float value) {
    transpositionKey key = { board->hash(), player };
    transpositionValue transpositionValue = { depth, value };
    transpositionTable[key] = transpositionValue;
}

void GameObserver::clearCaches() {
    evaluationCache.clear();
    utilityCache.clear();
    terminalCache.clear();
}

void GameObserver::clearTranspositionTable() {
    transpositionTable.clear();
}

void GameObserver::updateStrategy(Board* board, Board* nextBoard, int player, location piece, move move, float value, int remainingDepth) {
    // If the value is better (lower for -1, higher for 1) than what is currently in strategy, update it
    strategyKey key = { board->hash(), player, remainingDepth };
    if (strategy.find(key) == strategy.end()) {
        strategy[key] = { new Board(*nextBoard), piece, move, value };
    } else if (player == 1 && value > strategy[key].value) {
        // Delete the old board
        delete strategy[key].board;
        // std::cout << "Updating p1 strategy at piece (" << piece.x << ", " << piece.y << ") and move (" << move.dx << ", " << move.dy << ") because value " << value << " > " << strategy[key].value << std::endl;
        strategy[key] = { new Board(*nextBoard), piece, move, value };
    } else if (player == -1 && value < strategy[key].value) {
        // Delete the old board
        delete strategy[key].board;
        // std::cout << "Updating p2 strategy at piece (" << piece.x << ", " << piece.y << ") and move (" << move.dx << ", " << move.dy << ") because value " << value << " < " << strategy[key].value << std::endl;
        strategy[key] = { new Board(*nextBoard), piece, move, value };
    } else {
        // std::cout << "Not updating " << (player == 1 ? "p1" : "p2") << "strategy at piece (" << piece.x << ", " << piece.y << ") and move (" << move.dx << ", " << move.dy << ") because value " << value << (player == 1 ? " < " : " > ") << strategy[key].value << std::endl;
    }

}

std::vector<Board*> GameObserver::recoverStrategy(Board* board) {
    std::vector<Board*> strategyBoards;
    Board* currentBoard = board;
    strategyBoards.push_back(new Board(*currentBoard));
    int player = 1;
    // Find the minimum depth where board is in the strategy
    int remainingDepth = 0;
    for (remainingDepth = 0; remainingDepth < 100; remainingDepth++) {
        strategyKey key = { currentBoard->hash(), player, remainingDepth };
        if (strategy.find(key) != strategy.end()) {
            break;
        }
    }
    while (true) {
        strategyKey key = { currentBoard->hash(), player, remainingDepth };
        if (strategy.find(key) == strategy.end()) {
            // std::cout << "No strategy found for board: \n" << currentBoard->toString() << "\n\n" << std::endl;
            break;
        }
        strategyValue value = strategy[key];
        currentBoard = new Board(*value.board);
        // Check if this board is already in the strategy
        for (auto it = strategyBoards.begin(); it != strategyBoards.end(); it++) {
            if ((*it)->hash() == currentBoard->hash()) {
                // This would be an infinite loop
                std::cout << "Infinite loop detected in strategy" << std::endl;
                // Delete everything in the strategy and return an empty vector as a signal that the strategy is invalid
                for (auto it = strategyBoards.begin(); it != strategyBoards.end(); it++) {
                    delete *it;
                }
                return std::vector<Board*>();
            }
        }
        strategyBoards.push_back(currentBoard);
        player *= -1;
        remainingDepth--;
    }
    return strategyBoards;
}

void GameObserver::clearStrategy() {
    // Delete all the boards in the strategy
    for (auto it = strategy.begin(); it != strategy.end(); it++) {
        delete it->second.board;
    }
    strategy.clear();
}

minimaxResult iterativeMinimax(Board* board, int maxDepth, int maxTime, GameObserver* observer) {
    // Calls minimax with increasing odd depth starting at 3
    // Returns when value > 999 or depth == maxDepth
    minimaxResult result;
    int depth = 3;
    auto start = std::chrono::high_resolution_clock::now();
    auto endTime = start + std::chrono::milliseconds(maxTime);
    observer->setEndTime(endTime);
    while (depth <= maxDepth) {
        // std::cout << "Trying Depth: " << depth << std::endl;
        observer->clearStrategy();  // TODO: Does this not work with transposition table?
        result = minimax(board, depth, observer);
        if (result.value > 999 || depth == maxDepth) {
            // std::cout << "Exiting iterative minimax at depth " << depth << std::endl;
            break;
        }
        depth += 2;
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        if (duration > maxTime) {
            break;
        }
    }
    return result;
}

minimaxResult minimax(Board* board, int maxDepth, GameObserver* observer) {
    float bestValue = -INFINITY;
    std::vector<successor> successors = board->getSuccessors(1);
    if (successors.size() == 0) {
        return { NAN, NULL };
    }
    successor bestSuccessor = successors[0];

    // Sort the successors by their evaluation
    std::sort(successors.begin(), successors.end(), [observer](successor a, successor b) {
        return observer->evaluate(a.board, 1) > observer->evaluate(b.board, 1);
    });

    float alpha = -INFINITY;
    float beta = INFINITY;
    for (successor s : successors) {
        Board* b = s.board;
        float v = minimax_step(b, maxDepth - 1, -1, alpha, beta, observer);
        if (v > bestValue) {
            bestValue = v;
            bestSuccessor = s;
            alpha = std::max(alpha, v);
        }
        // observer->updateStrategy(board, s.board, 1, s.piece, s.move, v, maxDepth);
        if (beta <= alpha) {
            observer->pruneEvents++;
            break;
        }
        // if (bestValue > 999) {  // Early exit once it has found any solution
        //     break;
        // }
    }
    observer->updateStrategy(board, bestSuccessor.board, 1, bestSuccessor.piece, bestSuccessor.move, bestValue, maxDepth);
    minimaxResult result = { 
        bestValue,
        observer
    };
    // Now we delete all the boards
    for (successor s : successors) {
        delete s.board;
    }
    return result;
}

float minimax_step(Board* board, int remainingDepth, int player, float alpha, float beta, GameObserver* observer) {
    observer->nodesExpanded++;
    // bool isTestBoard = board->hash() == (size_t) 14005469065154668806;
    // if (isTestBoard) {
    //     std::cout << "Test board" << std::endl;
    // }
    int terminalValue = board->getTerminalValue();
    if (terminalValue == 1) {
        return (remainingDepth + 1) * 10000;
    } else if (terminalValue == -1) {
        return -(remainingDepth + 1) * 10000;
    }
    std::vector<successor> successors = board->getSuccessors(player);
    if (successors.size() == 0) {
        // Then this is a win for the other player
        return (remainingDepth + 1) * 10000 * -player;
    }
    if (remainingDepth == 0 || observer->shouldExit()) {
        for (successor s : successors) {
            delete s.board;
        }
        return observer->utility(board);
    }

    // Sort the successors by their evaluation
    std::sort(successors.begin(), successors.end(), [player, observer](successor a, successor b) {
        if (player == 1) {
            return observer->evaluate(a.board, player) > observer->evaluate(b.board, player);
        } else {
            return observer->evaluate(a.board, player) < observer->evaluate(b.board, player);
        }
    });

    float bestValue = player == 1 ? -INFINITY : INFINITY;
    successor bestSuccessor = { NULL, {0, 0}, {0, 0} };
    for (successor s : successors) {
        Board* b = s.board;
        // Check the transposition table
        float transpositionValue = observer->getTranspositionValue(b, remainingDepth, player);
        float v;
        if (!std::isnan(transpositionValue)) {
            v = transpositionValue;
        } else {
            v = minimax_step(b, remainingDepth - 1, -player, alpha, beta, observer);
            observer->storeTranspositionValue(b, remainingDepth, player, v);
        }
        observer->updateStrategy(board, s.board, player, s.piece, s.move, v, remainingDepth);
        // if (isTestBoard) {
        //     std::cout << s.board->toString() << "Value: " << v << "\n" << std::endl;
        // }

        if (player == 1 && v > bestValue) {
            bestValue = v;
            bestSuccessor = s;
            alpha = std::max(alpha, v);
        } else if (player == -1 && v < bestValue) {
            bestValue = v;
            bestSuccessor = s;
            beta = std::min(beta, v);
        }
        if (beta <= alpha) {
            observer->pruneEvents++;
            break;
        }
        if (bestValue > 9999 && player == 1) {  // Early exit once it has found any solution
            break;
        }
    }

    // std::cout << "Finished minimax step at depth " << remainingDepth << " with value " << bestValue << std::endl;
    observer->updateStrategy(board, bestSuccessor.board, player, bestSuccessor.piece, bestSuccessor.move, bestValue, remainingDepth);
    // delete bestSuccessor.board;
    // std::cout << std::endl;

    // Now we delete all the boards
    for (successor s : successors) {
        delete s.board;
    }
    // if (bestValue > 999) {
    //     std::cout << "Returning win at depth " << remainingDepth << std::endl;
    // }
    return bestValue;
}

void getOptimalContinuation(std::string boardFile, std::string outFile, int maxDepth, int maxTime) {
    // Calls iterative minimax on the board and outputs the recovered strategy to a file
    Board* board = Board::readFromFile(boardFile);
    GameObserver observer = GameObserver();
    minimaxResult result = iterativeMinimax(board, maxDepth, maxTime, &observer);
    std::vector<Board*> strategy = observer.recoverStrategy(board);
    std::ofstream out(outFile);
    for (Board* b : strategy) {
        out << b->toString() << std::endl;
        delete b;
    }
    out.close();
    // std::cout << "Total nodes expanded: " << observer.nodesExpanded << std::endl;
    // std::cout << "Evaluation cache hits: " << observer.evaluationCacheHits << std::endl;
    // std::cout << "Utility cache hits: " << observer.utilityCacheHits << std::endl;
    // std::cout << "Terminal cache hits: " << observer.terminalCacheHits << std::endl;
    // std::cout << "Transposition cache hits: " << observer.transpositionTableHits << std::endl;
    // std::cout << "Prune events: " << observer.pruneEvents << std::endl;
    delete board;
}

std::string getOptimalContinuationFromString(std::string inBoard, int maxDepth, int maxTime) {
    // Calls iterative minimax on the board and outputs the recovered strategy to a file
    Board* board = Board::fromString(inBoard);
    GameObserver observer = GameObserver();
    minimaxResult result = iterativeMinimax(board, maxDepth, maxTime, &observer);
    std::vector<Board*> strategy = observer.recoverStrategy(board);
    std::string out = "";
    for (Board* b : strategy) {
        out += b->toString() + "---\n";
        delete b;
    }
    delete board;
    return out;
}

int getOptimalWinLength(std::string boardFile, int maxDepth, int maxTime) {
    // Calls iterative minimax on the board and outputs the recovered strategy to a file
    Board* board = Board::readFromFile(boardFile);
    GameObserver observer = GameObserver();
    minimaxResult result = iterativeMinimax(board, maxDepth, maxTime, &observer);
    if (result.value < 999) {
        return -1;
    }
    std::vector<Board*> strategy = observer.recoverStrategy(board);
    int strategyLength = strategy.size() - 1;
    for (Board* b : strategy) {
        delete b;
    }
    delete board;
    return strategyLength;
}

int getOptimalWinLengthFromString(std::string inBoard, int maxDepth, int maxTime) {
    // Calls iterative minimax on the board and outputs the recovered strategy to a file
    Board* board = Board::fromString(inBoard);
    GameObserver observer = GameObserver();
    minimaxResult result = iterativeMinimax(board, maxDepth, maxTime, &observer);
    if (result.value < 999) {
        return -1;
    }
    std::vector<Board*> strategy = observer.recoverStrategy(board);
    int strategyLength = (int) strategy.size() - 1;
    for (Board* b : strategy) {
        delete b;
    }
    delete board;
    return strategyLength;
}

bool isWinning(std::string boardFile, int maxDepth, int maxTime) {
    // Returns true if the value of minimax is > 999
    Board* board = Board::readFromFile(boardFile);
    GameObserver observer = GameObserver();
    minimaxResult result = iterativeMinimax(board, maxDepth, maxTime, &observer);
    delete board;
    return result.value > 999;
}

bool isWinningFromString(std::string inBoard, int maxDepth, int maxTime) {
    // Returns true if the value of minimax is > 999
    Board* board = Board::fromString(inBoard);
    GameObserver observer = GameObserver();
    minimaxResult result = iterativeMinimax(board, maxDepth, maxTime, &observer);
    delete board;
    return result.value > 999;
}