#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>

/**
 * Checks if a set S satisfies the Lambda-set condition:
 * The number of times any distance d occurs is <= d - 1.
 */
bool is_lambda_set(const std::vector<int>& S) {
    if (S.size() < 2) return true;
    
    std::map<int, int> diff_counts;
    for (size_t i = 0; i < S.size(); ++i) {
        for (size_t j = i + 1; j < S.size(); ++j) {
            int d = std::abs(S[j] - S[i]);
            diff_counts[d]++;
            if (diff_counts[d] > d - 1) {
                return false;
            }
        }
    }
    return true;
}

bool backtrack(std::vector<int>& curr_set, int m, int limit, int start_val) {
    if (curr_set.size() == m) {
        return true;
    }

    for (int v = start_val; v <= limit; ++v) {
        curr_set.push_back(v);
        if (is_lambda_set(curr_set)) {
            if (backtrack(curr_set, m, limit, v + 1)) {
                return true;
            }
        }
        curr_set.pop_back();
    }
    return false;
}

/**
 * Finds the minimum diameter (W(m)) for a Lambda-set of size m.
 */
std::pair<std::vector<int>, int> find_optimal_lambda_set(int m) {
    if (m <= 0) return {{}, 0};
    if (m == 1) return {{0}, 0};

    // Iteratively increase the diameter limit until a solution is found
    for (int limit = 0; limit < m * m; ++limit) {
        std::vector<int> curr_set = {0};
        if (backtrack(curr_set, m, limit, 1)) {
            return {curr_set, limit};
        }
    }
    return {{}, -1};
}

int main() {
    std::cout << std::left << std::setw(4) << "m" 
              << " | " << std::setw(15) << "Diameter W(m)" 
              << " | " << "Optimal Set" << std::endl;
    std::cout << std::string(45, '-') << std::endl;

    for (int m = 12; m <= 13; ++m) {
        auto result = find_optimal_lambda_set(m);
        std::vector<int> opt_set = result.first;
        int opt_diam = result.second;

        std::cout << std::left << std::setw(4) << m 
                  << " | " << std::setw(15) << opt_diam 
                  << " | [";
        
        for (size_t i = 0; i < opt_set.size(); ++i) {
            std::cout << opt_set[i] << (i == opt_set.size() - 1 ? "" : ", ");
        }
        std::cout << "]" << std::endl;
    }

    return 0;
}