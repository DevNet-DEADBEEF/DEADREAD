#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <chrono>
#include <tr1/unordered_map>
#include <unordered_set>
#include <filesystem>

// for mmap:
#include <sys/stat.h>
#include <fcntl.h>

const std::string START_STRING = "*** START OF THIS PROJECT GUTENBERG EBOOK ";
const std::string END_STRING = "*** END "; // OF THIS PROJECT GUTENBERG EBOOK ";
const std::string TITLE_STRING = "Title: ";

double times2double(std::chrono::time_point<std::chrono::high_resolution_clock> start, std::chrono::time_point<std::chrono::high_resolution_clock> end) {
    return std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count() * 1000.0;
}

std::chrono::time_point<std::chrono::high_resolution_clock> time_now() {
    return std::chrono::high_resolution_clock::now();
}

std::unordered_set<std::string> bannedWords;
static const auto BUFFER_SIZE = 128*1024;

void handle_error(const char* msg) {
    perror(msg); 
    exit(255);
}

static void parse_csv(int fin) {
    std::string word;
    char buf[BUFFER_SIZE+1];

    while(size_t bytes_read = read(fin, buf, BUFFER_SIZE)){

        if(bytes_read == (size_t)-1)
            handle_error("read failed");
        if (!bytes_read)
            break;
        
        for (char c: buf){
            if (c == ',' || c == '\n'){
                bannedWords.insert(word);
                word.clear();
            }
            else {
                word.push_back(c);
            }
        }
    }

    if(close(fin) == -1)
        handle_error("csv close failed");

    //DEBUG:
    // for (std::string w : bannedWords){
    //     std::cout << w << "\n";

    // }
}

static void wc(int fd) {
    auto t1 = time_now();

    char buf[BUFFER_SIZE + 1];

    long sentenceSum = 0;
    long sentenceNum = 0;

    bool inBook = false;
    bool inHeader = false;
    bool gotTitle = false;
    bool exitting = false;

    std::vector<std::string> shortest_sentence = {};
    std::vector<std::string> longest_sentence = {};
    std::string smallest_word = {};
    std::string largest_word = {};

    int exitWords = 0;
    std::string title;
    std::vector<char> exitbuf = {};
    std::vector<char> startBuf = {};

    std::vector<std::string> sentencebuf = {};
    std::vector<char> wordbuf = {};
    std::tr1::unordered_map<std::string, int> word_count;

    auto t2 = time_now();
    std::cout << "File opened in " << times2double(t1, t2) << "ms" << std::endl;

    while(size_t bytes_read = read(fd, buf, BUFFER_SIZE)) {
        auto tc1 = time_now();

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
                    if (!gotTitle){
                        if (buf[i] == TITLE_STRING[startBuf.size()]){
                            startBuf.push_back(buf[i]);
                            if (startBuf.size() == TITLE_STRING.size()){
                                ++i;
                                while(buf[i] != '\n'){
                                    title += buf[i];
                                    ++i;
                                }
                                std::cout << "Title: " << title << '\n';
                                gotTitle = true;
                            }
                        }
                    } else
                    // Potential start of header
                    if (buf[i] == START_STRING[startBuf.size()]) {
                        // Potential end of book
                        startBuf.push_back(buf[i]);
                        if (startBuf.size() == START_STRING.size()) {
                            inHeader = true;
                        }
                    } else if (!startBuf.empty()) {
                        // Mismatch after some matches, reset
                        startBuf.clear();
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
            if (buf[i] == END_STRING[exitbuf.size()]) {
                // Potential end of book
                exitbuf.push_back(buf[i]);
                if (buf[i] == ' ') {
                    exitWords++;
                }
                if (exitbuf.size() == END_STRING.size()) {
                    for (int i = 0; i < exitWords; ++i) {
                        if (!sentencebuf.empty()) sentencebuf.pop_back();
                    }
                    inBook = false;
                    exitting = true;
                    isPunct = true;
                }
            } else if (!exitbuf.empty()) {
                // Mismatch after some matches, reset
                exitWords = 0;
                exitbuf.clear();
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

                if (std::isupper(word[0])){
                    word[0] = std::tolower(word[0]);
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

        //auto tc2 = time_now();
        //std::cout << "Chunk processed in " << times2double(tc1, tc2) << "ms" << std::endl;
    }

    auto t3 = time_now();
    std::cout << "File processed in " << times2double(t2, t3) << "ms" << std::endl;

    std::vector<std::string> top_words;
    top_words.resize(5);

    int num_words = 0;

    for (const std::pair<const std::string, int>& i : word_count) {
        num_words += i.second;

        if(bannedWords.find(i.first) != bannedWords.end()){
            continue;
        }

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
    }

    auto t4 = time_now();
    double avgSenLen = sentenceNum ? (double)sentenceSum / sentenceNum : 0;

    std::cout << title << " \\ ";
    for (size_t i = 0; i < top_words.size(); ++i){
        std::cout << top_words[i] << " ";
    }
    std::cout << "\\ " << avgSenLen << '\n'; 


    std::cout << "Top words computed in " << times2double(t3, t4) << "ms" << std::endl;

    std::cout << "Top 5 most common words:\n";

    for (size_t i = 0; i < top_words.size(); ++i) {
        std::cout << " - " << top_words[i] << " (" << word_count[top_words[i]] << ")\n";
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
    std::cout << "Average words per sentence: " << avgSenLen << "\n";
    std::cout << "Total words: " << num_words << "\n";
    std::cout << "Smallest word: " << smallest_word << " (" << smallest_word.size() << " characters)\n";
    std::cout << "Largest word: " << largest_word << " (" << largest_word.size() << " characters)\n";
    std::cout << "Unique words: " << word_count.size() - 1 << "\n";

    std::vector<std::pair<std::string, int>> sorted_words(word_count.begin(), word_count.end());
    std::sort(sorted_words.begin(), sorted_words.end());//, [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
    //     return a.second > b.second; // Sort by frequency (descending)
    // });
    // for (const auto& w : sorted_words) {
    //     if (w.first.empty()) continue;
    //     std::cout << " - " << w.first << " (" << w.second << ")\n";
    // }

    std::cout << "Finished reading file\n";

    if(close(fd) == -1)
        handle_error("close failed");

    auto t5 = time_now();
    std::cout << "Operations completed in " << times2double(t1, t5) << "ms" << std::endl;

    //std::cout << buf << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if(fd == -1)
        handle_error("open");

    /* Advise the kernel of our access pattern.  */
    posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);  // FDADVICE_SEQUENTIAL

    if (argc == 3){
        int fin = open(argv[2], O_RDONLY);
        if(fin == -1)
            handle_error("open csv");

            
        /* Advise the kernel of our access pattern.  */
        posix_fadvise(fin, 0, 0, POSIX_FADV_SEQUENTIAL);  // FDADVICE_SEQUENTIAL

        parse_csv(fin);
    }

    wc(fd);
}