#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <mutex>

using namespace std;
using namespace std::chrono;

struct UnionFind {
    int *sets;
    int size;

    // Constructor
    UnionFind(int size) : size(size) {
        // Initialize all values as roots.
        // We denote a root by making it a negative value
        // and its weight as its absolute value
        sets = new int[size];
        for (int i = 0; i < size; i++) sets[i] = -1;
    }

    // Destructor to avoid memory leaks
    ~UnionFind() {
        delete[] sets;
    }

    // Collapse find
    int Find(int val) {
        if (sets[val] < 0) return val;
        
        // Follow the pointers inside the elements until we find the root
        // once we find the root, we communicate this finding thorughout the 
        // values we visited. This way finding the root again will become easier
        int root = Find(sets[val]);
        sets[val] = root;
        return root;
    }

    // Weighted union
    bool Union(int A, int B) {
        int rootA = Find(A);
        int rootB = Find(B);
        
        // The two sets are not unifiable because 
        // it will create a cycle
        if (rootA == rootB) return false;

        // The sets are unifiable. 
        // We then find the set with the larger size 
        // and use its root as the root of our new unified set
        if (sets[rootA] <= sets[rootB]) {
            sets[rootA] += sets[rootB];
            sets[rootB] = rootA;
        } else {
            sets[rootB] += sets[rootA];
            sets[rootA] = rootB;
        }
        return true;
    }
};

vector<vector<int>> maintainBridges(vector<vector<int>> bridges, int numIslands) {
    vector<vector<int>> to_maintain;
    // Initialize the struct
    int size = bridges.size();
    UnionFind uf(numIslands);

    // Try to unify the islands connected by a bridge
    // If they can be unified then the can be maintained
    for (int i = 0; i < size; i++) {
        vector<int> bridge = bridges[i];

        const bool is_unifiable = uf.Union(bridge[0], bridge[1]);
        if (is_unifiable) to_maintain.push_back(bridge);
    }

    return to_maintain;
}

void printVectorOfVectors(const vector<vector<int>>& vec) {
    for (const auto& innerVec : vec) {
        for (int val : innerVec) {
            cout << val << " ";
        }
        cout << endl;
    }
}

vector<vector<int>> generateRandomBridges(const int numIslands, const int targetNumOfBridges) {
    vector<vector<bool>> visited(numIslands, vector<bool>(numIslands, false));
    vector<vector<int>> found;  
    // The random generator
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> distr(0, numIslands - 1);
    // Generate a random bridge and check if they are already used
    while (found.size() != targetNumOfBridges) {
        const int start = distr(gen);
        const int end  = distr(gen);

        if (start == end || visited[start][end]) continue;

        visited[start][end] = true;
        visited[end][start] = true; 
        found.push_back({start, end});
    }

    return found;
} 

// Function to measure the execution time of another function
template <typename Func, typename... Args>
duration<double> measureExecutionTime(Func func, Args&&... args) {
    auto start = high_resolution_clock::now();
    auto result = func(forward<Args>(args)...);
    auto end = high_resolution_clock::now();
    duration<double> duration = end - start;
    return duration;
}

void testTimeForKIslandsChunk(const int numIslands, const int start, const int end, duration<double>& totalAverageTime, mutex& mtx) {
    duration<double> localTotalTime;
    const int step = 1;
    
    for (int i = start; i <= end; i += step) {
        vector<vector<int>> bridges = generateRandomBridges(numIslands, i);
        localTotalTime += measureExecutionTime(maintainBridges, bridges, numIslands);
    }
    lock_guard<mutex> lock(mtx);
    totalAverageTime += localTotalTime;
}

// Test the time complexity for maintainBridges given a number of island
// and a range of all possible number of bridges
// Get the time complexity for each combination then the average. 
duration<double> testTimeForKIslands(const int numIslands, const int start, const int end) {
    const int numThreads = thread::hardware_concurrency();
    int chunkSize = (end - start + 1) / numThreads;
    vector<thread> threads;
    duration<double> totalAverageTime;
    mutex mtx;

    for (int i = 0; i < numThreads; ++i) {
        int chunkStart = start + i * chunkSize;
        int chunkEnd = (i == numThreads - 1) ? end : chunkStart + chunkSize - 1;
        threads.emplace_back(testTimeForKIslandsChunk, numIslands, chunkStart, chunkEnd, ref(totalAverageTime), ref(mtx));
    }

    for (auto& th : threads) {
        th.join();
    }

    return totalAverageTime/ (end - start + 1);
}

vector<vector<vector<double>>> timeComplexityForAllCases(const int numIslands, const int step){
    if (step == 0) return {};
    vector<vector<vector<double>>> res(3);

    double split[] = {0, 1};

    for (int i = 0; i < 1; i++){
        int current = step < 5 ? 5 : step;
        while (current < numIslands){
            const int rangeBridges = (current * (current - 1))/2;
            duration<double> time = testTimeForKIslands(current, (int) rangeBridges * split[i], (int) rangeBridges * split[i + 1]);
            cout << current << " | " << time.count() << endl;
            res[i].push_back({static_cast<double>(current), time.count()}); 
            cout << "pushed" << endl;
            current += step;

        /*
        vector<vector<int>> bridges = generateRandomBridges(numIslands, current);
        duration<double> time = measureExecutionTime(maintainBridges, bridges, numIslands);
        res[0].push_back({static_cast<double>(current), time.count()});

        current += step;
        */
        }
    }
    
    return res;
}


int main() {
    int numIslands;
    int step;

    cout << "Number Of Islands" << '\n';
    cin >> numIslands;
    cout << "Steps" << '\n';
    cin >> step;

    vector<vector<vector<double>>> res = timeComplexityForAllCases(numIslands, step);

    // Print result so it can be copied as a python list
    cout << "[";
    for (int i = 0; i < res.size(); ++i) {
        if (i > 0) {
            cout << ", ";
        }
        cout << "[";
        for (int j = 0; j < res[i].size(); ++j) {
            if (j > 0) {
                cout << ", ";
            }
            cout << "[";
            for (int k = 0; k < res[i][j].size(); ++k) {
                if (k > 0) {
                    cout << ", ";
                }
                if (res[i][j][k] == INFINITY) cout << "float('inf')";
                else cout << res[i][j][k];
            }
            cout << "]";
        }
        cout << "]";
    }
    cout << "]" << endl;

    return 0;
}