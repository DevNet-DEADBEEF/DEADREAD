import argparse
import os
import glob

parser = argparse.ArgumentParser(
    description="Analyze a Project Gutenberg ebook for average sentence length and top 5 words."
)
parser.add_argument(
    "command", help="Command to run", choices=["parse", "stats", "search"]
)
parser.add_argument(
    "param",
    help="Command parameters",
    nargs="*",
    default="",
)
parser.add_argument("--debug", action="store_true", help="Enable debug output.")
parser.add_argument(
    "-f", "--cachefile", type=str, default="cache.json", help="Path to the cache file."
)

args = parser.parse_args()

if args.debug:
    print("Debug mode is on.")
    print(f"Args: {args}")
command = args.command
param = args.param
cachefile = args.cachefile

match command:
    case "parse":
        if param == "":
            print("Error: 'parse' command requires a file path.")
            exit(1)
        
        # Run speedtest on all collected files
        os.system(f"./speedtest --silent {' '.join(param)}")

        # Collect all .txt files from parameters (handle both files and directories)
        txt_files = []
        for item in param:
            if os.path.isfile(item) and item.endswith('.txt'):
                txt_files.append(item)
            elif os.path.isdir(item):
                # Find all .txt files in the directory
                pattern = os.path.join(item, "*.txt")
                txt_files.extend(glob.glob(pattern))
            elif os.path.isfile(item):
                print(f"Warning: '{item}' is not a .txt file, skipping.")
            else:
                print(f"Warning: '{item}' not found, skipping.")
        
        if not txt_files:
            print("Error: No .txt files found to process.")
            exit(1)
        
        # Parse each file with vectorize_book
        if not os.fork():
            import vectorize_book as vb
            for file in txt_files:
                vb.parse_book(file, cachefile, debug=args.debug)
    case "stats":
        if param == "":
            __import__("book_stats").calc_stats("bookdata.json")
        else:
            __import__("book_stats").calc_stats_blacklist("bookdata.json", param[0])
    case "search":
        cache = __import__("vectorize_book").load_cache(cachefile)
        matches = __import__("vector_methods").sem_search(' '.join(param), cache["vec"], cache["books"], unique_books=True)
        for match, simi, score in matches:
            if args.debug:
                print(f"Match: {match}, Sentence Index: {simi}, Similarity Score: {score:.4f}")
            else:
                print(match[1])
    case _:
        print(f"Unknown command: {command}")
        exit(1)
