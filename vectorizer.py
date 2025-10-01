from gzip import GzipFile
from time import perf_counter, perf_counter_ns
from typing import TextIO

imp_start = perf_counter()
import os
import re

import itertools
from sentence_transformers import SentenceTransformer
from sentence_transformers.util import semantic_search
import pandas as pd
import pickle
import argparse
import torch
import numpy as np

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
parser.add_argument(
    "--saveprog", action="store_true", help="Save progress during generation (slower)."
)
parser.add_argument(
    "-f", "--cachefile", type=str, default="cache.pk.gz", help="Path to the cache file."
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

books = dict()

def get_words(file):
    try:
        with open(file, 'r', encoding='utf-8') as f:
            lines = f.readlines()

            start_line = 0
            end_line = -1
            title = "Unknown Title"
            author = "Unknown Author"

            for i, line in enumerate(lines):
                line = line.strip()
                if line.startswith("Title:"):
                    title = line.split("Title:")[1].strip()
                elif line.startswith("Author:"):
                    author = line.split("Author:")[1].strip()
                elif line.startswith("*** START OF THIS PROJECT GUTENBERG EBOOK"):
                    start_line = i + 1
                    continue
                elif line.startswith("*** END OF THIS PROJECT GUTENBERG EBOOK"):
                    end_line = i
                    break
    except Exception as e:
        print(f"Error reading {file}: {e}")
        return [], ["Unknown Book", 0, "Unknown Title", "Unknown Author"]

    if end_line == -1 or title == "Unknown Title" or author == "Unknown Author":
        return [], ["Unknown Book", 0, title, author]
    book_id = os.path.basename(file).split(".")[0]

    ebook = " ".join(lines[start_line:end_line]).strip()

    sentences = re.split(sentence_end_chars, ebook)
    sentences = list(
        map(
            lambda sentence: (book_id, sentence, title, author),
            filter(
                lambda s: s.strip() != "",
                sentences
            )
        )
    )

    return sentences, [book_id, len(sentences), title, author]

def sem_search(query: str, embeds):
    query_embed = model.encode_query(query)
    # sims = model.similarity(query_embed, embeds)[0]
    # scores, simi = torch.topk(sims, k=1)
    # return simi[0].item(), scores[0].item()
    return semantic_search(query_embed, embeds, top_k=5)[0]

def process_book(file, book_list, range_list):
    sentences, file_info = get_words(file)
    book_list.extend(sentences)
    range_list.append(file_info)

def save_cache(cache, path):
    print(f"Saving cache to {path}...")
    fgz: GzipFile
    with open(path, "wb") as fgz:
        pickle.dump(cache, fgz)

print(f"Basics loaded in {perf_counter() - basics_start:.3f} seconds")
ebook_timer = perf_counter()

print(f"Loading cache {args.cachefile}...")
cache = dict()
if os.path.isfile(args.cachefile):
    with open(args.cachefile, "rb") as f:
        try:
            cache = pickle.load(f)
        except Exception as e:
            print(f"Error loading cache: {e}")
else:
    print("No cache file found.")

data = pd.DataFrame(columns=["book_id", "ebook"])
embeds = []
model = SentenceTransformer("tomaarsen/static-retrieval-mrl-en-v1", truncate_dim=256)
if r_cache:
    print(f"Loading cached embeddings")
    data = cache["books"]
    embeds = cache["vec"]
else:
    ebooks = []
    cache_infos = []
    list(map(
        lambda file:
        process_book(
            os.path.join(
                str(args.parse[1]),
                str(file)
            ),
            ebooks,
            cache_infos
        ),
        filter(
            lambda file: file.endswith(".txt"),
            os.listdir(args.parse[1]),
        )
    ))

    book_ids, book_texts, titles, authors = zip(*ebooks)
    # store a first and last index of each book id
    cache_ids = []
    cache_titles = []
    cache_authors = []
    cache_low_index = []
    cache_high_index = []
    current_index = 0
    for book in cache_infos:
        book_id = book[0]
        length = book[1]
        title = book[2]
        author = book[3]
        if length == 0:
            continue
        cache_ids.append(book_id)
        cache_titles.append(title)
        cache_authors.append(author)
        cache_low_index.append(current_index)
        current_index += length
        cache_high_index.append(current_index - 1)

    data = pd.DataFrame(
        {
            "book_id": book_ids,
            "sent": book_texts,
        }
    )
    cache_df = pd.DataFrame({
        "book_id": cache_ids,
        "title": cache_titles,
        "author": cache_authors,
        "low_index": cache_low_index,
        "high_index": cache_high_index,
    })

    embeds = []
    cache["books"] = cache_df
    sents = data["sent"].tolist()
    del data
    chunk_size = 300
    n_chunks = len(sents) // chunk_size
    save_points = n_chunks // 2 if n_chunks >= 2 else 1  # Save 5 times during generation
    for i, data_chunk in enumerate(itertools.batched(sents, chunk_size)):
        try:
            if i % 10:
                print(f"Processing chunk {i + 1}/{n_chunks}", end="    \r")
            embeds.extend(model.encode(data_chunk))

            # Save our progress
            if args.saveprog and i % save_points == 0:
                cache["vec"] = embeds
                save_cache(cache, args.cachefile)
                print(f"Progress saved at chunk {i + 1}/{n_chunks}        ")
        except KeyboardInterrupt:
            print("Saving and exiting...")
            save_cache(cache, args.cachefile)
            exit(0)

    data = cache_df
    cache["vec"] = embeds
    save_cache(cache, args.cachefile)
    print(f"Embeddings generated in {perf_counter() - ebook_timer:.3f} seconds")

embeds = np.array(embeds)
embeds = torch.Tensor(embeds)
print(f"Embeddings loaded in {perf_counter() - ebook_timer:.3f} seconds")

query = input("Enter your query: ").strip()

while query != "exit":
    query_timer = perf_counter_ns()
    sims = sem_search(query, embeds)
    print(f"Top {len(sims)} results ({(perf_counter_ns() - query_timer) // 1_000_000}ms):")
    for sim in sims:
        simi = sim['corpus_id']
        score = sim['score']
        match = data.loc[(data['low_index'] <= simi) & (data['high_index'] >= simi)]['title'].values[0]
        print(f"{match} ({score:.3f})")

    query = input("Enter your query: ").strip()
