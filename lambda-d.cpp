#include <iostream>
#include <vector>

class DifferenceSetFinder {
private:
    std::vector<int> mSet;
    std::vector<int> mDiffCounts;

    bool backtrack(int size, int n, int limit, int startVal) {
        if (size == n) {
            return true;
        }

        int remaining = n - size;
        int maxV = limit - (remaining - 1);

        for (int v = startVal; v <= maxV; ++v) {
            bool ok = true;

            for (int i = 0; i < size; ++i) {
                int d = v - mSet[i];
                if (mDiffCounts[d] + 1 > d) { 
                    ok = false;
                    break;
                }
            }

            if (!ok) continue;

            for (int i = 0; i < size; ++i) {
                int d = v - mSet[i];
                mDiffCounts[d]++;
            }
            mSet[size] = v;

            if (backtrack(size + 1, n, limit, v + 1)) {
                return true;
            }

            for (int i = 0; i < size; ++i) {
                int d = v - mSet[i];
                mDiffCounts[d]--;
            }
        }
        return false;
    }

public:
    void solve(int n) {
        if (n <= 0) return;
        if (n == 1) {
            std::cout << "1\t0\t{0}" << std::endl;
            return;
        }

        for (int limit = n - 1; limit < n * n; ++limit) {
            mSet.assign(n, 0);
            mDiffCounts.assign(limit + 1, 0);
            
            mSet[0] = 0; 
            if (backtrack(1, n, limit, 1)) {
                std::cout << n << "\t" << limit << "\t{";
                for (int i = 0; i < n; ++i) {
                    std::cout << mSet[i] << (i == n - 1 ? "" : ", ");
                }
                std::cout << "}" << std::endl;
                return;
            }
        }
    }
};

int main() {
    DifferenceSetFinder finder;
    
    std::cout << "n\tDiam\tSet" << std::endl;
    std::cout << "------------------------------------" << std::endl;
    
    for (int n = 1; n <= 15; ++n) {
        finder.solve(n);
    }
    
    return 0;
}