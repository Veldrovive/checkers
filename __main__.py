import argparse

from board import SparseBoard
from extern import getBoardOptimalContinuation

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--inputfile",
        type=str,
        required=True,
        help="The input file that contains the puzzles."
    )
    parser.add_argument(
        "--outputfile",
        type=str,
        required=True,
        help="The output file that contains the solution."
    )
    args = parser.parse_args()

    board = SparseBoard.read_from_file(args.inputfile)
    print("Read board: ")
    board.display()
    solution = getBoardOptimalContinuation(board, 100, 110)
    print(f"Saving solution of length {len(solution) - 1} to {args.outputfile}...")
    with open(args.outputfile, "w") as f:
        for board in solution:
            f.write(str(board) + "\n")