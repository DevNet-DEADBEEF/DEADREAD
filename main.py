import argparse
import os

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
    "-f", "--cachefile", type=str, default="cache.pk", help="Path to the cache file."
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
        os.system(f"./speedtest {' '.join(param)}")
        import vectorize_book as vb
        for file in param:
            vb.parse_book(file, cachefile, debug=args.debug)
    case "stats":
        __import__("book_stats").calc_stats(cachefile, param[0])
    case "search":
        cache = __import__("vectorize_book").load_cache(cachefile)
        matches = __import__("vector_methods").sem_search(' '.join(param), cache["vec"], cache["books"])
        for match, simi, score in matches:
            if args.debug:
                print(f"Match: {match}, Sentence Index: {simi}, Similarity Score: {score:.4f}")
            else:
                print(match[1])
    case _:
        print(f"Unknown command: {command}")
        exit(1)
