#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <tr1/unordered_map>

// for mmap:
#include <sys/stat.h>
#include <fcntl.h>

const std::string START_STRING = "*** START OF THIS PROJECT GUTENBERG EBOOK ";
const std::string END_STRING = "*** END ";

void handle_error(const char* msg) {
    perror(msg); 
    exit(255);
}

static void wc(char const *fname)
{
    static const auto BUFFER_SIZE = 16*1024;
    int fd = open(fname, O_RDONLY);
    if(fd == -1)
        handle_error("open");

    /* Advise the kernel of our access pattern.  */
    posix_fadvise(fd, 0, 0, 1);  // FDADVICE_SEQUENTIAL

    char buf[BUFFER_SIZE + 1];

    long sentenceSum = 0;
    long sentenceNum = 0;

    bool inBook = false;
    bool inHeader = false;
    bool exitting = false;

    std::vector<std::string> shortest_sentence = {};
    std::vector<std::string> longest_sentence = {};
    std::string smallest_word = {};
    std::string largest_word = {};

    std::vector<std::string> sentencebuf = {};
    std::vector<char> wordbuf = {};
    std::tr1::unordered_map<std::string, int> word_count;

    while(size_t bytes_read = read(fd, buf, BUFFER_SIZE))
    {
        if(bytes_read == (size_t)-1)
            handle_error("read failed");
        if (!bytes_read)
            break;

        // for(char *p = buf; (p = (char*) memchr(p, '\n', (buf + bytes_read) - p)); ++p) {
        //     //std::cout << p << "\n";
        //     ++lines;
        // }

        char this_read[bytes_read + 1];
        std::memcpy(this_read, buf, bytes_read);
        this_read[bytes_read] = '\0';
        // std::cout << "\n\nCHUNK:" << this_read << "\n";

        // Parse words and sentences
        for (size_t i = 0; i < bytes_read; ++i) {
            if (exitting) {
                break;
            }

            bool isPunct = (buf[i] == '.' || buf[i] == '!' || buf[i] == '?' || buf[i] == '"');

            if (!inBook) {
                if (!inHeader) {
                    if (buf[i] == '*') {
                        // Potential start of header
                        if (i + START_STRING.size() <= bytes_read) {
                            std::string potential_start(&buf[i], START_STRING.size());
                            if (potential_start == START_STRING) {
                                inHeader = true;
                                // std::cout << "Start of header\n";
                            }
                        }
                    }
                } else {
                    if (buf[i] == '*') {
                        // Potential end of header
                        if (i + 2 < bytes_read && buf[i + 1] == '*' && buf[i + 2] == '*') {
                            inHeader = false;
                            inBook = true;
                            i += 2;  // Skip the next two asterisks
                            // std::cout << "End of header\n";
                        }
                    }
                }
                continue;
            }
            if (buf[i] == '*') {
                // Potential end of book
                if (i + END_STRING.size() <= bytes_read) {
                    std::string potential_end(&buf[i], END_STRING.size());
                    if (potential_end == END_STRING) {
                        inBook = false;
                        exitting = true;
                        isPunct = true;
                        // std::cout << "End of book\n";
                    }
                }
            }

            bool isDelimiter = (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\t'  || buf[i] == '\r' || isPunct);

            if (isalnum(buf[i])) {
                wordbuf.push_back(buf[i]);
            } else if (buf[i] == '\'' || buf[i] == '-') {
                if (wordbuf.empty()) {
                    continue;
                }
                if (wordbuf.back() == '\'' || wordbuf.back() == '-') {
                    continue;
                }
                wordbuf.push_back(buf[i]);
            } else if (isDelimiter && !wordbuf.empty()) {
                // std::cout << "Word: " << std::string(wordbuf.begin(), wordbuf.end()) << "\n";
                std::string word(wordbuf.begin(), wordbuf.end());
                wordbuf.clear();

                std::transform(word.begin(), word.end(), word.begin(), ::tolower);

                if (smallest_word.empty() || word.size() < smallest_word.size()) {
                    smallest_word = word;
                }

                if (largest_word.empty() || word.size() > largest_word.size()) {
                    largest_word = word;
                }

                if (word_count.find(word) == word_count.end()) {
                    word_count[word] = 1;
                } else {
                    word_count[word]++;
                }

                sentencebuf.push_back(word);
            } 
            if (isPunct) {
                if (sentencebuf.size() <= 1) {
                    if (!sentencebuf.empty()) sentencebuf.clear();
                    continue;
                }

                if (shortest_sentence.empty() || sentencebuf.size() < shortest_sentence.size()) {
                    shortest_sentence = sentencebuf;
                }

                if (longest_sentence.empty() || sentencebuf.size() > longest_sentence.size()) {
                    longest_sentence = sentencebuf;
                }

                ++sentenceNum;
                sentenceSum += sentencebuf.size();
                sentencebuf.clear();
            }
        }
    }

    std::cout << "Top 5 most common words:\n";
    std::vector<std::string> top_words;
    top_words.resize(5);

    int num_words = 0;

    for (const std::pair<const std::string, int>& i : word_count) {
        if (top_words.size() > 0) {
            if (word_count[top_words[0]] < i.second) {
                top_words.insert(top_words.begin(), i.first);
            } else if (word_count[top_words[1]] < i.second) {
                top_words.insert(top_words.begin() + 1, i.first);
            } else if (word_count[top_words[2]] < i.second) {
                top_words.insert(top_words.begin() + 2, i.first);
            } else if (word_count[top_words[3]] < i.second) {
                top_words.insert(top_words.begin() + 3, i.first);
            } else if (word_count[top_words[4]] < i.second) {
                top_words.insert(top_words.begin() + 4, i.first);
            }
        } else if (top_words.size() < 5) {
            top_words.push_back(i.first);
        } 
        if (top_words.size() > 5) {
            top_words.pop_back();
        }
        num_words += i.second;
    }

    for (size_t i = 0; i < top_words.size(); ++i) {
        std::cout << top_words[i] << ": " << word_count[top_words[i]] << "\n";
    }

    std::cout << "Shortest sentence (" << shortest_sentence.size() << " words): ";
    for (const auto& word : shortest_sentence) {
        std::cout << word << " ";
    }
    std::cout << "\n";

    std::cout << "Longest sentence (" << longest_sentence.size() << " words): ";
    for (const auto& word : longest_sentence) {
        std::cout << word << " ";
    }
    std::cout << "\n";

    std::cout << "Total sentences: " << sentenceNum << "\n";
    std::cout << "Average words per sentence: " << (sentenceNum ? (double)sentenceSum / sentenceNum : 0) << "\n";
    std::cout << "Total words: " << num_words << "\n";
    std::cout << "Smallest word: " << smallest_word << " (" << smallest_word.size() << " characters)\n";
    std::cout << "Largest word: " << largest_word << " (" << largest_word.size() << " characters)\n";
    std::cout << "Unique words: " << word_count.size() - 1 << "\n";

    std::vector<std::pair<std::string, int>> sorted_words(word_count.begin(), word_count.end());
    std::sort(sorted_words.begin(), sorted_words.end());//, [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
    //     return a.second > b.second; // Sort by frequency (descending)
    // });
    for (const auto& w : sorted_words) {
        if (w.first.empty()) continue;
        std::cout << " - " << w.first << " (" << w.second << ")\n";
    }

    std::cout << "Finished reading file\n";

    if(close(fd) == -1)
        handle_error("close failed");

    //std::cout << buf << "\n";
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }
    wc(argv[1]);
}