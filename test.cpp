#include <iostream>
#include <random>
#include <time.h>
#include <bits/stdc++.h>


using namespace std;


int main() {
    srand(time(NULL));

    vector<int> test = {0, 1, 2, 3, 4, 5, 6};

    for (int i = test.size() - 1; i >= 0; i--) {
        int j = rand() % (i + 1);
        swap(test[i], test[j]);
    }

    for (auto i : test) {
        cout << i << " ";
    }
    

    return 0;
}