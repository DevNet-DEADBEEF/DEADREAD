from time import perf_counter
from typing import Iterator

imp_start = perf_counter()
import os
import re

import itertools
from sentence_transformers import SentenceTransformer
import pandas as pd
import argparse

print(f"Imports finished in {perf_counter() - imp_start:.3f} seconds")

basics_start = perf_counter()

parser = argparse.ArgumentParser(
    description="Analyze a Project Gutenberg ebook for average sentence length and top 5 words."
)
parser.add_argument(
    "parse", help="Path to the Project Gutenberg ebook text files.",
    nargs=2,
)
parser.add_argument(
    "--debug", action="store_true", help="Enable debug output."
)
parser.add_argument(
    "--cache", action="store_true", help="Enable caching."
)

args = parser.parse_args()
debug = args.debug
if debug:
    print(f"Args: {args}")
r_cache = args.cache

# Ebook format
"""
... preamble ...
...
*** START OF THIS PROJECT GUTENBERG EBOOK <BOOK TITLE HERE> ***
<ebook here>
*** END OF THIS PROJECT GUTENBERG EBOOK <BOOK TITLE HERE> ***
... end matter ...
...
"""

sentence_end_chars = r'[.!?"]+'
word_delimiters = r"(\s|(--)|(''))+"
cleaning_pattern1 = r"[^a-zA-Z0-9-']+"
cleaning_pattern2 = r"(^['-]+(?=[^a-zA-Z]))|((?<=[^a-zAZ])['-]+$)"


def clean_word(word):
    clean = re.sub(
        cleaning_pattern1,
        '',
        word,
        flags=re.IGNORECASE | re.MULTILINE,
    ).lower()
    clean = re.sub(
        cleaning_pattern2,
        '',
        clean,
        flags=re.IGNORECASE | re.MULTILINE,
    ).lstrip("-").rstrip("-")
    if clean in ["", "-", "'"]:
        return None
    return clean


def get_words(file):
    with open(file, 'r', encoding='utf-8') as f:
        lines = f.readlines()

        start_line = 0
        end_line = -1

        for i, line in enumerate(lines):
            line = line.strip()
            if line.startswith("*** START OF THIS PROJECT GUTENBERG EBOOK"):
                start_line = i + 1
                continue
            elif line.startswith("*** END OF THIS PROJECT GUTENBERG EBOOK"):
                end_line = i
                break

    book_id = os.path.basename(file).split(".")[0]

    ebook = " ".join(lines[start_line:end_line]).strip()

    sentences = re.split(sentence_end_chars, ebook)
    sentences = list(
        map(
            lambda sentence: (book_id, sentence),
            filter(
                lambda s: s.strip() != "",
                sentences
            )
        )
    )
    print(f"Found {len(sentences)} sentences")

    return sentences


def grouper(iterator: Iterator, n: int) -> Iterator[list]:
    while chunk := list(itertools.islice(iterator, n)):
        yield chunk

print(f"Basics loaded in {perf_counter() - basics_start:.3f} seconds")
ebook_timer = perf_counter()

cache = pd.HDFStore(os.path.basename(args.parse[1]) + ".h5")

data = pd.DataFrame(columns=["book_id", "ebook"])
embeds = []
model = SentenceTransformer("keeeeenw/MicroLlama-text-embedding")
if r_cache:
    print(f"Loading cached embeddings")
    data = cache["df"]
    embeds = cache["vec"].values.tolist()
else:
    ebooks = []
    list(map(
        lambda file: ebooks.extend(get_words(os.path.join(args.parse[1], file))),
        filter(
            lambda file: file.endswith(".txt"),
            os.listdir(args.parse[1]),
        )
    ))

    book_ids = map(lambda x: x[0], ebooks)
    book_texts = map(lambda x: x[1], ebooks)
    data = pd.DataFrame(
        {
            "book_id": book_ids,
            "ebook": book_texts
        }
    )

    embeds = []
    n_chunks = data.shape[0] // 100
    for i, data_chunk in enumerate(grouper(data["ebook"].tolist(), 100)):
        try:
            print(f"Processing chunk {i + 1}/{n_chunks}", end="    \r")
            embeds.extend(model.encode(data_chunk))

            # Save our progress
            cache["df"] = data
            cache["vec"] = pd.DataFrame(embeds)
        except KeyboardInterrupt:
            print("Saving and exiting...")
            cache.close()
            exit(0)

print(f"Embeddings loaded in {perf_counter() - ebook_timer:.3f} seconds")
query_timer = perf_counter()

print(data)
cache["df"] = data
cache["vec"] = pd.DataFrame(embeds)
cache.close()

print(f"Parsed {len(data)} files from {args.parse[1]}.")

query = "Project Gutenberg"

sim = model.similarity(model.encode_query(query), embeds)
match = data["ebook"].iat[(sim == sim.max().item()).nonzero()[0][0].item()]

print(f"Query {query} matched to {match}")

query = "Encyclopedia"

sim = model.similarity(model.encode_query(query), embeds)
match = data["ebook"].iat[(sim == sim.max().item()).nonzero()[0][0].item()]

print(f"Query {query} matched to {match}")

print(f"Queries finished in {perf_counter() - query_timer:.3f} seconds")
