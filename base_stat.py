import sys
import re

file = sys.argv[1]
debug = "--debug" in sys.argv

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

sentence_end_chars = r"[.!?\"]+"
word_deilmiters = r"\s+"

def clean_word(word):
    clean = re.sub(r'^[^a-z0-9-\']+$', '', word).lower()
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
        if line.startswith("*** START OF THIS PROJECT GUTENBERG EBOOK"):
            in_ebook = True
            title = line.split("EBOOK")[-1].strip().strip('*').strip()
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
for sentence in sentences:
    sentence = sentence.strip()
    if not sentence:
        continue
    words = re.split(word_deilmiters , sentence)
    words = [word.lower() for word in words if word]
    if len(words) <= 1:
        continue
    sentence_count += 1
    sentence_words.append(words)
    sentence_len_sum += len(words)
    for word in words:
        word = clean_word(word)
        if not word:
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
    print(f"Total sentences: {sentence_count}")
    print(f"Total words: {sum(word_freq.values())}")
    print(f"Unique words: {len(word_freq)}")
    print(f"Smallest word: {min(word_freq.keys(), key=len)}")
    print(f"Largest word: {max(word_freq.keys(), key=len)}")

    smallest_sentence = " ".join(min(sentence_words, key=len))
    largest_sentence = " ".join(max(sentence_words, key=len))
    print(f"Smallest sentence: {smallest_sentence.strip()}")
    print(f"Largest sentence: {largest_sentence.strip()}")
