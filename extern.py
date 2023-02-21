import ctypes
from board import SparseBoard
import os

dir_path = os.path.dirname(os.path.realpath(__file__))

def with_lib():
    # Returns a ctypes.CDLL containing the library
    # This file has the shared library embedded in it in base64 format
    # Check the OS. We support MacOS and ubuntu
    import platform
    print("Getting library for platform: ", platform.system())
    if platform.system() == "Darwin":
        # We are on MacOS
        from mac_library import Resource
        with Resource.load("libcheckers.so") as lib_file:
            lib = ctypes.CDLL(os.path.join(dir_path, lib_file))
            lib.B_getOptimalContinuationFromString.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
            lib.B_getOptimalContinuationFromString.restype = ctypes.c_char_p
            return lib
    elif platform.system() == "Linux":
        # We are on Ubuntu
        from linux_library import Resource
        with Resource.load("libcheckers.so", delete=True) as lib_file:
            lib = ctypes.CDLL(os.path.join(dir_path, lib_file))
            lib.B_getOptimalContinuationFromString.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
            lib.B_getOptimalContinuationFromString.restype = ctypes.c_char_p
            return lib
    else:
        raise RuntimeError("Unsupported system: " + platform.system())

def getBoardOptimalContinuation(board: SparseBoard, maxDepth: int, maxTime: float):
    _lib = with_lib()
    board_string = str(board)
    # Create a c string for the output to be written to
    max_len = 10000
    out_string = ctypes.create_string_buffer(max_len)
    _lib.B_getOptimalContinuationFromString(board_string.encode(), out_string, ctypes.sizeof(out_string), maxDepth, int(maxTime*1000))
    # Decode the output string
    out_string = out_string.value.decode()
    # In order to separate the boards, we use a "---\n" separator so in order to recreate the boards we need to split on this
    out_string = out_string.split("---\n")
    # Remove the last element, which is just an empty string
    out_string.pop()
    # Convert the strings to SparseBoard objects
    out_boards = [SparseBoard.read_from_string(board_string) for board_string in out_string]
    return out_boards

if __name__ == "__main__":
    import time
    file = "./test_boards/d11_normal_1.txt"
    start_time = time.time()
    
    board = SparseBoard.read_from_file(file)
    print(board)
    print(f"Optimal continuation: {getBoardOptimalContinuation(board, 15, 40)}")

    print(f"Time taken: {time.time() - start_time}")
    

    
