import argparse
import re

parser = argparse.ArgumentParser(
    description="Analyze a Project Gutenberg ebook for average sentence length and top 5 words."
)
parser.add_argument(
    "parse", help="Path to the Project Gutenberg ebook text file.",
    nargs=2,
)
parser.add_argument(
    "stats", help="Path to csv of ignored words",
    nargs="*",
    default="",
)
parser.add_argument(
    "--debug", action="store_true", help="Enable debug output."
)

args = parser.parse_args()
debug = args.debug
if debug:
    print(f"Args: {args}")

file = args.parse[1]
blacklist = set()
if args.stats:
    with open(args.stats[1], 'r', encoding='utf-8') as f:
        file_text = f.read().replace('\n', '').lower()
        blacklist = set(file_text.split(','))

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
word_delimiters = r"(\s|(''))+"
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

with open(file, 'r', encoding='utf-8') as f:
    lines = f.readlines()
    in_ebook = False
    sentence_count = 0
    sentence_len_sum = 0
    word_freq = {}
    title = "Unknown Title"
    sentence_buffer = ""

    start_line = 0
    end_line = -1

    for i, line in enumerate(lines):
        line = line.strip()
        if line.startswith("Title: "):
            title = line.split("Title:")[-1].strip()
        elif line.startswith("*** START OF THIS PROJECT GUTENBERG EBOOK"):
            in_ebook = True
            start_line = i + 1
            continue
        elif line.startswith("*** END OF THIS PROJECT GUTENBERG EBOOK"):
            end_line = i
            break

ebook = " ".join(lines[start_line:end_line]).strip()

sentences = re.split(sentence_end_chars, ebook)
sentences = list(
    filter(
        lambda s: s.strip() != "",
        sentences
    )
)
sentence_words = []
largest_word = ""
for sentence in sentences:
    sentence = sentence.strip()
    if not sentence:
        continue
    words = re.split(word_delimiters, sentence)
    words = [word.lower() for word in words if word]
    if len(words) <= 1:
        word = words[0]
        if len(word) > len(largest_word):
            largest_word = word
        word = clean_word(word)
        if not word or word in blacklist:
            continue
        if word in word_freq:
            word_freq[word] += 1
        else:
            word_freq[word] = 1
        continue
    sentence_count += 1
    sentence_words.append(words)
    sentence_len_sum += len(words)
    for word in words:
        if len(word) > len(largest_word):
            largest_word = word
        word = clean_word(word)
        if not word or word in blacklist:
            continue
        if word in word_freq:
            word_freq[word] += 1
        else:
            word_freq[word] = 1

avg_sentence_len = sentence_len_sum / sentence_count if sentence_count > 0 else 0
print(title, "\\ ", end="")
print(avg_sentence_len, "\\ ", end="")
top_five = sorted(word_freq.items(), key=lambda x: x[1], reverse=True)[:5]
print(" ".join(word for word, freq in top_five))

if debug:
    print("\n--- DEBUG INFO ---")
    print(f"Total sentences: {sentence_count:,}")
    print(f"Total words:    {sum(word_freq.values()):,}")
    print(f"Sum sent words: {sentence_len_sum:,}")
    print(f"Unique words: {len(word_freq):,}")
    print(f"Smallest word: {min(word_freq.keys(), key=len)}")
    print(f"Largest word: {max(word_freq.keys(), key=len)}")

    smallest_sentence = " ".join(min(sentence_words, key=len))
    largest_sentence = " ".join(max(sentence_words, key=len))
    print(f"Smallest sentence: {smallest_sentence.strip()}")
    print(f"Largest sentence: {largest_sentence.strip()[:15]}...{largest_sentence.strip()[-15:]} ({len(largest_sentence.split())} words)")

    with open("py.dump.debugify.help.pls.fix", "w", encoding='utf-8') as f:
        for word, freq in sorted(word_freq.items(), key=lambda x: x[0]):
            f.write(f"- {word} ({freq})\n")
