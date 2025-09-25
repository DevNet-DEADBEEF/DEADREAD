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
        std::cout << "\n\nCHUNK:" << this_read << "\n";

        for (size_t i = 0; i < bytes_read; ++i) {
            if (isalpha(buf[i])) {
                wordbuf.push_back(buf[i]);
            } else if (buf[i] == ' ' || buf[i] == '.') {
                if (wordbuf.empty()) {
                    continue;
                }
                std::cout << "Word: " << std::string(wordbuf.begin(), wordbuf.end()) << "\n";
                sentencebuf.push_back(std::string(wordbuf.begin(), wordbuf.end()));
                if (!wordbuf.empty()) {
                    wordbuf.clear();
                }
                if (buf[i] == '.') {
                    sentenceNum++;
                    sentenceSum += sentencebuf.size();
                    if (!sentencebuf.empty()) {
                        sentencebuf.clear();
                    }
                }
            } 
        }
    }

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