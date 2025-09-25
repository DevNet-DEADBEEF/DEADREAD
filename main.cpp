#include <algorithm>
#include <iostream>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <memory>
#include <chrono>

using namespace std;

const char* read_file(const char* fname);
const char* c_read_file(char* filename);
const char* c_read_file_dynamic(char* fname);

// for mmap:
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h> // stat

using namespace std;
using namespace chrono;

const char* map_file(const char* fname, size_t& length);
vector<string> top_five(const unordered_map<string, int>& words);

const string start_string = "*** START OF THIS PROJECT GUTENBERG EBOOK ";
const string end_string = "*** END OF THIS PROJECT GUTENBERG EBOOK ";
const string sentence_end_chars = ".!?";
const string word_deilmiters = " \n\t\r,;:\"'()[]{}<>-";


double times2double(time_point<high_resolution_clock> start, time_point<high_resolution_clock> end) {
    return duration_cast<duration<double>>(end - start).count() * 1000.0;
}

time_point<high_resolution_clock> time_now() {
    return high_resolution_clock::now();
}

int main(int argc, char* argv[])
{
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    bool quiet = false;
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <filename> [-q]" << endl;
        return -1;
    }
    if (argc == 3 && string(argv[2]) == "-q") {
        // quiet mode, dont print
        quiet = true;
    }

    auto t1 = time_now();
    //size_t length;
    //auto file_data = read_file(argv[1], length);
    const char* raw_file_data = read_file(argv[1]);
    string file_data(raw_file_data);
    free((void*)raw_file_data);

    if (file_data.empty()) {
        cerr << "Error reading file: " << argv[1] << endl;
        return -1;
    }
    //auto f = file_data.get();
    //auto l = f + length;

    unsigned long sum_sentances = 0;
    int num_sentences = 0;
    std::unordered_map<string, int> word_count;

    vector<string> buffer;
    string current_word;
    auto t2 = time_now();
    if (!quiet)
        clog << "File opened in " << times2double(t1, t2) << "ms" << endl;
    t1 = time_now();

    // loop over the file
    for (long long unsigned int i = 0; i < file_data.length(); i++) { // why does length use a long long unsigned int...
        char c = file_data[i];

        // skip until start string is found
        if (file_data.substr(i, start_string.size()) == start_string) {
            i += start_string.size();
            continue;
        }

        // stop when end string is found
        if (file_data.substr(i, end_string.size()) == end_string) {
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
    t2 = time_now();
    if (!quiet)
        clog << "File processed in " << times2double(t1, t2) << "ms" << endl;
    t1 = time_now();

    // file_data will be **NOT** automatically cleaned up when it goes out of scope
    file_data.clear();

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
    t2 = time_now();
    if (!quiet)
        clog << "Results computed in " << times2double(t1, t2) << "ms" << endl;

    return 0;
}

void handle_error(const char* msg) {
    cerr << "Error: " << msg << endl;
    exit(255);
}

const char* read_file(const char* fname) {
    size_t length;
    ifstream file(fname, ios::binary | ios::ate);
    if (!file) {
        handle_error("Could not open file");
    }

    length = static_cast<size_t>(file.tellg());
    file.seekg(0, ios::beg);

    char* buffer = new char[length];
    if (!file.read(buffer, static_cast<streamsize>(length))) {
        delete[] buffer;
        handle_error("Could not read file");
    }

    return buffer;
}

const int BUFSIZE = 4096;
const char* c_read_file(char* filename) {
	// Get file and verify access
	FILE* fp = fopen(filename, "r");


	if (fp != NULL) { // File exists and is readable
		char buffer[BUFSIZE * sizeof(char)];
		char* output = NULL;
		int total = 0;

		while (!feof(fp)) {  // Until the end of the file
			int read = fread(buffer, sizeof(char), BUFSIZE, fp);  // Read the next BUFSIZE characters from the file
			output = (char*)realloc(output, total + read);   // Note:  skipping error handling from realloc!
			memcpy(output + total, buffer, read);  // Copy what was read to the end of the output buffer
			total += read;  // Keep track of the total read
		}
		fclose(fp);

		output = (char*)realloc(output, total + 1);   // Note:  skipping error handling from realloc!
		output[total] = '\0';   // Null-terminate the string
		//filelen = strlen(output);

		return output;
	}
	else { // File does not exist or cannot be read from
		printf("Unable to access BLARG! source file '%s'", filename);
		exit(-1);
		return NULL;
	}
}

long get_file_size(char *filename) {
    struct stat file_status;
    if (stat(filename, &file_status) < 0) {
        return -1;
    }

    return file_status.st_size;
}

const char* c_read_file_dynamic(char* filename) {
	// Get file and verify access
	FILE* fp = fopen(filename, "r");


	if (fp != NULL) { // File exists and is readable
        const long bufsize = get_file_size(filename);
		char buffer[bufsize * sizeof(char)];
		char* output = NULL;
		int total = 0;

		while (!feof(fp)) {  // Until the end of the file
			int read = fread(buffer, sizeof(char), bufsize, fp);  // Read the next BUFSIZE characters from the file
			output = (char*)realloc(output, total + read);   // Note:  skipping error handling from realloc!
			memcpy(output + total, buffer, read);  // Copy what was read to the end of the output buffer
			total += read;  // Keep track of the total read
		}
		fclose(fp);

		output = (char*)realloc(output, total + 1);   // Note:  skipping error handling from realloc!
		output[total] = '\0';   // Null-terminate the string
		//filelen = strlen(output);

		return output;
	}
	else { // File does not exist or cannot be read from
		printf("Unable to access BLARG! source file '%s'", filename);
		exit(-1);
		return NULL;
	}
}

vector<string> top_five(const unordered_map<string, int>& words) {
    // Create vector of pairs (count, word) for sorting
    vector<pair<int, string>> word_counts;
    word_counts.reserve(words.size());
    
    for (const auto& pair : words) {
        word_counts.emplace_back(pair.second, pair.first);
    }
    
    // Sort by count (descending) - only need top 5
    partial_sort(word_counts.begin(), 
                 word_counts.begin() + min(5, static_cast<int>(word_counts.size())), 
                 word_counts.end(), 
                 greater<pair<int, string>>());
    
    // Extract just the words
    vector<string> result;
    const int num_words = min(5, static_cast<int>(word_counts.size()));
    result.reserve(num_words);
    
    for (int i = 0; i < num_words; ++i) {
        result.push_back(word_counts[i].second);
    }
    
    return result;
}