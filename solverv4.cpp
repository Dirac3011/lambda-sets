#include <iostream>
#include <vector>
#include <chrono>
#include <atomic>
#include <omp.h>
#include <mutex>
#include <fstream>
#include <string>
#include <algorithm>
#include <filesystem>

using namespace std;

using bitmask = __uint128_t;

struct Result {
    int g;
    int n;
    int alpha;
    double time;
    vector<int> marks;
};

mutex print_mutex;
atomic<bool> found_solution(false);
vector<int> global_best_marks;

const int MAX_G = 50;
const int MAX_ALPHA = 127;

const int i_val = 12;
const int g_start = 26;
const int g_stop = 50;

inline bitmask bit_at(int pos) {
    return (static_cast<bitmask>(1) << pos);
}

int lower_bound_alpha(int g, int b) {
    return g + 2 * b - 2;
}

bool is_strictly_increasing(const vector<int>& marks) {
    for (size_t i = 1; i < marks.size(); ++i) {
        if (marks[i] <= marks[i - 1]) return false;
    }
    return true;
}

int max_rep_count(const vector<int>& marks) {
    if (marks.size() < 2) return 0;

    int alpha = marks.back() - marks.front();
    vector<int> counts(alpha + 1, 0);

    int best = 0;
    for (size_t i = 0; i < marks.size(); ++i) {
        for (size_t j = i + 1; j < marks.size(); ++j) {
            int d = marks[j] - marks[i];
            best = max(best, ++counts[d]);
        }
    }

    return best;
}

bool is_g_golomb(const vector<int>& marks, int g) {
    if (marks.empty()) return false;
    if (!is_strictly_increasing(marks)) return false;
    return max_rep_count(marks) <= g;
}

void backtrack(int* current_set, int size,
               const bitmask* seen, bitmask rev_mask,
               const int n_target, const int alpha, const int g) {

    if (found_solution.load(memory_order_acquire)) return;

    const int last_val = current_set[size - 1];
    const int remaining = n_target - size;

    // Need room for exactly `remaining` more increasing marks, ending at alpha.
    if (last_val + remaining > alpha) return;

    // Final mark is forced to be alpha.
    if (size == n_target - 1) {
        const bitmask final_diffs = (rev_mask >> (MAX_ALPHA - alpha));

        if (!(seen[g] & final_diffs)) {
            vector<int> candidate(current_set, current_set + size);
            candidate.push_back(alpha);

            // Independent verification before saving.
            if (!is_g_golomb(candidate, g)) return;

            lock_guard<mutex> lock(print_mutex);
            if (!found_solution.load(memory_order_relaxed)) {
                global_best_marks = candidate;
                found_solution.store(true, memory_order_release);
            }
        }
        return;
    }


    const int max_cand = alpha - remaining + 1;

    for (int cand = last_val + 1; cand <= max_cand; ++cand) {
        if (found_solution.load(memory_order_acquire)) return;

        const bitmask new_diffs = (rev_mask >> (MAX_ALPHA - cand));

        // If this candidate makes some distance appear g+1 times, skip it.
        if (seen[g] & new_diffs) continue;

        bitmask next_seen[MAX_G + 1] = {};

        for (int i = 1; i <= g; ++i) {
            next_seen[i] = seen[i];
        }

        for (int j = g; j >= 2; --j) {
            next_seen[j] |= (next_seen[j - 1] & new_diffs);
        }

        next_seen[1] |= new_diffs;

        const bitmask next_rev = rev_mask | bit_at(MAX_ALPHA - cand);

        current_set[size] = cand;

        backtrack(current_set, size + 1, next_seen, next_rev, n_target, alpha, g);
    }
}

bool try_endpoint_extension(const Result& prev, int g, int b, Result& out_result) {
    if (prev.g + 1 != g) return false;

    const int n = g + b;
    const int lb = lower_bound_alpha(g, b);

    vector<int> extended = prev.marks;
    extended.push_back(prev.alpha + 1);

    if ((int)extended.size() != n) return false;
    if (extended.back() != lb) return false;

    // If the endpoint extension reaches the lower bound, it is optimal.
    if (!is_g_golomb(extended, g)) return false;

    out_result = {g, n, lb, 0.0, extended};
    return true;
}

void save_to_json(int b, const vector<Result>& results) {
    filesystem::create_directories("data3");

    string filename = "data3/b" + to_string(b) + ".json";
    ofstream file(filename);

    if (!file) {
        cerr << "\nCould not open " << filename << " for writing.\n";
        return;
    }

    file << "{\n";
    file << "  \"b\": " << b << ",\n";
    file << "  \"results\": [\n";

    for (size_t idx = 0; idx < results.size(); ++idx) {
        const auto& r = results[idx];

        file << "    {\n";
        file << "      \"g\": " << r.g << ",\n";
        file << "      \"n\": " << r.n << ",\n";
        file << "      \"alpha\": " << r.alpha << ",\n";
        file << "      \"time_sec\": " << r.time << ",\n";
        file << "      \"max_rep_count\": " << max_rep_count(r.marks) << ",\n";
        file << "      \"set\": [";

        for (size_t m = 0; m < r.marks.size(); ++m) {
            file << r.marks[m];
            if (m + 1 < r.marks.size()) file << ", ";
        }

        file << "]\n";
        file << "    }";

        if (idx + 1 < results.size()) file << ",";
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    cout << "\nResults saved to " << filename << endl;
}

int main() {
    if (g_stop > MAX_G) {
        cerr << "Error: g_stop is larger than MAX_G.\n";
        cerr << "Increase MAX_G before running.\n";
        return 1;
    }

    if (MAX_ALPHA >= 128) {
        cerr << "Error: MAX_ALPHA must be at most 127 when using __uint128_t.\n";
        return 1;
    }

    vector<Result> all_results;

    cout << "Testing Diagonal Sequence G(g, g + " << i_val << ")" << endl;

    for (int g = g_start; g <= g_stop; ++g) {
        const int n = g + i_val;
        const int lb = lower_bound_alpha(g, i_val);

        global_best_marks.clear();

        cout << "Checking g=" << g << " (n=" << n << ")..." << flush;

        // Exact shortcut:
        // If the previous row already hit the lower bound, then adding the new endpoint
        // gives the next lower bound exactly.
        if (!all_results.empty()) {
            Result extended_result;

            if (try_endpoint_extension(all_results.back(), g, i_val, extended_result)) {
                cout << " Forced alpha=" << extended_result.alpha
                     << " by endpoint extension"
                     << " (max multiplicity=" << max_rep_count(extended_result.marks)
                     << ")" << endl;

                all_results.push_back(extended_result);
                continue;
            }
        }

        auto start_time = chrono::high_resolution_clock::now();

        // Start at the proven lower bound.
        int alpha = max(n - 1, lb);

        bool unsupported = false;

        while (true) {
            found_solution.store(false, memory_order_release);

            if (alpha > MAX_ALPHA) {
                unsupported = true;
                break;
            }

            #pragma omp parallel
            {
                int thread_set[MAX_ALPHA + 1];

                thread_set[0] = 0;

                const bitmask initial_rev = bit_at(MAX_ALPHA);

                #pragma omp for schedule(dynamic, 1)
                for (int cand1 = 1; cand1 <= alpha - (n - 2); ++cand1) {
                    if (found_solution.load(memory_order_acquire)) continue;

                    thread_set[1] = cand1;

                    bitmask initial_seen[MAX_G + 1] = {};
                    initial_seen[1] = bit_at(cand1);

                    backtrack(thread_set, 2, initial_seen,
                              initial_rev | bit_at(MAX_ALPHA - cand1),
                              n, alpha, g);
                }
            }

            if (found_solution.load(memory_order_acquire)) {
                auto end_time = chrono::high_resolution_clock::now();
                double elapsed = chrono::duration<double>(end_time - start_time).count();

                cout << " Found alpha=" << alpha
                     << " (" << elapsed << "s"
                     << ", max multiplicity=" << max_rep_count(global_best_marks)
                     << ")" << endl;

                all_results.push_back({g, n, alpha, elapsed, global_best_marks});
                break;
            }

            ++alpha;
        }

        if (unsupported) {
            cout << " stopped: alpha exceeded " << MAX_ALPHA
                 << " because __uint128_t only supports 128 bit positions." << endl;
        }
    }

    save_to_json(i_val, all_results);

    return 0;
}