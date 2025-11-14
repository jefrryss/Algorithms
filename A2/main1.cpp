#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <string>

using namespace std;

//Сортировки 

const int THRESHOLD = 50;

void insertion_sort(vector<long long>& a, int l, int r) {
    for (int i = l + 1; i < r; ++i) {
        long long key = a[i];
        int j = i - 1;
        while (j >= l && a[j] > key) {
            a[j + 1] = a[j];
            --j;
        }
        a[j + 1] = key;
    }
}

void merge_ranges(vector<long long>& a,
                  vector<long long>& tmp,
                  int l, int m, int r) {
    int i = l;
    int j = m;
    int k = l;

    while (i < m && j < r) {
        if (a[i] <= a[j]) {
            tmp[k++] = a[i++];
        } else {
            tmp[k++] = a[j++];
        }
    }
    while (i < m) tmp[k++] = a[i++];
    while (j < r) tmp[k++] = a[j++];

    for (int t = l; t < r; ++t) {
        a[t] = tmp[t];
    }
}

void merge_sort_rec(vector<long long>& a,
                    vector<long long>& tmp,
                    int l, int r) {
    int len = r - l;
    if (len <= 1) return;

    int m = l + len / 2;
    merge_sort_rec(a, tmp, l, m);
    merge_sort_rec(a, tmp, m, r);
    merge_ranges(a, tmp, l, m, r);
}

void merge_sort(vector<long long>& a) {
    if (a.size() <= 1) return;
    vector<long long> tmp(a.size());
    merge_sort_rec(a, tmp, 0, (int)a.size());
}

void merge_sort_hybrid_rec(vector<long long>& a,
                           vector<long long>& tmp,
                           int l, int r) {
    int len = r - l;
    if (len <= 1) return;

    if (len <= THRESHOLD) {
        insertion_sort(a, l, r);
        return;
    }

    int m = l + len / 2;
    merge_sort_hybrid_rec(a, tmp, l, m);
    merge_sort_hybrid_rec(a, tmp, m, r);
    merge_ranges(a, tmp, l, m, r);
}

void merge_sort_hybrid(vector<long long>& a) {
    if (a.size() <= 1) return;
    vector<long long> tmp(a.size());
    merge_sort_hybrid_rec(a, tmp, 0, (int)a.size());
}


class ArrayGenerator {
public:
    ArrayGenerator(int maxSize = 100000)
        : maxSize_(maxSize),
          randomBase_(maxSize),
          reversedBase_(maxSize),
          almostSortedBase_(maxSize)
    {
        mt19937_64 gen(42);
        uniform_int_distribution<long long> dist(0, 6000);

        for (int i = 0; i < maxSize_; ++i) {
            randomBase_[i] = dist(gen);
        }

        vector<long long> tmp = randomBase_;
        sort(tmp.begin(), tmp.end());             
        for (int i = 0; i < maxSize_; ++i) {
            reversedBase_[i] = tmp[maxSize_ - 1 - i]; 
        }

        almostSortedBase_ = tmp;                 
        int swapsCount = maxSize_ / 100;        
        if (swapsCount == 0 && maxSize_ > 1) {
            swapsCount = 1;
        }

        uniform_int_distribution<int> idxDist(0, maxSize_ - 1);
        for (int k = 0; k < swapsCount; ++k) {
            int i = idxDist(gen);
            int j = idxDist(gen);
            swap(almostSortedBase_[i], almostSortedBase_[j]);
        }
    }

    int maxSize() const { return maxSize_; }

    vector<long long> getRandomArray(int n) const {
        if (n > maxSize_) n = maxSize_;
        return vector<long long>(randomBase_.begin(),
                                 randomBase_.begin() + n);
    }

    vector<long long> getReversedArray(int n) const {
        if (n > maxSize_) n = maxSize_;
        return vector<long long>(reversedBase_.begin(),
                                 reversedBase_.begin() + n);
    }

    vector<long long> getAlmostSortedArray(int n) const {
        if (n > maxSize_) n = maxSize_;
        return vector<long long>(almostSortedBase_.begin(),
                                 almostSortedBase_.begin() + n);
    }

private:
    int maxSize_;
    vector<long long> randomBase_;
    vector<long long> reversedBase_;
    vector<long long> almostSortedBase_;
};


class SortTester {
public:
    SortTester(ArrayGenerator& gen, int repeats)
        : gen_(gen), repeats_(repeats) {}

    void run() {
        ofstream out("results.txt");
        if (!out) {
            return; 
        }

        out << "n pattern algo avg_time_micro\n";

        int maxN = gen_.maxSize();

        // random
        for (int n = 500; n <= maxN; n += 100) {
            test_one_size(out, n, "random");
        }
        // reversed
        for (int n = 500; n <= maxN; n += 100) {
            test_one_size(out, n, "reversed");
        }
        // almost
        for (int n = 500; n <= maxN; n += 100) {
            test_one_size(out, n, "almost");
        }
    }

private:
    ArrayGenerator& gen_;
    int repeats_;

    void test_one_size(ofstream& out, int n, const string& pattern) {
        auto getArray = [&](int size) -> vector<long long> {
            if (pattern == "random")   return gen_.getRandomArray(size);
            if (pattern == "reversed") return gen_.getReversedArray(size);
            return gen_.getAlmostSortedArray(size); 
        };

        long long totalStd = 0;
        for (int rep = 0; rep < repeats_; ++rep) {
            vector<long long> a = getArray(n);
            auto start = chrono::high_resolution_clock::now();
            merge_sort(a);
            auto end = chrono::high_resolution_clock::now();
            totalStd += chrono::duration_cast<
                chrono::microseconds>(end - start).count();
        }
        double avgStd = (double)totalStd / repeats_;
        out << n << " " << pattern << " merge " << avgStd << "\n";

        long long totalHybrid = 0;
        for (int rep = 0; rep < repeats_; ++rep) {
            vector<long long> a = getArray(n);
            auto start = chrono::high_resolution_clock::now();
            merge_sort_hybrid(a);
            auto end = chrono::high_resolution_clock::now();
            totalHybrid += chrono::duration_cast<
                chrono::microseconds>(end - start).count();
        }
        double avgHybrid = (double)totalHybrid / repeats_;
        out << n << " " << pattern << " hybrid " << avgHybrid << "\n";
    }
};


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ArrayGenerator gen(100000);   
    int repeats = 5;              

    SortTester tester(gen, repeats);
    tester.run();                 

    return 0; 
}
