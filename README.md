# DEADREAD

[current benchmark](https://pilot1782.org/benchmark)

## Python files:
- `main.py`
- `vector_methods.py`
- `vectorize_book.py`
- `book_stats.py`

## C++ files
- `speedtest.cpp` (`speedtest`)

## Usage

> [!NOTE] 
> For non-nvidia GPUs:
> The code can still be ran, although it will be significantly slower. You'll need to install a version of pytorch that is CPU-only, which can be installed as shown below.
 > ```
> pip uninstall torch
> pip install torch --index-url https://download.pytorch.org/whl/cpu
> ```