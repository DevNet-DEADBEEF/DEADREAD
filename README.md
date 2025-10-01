# DEADREAD

[current benchmark](https://pilot1782.org/benchmark)

## Python files:
- `main.py`
- `vector_methods.py`
- `vectorize_book.py`
- `book_stats.py`

## C++ files
- `speedtest.cpp` (`speedtest`)

## Installation

> [!NOTE] 
> For non-nvidia GPUs:
> The code can still be ran, although it will be significantly slower. You'll need to install a version of pytorch that is CPU-only, which can be installed as shown below.
> ```
> pip uninstall torch
> pip install torch --index-url https://download.pytorch.org/whl/cpu
> ```
### Requirements:
- Linux (developed for Ubuntu)
- `Python 3.12`
- `make` (if using Makefile)
- `g++`


Install the dependancies for python:
```
pip install -r requirements.txt
```

Build `speedtest.cpp`:
```
make speedtest
```

## Usage

### Load data
Preprocesses all data for later query. Note that inproperly formatted files are ignored.
```
python3 main.py parse <file|folder>
```
> You can provide multiple files and folders, seperated with spaces.

### File stats
Displays the title, top 5 most frequent words (excluding any in the blacklist), and average sentance length of all processed files.
```
python3 main.py stats [<blacklist>]
```
> You can optionally provide a path to a blacklist csv of words to be ignored when determining stats.

### File search
Searches the data for the best matches to the input string. It will attempt to produce the top 5 closest matches that are from unique files, from `5*n` matches.
```
python3 main.py search <string>
```
> The string will include all characters after the search, including whitespace.