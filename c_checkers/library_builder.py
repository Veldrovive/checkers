import pathlib
import zlib
import pickle
import pickletools
import base64
import sys
import contextlib

save_file = './library.bin'

class Resource:

    """Manager for resources that would normally be held externally."""

    WIDTH = 76
    __CACHE = None
    DATA = b''''''

    @classmethod
    def package(cls, *paths):
        """Creates a resource string to be copied into the class."""
        cls.__generate_data(paths, {})

    @classmethod
    def add(cls, *paths):
        """Include paths in the pre-generated DATA block up above."""
        cls.__preload()
        cls.__generate_data(paths, cls.__CACHE.copy())

    @classmethod
    def __generate_data(cls, paths, buffer):
        """Load paths into buffer and output DATA code for the class."""
        for path in map(pathlib.Path, paths):
            if not path.is_file():
                raise ValueError('{!r} is not a file'.format(path))
            key = path.name
            if key in buffer:
                raise KeyError('{!r} has already been included'.format(key))
            with path.open('rb') as file:
                buffer[key] = file.read()
        pickled = pickle.dumps(buffer, pickle.HIGHEST_PROTOCOL)
        optimized = pickletools.optimize(pickled)
        compressed = zlib.compress(optimized, zlib.Z_BEST_COMPRESSION)
        encoded = base64.b85encode(compressed)
        cls.__print("    DATA = b'''")
        for offset in range(0, len(encoded), cls.WIDTH):
            cls.__print("\\\n" + encoded[
                slice(offset, offset + cls.WIDTH)].decode('ascii'))
        cls.__print("'''")

    @staticmethod
    def __print(line):
        """Provides alternative printing interface for simplicity."""
        with open(save_file, 'a') as f:
            f.write(line)
            f.flush()
        sys.stdout.write(line)
        sys.stdout.flush()

    @classmethod
    @contextlib.contextmanager
    def load(cls, name, delete=True):
        """Dynamically loads resources and makes them usable while needed."""
        cls.__preload()
        if name not in cls.__CACHE:
            raise KeyError('{!r} cannot be found'.format(name))
        path = pathlib.Path(name)
        with path.open('wb') as file:
            file.write(cls.__CACHE[name])
        yield path
        if delete:
            path.unlink()

    @classmethod
    def __preload(cls):
        """Warm up the cache if it does not exist in a ready state yet."""
        if cls.__CACHE is None:
            decoded = base64.b85decode(cls.DATA)
            decompressed = zlib.decompress(decoded)
            cls.__CACHE = pickle.loads(decompressed)

    def __init__(self):
        """Creates an error explaining class was used improperly."""
        raise NotImplementedError('class was not designed for instantiation')

if __name__ == '__main__':
    Resource.package('libcheckers.so')