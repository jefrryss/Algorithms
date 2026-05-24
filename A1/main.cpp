#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <stdexcept>
#include <chrono>
#include <functional>
#include <fstream>
#include <utility>

class StringGenerator {
private:
    std::mt19937 rng;
    
    const std::string alphabet = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz" 
        "0123456789"                 
        "!@#%:;^&*()-";           

    std::string generateSingleString() {
        std::uniform_int_distribution<size_t> lenDist(10, 200);
        std::uniform_int_distribution<size_t> charDist(0, alphabet.size() - 1);
        
        size_t len = lenDist(rng);
        std::string s(len, ' ');
        for (size_t i = 0; i < len; ++i) {
            s[i] = alphabet[charDist(rng)];
        }
        return s;
    }

public:
    explicit StringGenerator(uint32_t seed = 42) : rng(seed) {}

    std::vector<std::string> generateRandom(size_t size) {
        std::vector<std::string> res(size);
        for (size_t i = 0; i < size; ++i) {
            res[i] = generateSingleString();
        }
        return res;
    }

    std::vector<std::string> generateReversed(size_t size) {
        auto res = generateRandom(size);
        std::sort(res.begin(), res.end(), std::greater<std::string>());
        return res;
    }

    std::vector<std::string> generateNearlySorted(size_t size, int swaps) {
        auto res = generateRandom(size);
        std::sort(res.begin(), res.end());
        
        if (size < 2) return res;

        std::uniform_int_distribution<size_t> indexDist(0, size - 1);
        for (int i = 0; i < swaps; ++i) {
            size_t idx1 = indexDist(rng);
            size_t idx2 = indexDist(rng);
            std::swap(res[idx1], res[idx2]);
        }
        return res;
    }

    std::vector<std::string> generateWithSharedPrefix(size_t size, const std::string& prefix) {
        std::vector<std::string> res(size);
        std::uniform_int_distribution<size_t> charDist(0, alphabet.size() - 1);

        for (size_t i = 0; i < size; ++i) {
            size_t maxSuffixLen = (200 > prefix.length()) ? 200 - prefix.length() : 0;
            size_t minSuffixLen = (10 > prefix.length()) ? 10 - prefix.length() : 0;
            
            std::uniform_int_distribution<size_t> suffixDist(minSuffixLen, maxSuffixLen);
            size_t suffixLen = suffixDist(rng);
            
            std::string s = prefix;
            for (size_t j = 0; j < suffixLen; ++j) {
                s += alphabet[charDist(rng)];
            }
            res[i] = s;
        }
        std::shuffle(res.begin(), res.end(), rng);
        return res;
    }

    static std::vector<std::string> getSubArray(const std::vector<std::string>& source, size_t newSize) {
        if (newSize > source.size()) throw std::invalid_argument("Size error");
        return std::vector<std::string>(source.begin(), source.begin() + newSize);
    }
};



struct SortMetrics {
    double averageTimeMicroseconds;
    uint64_t averageCharComparisons;
};

class StringSortTester {
private:
    int numRuns;

public:
    StringSortTester(int runs = 5) : numRuns(runs) {}

    SortMetrics testSort(
        const std::vector<std::string>& data, 
        std::function<void(std::vector<std::string>&, uint64_t&)> sortFunction) 
    {
        uint64_t totalComparisons = 0;
        double totalTimeUs = 0;

        for (int i = 0; i < numRuns; ++i) {
            std::vector<std::string> dataCopy = data;
            uint64_t currentComparisons = 0;

            auto start = std::chrono::high_resolution_clock::now();
            sortFunction(dataCopy, currentComparisons);
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double, std::micro> elapsed = end - start;
            totalTimeUs += elapsed.count();
            totalComparisons += currentComparisons;
        }

        return {
            totalTimeUs / numRuns,
            totalComparisons / (uint64_t)numRuns
        };
    }
};

class StandardSorts {
private:
    static bool countCharCompares(const std::string& a, const std::string& b, uint64_t& comparesCount) {
        size_t minLen = std::min(a.length(), b.length());
        for (size_t i = 0; i < minLen; ++i) {
            comparesCount++;
            if (a[i] != b[i]) return a[i] < b[i];
        }
        comparesCount++; 
        return a.length() < b.length();
    }

    static void merge(std::vector<std::string>& arr, size_t left, size_t mid, size_t right, uint64_t& comparesCount, std::vector<std::string>& temp) {
        size_t i = left, j = mid + 1, k = left;
        while (i <= mid && j <= right) {
            if (countCharCompares(arr[i], arr[j], comparesCount)) temp[k++] = std::move(arr[i++]);
            else temp[k++] = std::move(arr[j++]);
        }
        while (i <= mid) temp[k++] = std::move(arr[i++]);
        while (j <= right) temp[k++] = std::move(arr[j++]);
        for (size_t p = left; p <= right; ++p) arr[p] = std::move(temp[p]);
    }

    static void mergeSortRec(std::vector<std::string>& arr, size_t left, size_t right, uint64_t& comparesCount, std::vector<std::string>& temp) {
        if (left >= right) return;
        size_t mid = left + (right - left) / 2;
        mergeSortRec(arr, left, mid, comparesCount, temp);
        mergeSortRec(arr, mid + 1, right, comparesCount, temp);
        merge(arr, left, mid, right, comparesCount, temp);
    }

    static size_t partition(std::vector<std::string>& arr, size_t left, size_t right, uint64_t& comparesCount) {
        size_t mid = left + (right - left) / 2;
        std::swap(arr[mid], arr[right]);
        
        const std::string& pivot = arr[right];
        size_t i = left;

        for (size_t j = left; j < right; ++j) {
            if (countCharCompares(arr[j], pivot, comparesCount)) {
                std::swap(arr[i], arr[j]);
                i++;
            }
        }
        std::swap(arr[i], arr[right]);
        return i;
    }

    static void quickSortRec(std::vector<std::string>& arr, size_t left, size_t right, uint64_t& comparesCount) {
        if (left >= right) return;
        size_t pi = partition(arr, left, right, comparesCount);
        if (pi > 0) quickSortRec(arr, left, pi - 1, comparesCount);
        quickSortRec(arr, pi + 1, right, comparesCount);
    }

public:
    static void MergeSort(std::vector<std::string>& arr, uint64_t& comparesCount) {
        if (arr.empty()) return;
        std::vector<std::string> temp(arr.size());
        mergeSortRec(arr, 0, arr.size() - 1, comparesCount, temp);
    }

    static void QuickSort(std::vector<std::string>& arr, uint64_t& comparesCount) {
        if (arr.empty()) return;
        quickSortRec(arr, 0, arr.size() - 1, comparesCount);
    }
};

class AdaptedSorts {
private:
    struct LcpItem {
        std::string s;
        size_t lcp;
    };

    struct LcpResult {
        char cmp;
        size_t h;
    };

    static LcpResult lcpCompare(const std::string& a, const std::string& b, size_t k, uint64_t& comps) {
        size_t minLen = std::min(a.length(), b.length());
        size_t h = k;
        while (h < minLen) {
            comps++;
            if (a[h] != b[h]) {
                return { a[h] < b[h] ? '<' : '>', h };
            }
            h++;
        }
        comps++; 
        if (a.length() < b.length()) return {'<', h};
        if (a.length() > b.length()) return {'>', h};
        return {'=', h};
    }

    static void stringMerge(std::vector<LcpItem>& arr, size_t left, size_t mid, size_t right, uint64_t& comps, std::vector<LcpItem>& temp) {
        size_t i = left;
        size_t j = mid + 1;
        size_t k_idx = left;

        while (i <= mid && j <= right) {
            if (arr[i].lcp > arr[j].lcp) {
                temp[k_idx++] = std::move(arr[i++]);
            } else if (arr[i].lcp < arr[j].lcp) {
                temp[k_idx++] = std::move(arr[j++]);
            } else {
                LcpResult res = lcpCompare(arr[i].s, arr[j].s, arr[i].lcp, comps);
                if (res.cmp == '<') {
                    arr[j].lcp = res.h;
                    temp[k_idx++] = std::move(arr[i++]);
                } else {
                    arr[i].lcp = res.h;
                    temp[k_idx++] = std::move(arr[j++]);
                }
            }
        }

        while (i <= mid) temp[k_idx++] = std::move(arr[i++]);
        while (j <= right) temp[k_idx++] = std::move(arr[j++]);

        for (size_t p = left; p <= right; ++p) {
            arr[p] = std::move(temp[p]);
        }
    }

    static void lcpMergeSortRec(std::vector<LcpItem>& arr, size_t left, size_t right, uint64_t& comps, std::vector<LcpItem>& temp) {
        if (left >= right) return;
        size_t mid = left + (right - left) / 2;
        lcpMergeSortRec(arr, left, mid, comps, temp);
        lcpMergeSortRec(arr, mid + 1, right, comps, temp);
        stringMerge(arr, left, mid, right, comps, temp);
    }

    static int charToIndex(char c) {
        static int map[256] = {0};
        static bool init = false;
        if (!init) {
            std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#%:;^&*()-";
            for (size_t i = 0; i < alphabet.size(); ++i) {
                map[(unsigned char)alphabet[i]] = i + 1;
            }
            init = true;
        }
        return map[(unsigned char)c];
    }

    static void tsqs(std::vector<std::string>& a, int lo, int hi, int d, uint64_t& comps) {
        if (hi <= lo) return;
        int mid = lo + (hi - lo) / 2;
        std::swap(a[lo], a[mid]);
        
        int v = -1;
        if (d < a[lo].length()) v = a[lo][d];
        
        int lt = lo, gt = hi, i = lo + 1;
        
        while (i <= gt) {
            int c = -1;
            if (d < a[i].length()) c = a[i][d];
            comps++; 
            
            if (c < v) std::swap(a[lt++], a[i++]);
            else if (c > v) std::swap(a[i], a[gt--]);
            else i++;
        }
        
        tsqs(a, lo, lt - 1, d, comps);
        if (v >= 0) tsqs(a, lt, gt, d + 1, comps);
        tsqs(a, gt + 1, hi, d, comps);
    }

    static void msd(std::vector<std::string>& a, int lo, int hi, int d, std::vector<std::string>& aux, uint64_t& comps) {
        if (hi <= lo) return;
        const int R = 74; 
        std::vector<int> count(R + 2, 0);
        
        for (int i = lo; i <= hi; i++) {
            comps++;
            int c = (d < a[i].length()) ? charToIndex(a[i][d]) : 0;
            count[c + 1]++;
        }
        
        for (int r = 0; r < R + 1; r++) count[r + 1] += count[r];
        
        for (int i = lo; i <= hi; i++) {
            int c = (d < a[i].length()) ? charToIndex(a[i][d]) : 0;
            aux[count[c]++] = std::move(a[i]);
        }
        
        for (int i = lo; i <= hi; i++) a[i] = std::move(aux[i - lo]);
        for (int r = 0; r < R + 1; r++) msd(a, lo + count[r], lo + count[r + 1] - 1, d + 1, aux, comps);
    }

    static void msdHybrid(std::vector<std::string>& a, int lo, int hi, int d, std::vector<std::string>& aux, uint64_t& comps) {
        if (hi <= lo) return;
        if (hi - lo + 1 <= 74) { 
            tsqs(a, lo, hi, d, comps);
            return;
        }
        
        const int R = 74;
        std::vector<int> count(R + 2, 0);
        
        for (int i = lo; i <= hi; i++) {
            comps++; 
            int c = (d < a[i].length()) ? charToIndex(a[i][d]) : 0;
            count[c + 1]++;
        }
        
        for (int r = 0; r < R + 1; r++) count[r + 1] += count[r];
        
        for (int i = lo; i <= hi; i++) {
            int c = (d < a[i].length()) ? charToIndex(a[i][d]) : 0;
            aux[count[c]++] = std::move(a[i]);
        }
        
        for (int i = lo; i <= hi; i++) a[i] = std::move(aux[i - lo]);
        for (int r = 0; r < R + 1; r++) msdHybrid(a, lo + count[r], lo + count[r + 1] - 1, d + 1, aux, comps);
    }

public:
    static void TernaryStringQuickSort(std::vector<std::string>& arr, uint64_t& comparesCount) {
        if (arr.empty()) return;
        tsqs(arr, 0, arr.size() - 1, 0, comparesCount);
    }

    static void MSDRadixSort(std::vector<std::string>& arr, uint64_t& comparesCount) {
        if (arr.empty()) return;
        std::vector<std::string> aux(arr.size());
        msd(arr, 0, arr.size() - 1, 0, aux, comparesCount);
    }

    static void MSDRadixSortHybrid(std::vector<std::string>& arr, uint64_t& comparesCount) {
        if (arr.empty()) return;
        std::vector<std::string> aux(arr.size());
        msdHybrid(arr, 0, arr.size() - 1, 0, aux, comparesCount);
    }

    static void StringMergeSort(std::vector<std::string>& arr, uint64_t& comparesCount) {
        if (arr.empty()) return;
        
        std::vector<LcpItem> lcpArr(arr.size());
        for (size_t i = 0; i < arr.size(); ++i) {
            lcpArr[i] = {std::move(arr[i]), 0};
        }
        
        std::vector<LcpItem> temp(arr.size());
        lcpMergeSortRec(lcpArr, 0, arr.size() - 1, comparesCount, temp);
        
        for (size_t i = 0; i < arr.size(); ++i) {
            arr[i] = std::move(lcpArr[i].s);
        }
    }
};

void runTests() {
    StringGenerator gen(42);
    StringSortTester tester(5); 
    const size_t MAX_SIZE = 3000;

    auto baseRandom = gen.generateRandom(MAX_SIZE);
    auto baseReversed = gen.generateReversed(MAX_SIZE);
    auto baseNearlySorted = gen.generateNearlySorted(MAX_SIZE, 50);
    auto basePrefixed = gen.generateWithSharedPrefix(MAX_SIZE, "COMMON_PREFIX_TESTING_DATA_");

    std::ofstream outFile("results.csv");
    outFile << "Size,ArrayType,Algorithm,TimeUs,Comparisons\n";

    std::vector<std::string> types = {"Random", "Reversed", "NearlySorted", "Prefixed"};
    std::vector<std::vector<std::string>> baseArrays = {baseRandom, baseReversed, baseNearlySorted, basePrefixed};
    
    std::vector<std::string> algoNames = {
        "Std_QuickSort", "Std_MergeSort", "Ternary_QS", "LCP_MergeSort", "MSD_Radix", "MSD_Hybrid"
    };
    std::vector<std::function<void(std::vector<std::string>&, uint64_t&)>> algos = {
        StandardSorts::QuickSort,
        StandardSorts::MergeSort,
        AdaptedSorts::TernaryStringQuickSort,
        AdaptedSorts::StringMergeSort,
        AdaptedSorts::MSDRadixSort,
        AdaptedSorts::MSDRadixSortHybrid
    };

    for (size_t size = 100; size <= MAX_SIZE; size += 100) {
        std::cout << "Testing size: " << size << "...\n";
        for (size_t t = 0; t < types.size(); ++t) {
            auto testArray = StringGenerator::getSubArray(baseArrays[t], size);
            
            for (size_t a = 0; a < algos.size(); ++a) {
                SortMetrics metrics = tester.testSort(testArray, algos[a]);
                outFile << size << "," 
                        << types[t] << "," 
                        << algoNames[a] << "," 
                        << metrics.averageTimeMicroseconds << "," 
                        << metrics.averageCharComparisons << "\n";
            }
        }
    }
    outFile.close();
    std::cout << "финиш. results.csv\n";
}

int main() {
    runTests();
    return 0;
}
