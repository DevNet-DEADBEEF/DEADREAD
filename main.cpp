#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <unordered_map>

// for mmap:
#include <cstdint>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

const char* map_file(const char* fname, size_t& length);
vector<string> top_five(const unordered_map<string, int>& words);

const string start_string = "*** START OF THIS PROJECT GUTENBERG EBOOK ";
const string end_string = "*** END OF THIS PROJECT GUTENBERG EBOOK ";
const string sentence_end_chars = ".!?";
const string word_deilmiters = " \n\t\r,;:\"'()[]{}<>-";

int main(int argc, char* argv[])
{
    bool quiet = false;
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <filename> [-q]" << endl;
        return -1;
    }
    if (argc == 3 && string(argv[2]) == "-q") {
        // quiet mode, dont print
        quiet = true;
    }
    size_t length;
    auto f = map_file(argv[1], length);
    auto l = f + length;

    unsigned long sum_sentances = 0;
    int num_sentences = 0;
    std::unordered_map<string, int> word_count;

    vector<string> buffer;
    string current_word;

    // loop over the file
    for (auto p = f; p != l; ++p) {
        char c = *p;

        // skip until start string is found
        if (p + start_string.size() < l && string(p, p + start_string.size()) == start_string) {
            p += start_string.size();
            continue;
        }

        // stop when end string is found
        if (p + end_string.size() < l && string(p, p + end_string.size()) == end_string) {
            break;
        }

        // if we encounter a letter, add it to the current word
        if (isalpha(c)) {
            current_word += static_cast<char>(tolower(c));
        } else {
            // if we encounter a non-letter and we have a current word, add it to the buffer
            if (!current_word.empty()) {
                buffer.push_back(current_word);
                current_word.clear();
            }
            // if we encounter a sentence-ending punctuation, count the sentence
            if (sentence_end_chars.find(c) != string::npos) {
                const unsigned int sentence_length = buffer.size();
                if (sentence_length == 0) continue; // avoid counting empty sentences

                sum_sentances += sentence_length;
                for (const auto& w : buffer) {
                    word_count[w]++;
                }

                num_sentences++;
                buffer.clear();
            }
        }
    }

    // close the file
    if (munmap(const_cast<char*>(f), length) == -1) {
        cerr << "Error un-mmapping the file" << endl;
        return -1;
    }

    double avg_sentance_length = num_sentences > 0 ? static_cast<double>(sum_sentances) / num_sentences : 0.0;

    // delete the variables sum_sentances and num_sentences
    if (!quiet)
        cout << avg_sentance_length;

    // sort the words
    auto sorted_words = top_five(word_count);
    if (!quiet) {
        for (const auto& w : sorted_words) {
            cout << " " << w;
        }
        cout << endl;
    }
    return 0;
}

void handle_error(const char* msg) {
    perror(msg);
    exit(255);
}

const char* map_file(const char* fname, size_t& length)
{
    int fd = open(fname, O_RDONLY);
    if (fd == -1)
        handle_error("open");

    // obtain file size
    struct stat sb{};
    if (fstat(fd, &sb) == -1)
        handle_error("fstat");

    length = sb.st_size;

    const char* addr = static_cast<const char*>(mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0u));
    if (addr == MAP_FAILED)
        handle_error("mmap");

    return addr;
}

void vec_insert(vector<int> &vec, int pos, int val) {
    vec.insert(vec.begin() + pos, val);
    vec.erase(vec.end() - 1, vec.end());
}

void vec_insert(vector<string> &vec, int pos, const string& val) {
    vec.insert(vec.begin() + pos, val);
    vec.erase(vec.end() - 1, vec.end());
}

bool vec_cont(vector<string> &vec, const string &val) {
    return find(vec.begin(), vec.end(), val) != vec.end();
}

vector<string> top_five(const unordered_map<string, int>& words) {
    vector counts = {-1, -1, -1, -1, -1};
    vector<string> word = {"..", "..", "..", "..", ".."};

    for (const auto& pair : words) {
        // skip words already in top_words
        // if (vec_cont(word, pair.first)) {
        //     continue;
        // }
        if (pair.second > counts[4]) {
            if (pair.second > counts[3]) {
                if (pair.second > counts[2]) {
                    if (pair.second > counts[1]) {
                        if (pair.second > counts[0]) {
                            vec_insert(counts, 0, pair.second);
                            vec_insert(word, 0, pair.first);
                        } else {
                            vec_insert(counts, 1, pair.second);
                            vec_insert(word, 1, pair.first);
                        }
                    } else {
                        vec_insert(counts, 2, pair.second);
                        vec_insert(word, 2, pair.first);
                    }
                } else {
                    vec_insert(counts, 3, pair.second);
                    vec_insert(word, 3, pair.first);
                }
            } else {
                vec_insert(counts, 4, pair.second);
                vec_insert(word, 4, pair.first);
            }
        }
    }
    return word;
}