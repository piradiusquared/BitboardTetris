#include <iostream>
#include <random>
#include <time.h>
#include <bits/stdc++.h>
#include <deque>

using namespace std;

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int main() {
    srand(time(NULL));

    deque<int> out;

    int test[7] = {0, 1, 2, 3, 4, 5, 6};

    for (int i = 6; i >= 0; i--) {
        int j = rand() % (i + 1);
        swap(&test[i], &test[j]);
    }

    for (auto i : test) {
        out.push_back(i);
    }

    cout << out.front();
    out.pop_front();
    cout << out.front();
    return 0;
}