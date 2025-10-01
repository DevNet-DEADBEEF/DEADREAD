#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <string_view>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string.h> 

#include <sys/stat.h>
#include <fcntl.h>

// Vectors for identifying book start/end and title:
const std::vector<char> START_STRING = {'*', '*', '*', ' ', 'S', 'T', 'A', 'R', 'T', ' ', 'O', 'F', ' ', 'T', 'H', 'I', 'S', ' ', 'P', 'R', 'O', 'J', 'E', 'C', 'T', ' ', 'G', 'U', 'T', 'E', 'N', 'B', 'E', 'R', 'G', ' ', 'E', 'B', 'O', 'O', 'K', ' '};
const std::vector<char> HEADER_END = {'*', '*', '*'};
const std::vector<char> END_STRING = {'*', '*', '*', ' ', 'E', 'N', 'D', ' ', 'O', 'F', ' ', 'T', 'H', 'I', 'S', ' ', 'P', 'R', 'O', 'J', 'E', 'C', 'T', ' ', 'G', 'U', 'T', 'E', 'N', 'B', 'E', 'R', 'G', ' ', 'E', 'B', 'O', 'O', 'K', ' '};
const std::vector<char> TITLE_STRING = {'T', 'i', 't', 'l', 'e', ':', ' '};
static const auto BUFFER_SIZE = 256 * 1024;

bool DEBUG = false;
bool SILENT = false;

bool punct_table[256] = {false};
bool alnum_table[256] = {false};
bool delim_table[256] = {false};

void initialize_lookup_tables() {
    punct_table['.'] = punct_table['!'] = punct_table['?'] = punct_table['"'] = true;
    for (int c = 0; c < 256; ++c) {
        alnum_table[c] = std::isalnum(c);
        delim_table[c] = (c == ' ' || c == '\n' || c == '\t' || c == '\r' || punct_table[c]);
    }
}


std::unordered_set<std::string> bannedWords;

double times2double(std::chrono::time_point<std::chrono::high_resolution_clock> start, std::chrono::time_point<std::chrono::high_resolution_clock> end) {
    return std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count() * 1000.0;
}

std::chrono::time_point<std::chrono::high_resolution_clock> time_now() {
    return std::chrono::high_resolution_clock::now();
}

void handle_error(const char* msg) {
    perror(msg); 
    exit(255);
}

size_t find_pattern_simd(const char* text, size_t text_len, const std::vector<char>& pattern) {
    if (pattern.size() > text_len) return SIZE_MAX;
    
    const void* result = memmem(text, text_len, pattern.data(), pattern.size());
    return result ? static_cast<const char*>(result) - text : SIZE_MAX;
}

// static void parse_csv(int fin) {
//     std::string word;
//     char buf[BUFFER_SIZE+1];

//     while(size_t bytes_read = read(fin, buf, BUFFER_SIZE)){

//         if(bytes_read == (size_t)-1)
//             handle_error("read failed");
//         if (!bytes_read)
//             break;
        
//         for (char c: buf){
//             if (c == ',' || c == '\n'){
//                 bannedWords.insert(word);
//                 word.clear();
//             }
//             else {
//                 word.push_back(c);
//             }
//         }
//     }

//     if(close(fin) == -1)
//         handle_error("csv close failed");

//     //DEBUG:
//     // for (std::string w : bannedWords){
//     //     std::cout << w << "\n";

//     // }
// }

struct ParseResult {
    std::string title;
    std::vector<std::string> words;
    double avgSenLen;
};

// Function to escape JSON strings
std::string escapeJsonString(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.size() * 2); // Pre-allocate for efficiency
    
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (c < 0x20) {
                    // Control characters
                    escaped += "\\u";
                    char hex[5];
                    snprintf(hex, sizeof(hex), "%04x", static_cast<unsigned char>(c));
                    escaped += hex;
                } else {
                    escaped += c;
                }
                break;
        }
    }
    return escaped;
}

// Function to convert ParseResult to JSON string
std::string parseResultToJson(const ParseResult& result) {
    std::ostringstream json;
    json << "{";
    json << "\"title\":\"" << escapeJsonString(result.title) << "\",";
    json << "\"avgSenLen\":" << result.avgSenLen << ",";
    json << "\"words\":[";
    
    for (size_t i = 0; i < result.words.size(); ++i) {
        if (i > 0) json << ",";
        json << "\"" << escapeJsonString(result.words[i]) << "\"";
    }
    
    json << "]";
    json << "}";
    return json.str();
}

static ParseResult parseFile(int fd) {
    auto t1 = time_now();

    char buf[BUFFER_SIZE + 1];

    long sentenceSum = 0;
    long sentenceNum = 0;

    bool inBook = false;
    bool inHeader = false;
    // bool gotTitle = false;
    bool exiting = false;

    std::vector<std::string> shortest_sentence = {};
    std::vector<std::string> longest_sentence = {};
    std::string smallest_word = {};
    std::string largest_word = {};

    // int exitWords = 0;
    std::vector<char> title = {};
    // std::vector<char> titleHead = {};
    //std::vector<char> exitbuf = {};
    //std::vector<char> startBuf = {};

    std::vector<std::string> sentencebuf = {};
    std::vector<char> wordbuf = {};
    std::unordered_map<std::string, int> word_count;
    
    auto t2 = time_now();
    if (!SILENT) {
        std::cout << "File opened in " << times2double(t1, t2) << "ms" << std::endl;
    }

    while(size_t bytes_read = read(fd, buf, BUFFER_SIZE)) {
        // auto tc1 = time_now();

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
        
        size_t fast_start_pos = SIZE_MAX;
        size_t fast_end_pos = SIZE_MAX;
        size_t title_pos = SIZE_MAX;

        if (inBook) {
            fast_end_pos = find_pattern_simd(buf, bytes_read, END_STRING);
            if (fast_end_pos != SIZE_MAX) {
                inBook = false;
                exiting = true;
                if (DEBUG) {
                    std::cout << "SIMD found END pattern at position " << fast_end_pos << std::endl;
                }
            }
        }
        
        if (title.empty() && !inBook) {
            title_pos = find_pattern_simd(buf, bytes_read, TITLE_STRING);
            if (title_pos != SIZE_MAX) {
                size_t title_start = title_pos + TITLE_STRING.size();
                size_t title_end = title_start;
                
                while (title_end < bytes_read && buf[title_end] != '\n' && buf[title_end] != '\r') {
                    title_end++;
                }
                
                if (title_end > title_start) {
                    title.clear();
                    for (size_t i = title_start; i < title_end; ++i) {
                        title.push_back(buf[i]);
                    }
                    if (DEBUG) {
                        std::cout << "Found title: " << std::string(title.begin(), title.end()) << std::endl;
                    }
                }
            }
        }
        
        if (!inBook && !inHeader) {
            fast_start_pos = find_pattern_simd(buf, bytes_read, START_STRING);
            if (fast_start_pos != SIZE_MAX) {
                inHeader = true;
                if (DEBUG) {
                    std::cout << "SIMD found START pattern at position " << fast_start_pos << std::endl;
                }
            }
        }
        
        size_t header_end_pos = SIZE_MAX;
        if (inHeader && !inBook) {
            size_t search_start_offset = fast_start_pos + START_STRING.size();
            const char* search_buf = buf + search_start_offset;
            size_t search_len = bytes_read - search_start_offset;
            
            size_t relative_pos = find_pattern_simd(search_buf, search_len, HEADER_END);
            if (relative_pos != SIZE_MAX) {
                header_end_pos = search_start_offset + relative_pos;
                inBook = true;
                inHeader = false;
                if (DEBUG) {
                    std::cout << "SIMD found header end (***) at position " << header_end_pos << std::endl;
                }
            }
        }
        
        size_t start_processing = 0;
        if (header_end_pos != SIZE_MAX) {
            start_processing = header_end_pos + 3;
        } else if (fast_start_pos != SIZE_MAX && !inHeader) {
            start_processing = bytes_read;
        } else if (fast_end_pos != SIZE_MAX) {
            bytes_read = fast_end_pos;
        }

        for (size_t i = start_processing; i < bytes_read; ++i) {
            if (!inBook) {
                continue;
            }
            
            bool isPunct = punct_table[static_cast<unsigned char>(buf[i])];

            // if (!inBook) {
            //     if (!inHeader) {
            //         if (!gotTitle){
            //             if (buf[i] == TITLE_STRING[titleHead.size()]){
            //                 titleHead.push_back(buf[i]);
            //                 if (titleHead.size() == TITLE_STRING.size()){
            //                     ++i;
            //                     while(buf[i] != '\n'){
            //                         title.push_back(buf[i]);
            //                         ++i;
            //                     }
            //                     //std::cout << "Title: " << std::string(title.begin(), title.end()) << '\n';
            //                     gotTitle = true;
            //                 }
            //             } else if (!titleHead.empty()) {
            //                 // Mismatch after some matches, reset
            //                 titleHead.clear();
                            
            //             }
            //         }
            //         // Potential start of header
            //         if (buf[i] == START_STRING[startBuf.size()]) {
            //             // Potential end of book
            //             startBuf.push_back(buf[i]);
            //             if (startBuf.size() == START_STRING.size()) {
            //                 inHeader = true;
            //             }
            //         } else if (!startBuf.empty()) {
            //             // Mismatch after some matches, reset
            //             startBuf.clear();
            //         }
            //     } else {
            //         if (buf[i] == '*') {
            //             // Potential end of header
            //             if (i + 2 < bytes_read && buf[i + 1] == '*' && buf[i + 2] == '*') {
            //                 inHeader = false;
            //                 inBook = true;
            //                 i += 2;  // Skip the next two asterisks
            //                 // std::cout << "End of header\n";
            //             }
            //         }
            //     }
            //     continue;
            // }
            // if (buf[i] == END_STRING[exitbuf.size()]) {
            //     // Potential end of book
            //     exitbuf.push_back(buf[i]);
            //     if (buf[i] == ' ') {
            //         exitWords++;
            //     }
            //     if (exitbuf.size() == END_STRING.size()) {
            //         for (int i = 0; i < exitWords; ++i) {
            //             if (!sentencebuf.empty()) sentencebuf.pop_back();
            //         }
            //         inBook = false;
            //         exiting = true;
            //         isPunct = true;
            //     }
            // } else if (!exitbuf.empty()) {
            //     // Mismatch after some matches, reset
            //     exitWords = 0;
            //     exitbuf.clear();
            // }

            bool isDelimiter = delim_table[static_cast<unsigned char>(buf[i])] || isPunct;

            // if (alnum_table[static_cast<unsigned char>(buf[i])]) {
            //} else 
            if (buf[i] == '\'' || buf[i] == '-') {
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

                auto [it, inserted] = word_count.try_emplace(word, 1);
                if (!inserted) {
                    it->second++;
                }

                sentencebuf.push_back(word);
            } else if (!isDelimiter) {
                wordbuf.push_back(buf[i]);
            }
            if (isPunct) {
                if (sentencebuf.size() <= 1) {
                    if (!sentencebuf.empty()) sentencebuf.clear();
                    continue;
                }
                
                if (DEBUG) {
                    if (shortest_sentence.empty() || sentencebuf.size() < shortest_sentence.size()) {
                        shortest_sentence = sentencebuf;
                    }

                    if (longest_sentence.empty() || sentencebuf.size() > longest_sentence.size()) {
                        longest_sentence = sentencebuf;
                    }
                }

                ++sentenceNum;
                sentenceSum += sentencebuf.size();
                sentencebuf.clear();
            }
        }

        if (exiting) {
            break;
        }
        //auto tc2 = time_now();
        //std::cout << "Chunk processed in " << times2double(tc1, tc2) << "ms" << std::endl;
    }

    auto t3 = time_now();
    if (!SILENT) {
        std::cout << "File processed in " << times2double(t2, t3) << "ms" << std::endl;
    }

    if(close(fd) == -1)
        handle_error("close failed");
    
    double avgSenLen = sentenceNum ? (double)sentenceSum / sentenceNum : 0;

    if (DEBUG) {

        std::vector<std::string> top_words;
        int num_words = 0;

        for (const std::pair<const std::string, int>& i : word_count) {
            num_words += i.second;

                if(bannedWords.find(i.first) != bannedWords.end()){
                continue;
            }

            if (top_words.size() > 0 && word_count[top_words[0]] < i.second) {
                top_words.insert(top_words.begin(), i.first);
            } else if (top_words.size() > 1 && word_count[top_words[1]] < i.second) {
                top_words.insert(top_words.begin() + 1, i.first);
            } else if (top_words.size() > 2 && word_count[top_words[2]] < i.second) {
                top_words.insert(top_words.begin() + 2, i.first);
            } else if (top_words.size() > 3 && word_count[top_words[3]] < i.second) {
                top_words.insert(top_words.begin() + 3, i.first);
            } else if (top_words.size() > 4 && word_count[top_words[4]] < i.second) {
                top_words.insert(top_words.begin() + 4, i.first);
            } else if (top_words.size() < 5) {
                top_words.push_back(i.first);
            } 
            if (top_words.size() > 5) {
                top_words.pop_back();
            }
        }

        auto t4 = time_now();

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

        std::cout << std::string(title.begin(), title.end()) << " \\ ";
        for (size_t i = 0; i < top_words.size(); ++i){
            std::cout << top_words[i] << " ";
        }
        std::cout << "\\ " << avgSenLen << '\n'; 

        std::cout << "Total sentences: " << sentenceNum << "\n";
        std::cout << "Average words per sentence: " << avgSenLen << "\n";
        std::cout << "Total words: " << num_words << "\n";
        std::cout << "Smallest word: " << smallest_word << " (" << smallest_word.size() << " characters)\n";
        std::cout << "Largest word: " << largest_word << " (" << largest_word.size() << " characters)\n";
        std::cout << "Unique words: " << word_count.size() - 1 << "\n";
    }

    std::vector<std::pair<std::string, int>> sorted_words(word_count.begin(), word_count.end());
    std::sort(sorted_words.begin(), sorted_words.end(), [](const std::pair<std::string, int>& a, const std::pair<std::string, int>& b) {
        return a.second > b.second; // Sort by frequency (descending)
    });
    
    std::vector<std::string> words_only;
    words_only.reserve(sorted_words.size());
    for (const auto& word_pair : sorted_words) {
        if (!word_pair.first.empty()) {
            words_only.push_back(word_pair.first);
        }
    }

    auto t5 = time_now();
    if (DEBUG) {
        std::cout << "Operations completed in " << times2double(t1, t5) << "ms" << std::endl;
    }

    ParseResult result;
    result.title = std::string(title.begin(), title.end());
    result.avgSenLen = avgSenLen;
    result.words = std::move(words_only);
    return result;
    
    //std::cout << buf << "\n";
}

int main(int argc, char* argv[]) {
    // if (argc < 2 || argc > 3) {
    //     std::cerr << "Usage: " << argv[0] << " <txtfilename or txtfolder> <blacklistCSV>\n";
    //     return 1;
    // }
    
    initialize_lookup_tables();

    std::vector<std::string> bfStrs;
    std::vector<int> files;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--debug") == 0) {
            DEBUG = true;
        } else if (strcmp(argv[i], "--silent") == 0) {
            SILENT = true;
        } else if (std::filesystem::is_directory(argv[i])){
            for (auto const& bookFile : std::filesystem::directory_iterator(argv[i])){
                std::string bfStr = bookFile.path().string();
                //std::cout << bfStr << '\n'; //DEBUG
                if (bfStr.size() > 4 && bfStr.substr(bfStr.size()-4) != ".txt"){
                    if (!SILENT) {
                        std::cout << bfStr << " is not a .txt file!" << "\n";
                    }
                    continue;
                }

                int fd = open(bookFile.path().c_str(), O_RDONLY);
                
                if(fd == -1)
                    handle_error(("Failed to open file " + bfStr).c_str());

                /* Advise the kernel of our access pattern.  */
                posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);

                bfStrs.push_back(bfStr);
                files.push_back(fd);
            }
        } else {
            std::string arg(argv[i]);
            if (arg.size() > 4 && arg.substr(arg.size()-4) != ".txt" || arg.size() < 4){
                std::cerr << "Invalid file provided.\nUsage: " << argv[0] << " <txtfilename or txtfolder>\n";
                return 1;
            }

            int fd = open(argv[i], O_RDONLY);
            if(fd == -1)
                handle_error(("Failed to open file " + arg).c_str());

            /* Advise the kernel of our access pattern.  */
            posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);  // FDADVICE_SEQUENTIAL

            bfStrs.push_back(arg);
            files.push_back(fd);
        }
    }

    // if (argc == 3){
    //     std::string_view arg(argv[2]);
    //     if (arg.size() > 4 && arg.substr(arg.size()-4) != ".csv"){
    //         std::cerr << "Usage: " << argv[0] << " <txtfilename or txtfolder> <blacklistCSV>\n";
    //         return 1;
    //     }

    //     int fin = open(argv[2], O_RDONLY);
    //     if(fin == -1)
    //         handle_error("open csv");

            
    //     /* Advise the kernel of our access pattern.  */
    //     posix_fadvise(fin, 0, 0, POSIX_FADV_SEQUENTIAL);  // FDADVICE_SEQUENTIAL

    //     parse_csv(fin);
    // }

    // Collect all parse results
    std::unordered_map<std::string, ParseResult> allResults;
    
    for (std::vector<int>::size_type i = 0; i < files.size(); ++i){
        if (DEBUG) {
            std::cout << bfStrs[i] << '\n'; //DEBUG
        }

        ParseResult result = parseFile(files[i]);
        if (!result.words.empty()) {
            allResults[bfStrs[i]] = std::move(result);
        }

        if (DEBUG) {
            std::cout << '\n' << "--------------------------------------------------" << "\n\n"; //For readability.
        }
    }
    
    // Convert all results to JSON and write to file
    std::ofstream outFile("cache.json");
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open out.json for writing" << std::endl;
        return 1;
    }
    
    outFile << "{" << std::endl;
    bool firstFile = true;
    
    for (const auto& [filePath, result] : allResults) {
        if (!firstFile) {
            outFile << "," << std::endl;
        }
        firstFile = false;
        
        outFile << "  \"" << escapeJsonString(filePath) << "\": ";
        outFile << parseResultToJson(result);
    }
    
    outFile << std::endl << "}" << std::endl;
    outFile.close();

    if (!SILENT) {
        std::cout << "Results written to out.json" << std::endl;
    }
}