#include <bits/stdc++.h>
#define REP(i, n) for (int i = 0; (i) < (int)(n); ++ (i))
#define REP3(i, m, n) for (int i = (m); (i) < (int)(n); ++ (i))
#define REP_R(i, n) for (int i = (int)(n) - 1; (i) >= 0; -- (i))
#define REP3R(i, m, n) for (int i = (int)(n) - 1; (i) >= (int)(m); -- (i))
#define ALL(x) std::begin(x), std::end(x)
using namespace std;

class xor_shift_128 {
public:
    typedef uint32_t result_type;
    xor_shift_128(uint32_t seed = 42) {
        set_seed(seed);
    }
    void set_seed(uint32_t seed) {
        a = seed = 1812433253u * (seed ^ (seed >> 30));
        b = seed = 1812433253u * (seed ^ (seed >> 30)) + 1;
        c = seed = 1812433253u * (seed ^ (seed >> 30)) + 2;
        d = seed = 1812433253u * (seed ^ (seed >> 30)) + 3;
    }
    uint32_t operator() () {
        uint32_t t = (a ^ (a << 11));
        a = b; b = c; c = d;
        return d = (d ^ (d >> 19)) ^ (t ^ (t >> 8));
    }
    static constexpr uint32_t max() { return numeric_limits<result_type>::max(); }
    static constexpr uint32_t min() { return numeric_limits<result_type>::min(); }
private:
    uint32_t a, b, c, d;
};

constexpr int N = 50;

constexpr array<int, 4> DIRS = {{0, 1, 2, 3}};
constexpr array<int, 4> DIR_Y = {-1, 1, 0, 0};
constexpr array<int, 4> DIR_X = {0, 0, 1, -1};
inline bool is_on_tiles(int y, int x) {
    return 0 <= y and y < N and 0 <= x and x < N;
}

inline uint16_t pack_point(int y, int x) {
    return (y << 8) + x;
}

inline pair<int, int> unpack_point(int packed) {
    return {packed >> 8, packed & ((1 << 8) - 1)};
}

string convert_to_command_string(const vector<uint16_t>& result) {
    assert (not result.empty());
    string ans;
    REP (i, (int)result.size() - 1) {
        auto [ay, ax] = unpack_point(result[i]);
        auto [by, bx] = unpack_point(result[i + 1]);
        if (by == ay - 1 and bx == ax) {
            ans.push_back('U');
        } else if (by == ay + 1 and bx == ax) {
            ans.push_back('D');
        } else if (by == ay and bx == ax + 1) {
            ans.push_back('R');
        } else if (by == ay and bx == ax - 1) {
            ans.push_back('L');
        } else {
            assert (false);
        }
    }
    return ans;
}

template <class RandomEngine>
string solve(const int sy, const int sx, const array<array<int, N>, N>& tile, const array<array<int, N>, N>& point, RandomEngine& gen, chrono::high_resolution_clock::time_point clock_end) {
    chrono::high_resolution_clock::time_point clock_begin = chrono::high_resolution_clock::now();

    int M = 0;
    REP (y, N) {
        REP (x, N) {
            M = max(M, tile[y][x] + 1);
        }
    }

    vector<uint16_t> path_prev;
    path_prev.push_back(pack_point(sy, sx));
    vector<int16_t> used_tile_prev(M, INT16_MAX);
    used_tile_prev[tile[sy][sx]] = 0;
    array<array<bool, N>, N> used_pos_prev = {};
    used_pos_prev[sy][sx] = true;
    vector<int> score_prev;
    score_prev.push_back(0);
    score_prev.push_back(point[sy][sx]);

    vector<uint16_t> result = path_prev;
    int highscore = score_prev.back();
#ifdef VISUALIZE
    int highscore_index = 0;
    cerr << "-----BEGIN-----" << endl;
    cerr << convert_to_command_string(result) << endl;
    cerr << "-----END-----" << endl;
#endif  // VISUALIZE

    // simulated annealing
    int64_t iteration = 0;
    double temperature = 1.0;
    for (; ; ++ iteration) {
        if (iteration % 64 == 0) {
            chrono::high_resolution_clock::time_point clock_now = chrono::high_resolution_clock::now();
            temperature = static_cast<long double>((clock_end - clock_now).count()) / (clock_end - clock_begin).count();
            if (temperature <= 0.0) {
                cerr << "done  (iteration = " << iteration << ")" << endl;
                break;
            }
        }

        int start = uniform_int_distribution<int>(1, path_prev.size())(gen);
        vector<char> used_tile_next(M);
        auto get_used_tile = [&](int y, int x) {
            int i = tile[y][x];
            return used_tile_prev[i] < start or used_tile_next[i];
        };
        vector<uint16_t> diff;
        int score_next = score_prev[start];
        auto [y, x] = unpack_point(path_prev[start - 1]);
        while (true) {
            array<int, 4> dirs = {{0, 1, 2, 3}};
            shuffle(ALL(dirs), gen);
            bool found = false;
            for (int dir : dirs) {
                int ny = y + DIR_Y[dir];
                int nx = x + DIR_X[dir];
                if (not is_on_tiles(ny, nx)) {
                    continue;
                }
                if (diff.empty() and start < path_prev.size() and path_prev[start] == pack_point(ny, nx)) {
                    continue;
                }
                if (not get_used_tile(ny, nx)) {
                    found = true;
                    diff.push_back(pack_point(ny, nx));
                    y = ny;
                    x = nx;
                    used_tile_next[tile[y][x]] = true;
                    score_next += point[y][x];
                    break;
                }
            }
            if (not found) {
                break;
            }
            if (used_pos_prev[y][x]) {
                break;
            }
        }
        if (diff.empty()) {
            continue;
        }
        int tail_first = path_prev.size();
        int tail_last = path_prev.size();
        if (used_pos_prev[y][x]) {
            tail_first = start;
            while (tail_first < path_prev.size() and path_prev[tail_first] != pack_point(y, x)) {
                ++ tail_first;
            }
            assert (tail_first < path_prev.size());
            ++ tail_first;
            REP3 (i, tail_first, path_prev.size()) {
                auto [y, x] = unpack_point(path_prev[i]);
                if (get_used_tile(y, x)) {
                    tail_last = i;
                    break;
                }
                used_tile_next[tile[y][x]] = true;
            }
        }
        score_next += score_prev[tail_last] - score_prev[tail_first];

        int delta = score_next - score_prev.back();
        auto probability = [&]() {
            constexpr long double boltzmann = 0.01;
            return exp(boltzmann * delta / temperature);
        };
        if (delta >= 0 or bernoulli_distribution(probability())(gen)) {
            // accept
            if (delta < 0) {
#ifdef VERBOSE
                cerr << "decreasing move  (delta = " << delta << ", iteration = " << iteration << ")" << endl;
#endif  // VERBOSE
            }

            diff.insert(diff.end(), path_prev.begin() + tail_first, path_prev.begin() + tail_last);
            path_prev.resize(start);
            path_prev.insert(path_prev.end(), ALL(diff));
            used_tile_prev.assign(M, INT16_MAX);
            used_pos_prev = {};
            score_prev.clear();
            score_prev.push_back(0);
            REP (i, path_prev.size()) {
                auto [y, x] = unpack_point(path_prev[i]);
                used_tile_prev[tile[y][x]] = i;
                used_pos_prev[y][x] = true;
                score_prev.push_back(score_prev.back() + point[y][x]);
            }

            if (highscore < score_prev.back()) {
                highscore = score_prev.back();
                result = path_prev;
#ifdef VERBOSE
                cerr << "highscore = " << highscore << "  (iteration = " << iteration << ")" << endl;
#endif  // VERBOSE
#ifdef VISUALIZE
                cerr << "-----BEGIN-----" << endl;
                cerr << convert_to_command_string(result) << endl;
                cerr << "-----END-----" << endl;
#endif  // VISUALIZE
            }
        }
    }

    string ans = convert_to_command_string(result);
    cerr << "ans = " << ans << endl;
    cerr << "score = " << highscore << endl;
    return ans;
}

int main() {
    constexpr auto TIME_LIMIT = chrono::milliseconds(2000);
    chrono::high_resolution_clock::time_point clock_begin = chrono::high_resolution_clock::now();
    xor_shift_128 gen(20210425);

    int sy, sx; cin >> sy >> sx;
    array<array<int, N>, N> tile;
    REP (y, N) {
        REP (x, N) {
            cin >> tile[y][x];
        }
    }
    array<array<int, N>, N> point;
    REP (y, N) {
        REP (x, N) {
            cin >> point[y][x];
        }
    }
    string ans = solve(sy, sx, tile, point, gen, clock_begin + chrono::duration_cast<chrono::milliseconds>(TIME_LIMIT * 0.95));
    cout << ans << endl;
    return 0;
}
