#include <iostream>
#include <vector>

using namespace std;

void vec_insert(std::vector<int> &vec, int pos, int val) {
    vec.insert(vec.begin() + pos, val);
    vec.erase(vec.end() - 1, vec.end());
}

void print_vec(vector<int> &vec) {
    for (auto v : vec)
        cout << v << " ";
    cout << endl;
}

int main() {
    vector v = {1, 2, 3, 4, 5};
    print_vec(v);

    vec_insert(v, 0, 7);
    print_vec(v);

    vec_insert(v, 4, 8);
    print_vec(v);

    return 0;
}