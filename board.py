"""
Represents a board for checkers and allows for consistent manipulation of the board so different implementations can be used.
"""

from typing import Tuple, Dict, List, Optional, Callable
from abc import ABC, abstractmethod
import random
import json
from datetime import datetime

class Board(ABC):
    def __init__(self, width=8, height=8):
        self.width = width
        self.height = height

    @staticmethod
    @abstractmethod
    def read_from_file(filename: str) -> "Board":
        pass

    @abstractmethod
    def display(self) -> None:
        pass

    @abstractmethod
    def invert(self) -> "Board":
        """
        Returns the board from the other perspective
        """
        pass

    @abstractmethod
    def get_successors(self, player: int) -> List["Board"]:
        """
        Returns successors of the current board as seen by red.
        Successors consist of two types of moves:
        1. Single move: A single piece moves one square diagonally forward.
        2. Jump: A single piece jumps over an opponent's piece and lands in a square diagonally forward.

        Jumps are forced so if there is at least one successor in the jump set, we only return the jump set.
        Multi-jumps are also mandatory so if there is another jump after a jump we add that to the move.
            If there are multiple paths for a multi-jump, we include both of them. (Hint for me later. Just always do a recursive dfs and add all end states.)
        """
        pass

    @abstractmethod
    def is_end(self) -> int:
        """
        Returns 1 if red won, -1 if black won, and 0 if the game is not over.
        """
        pass

    @abstractmethod
    def utility(self) -> float:
        """
        Called when at maximum depth. Returns a good estimate of the expected value of the current board.
        """
        pass
        
    @abstractmethod
    def evaluate(self) -> float:
        """
        Used to sort the successors of a board. Returns a rough estimate of the value of the board.
        """
        pass

    @abstractmethod
    def __hash__(self) -> int:
        """
        Returns a hash of the board that is equal for boards with the same pieces in the same positions
        """
        pass

char_to_int = {
    "r": 1,
    "R": 2,
    "b": -1,
    "B": -2,
}
int_to_char = { v: k for k, v in char_to_int.items() }
int_to_char[0] = "."

class SparseBoard(Board):
    """
    For a sparse board, we only store non-empty squares.
    For square values 1 represents regular red, 2 represents king red, -1 represents regular black, and -2 represents king black.
    """
    def __init__(self, width=8, height=8):
        super().__init__(width, height)
        self.sparse_board: Dict[Tuple[int, int], int] = {}
        self.char_to_int = char_to_int
        self.int_to_char = int_to_char

    @staticmethod
    def read_from_file(filename: str) -> "SparseBoard":
        board = SparseBoard()
        with open(filename) as f:
            for y, line in enumerate(f):
                for x, char in enumerate(line.rstrip()):
                    if char != ".":
                        board.sparse_board[(x, y)] = board.char_to_int[char]
        return board
    
    @staticmethod
    def read_from_string(string: str) -> "SparseBoard":
        board = SparseBoard()
        for y, line in enumerate(string.splitlines()):
            for x, char in enumerate(line.rstrip()):
                if char != ".":
                    board.sparse_board[(x, y)] = board.char_to_int[char]
        return board
    
    def display(self) -> None:
        for y in range(self.height):
            for x in range(self.width):
                print(self.int_to_char.get(self.sparse_board.get((x, y), 0)), end="")
            print("")

    def invert(self) -> "SparseBoard":
        new_board = SparseBoard(self.width, self.height)
        for (x, y), val in self.sparse_board.items():
            new_board.sparse_board[(x, self.height - y - 1)] = -val
        return new_board
    
    def _copy(self) -> "SparseBoard":
        new_board = SparseBoard(self.width, self.height)
        new_board.sparse_board = self.sparse_board.copy()
        return new_board
    
    def _perform_move(self, x: int, y: int, move: Tuple[int, int]):
        """
        Copies the board and performs a single move on it
        """
        new_board = self._copy()
        val = new_board.sparse_board[(x, y)]
        del new_board.sparse_board[(x, y)]
        new_val = val
        if abs(val) == 1:
            king_row = 0 if val > 0 else self.height - 1
            new_val = val*2 if y + move[1] == king_row else val
        new_board.sparse_board[(x + move[0], y + move[1])] = new_val
        return new_board
    
    def _perform_jump(self, x: int, y: int, move: Tuple[int, int]):
        """
        Copies the board and performs a single jump on it
        """
        new_board = self._copy()
        val = new_board.sparse_board[(x, y)]
        del new_board.sparse_board[(x, y)]
        del new_board.sparse_board[(x + move[0], y + move[1])]
        new_val = val
        if abs(val) == 1:
            king_row = 0 if val > 0 else self.height - 1
            new_val = val*2 if y + move[1] * 2 == king_row else val
        new_board.sparse_board[(x + move[0] * 2, y + move[1] * 2)] = new_val
        return new_board
    
    def is_end(self) -> int:
        """
        Returns 1 if red wins, -1 if black wins, 0 if the game is not over
        TODO: Handle case where there are no valid moves for a player
        """
        red_pieces = 0
        black_pieces = 0
        for val in self.sparse_board.values():
            if val > 0:
                red_pieces += 1
            elif val < 0:
                black_pieces += 1
        if red_pieces == 0:
            return float("-inf")
        elif black_pieces == 0:
            return float("inf")
        else:
            return 0

    def evaluate(self) -> float:
        """
        Used to sort the successors of a board. Returns a rough estimate of the value of the board for red.
        """
        total_score = 0
        total_pieces = 0
        for val in self.sparse_board.values():
            total_score += val
            total_pieces += 1
        return total_score / total_pieces
        
    def utility(self) -> float:
        """
        Called when at maximum depth. Returns a good estimate of the expected value of the current board for red.

        Strategy:
        1. Count the number of pieces on the board
        2. Move pieces towards each other
        3. Move pieces towards the center
        """
        return self.evaluate()

    def __hash__(self) -> int:
        # return hash(frozenset(self.sparse_board.items()))  # Collision: hash(frozenset({((4, 5), -1), ((5, 6), 1)})) == hash(frozenset({((4, 5), -2), ((5, 6), 1)}))
        # return hash(tuple(sorted(self.sparse_board.items())))  # Collision: hash((((4, 5), -1), ((5, 6), 1))) == hash((((4, 5), -2), ((5, 6), 1)))
        return hash(json.dumps(tuple(sorted(self.sparse_board.items())), sort_keys=True))
    
    def __str__(self) -> str:
        board = ""
        for y in range(self.height):
            for x in range(self.width):
                board += self.int_to_char.get(self.sparse_board.get((x, y), 0))
            board += "\n"
        return board
    
    def __repr__(self) -> str:
        return self.__str__()
    
    def _follow_jump(self, x: int, y: int, move: Tuple[int, int], is_king: bool, player: int) -> List["SparseBoard"]:
        """
        Recursively follows a multi-jump until there are no more jumps then returns that board
        """
        successors = []

        # new_board = self._copy()
        # # We need to remove the piece we jumped over and the piece we jumped from and then add the piece we jumped to
        # val = self.sparse_board[(x, y)]
        # del new_board.sparse_board[(x, y)]
        # del new_board.sparse_board[(x + move[0], y + move[1])]
        # new_board.sparse_board[(x + move[0] * 2, y + move[1] * 2)] = val
        new_board = self._perform_jump(x, y, move)
        new_x, new_y = x + move[0] * 2, y + move[1] * 2
        
        # Now we need to check if this piece can make any more jumps. If not, then we can return [new_board]
        # Otherwise we recursively call this function on all possible moves
        potential_moves = [(1, -player), (-1, -player)]
        if is_king:
            potential_moves.extend([(1, player), (-1, player)])

        for move in potential_moves:
            move_location = (new_x + move[0], new_y + move[1])
            jump_location = (new_x + move[0] * 2, new_y + move[1] * 2)
            move_occupation = self.sparse_board.get(move_location, 0) if 0 <= move_location[0] < self.width and 0 <= move_location[1] < self.height else None
            jump_occupation = self.sparse_board.get(jump_location, 0) if 0 <= jump_location[0] < self.width and 0 <= jump_location[1] < self.height else None
            if move_occupation is not None and move_occupation * player < 0 and jump_occupation == 0:
                # Then we must make another jump
                successors.extend(new_board._follow_jump(new_x, new_y, move, is_king, player))
        if len(successors) == 0:
            successors.append(new_board)

        return successors
    
    def get_successors(self, player: int = 1) -> List["SparseBoard"]:
        """
        Returns successors of the current board as seen by red.

        If player == 1, then we return successors as seen by red
        If player == -1, then we return successors as seen by black
        """
        move_successors = []
        jump_successors = []
        for (x, y), val in self.sparse_board.items():
            if val * player > 0:
                # Then this is a piece we can move
                potential_moves = [(1, -player), (-1, -player)]  # The direction we can move in y is the opposite of the player we are
                if val * player == 2:
                    # This is a king piece
                    potential_moves.extend([(1, player), (-1, player)])

                for move in potential_moves:
                    move_location = (x + move[0], y + move[1])
                    jump_location = (x + move[0] * 2, y + move[1] * 2)
                    move_occupation = self.sparse_board.get(move_location, 0) if 0 <= move_location[0] < self.width and 0 <= move_location[1] < self.height else None
                    jump_occupation = self.sparse_board.get(jump_location, 0) if 0 <= jump_location[0] < self.width and 0 <= jump_location[1] < self.height else None
                    if move_occupation is None:
                        # We are trying to move outside the board
                        continue
                    elif move_occupation != 0:
                        # Then we will attempt a jump
                        if move_occupation * player < 0 and jump_occupation == 0:
                            # Then we are making a jump
                            jump_successors.extend(self._follow_jump(x, y, move, is_king=(val * player == 2), player=player))
                        else:
                            # Then we are blocked
                            pass
                    elif move_occupation == 0:
                        # Then this is a valid move
                        # new_board = self._copy()
                        # new_board.sparse_board[move_location] = val
                        # del new_board.sparse_board[(x, y)]
                        new_board = self._perform_move(x, y, move)
                        move_successors.append(new_board)
                    else:
                        # Ya know, not sure how we got here
                        raise Exception("Ya know, not sure how we got here")
        
        successors = jump_successors if len(jump_successors) > 0 else move_successors
        unique_successors = []
        seen_hashes = set()
        for successor in successors:
            if successor.__hash__() not in seen_hashes:
                unique_successors.append(successor)
                seen_hashes.add(successor.__hash__())
        return unique_successors
    
class BoardGenerator:
    """
    Generates random boards by taking inverse successors of end states.
    Once we have generated inverse successors we can run minimax on them to check which ones are still definitely solvable in reasonable depth
    """
    def _generate_seed_board(self) -> Board:
        """
        Places some number of random red pieces on the bored and runs one iteration of inverse jump successors to get a random board that is winnable in one move
        """
        board = SparseBoard()
        num_pieces = random.randint(1, 5)
        for _ in range(num_pieces):
            value = random.choice([1, 2])
            board.sparse_board[(random.randint(0, 7), random.randint(0, 7))] = value
        self.possible_seeds = self._get_inverse_jump_successors(board)
        return random.choice(self.possible_seeds)
    
    def _validate_pre_successor(self, board: Board, pre_successor: Board, player: int):
        """
        Some potential pre-successors we generate may not actually be valid because it is hard to check for all the logic.
        To get around this, we check if the pre-successor is a valid successor of the board we are trying to generate pre-successors for.
        We also need to check if the pre-successor is a terminal state because if it is the it cant come before the board we are trying to generate pre-successors for
        """
        if pre_successor.is_end() != 0:
            return False
        successors = pre_successor.get_successors(player=player)
        for successor in successors:
            if hash(successor) == hash(board):
                return True
        return False
    
    def _perform_inverse_move(self, board: SparseBoard, x: int, y: int, move: Tuple[int, int]) -> List[Board]:
        """
        Performs an inverse move on a board
        To do this, we place the piece of the same value at position+move unless the current y is 0 or 7 and abs(val) == 1 in which case we add two board
        one where val = val and one where val = 2*val
        """
        pre_successors = []
        val = board.sparse_board[(x, y)]
        king_row = 0 if val > 0 else 7
        pre_location = (x + move[0], y + move[1])
        if y == king_row and abs(val) == 1:
            for multiple in [1, 2]:
                new_board = board._copy()
                del new_board.sparse_board[(x, y)]
                new_board.sparse_board[pre_location] = val * multiple
                pre_successors.append(new_board)
        else:
            new_board = board._copy()
            del new_board.sparse_board[(x, y)]
            new_board.sparse_board[pre_location] = val
            pre_successors.append(new_board)
        return pre_successors
    
    def _get_inverse_jump_successors(self, board: SparseBoard, x: int, y: int, move: Tuple[int, int]) -> List[Board]:
        """
        Recursively generates inverse jump successors and multi-jump successors
        """
        def _get_mutli_jump_continuation(board: SparseBoard, pre_location: Tuple[int, int], player: int) -> List[Board]:
            pre_successors = []
            x, y = pre_location
            potential_moves = [(1, player), (-1, player)]  # The direction we can move in y is the same as the player we are
            if val * player == 2:
                # This is a king piece
                potential_moves.extend([(1, -player), (-1, -player)])
            for move in potential_moves:
                moved_location = (x + move[0], y + move[1])
                jumped_location = (x + move[0] * 2, y + move[1] * 2)
                moved_occupation = board.sparse_board.get(moved_location, 0) if 0 <= moved_location[0] < board.width and 0 <= moved_location[1] < board.height else None
                jumped_occupation = board.sparse_board.get(jumped_location, 0) if 0 <= jumped_location[0] < board.width and 0 <= jumped_location[1] < board.height else None
                if moved_occupation == 0 and jumped_occupation == 0:
                    # Then this could have been a jump
                    pre_successors.extend(self._get_inverse_jump_successors(board, x, y, move))
            return pre_successors

        pre_successors = []
        val = board.sparse_board[(x, y)]
        player = 1 if val > 0 else -1
        pre_location = (x + move[0] * 2, y + move[1] * 2)
        king_row = 0 if val > 0 else 7
        if y == king_row and abs(val) == 1:
            for multiple in [1, 2]:
                for opponent_multiple in [1, 2]:
                    new_board = board._copy()
                    # This time we need to add the piece we jumped over as well as the piece we are moving
                    del new_board.sparse_board[(x, y)]
                    new_board.sparse_board[pre_location] = player * multiple
                    new_board.sparse_board[(x + move[0], y + move[1])] = -1 * player * opponent_multiple
                    # At this point, this could have been the pre-successor, but it also could have been a multi-jump
                    pre_successors.extend(_get_mutli_jump_continuation(new_board, pre_location, player))
                    pre_successors.append(new_board)
        else:
            for opponent_multiple in [1, 2]:
                new_board = board._copy()
                # This time we need to add the piece we jumped over as well as the piece we are moving
                del new_board.sparse_board[(x, y)]
                new_board.sparse_board[pre_location] = val
                new_board.sparse_board[(x + move[0], y + move[1])] = -1 * player * opponent_multiple
                # At this point, this could have been the pre-successor, but it also could have been a multi-jump
                pre_successors.extend(_get_mutli_jump_continuation(new_board, pre_location, player))
                pre_successors.append(new_board)
        return pre_successors
    
    def get_inverse_successors(self, board: SparseBoard, player: int, successor_limit=None) -> List[Board]:
        """
        Returns the inverse successors of a board as if player has just made a move
        """
        pre_successors = []
        for (x, y), val in board.sparse_board.items():
            # We need to find all pieces that could have just made a move
            if val * player < 0:
                continue
            potential_moves = [(1, player), (-1, player)]  # The direction we can move in y is the same as the player we are
            if val * player == 2:
                # This is a king piece
                potential_moves.extend([(1, -player), (-1, -player)])
            for move in potential_moves:
                moved_location = (x + move[0], y + move[1])
                jumped_location = (x + move[0] * 2, y + move[1] * 2)
                moved_occupation = board.sparse_board.get(moved_location, 0) if 0 <= moved_location[0] < board.width and 0 <= moved_location[1] < board.height else None
                jumped_occupation = board.sparse_board.get(jumped_location, 0) if 0 <= jumped_location[0] < board.width and 0 <= jumped_location[1] < board.height else None
                # If moved_location is not occupied, then we could have moved from there
                # If jumped_location is not occupied and moved_location is not occupied, we could have jumped from there
                if moved_occupation == 0:
                    # Then we could have moved from here
                    pre_successors.extend(self._perform_inverse_move(board, x, y, move))
                    if jumped_occupation == 0:
                        # Then we could have jumped from here
                        pre_successors.extend(self._get_inverse_jump_successors(board, x, y, move))
                if successor_limit is not None and len(pre_successors) >= successor_limit:
                    break
            if successor_limit is not None and len(pre_successors) >= successor_limit:
                break
        # Now we have generated all the pre-successors, we need to remove duplicates and invalid boards
        unique_pre_successors = []
        unique_hashes = set()
        for pre_successor in pre_successors:
            if hash(pre_successor) not in unique_hashes:
                unique_pre_successors.append(pre_successor)
                unique_hashes.add(hash(pre_successor))

        valid_pre_successors = []
        for pre_successor in unique_pre_successors:
            if self._validate_pre_successor(board, pre_successor, player):
                valid_pre_successors.append(pre_successor)

        return valid_pre_successors
    
    def get_winnable_inverse_boards(self, board: SparseBoard, player: int, rating_method: Callable[[SparseBoard], bool], return_n_winnable=5, prioritize_small_boards=False, force_take=True, successor_limit=None) -> List[Board]:
        """
        Assumes board is winnable for player 1 if it is player 1's turn
        Performs two steps of inverse successor generation and returns the boards that are winnable from this new state
        """
        print("Getting inverse successors")
        first_step_successors = self.get_inverse_successors(board, player, successor_limit=successor_limit)
        second_step_successors = []
        random.shuffle(first_step_successors)
        for i, first_step_successor in enumerate(first_step_successors[:30]):
            print(f"Getting inverse successors for {i+1}/{len(first_step_successors)}")
            second_step_successors.extend(self.get_inverse_successors(first_step_successor, -1*player, successor_limit=successor_limit))
        # Now we de-duplicate
        unique_second_step_successors = []
        unique_hashes = set()
        for second_step_successor in second_step_successors:
            if hash(second_step_successor) not in unique_hashes:
                unique_second_step_successors.append(second_step_successor)
                unique_hashes.add(hash(second_step_successor))
        if force_take:
            # Then we want to filter out the boards the don't have more pieces than the original board
            unique_second_step_successors = [second_step_successor for second_step_successor in unique_second_step_successors if len(second_step_successor.sparse_board) > len(board.sparse_board)]
        random.shuffle(unique_second_step_successors)
        if prioritize_small_boards:
            # Then we want to start by searching boards with less pieces
            unique_second_step_successors.sort(key=lambda x: len(x.sparse_board))
        # Now we filter out the boards that are not winnable
        # If the rating_method called on the board returns True, then the board is winnable
        print("Got inverse successors. Checking winnable boards")
        winnable_boards = []
        for i, second_step_successor in enumerate(unique_second_step_successors):
            # Get the time in the format HH:MM:SS
            time = datetime.now().strftime("%H:%M:%S")
            print(f"{time}: Checking board {i+1}/{len(unique_second_step_successors)}. Have found {len(winnable_boards)} winnable boards so far out of {return_n_winnable} requested.")
            second_step_successor.display()
            rating = rating_method(second_step_successor)
            if rating:
                winnable_boards.append(second_step_successor)
            else:
                # print("Board is not winnable:")
                # second_step_successor.display()
                pass
            if len(winnable_boards) >= return_n_winnable:
                print("Found enough winnable boards. Stopping search.")
                break
        return winnable_boards
    
class Stack:
    """
    Implements a stack with constant time lookup
    """
    def __init__(self):
        self.stack = []
        self.lookup = {}

    def push(self, item):
        self.stack.append(item)
        if item in self.lookup:
            self.lookup[item] += 1
        else:
            self.lookup[item] = 1

    def pop(self):
        item = self.stack.pop()
        self.lookup[item] -= 1
        if self.lookup[item] == 0:
            del self.lookup[item]
        return item

    def __contains__(self, item):
        return item in self.lookup

class ExploreState:
    def __init__(self, use_evaluation_cache: bool = True, use_utility_cache: bool = True, use_score_cache: bool = True, use_pruning: bool = True, use_cycle_detection: bool = True):
        # Caches map from a board hash to the value of the board
        self.evaluation_cache: Dict[int, float] = {}
        self.utility_cache: Dict[int, float] = {}
        # self.terminal_value_cache: Dict[int, float] = {}
        self.score_cache: Dict[int, Tuple[float, int, int]] = {}  # The score hash maps from a board to the score, the depth that this minimax was called at, and the player that called this minimax

        self.use_evaluation_cache = use_evaluation_cache
        self.use_utility_cache = use_utility_cache
        self.use_score_cache = use_score_cache
        self.use_pruning = use_pruning
        self.use_cycle_detection = use_cycle_detection

        self.strategy: Dict[Tuple[Board, int], Tuple[Board, float]] = {}  # Maps from a state and player to the best state to move to

        self.successor_stack = Stack()
        self.paths: List[Tuple[str, int, List[Board]]] = []

        self.pruned_count = 0
        self.explored_count = 0
        self.cache_hits = 0

    def push_successor(self, successor: Board, player: int) -> None:
        self.successor_stack.push((successor, player))

    def pop_successor(self) -> Tuple[Board, int]:
        return self.successor_stack.pop()
    
    def save_successor_stack(self, name: str, depth: int):
        self.paths.append((name, depth, list(self.successor_stack.stack)))

    def save_successor_stacks(self):
        """
        Formats and writes the saved successor stacks to a file formatted like so
        *****************
        * Result: Name
        * Depth: depth
        Init:        D: 1      D: 2
        P: r         P: b      P: r
        ......b.  |  ......b.  ......b.
        .......r  |  .......r  .......r
        ....r.r.  |  ....r.r.  ....r.r.
        ........  |  ........  ........
        ........  |  ........  ........
        ........  |  ........  ........
        .....r..  |  .....r..  .....r..
        ........  |  ........  ........
        """
        player_to_char = {1: "r", -1: "b"}
        with open("successor_stacks.txt", "w") as f:
            # Print paths with low depth first
            self.paths.sort(key=lambda x: x[1], reverse=True)
            for name, depth, stack in self.paths:
                f.write(f"*****************\n")
                f.write(f"* Result: {name}\n")
                f.write(f"* Depth: {depth}\n")
                init = stack[0]
                continuation = stack[1:]
                depth_row = "Init:        "
                player_row = f"P: {player_to_char[init[1]]}         "
                board_lines = str(init[0]).split("\n")
                lines = [""]*len(board_lines)
                for i, line in enumerate(board_lines):
                    lines[i] += f"{line}  |  "
                
                move_num = 1
                for board, player in continuation:
                    depth_row += f"D: {str(move_num).ljust(7, ' ')}"
                    player_row += f"P: {player_to_char[player]}      "
                    board_lines = str(board).split("\n")
                    for i, line in enumerate(board_lines):
                        lines[i] += f"{line}  "
                    move_num += 1
                f.write(f"{depth_row}\n")
                f.write(f"{player_row}\n")
                for line in lines:
                    f.write(f"{line}\n")
                f.write(f"\n")

    def successor_in_current_path(self, successor: Board, player: int) -> bool:
        """
        Useful for cycle detection
        """
        return (successor, player) in self.successor_stack

    def update_strategy(self, current_board: Board, board_hash: int, successor: Board, player: int, score: float):
        """
        We want to update strategy if this is the best move we have seen for the player
        If the player is 1, this means that we want to maximize the score
        If the player is -1, this means that we want to minimize the score
        """
        if (board_hash, player) not in self.strategy or self.strategy[(board_hash, player)][1] * player < score * player:
            self.strategy[(board_hash, player)] = (successor, score)

    def get_terminal_value_and_succ(self, board: Board, player: int) -> Tuple[float, List[Board]]:
        """
        Returns the terminal value of the board and the list of successors.
        """
        value = board.is_end()
        if value > 0:
            # print("Found player 1 win")
            return float("inf"), []
        elif value < 0:
            # print("Found player -1 win")
            return float("-inf"), []
        
        successors = board.get_successors(player)
        if len(successors) == 0:
            # Then the player has no valid moves so the other player wins
            # print(f"Found player {-1*player} win (no valid moves)")
            return player * float("-inf"), []
        
        return 0, successors
    
    def get_cached_score(self, board: Board, board_hash: int, depth: int, player: int) -> Optional[float]:
        """
        If we have already calculated the score for this board at the same or lower depth for this player, there is no need to recalculate it.
        However, if the depth is higher than the depth we have cached or if the player is different, we need to recalculate it.
        """
        if not self.use_score_cache:
            return None
        if board_hash in self.score_cache:
            cached_score, cached_depth, cached_player = self.score_cache[board_hash]
            if depth <= cached_depth and player == cached_player:
                self.cache_hits += 1
                return cached_score
        return None
    
    def cache_score(self, board: Board, board_hash: int, depth: int, player: int, score: float):
        self.score_cache[board_hash] = (score, depth, player)
    
    def get_utility(self, board: Board, board_hash: int) -> float:
        """
        If the utility for this board has already been calculated, return that
        Otherwise calculate the utility, add it to the cache, and return it
        """
        if self.use_utility_cache and board_hash in self.utility_cache:
            return self.utility_cache[board_hash]
        else:
            utility = board.utility()
            self.utility_cache[board_hash] = utility
            return utility
        
    def get_evaluation(self, board: Board, board_hash: int) -> float:
        """
        The order of what to use goes utility, evaluation, compute evaluation.
        """
        if self.use_evaluation_cache and board_hash in self.evaluation_cache:
            return self.evaluation_cache[board_hash]
        else:
            evaluation = board.evaluate()
            self.evaluation_cache[board_hash] = evaluation
            return evaluation
        
    def recover_best_path(self, board: Board, player: int) -> List[Board]:
        """
        Recovers the best path from the strategy dictionary.
        """
        path = [board]
        in_path = set()
        while (hash(board), player) in self.strategy:
            board, _ = self.strategy[(hash(board), player)]
            if hash(board) in in_path:
                print("Cycle detected")
                break
            path.append(board)
            in_path.add(hash(board))
            player *= -1
        return path
    
    

        