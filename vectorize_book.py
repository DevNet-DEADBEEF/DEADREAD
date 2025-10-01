import os
import pickle
import re

from vector_methods import encode_sentences

sentence_end_chars = r'[.!?"]+'


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
        filter(
            lambda s: s.strip() != "",
            sentences
        )
    )

    return sentences, [book_id, len(sentences), title, author]

def load_cache(path):
    if os.path.isfile(path):
        with open(path, "rb") as f:
            try:
                cache = pickle.load(f)
                return cache
            except Exception as e:
                print(f"Error loading cache: {e}")
    return {
        "vec": [],
        "books": []
    }

def save_cache(cache, path):
    with open(path, "wb") as f:
        pickle.dump(cache, f)

def book_exists(book_id: str, data) -> bool:
    for entry in data:
        if entry[0] == book_id:
            return True
    return False

def parse_book(book_file: str, cache_file: str, debug: bool = False):
    if debug:
        print(f"Parsing book: {book_file}")
        print(f"Using cache file: {cache_file}")
    cache = load_cache(cache_file)

    data = cache["books"]
    embeds = cache["vec"]

    sentences, (book_id, num_sent, title, author) = get_words(book_file)

    if debug:
        print(f"Book ID: {book_id}")
        print(f"Title: {title}")
        print(f"Author: {author}")
        print(f"Number of sentences: {num_sent}")
    # check if the book_id is already in the data
    if book_exists(book_id, data):
        print(f"Book {book_id} already in cache, skipping.")
        return

    new_embeds = encode_sentences(sentences)
    if len(data) > 0:
        last_index = data[-1][4]
        data.append([
            book_id,
            title,
            author,
            last_index + 1,
            last_index + num_sent,
        ])
        embeds.extend(new_embeds)
    else:
        data.append([book_id, title, author, 0, num_sent - 1])
        embeds = new_embeds

    cache["vec"] = embeds
    cache["books"] = data
    save_cache(cache, cache_file)
    if debug:
        print(f"Saved {num_sent} sentences for book {book_id} to cache.")
        print("Cached books:")
        for entry in data:
            print(entry)
