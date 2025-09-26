#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>

// for mmap:
#include <sys/stat.h>
#include <fcntl.h>

const std::string START_STRING = "*** START OF THIS PROJECT GUTENBERG EBOOK ";
const std::string END_STRING = "*** END OF THIS PROJECT GUTENBERG EBOOK ";

const std::vector<char> PUNCTUATION = {'.', '!', '?', '"'};

void handle_error(const char* msg) {
    perror(msg); 
    exit(255);
}

static uintmax_t wc(char const *fname)
{
    static const auto BUFFER_SIZE = 16*1024;
    int fd = open(fname, O_RDONLY);
    if(fd == -1)
        handle_error("open");

    /* Advise the kernel of our access pattern.  */
    posix_fadvise(fd, 0, 0, 1);  // FDADVICE_SEQUENTIAL

    char buf[BUFFER_SIZE + 1];
    uintmax_t lines = 0;

    long sentenceSum = 0;
    long sentenceNum = 0;
    long wordNum = 0;

    bool inBook = false;
    bool inHeader = false;

    std::vector<std::string> sentencebuf = {};
    std::vector<char> wordbuf = {};

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

            if (!inBook) {
                if (!inHeader) {
                    if (buf[i] == '*') {
                        // Potential start of header
                        if (i + START_STRING.size() < bytes_read) {
                            std::string potential_start(&buf[i], START_STRING.size());
                            if (potential_start == START_STRING) {
                                inHeader = true;
                                // std::cout << "Start of header\n";
                            }
                        } else if (i + END_STRING.size() < bytes_read) {
                            std::string potential_end(&buf[i], END_STRING.size());
                            if (potential_end == END_STRING) {
                                break;  // End of book reached
                                // std::cout << "End of book\n";
                            }
                        }
                    }
                } else {
                    if (buf[i] == '*') {
                        // Potential end of header
                        if (buf[i + 1] == '*' && buf[i + 2] == '*') {
                            inHeader = false;
                            inBook = true;
                            i += 2;  // Skip the next two asterisks
                            // std::cout << "End of header\n";
                        }
                    }
                }
                continue;
            } 

            bool isPunct = std::find(PUNCTUATION.begin(), PUNCTUATION.end(), buf[i]) != PUNCTUATION.end();

            if (isalnum(buf[i])) {
                wordbuf.push_back(buf[i]);
            } else if (buf[i] == ' ' || isPunct) {
                if (wordbuf.empty()) {
                    continue;
                }
                // std::cout << "Word: " << std::string(wordbuf.begin(), wordbuf.end()) << "\n";
                sentencebuf.push_back(std::string(wordbuf.begin(), wordbuf.end()));
                wordbuf.clear();
                if (isPunct) {
                    if (sentencebuf.empty() || sentencebuf.size() <= 1) {
                        continue;
                    }
                    ++sentenceNum;
                    sentenceSum += sentencebuf.size();
                    sentencebuf.clear();
                }
            } 
        }
    }

    std::cout << "Total sentences: " << sentenceNum << "\n";
    std::cout << "Average words per sentence: " << (sentenceNum ? (double)sentenceSum / sentenceNum : 0) << "\n";
    std::cout << "Total words: " << sentenceSum << "\n";

    std::cout << "Finished reading file\n";

    if(close(fd) == -1)
        handle_error("close failed");

    //std::cout << buf << "\n";

    return lines;
}

int main(int argc, char* argv[])
{

    uintmax_t m_numLines = wc(argv[1]);

    std::cout << "m_numLines = " << m_numLines << "\n";
}