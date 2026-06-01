
#ifndef LDPC_FINAL_COMMON_HPP
#define LDPC_FINAL_COMMON_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <sstream>
#include <set>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

using std::string;
using std::vector;

struct LDPCCode {
    string name;
    int n = 0;
    int m = 0;
    int k = 0;
    int Z = 1;
    vector<vector<int>> checks;
};

struct StructDiag {
    double weak_concentration = 0.0;   // rho_h: weakness mass in the top 10% information variables
    double block_imbalance = 0.0;      // rho_b: coefficient of variation across QC blocks
    double coupling_risk = 0.0;        // rho_c: hot variables coupled inside original checks
    double target_rate = 0.0;
    int extra_budget = 0;
    string rate_zone = "";
    string selected_mode = "";

    // Static construction diagnostics used by AdaptiveStructV2. These are not short simulations;
    // they are computed from candidate parity-extension graphs before transmission.
    double hot_coverage_ratio = 0.0;
    double hot_top10_edge_share = 0.0;
    double hot_duplicate_pattern_ratio = 0.0;
    int hot_max_use_count = 0;
    int hot_pair_reuse_max = 0;
    double hot_quality_score = 0.0;

    double ext_coverage_ratio = 0.0;
    double ext_top10_edge_share = 0.0;
    double ext_duplicate_pattern_ratio = 0.0;
    int ext_max_use_count = 0;
    int ext_pair_reuse_max = 0;
    double ext_quality_score = 0.0;

    string adaptive_reason = "";
};

struct ResultRow {
    string experiment;
    string matrix;
    string scheme;
    string selected_mode;
    int frames = 0;
    int n = 0;
    int k = 0;
    int tx_len = 0;
    double effective_rate = 0.0;
    double EbN0_dB = 0.0;
    double BER = 0.0;
    double BER_info = 0.0;
    double FER = 0.0;       // Backward-compatible alias of FER_info.
    double FER_info = 0.0;  // Information-frame error rate.
    double FER_code = 0.0;  // Full decoded-codeword frame error rate.
    double AvgIter = 0.0;
    long long bit_errors = 0;
    long long info_bit_errors = 0;
    long long frame_errors = 0;       // Backward-compatible: information-frame errors.
    long long code_frame_errors = 0;
    double upper95_if_zero_info = 0.0;
    double weak_concentration = 0.0;
    double block_imbalance = 0.0;
    double coupling_risk = 0.0;
    string rate_zone;

    double hot_coverage_ratio = 0.0;
    double hot_top10_edge_share = 0.0;
    double hot_duplicate_pattern_ratio = 0.0;
    int hot_max_use_count = 0;
    int hot_pair_reuse_max = 0;
    double hot_quality_score = 0.0;
    double ext_coverage_ratio = 0.0;
    double ext_top10_edge_share = 0.0;
    double ext_duplicate_pattern_ratio = 0.0;
    int ext_max_use_count = 0;
    int ext_pair_reuse_max = 0;
    double ext_quality_score = 0.0;
    string adaptive_reason = "";

    int selection_short_frames = 0;
    int selection_extra_decodes = 0;
};

static uint64_t make_seed(uint64_t base_seed, const string& tag, int serial) {
    uint64_t h = base_seed ^ 1469598103934665603ULL;
    for (unsigned char c : tag) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    h ^= (uint64_t)(serial + 0x9e3779b97f4a7c15ULL);
    h *= 1099511628211ULL;
    return h;
}

static int target_tx_len(int k, int n, double target_rate) {
    int tx = (int)std::llround(k / target_rate);
    if (tx < n) tx = n;
    return tx;
}

static LDPCCode build_from_base(const string& name, const vector<vector<int>>& base, int Z) {
    LDPCCode code;
    code.name = name;
    code.Z = Z;
    int Mb = (int)base.size();
    int Nb = (int)base[0].size();
    code.m = Mb * Z;
    code.n = Nb * Z;
    code.k = code.n - code.m;
    code.checks.assign(code.m, {});
    for (int br = 0; br < Mb; ++br) {
        for (int zr = 0; zr < Z; ++zr) {
            int check = br * Z + zr;
            for (int bc = 0; bc < Nb; ++bc) {
                int shift = base[br][bc];
                if (shift < 0) continue;
                int var = bc * Z + ((zr + shift) % Z);
                code.checks[check].push_back(var);
            }
        }
    }
    return code;
}

static vector<vector<int>> make_base_matrix(int Mb, int Nb, int style, int Z) {
    if (Nb <= Mb) throw std::runtime_error("Bad base size.");
    int Kb = Nb - Mb;
    vector<vector<int>> B(Mb, vector<int>(Nb, -1));

    // Information part: deterministic irregular QC structure.
    for (int c = 0; c < Kb; ++c) {
        int deg = 3 + ((c + style) % 2);       // 3 or 4
        if (style == 3 && c % 4 == 0) deg = 5; // hotspot-like short matrix
        if (style == 4 && c % 5 == 0) deg = 2; // more uneven matrix
        for (int e = 0; e < deg; ++e) {
            int r = (c * (2 + style) + e * (3 + style) + (c / 3)) % Mb;
            int sh = (c * 7 + e * 11 + style * 5 + r * 3) % Z;
            B[r][c] = sh;
        }
    }

    // Parity part: lower-bidiagonal style, stable enough for the all-zero codeword simulation.
    for (int r = 0; r < Mb; ++r) {
        B[r][Kb + r] = 0;
        if (r > 0) B[r][Kb + r - 1] = (1 + style + r) % Z;
        if ((r + style) % 4 == 0 && r + 2 < Mb) B[r][Kb + r + 2] = (5 + 2 * r + style) % Z;
    }
    return B;
}

static LDPCCode make_matrix_A() { return build_from_base("A_main_QC_12x24_Z32", make_base_matrix(12, 24, 1, 32), 32); }
static LDPCCode make_matrix_B() { return build_from_base("B_balanced_QC_12x24_Z32", make_base_matrix(12, 24, 2, 32), 32); }
static LDPCCode make_matrix_C() { return build_from_base("C_long_QC_14x28_Z32", make_base_matrix(14, 28, 2, 32), 32); }
static LDPCCode make_matrix_D() { return build_from_base("D_short_hotspot_QC_8x16_Z32", make_base_matrix(8, 16, 3, 32), 32); }

// ------------------------- Graph scoring -------------------------

static vector<int> variable_degrees(const LDPCCode& code) {
    vector<int> deg(code.n, 0);
    for (const auto& row : code.checks) {
        for (int v : row) deg[v]++;
    }
    return deg;
}

static vector<vector<int>> variable_to_checks(const LDPCCode& code) {
    vector<vector<int>> vc(code.n);
    for (int ci = 0; ci < (int)code.checks.size(); ++ci) {
        for (int v : code.checks[ci]) vc[v].push_back(ci);
    }
    return vc;
}

// Higher score = more vulnerable / more important.
// The original score mostly duplicated the degree term and used fixed block/phase biases.
// This deterministic score is tied to LDPC graph features: low variable degree, heavy
// adjacent checks, local 4-cycle pressure and block-level weakness.
static vector<double> weakness_score(const LDPCCode& code) {
    vector<int> deg = variable_degrees(code);
    vector<vector<int>> vc = variable_to_checks(code);
    vector<int> row_deg(code.m, 0);
    for (int i = 0; i < code.m; ++i) row_deg[i] = (int)code.checks[i].size();

    vector<double> raw_low(code.n, 0.0), raw_check(code.n, 0.0), raw_cycle(code.n, 0.0);
    for (int v = 0; v < code.n; ++v) {
        raw_low[v] = 1.0 / (0.5 + deg[v]);
        double adj = 0.0;
        for (int c : vc[v]) adj += row_deg[c];
        raw_check[v] = adj / std::max(1, (int)vc[v].size());

        // 4-cycle proxy: two checks adjacent to v sharing another variable create
        // local short-cycle stress for iterative min-sum decoding.
        double cyc = 0.0;
        for (int a = 0; a < (int)vc[v].size(); ++a) {
            const auto& ra = code.checks[vc[v][a]];
            for (int b = a + 1; b < (int)vc[v].size(); ++b) {
                const auto& rb = code.checks[vc[v][b]];
                int common = 0;
                for (int x : ra) {
                    if (x == v) continue;
                    if (std::find(rb.begin(), rb.end(), x) != rb.end()) common++;
                }
                cyc += common;
            }
        }
        raw_cycle[v] = cyc;
    }

    int info_k = std::max(1, code.k);
    int blocks = std::max(1, info_k / std::max(1, code.Z));
    vector<double> block_low(blocks, 0.0);
    for (int v = 0; v < info_k; ++v) block_low[v / code.Z] += raw_low[v];
    double block_mean = std::accumulate(block_low.begin(), block_low.end(), 0.0) / blocks;

    auto norm01 = [&](const vector<double>& x, int upto, int v) -> double {
        double mn = x[0], mx = x[0];
        for (int i = 1; i < upto; ++i) {
            mn = std::min(mn, x[i]);
            mx = std::max(mx, x[i]);
        }
        return (x[v] - mn) / (mx - mn + 1e-12);
    };

    vector<double> score(code.n, 0.0);
    for (int v = 0; v < code.n; ++v) {
        int iv = std::min(v, info_k - 1);
        int b = std::min(blocks - 1, std::max(0, iv / std::max(1, code.Z)));
        double low_term = norm01(raw_low, info_k, iv);
        double check_term = norm01(raw_check, info_k, iv);
        double cycle_term = norm01(raw_cycle, info_k, iv);
        double block_term = std::max(0.0, (block_low[b] - block_mean) / (block_mean + 1e-12));
        double deterministic_tie = 1e-5 * ((v * 1103515245u + 12345u) & 1023u);
        score[v] = 0.48 * low_term + 0.18 * check_term + 0.24 * cycle_term
                 + 0.10 * std::min(2.0, block_term) + deterministic_tie;
    }
    return score;
}

static vector<int> order_desc(const vector<double>& x, int limit) {
    vector<int> idx(limit);
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](int a, int b) {
        if (x[a] != x[b]) return x[a] > x[b];
        return a < b;
    });
    return idx;
}

static StructDiag diagnose_structure(const LDPCCode& base, double target_rate) {
    StructDiag d;
    d.target_rate = target_rate;
    d.extra_budget = std::max(0, target_tx_len(base.k, base.n, target_rate) - base.n);
    if (target_rate <= 0.36) d.rate_zone = "low_rate_large_redundancy";
    else if (target_rate <= 0.405) d.rate_zone = "middle_rate_limited_redundancy";
    else d.rate_zone = "high_rate_small_redundancy";

    vector<double> w = weakness_score(base);
    vector<int> ord = order_desc(w, base.k);

    double total = 0.0;
    for (int i = 0; i < base.k; ++i) total += w[i];

    int topN = std::max(1, base.k / 10);
    double top = 0.0;
    for (int i = 0; i < topN; ++i) top += w[ord[i]];
    d.weak_concentration = top / (total + 1e-12);

    int blocks = std::max(1, base.k / base.Z);
    vector<double> block_sum(blocks, 0.0);
    for (int v = 0; v < base.k; ++v) block_sum[v / base.Z] += w[v];
    double mean = std::accumulate(block_sum.begin(), block_sum.end(), 0.0) / blocks;
    double var = 0.0;
    for (double x : block_sum) var += (x - mean) * (x - mean);
    d.block_imbalance = std::sqrt(var / blocks) / (mean + 1e-12);

    vector<int> hot_mark(base.n, 0);
    int hotN = std::max(1, base.k / 8);
    for (int i = 0; i < hotN; ++i) hot_mark[ord[i]] = 1;

    double risky = 0.0;
    for (const auto& row : base.checks) {
        int cnt = 0;
        for (int v : row) if (v < base.k && hot_mark[v]) cnt++;
        if (cnt >= 2) risky += (cnt - 1);
    }
    d.coupling_risk = risky / (base.m + 1e-12);
    return d;
}

// ------------------------- Redundancy construction -------------------------

struct ConstructionDiagnostics {
    string mode = "";
    int extra = 0;
    int degree = 0;
    int total_edges = 0;
    int covered_vars = 0;
    int uncovered_vars = 0;
    int max_use_count = 0;
    int max_block_use = 0;
    int pair_reuse_max = 0;
    int duplicate_patterns = 0;
    double coverage_ratio = 0.0;
    double top10_edge_share = 0.0;
    double duplicate_pattern_ratio = 0.0;
    double block_imbalance_after = 0.0;
    double weak_edge_gain = 0.0;
    double quality_score = 0.0;
    double risk_score = 0.0;
};

struct ConstructionState {
    const LDPCCode& base;
    string mode;
    int extra = 0;
    int degree = 5;
    int total_edges = 0;
    int blocks = 1;
    int min_coverage_target = 0;
    int max_use_allowed = 1;
    int max_block_allowed = 1;
    int max_pair_reuse_allowed = 1;
    vector<double> w;
    vector<int> ord;
    vector<int> use_count;
    vector<int> block_use;
    std::map<std::pair<int,int>, int> pair_use;
    std::map<string, int> pattern_use;
    int covered_count = 0;
    int total_selected_edges = 0;
    double selected_weak_sum = 0.0;

    ConstructionState(const LDPCCode& b, const string& m, int ex, int deg)
        : base(b), mode(m), extra(ex), degree(deg) {
        total_edges = std::max(0, extra * degree);
        blocks = std::max(1, base.k / std::max(1, base.Z));
        w = weakness_score(base);
        ord = order_desc(w, base.k);
        use_count.assign(base.k, 0);
        block_use.assign(blocks, 0);

        double avg_use = (double)std::max(1, total_edges) / std::max(1, base.k);
        double avg_block = (double)std::max(1, total_edges) / std::max(1, blocks);
        if (mode == "HotspotOnlyParity") {
            // Hotspot remains weak-node first, but with hard constraints to avoid the
            // coverage collapse and duplicate information-subsets found in the original code.
            min_coverage_target = std::min(base.k, std::max(1, (int)std::llround(0.72 * base.k)));
            max_use_allowed = std::max(2, (int)std::ceil(avg_use) + 2);
            max_pair_reuse_allowed = 2;
        } else {
            // ExtParityV2 is the weak-coverage balanced construction.
            min_coverage_target = std::min(base.k, std::max(1, (int)std::llround(0.98 * base.k)));
            max_use_allowed = std::max(2, (int)std::ceil(avg_use) + 1);
            max_pair_reuse_allowed = 1;
        }
        if (total_edges < min_coverage_target) min_coverage_target = total_edges;
        max_block_allowed = std::max(degree, (int)std::ceil(avg_block) + degree);
    }
};

static string pattern_signature(vector<int> vars) {
    std::sort(vars.begin(), vars.end());
    std::ostringstream ss;
    for (int i = 0; i < (int)vars.size(); ++i) {
        if (i) ss << '-';
        ss << vars[i];
    }
    return ss.str();
}

static int pair_count_for(const ConstructionState& st, int a, int b) {
    if (a > b) std::swap(a, b);
    auto it = st.pair_use.find({a, b});
    return (it == st.pair_use.end()) ? 0 : it->second;
}

static int pair_penalty_sum(const ConstructionState& st, int v, const vector<int>& vars, int* max_pair = nullptr) {
    int s = 0, mx = 0;
    for (int u : vars) {
        int c = pair_count_for(st, u, v);
        s += c;
        mx = std::max(mx, c);
    }
    if (max_pair) *max_pair = mx;
    return s;
}

static double selection_score(const LDPCCode& base, const ConstructionState& st, int row_id,
                              int t, int v, const vector<int>& vars,
                              const vector<int>& local_block_used, bool hotspot, int relax_pass) {
    int b = std::min(st.blocks - 1, std::max(0, v / std::max(1, base.Z)));
    int pair_max = 0;
    int pair_sum = pair_penalty_sum(st, v, vars, &pair_max);

    double avg_use_now = std::max(1.0, (double)std::max(1, st.total_selected_edges) / std::max(1, base.k));
    double avg_block_now = std::max(1.0, (double)std::max(1, st.total_selected_edges) / std::max(1, st.blocks));
    double uncovered_bonus = (st.use_count[v] == 0) ? ((st.covered_count < st.min_coverage_target) ? 2.40 : 0.42) : 0.0;
    double weak_weight = hotspot ? 2.65 : 1.45;
    double use_weight = hotspot ? 0.78 : 1.10;
    double block_weight = hotspot ? 0.34 : 0.48;
    double pair_weight = hotspot ? 1.00 : 1.55;

    double s = weak_weight * st.w[v]
             + uncovered_bonus
             - use_weight * ((double)st.use_count[v] / avg_use_now)
             - block_weight * ((double)st.block_use[b] / avg_block_now)
             - pair_weight * pair_sum
             - 0.40 * pair_max
             - 0.38 * local_block_used[b]
             + 1e-6 * ((row_id + 1) * 131 + (t + 7) * 17 + v % 97);

    if (relax_pass == 0) {
        if (st.use_count[v] >= st.max_use_allowed) s -= 1e6;
        if (st.block_use[b] >= st.max_block_allowed) s -= 1e5;
        if (pair_max >= st.max_pair_reuse_allowed) s -= 1e5;
    } else if (relax_pass == 1) {
        if (st.use_count[v] >= st.max_use_allowed + 1) s -= 1e6;
        if (pair_max >= st.max_pair_reuse_allowed + 1) s -= 1e5;
    }
    return s;
}

static vector<int> select_scored_vars(const LDPCCode& base, int row_id, int degree,
                                      ConstructionState& st, bool hotspot) {
    vector<int> vars;
    vector<int> local_block_used(st.blocks, 0);
    int pool = base.k;
    if (hotspot) {
        int min_pool = std::max(st.min_coverage_target + degree * 8, (int)std::llround(0.76 * base.k));
        pool = std::min(base.k, std::max(degree * 32, min_pool));
    }

    for (int t = 0; t < degree; ++t) {
        int best = -1;
        double best_score = -std::numeric_limits<double>::infinity();
        for (int pass = 0; pass < 3 && best < 0; ++pass) {
            best_score = -std::numeric_limits<double>::infinity();
            for (int rank = 0; rank < pool; ++rank) {
                int idx = (rank + row_id * 17 + t * 29) % pool;
                int v = st.ord[idx];
                if (std::find(vars.begin(), vars.end(), v) != vars.end()) continue;
                double s = selection_score(base, st, row_id, t, v, vars, local_block_used, hotspot, pass);
                if (s > best_score) {
                    best_score = s;
                    best = v;
                }
            }
            if (best_score < -1e4) best = -1; // all candidates violated hard constraints; relax.
        }
        if (best < 0) {
            for (int v = 0; v < base.k; ++v) {
                if (std::find(vars.begin(), vars.end(), v) == vars.end()) { best = v; break; }
            }
        }
        vars.push_back(best);
        local_block_used[best / base.Z]++;
    }

    // Avoid exactly duplicated information-subsets; this reduces redundant parity checks and short cycles.
    if (st.pattern_use.count(pattern_signature(vars))) {
        for (int pos = degree - 1; pos >= 0 && st.pattern_use.count(pattern_signature(vars)); --pos) {
            int old = vars[pos];
            int best = -1;
            double best_score = -std::numeric_limits<double>::infinity();
            vector<int> tmp = vars;
            for (int v = 0; v < base.k; ++v) {
                if (std::find(vars.begin(), vars.end(), v) != vars.end() && v != old) continue;
                tmp[pos] = v;
                if (st.pattern_use.count(pattern_signature(tmp))) continue;
                int b = v / base.Z;
                if (st.use_count[v] > st.max_use_allowed + 1) continue;
                if (st.block_use[b] > st.max_block_allowed + degree) continue;
                double s = selection_score(base, st, row_id, pos, v, tmp, local_block_used, hotspot, 2);
                if (s > best_score) { best_score = s; best = v; }
            }
            if (best >= 0) vars[pos] = best;
        }
    }
    return vars;
}

static void commit_selected_vars(const LDPCCode& base, ConstructionState& st, const vector<int>& vars) {
    for (int v : vars) {
        if (st.use_count[v] == 0) st.covered_count++;
        st.use_count[v]++;
        st.block_use[v / base.Z]++;
        st.total_selected_edges++;
        st.selected_weak_sum += st.w[v];
    }
    for (int i = 0; i < (int)vars.size(); ++i) {
        for (int j = i + 1; j < (int)vars.size(); ++j) {
            int a = vars[i], b = vars[j];
            if (a > b) std::swap(a, b);
            st.pair_use[{a, b}]++;
        }
    }
    st.pattern_use[pattern_signature(vars)]++;
}

static vector<int> select_uniform_vars(const LDPCCode& base, int row_id, int degree) {
    vector<int> vars;
    int stride = std::max(1, base.k / degree);
    for (int t = 0; t < degree; ++t) {
        int v = (row_id * 37 + t * stride + t * 11) % base.k;
        while (std::find(vars.begin(), vars.end(), v) != vars.end()) v = (v + 1) % base.k;
        vars.push_back(v);
    }
    return vars;
}

static ConstructionDiagnostics summarize_construction(const LDPCCode& base, const ConstructionState& st) {
    ConstructionDiagnostics d;
    d.mode = st.mode;
    d.extra = st.extra;
    d.degree = st.degree;
    d.total_edges = st.total_edges;
    d.covered_vars = st.covered_count;
    d.uncovered_vars = std::max(0, base.k - st.covered_count);
    d.coverage_ratio = (double)d.covered_vars / std::max(1, base.k);
    d.max_use_count = st.use_count.empty() ? 0 : *std::max_element(st.use_count.begin(), st.use_count.end());
    d.max_block_use = st.block_use.empty() ? 0 : *std::max_element(st.block_use.begin(), st.block_use.end());
    for (const auto& kv : st.pair_use) d.pair_reuse_max = std::max(d.pair_reuse_max, kv.second);
    for (const auto& kv : st.pattern_use) if (kv.second > 1) d.duplicate_patterns += kv.second - 1;
    d.duplicate_pattern_ratio = (double)d.duplicate_patterns / std::max(1, st.extra);

    vector<int> uses = st.use_count;
    std::sort(uses.begin(), uses.end(), std::greater<int>());
    int topN = std::max(1, base.k / 10);
    long long top_edges = 0;
    for (int i = 0; i < std::min(topN, (int)uses.size()); ++i) top_edges += uses[i];
    d.top10_edge_share = (double)top_edges / std::max(1, st.total_edges);

    double mean_block = 0.0;
    for (int x : st.block_use) mean_block += x;
    mean_block /= std::max(1, (int)st.block_use.size());
    double var = 0.0;
    for (int x : st.block_use) var += (x - mean_block) * (x - mean_block);
    d.block_imbalance_after = std::sqrt(var / std::max(1, (int)st.block_use.size())) / (mean_block + 1e-12);

    double mean_w = 0.0;
    for (int v = 0; v < base.k; ++v) mean_w += st.w[v];
    mean_w /= std::max(1, base.k);
    d.weak_edge_gain = (st.selected_weak_sum / std::max(1, st.total_selected_edges)) / (mean_w + 1e-12);

    double coverage_part = d.coverage_ratio;
    double weakness_part = std::min(1.0, d.weak_edge_gain / 1.65);
    double spread_part = 1.0 - std::min(1.0, d.top10_edge_share / 0.35);
    double block_part = 1.0 - std::min(1.0, d.block_imbalance_after / 0.65);
    double dup_part = 1.0 - std::min(1.0, d.duplicate_pattern_ratio / 0.08);
    d.quality_score = 0.38 * coverage_part + 0.28 * weakness_part + 0.18 * spread_part
                    + 0.10 * block_part + 0.06 * dup_part;
    d.risk_score = 1.45 * (1.0 - d.coverage_ratio)
                 + 2.00 * d.duplicate_pattern_ratio
                 + 1.40 * std::max(0.0, d.top10_edge_share - 0.24)
                 + 0.08 * std::max(0, d.pair_reuse_max - 2)
                 + 0.30 * d.block_imbalance_after;
    return d;
}

static LDPCCode make_extended_code(const LDPCCode& base, double target_rate, const string& mode,
                                   int degree = 5, ConstructionDiagnostics* cd = nullptr) {
    int tx = target_tx_len(base.k, base.n, target_rate);
    int extra = std::max(0, tx - base.n);
    if (extra == 0) {
        if (cd) {
            ConstructionDiagnostics z;
            z.mode = mode;
            z.covered_vars = base.k;
            z.coverage_ratio = 1.0;
            z.quality_score = 1.0;
            *cd = z;
        }
        return base;
    }

    string actual = mode;
    if (actual != "UniformParity" && actual != "HotspotOnlyParity" && actual != "ExtParityV2") actual = "ExtParityV2";
    ConstructionState st(base, actual, extra, degree);

    LDPCCode c = base;
    c.name = base.name + "_" + actual;
    c.n = base.n + extra;
    c.m = base.m + extra;
    c.k = base.k;
    c.checks.reserve(base.checks.size() + extra);

    for (int j = 0; j < extra; ++j) {
        vector<int> vars;
        if (actual == "UniformParity") {
            vars = select_uniform_vars(base, j, degree);
        } else if (actual == "HotspotOnlyParity") {
            vars = select_scored_vars(base, j, degree, st, true);
        } else {
            vars = select_scored_vars(base, j, degree, st, false);
        }
        commit_selected_vars(base, st, vars);

        int new_var = base.n + j;
        vector<int> row;
        row.reserve(degree + 1);
        for (int v : vars) row.push_back(v);
        row.push_back(new_var);
        c.checks.push_back(row);
    }
    if (cd) *cd = summarize_construction(base, st);
    return c;
}

// Formal AdaptiveStructV2 gate from the thesis plan: static structural diagnostics plus rate gate.
// It compares HotspotOnlyParity and ExtParityV2 candidate graphs without using short BER simulation.
static string choose_adaptive_struct_v2(const LDPCCode& base, double target_rate, StructDiag* out = nullptr) {
    StructDiag d = diagnose_structure(base, target_rate);
    ConstructionDiagnostics hot, ext;
    (void)make_extended_code(base, target_rate, "HotspotOnlyParity", 5, &hot);
    (void)make_extended_code(base, target_rate, "ExtParityV2", 5, &ext);

    d.hot_coverage_ratio = hot.coverage_ratio;
    d.hot_top10_edge_share = hot.top10_edge_share;
    d.hot_duplicate_pattern_ratio = hot.duplicate_pattern_ratio;
    d.hot_max_use_count = hot.max_use_count;
    d.hot_pair_reuse_max = hot.pair_reuse_max;
    d.hot_quality_score = hot.quality_score;
    d.ext_coverage_ratio = ext.coverage_ratio;
    d.ext_top10_edge_share = ext.top10_edge_share;
    d.ext_duplicate_pattern_ratio = ext.duplicate_pattern_ratio;
    d.ext_max_use_count = ext.max_use_count;
    d.ext_pair_reuse_max = ext.pair_reuse_max;
    d.ext_quality_score = ext.quality_score;

    double avg_use = (double)std::max(1, d.extra_budget * 5) / std::max(1, base.k);
    bool low_rate = target_rate <= 0.36;
    bool hot_safe = hot.coverage_ratio >= (low_rate ? 0.68 : 0.62)
                 && hot.duplicate_pattern_ratio <= 0.03
                 && hot.pair_reuse_max <= 2
                 && hot.top10_edge_share <= 0.24
                 && hot.max_use_count <= (int)std::ceil(avg_use) + 3;
    bool structure_supports_hot = d.weak_concentration >= 0.112
                               && d.coupling_risk <= 0.42
                               && (d.block_imbalance >= 0.50 || d.coupling_risk >= 0.10 || hot.coverage_ratio >= 0.85);

    // Coverage-heavy ExtParityV2 is the safe default. Hotspot is selected only when the base
    // graph shows concentrated/block-local weakness and the repaired Hotspot candidate itself is safe.
    double structural_bias = 0.55 * std::max(0.0, d.block_imbalance - 0.42)
                           + 0.35 * std::max(0.0, d.coupling_risk - 0.08)
                           + 0.80 * std::max(0.0, hot.coverage_ratio - 0.84)
                           + 0.18 * std::max(0.0, d.weak_concentration - 0.14)
                           + (low_rate ? 0.020 : 0.0);
    double gate = (hot.quality_score - ext.quality_score) + structural_bias;

    if (hot_safe && structure_supports_hot && gate > 0.0) {
        d.selected_mode = "HotspotOnlyParity";
        d.adaptive_reason = "hot_safe_static_gate";
    } else {
        d.selected_mode = "ExtParityV2";
        if (!hot_safe) d.adaptive_reason = "hot_candidate_structural_risk";
        else if (!structure_supports_hot) d.adaptive_reason = "weakness_not_concentrated_enough";
        else d.adaptive_reason = "ext_quality_or_rate_gate";
    }

    if (out) *out = d;
    return d.selected_mode;
}

static vector<int> repeat_counts(const LDPCCode& base, double target_rate, const string& scheme) {
    int tx = target_tx_len(base.k, base.n, target_rate);
    int extra = std::max(0, tx - base.n);
    vector<int> cnt(base.n, 1);
    if (extra == 0) return cnt;

    vector<double> w = weakness_score(base);
    int blocks = std::max(1, base.k / std::max(1, base.Z));
    vector<int> block_extra(blocks, 0);
    int max_extra_per_var = std::max(1, (int)std::ceil((double)extra / std::max(1, base.k)) + 2);

    for (int j = 0; j < extra; ++j) {
        int v = 0;
        if (scheme == "UniformRepeat") {
            v = (int)((1LL * j * base.k) / std::max(1, extra)) % base.k;
        } else {
            double best = -std::numeric_limits<double>::infinity();
            for (int cand = 0; cand < base.k; ++cand) {
                int used = cnt[cand] - 1;
                int b = cand / base.Z;
                double avg_block = std::max(1.0, (double)j / std::max(1, blocks));
                double uncovered_bonus = (used == 0) ? 0.55 : 0.0;
                double score = 1.85 * w[cand] + uncovered_bonus
                             - 0.95 * used
                             - 0.18 * ((double)block_extra[b] / avg_block)
                             + 1e-6 * ((j + 3) * 97 + cand % 89);
                if (used >= max_extra_per_var) score -= 1e6;
                if (score > best) { best = score; v = cand; }
            }
        }
        cnt[v]++;
        block_extra[v / base.Z]++;
    }
    return cnt;
}

// ------------------------- Decoder and simulation -------------------------

struct Decoder {
    const LDPCCode& code;
    vector<vector<double>> old_msg;

    explicit Decoder(const LDPCCode& c) : code(c), old_msg(c.checks.size()) {
        for (int i = 0; i < (int)c.checks.size(); ++i) old_msg[i].assign(c.checks[i].size(), 0.0);
    }

    bool syndrome_ok(const vector<double>& L) const {
        for (const auto& row : code.checks) {
            int s = 0;
            for (int v : row) s ^= (L[v] < 0.0);
            if (s) return false;
        }
        return true;
    }

    int decode(vector<double>& L, int max_iter, double alpha) {
        for (auto& row : old_msg) std::fill(row.begin(), row.end(), 0.0);

        for (int it = 1; it <= max_iter; ++it) {
            for (int ci = 0; ci < (int)code.checks.size(); ++ci) {
                const auto& row = code.checks[ci];
                auto& msg = old_msg[ci];
                int deg = (int)row.size();

                vector<double> q(deg);
                vector<int> sign(deg);
                double min1 = 1e100, min2 = 1e100;
                int minpos = -1;
                int prod = 1;

                for (int e = 0; e < deg; ++e) {
                    int v = row[e];
                    q[e] = L[v] - msg[e];
                    sign[e] = (q[e] < 0.0) ? -1 : 1;
                    prod *= sign[e];
                    double a = std::fabs(q[e]);
                    if (a < min1) {
                        min2 = min1;
                        min1 = a;
                        minpos = e;
                    } else if (a < min2) {
                        min2 = a;
                    }
                }

                for (int e = 0; e < deg; ++e) {
                    int v = row[e];
                    double mag = (e == minpos) ? min2 : min1;
                    double new_msg = alpha * (prod * sign[e]) * mag;
                    L[v] += new_msg - msg[e];
                    msg[e] = new_msg;
                }
            }
            if (syndrome_ok(L)) return it;
        }
        return max_iter;
    }
};

struct Counters {
    long long bit_errors = 0;
    long long info_bit_errors = 0;
    long long frame_errors = 0;       // information-frame errors
    long long code_frame_errors = 0;  // full decoded-codeword frame errors
    long long total_bits = 0;
    long long total_info_bits = 0;
    long long total_iters = 0;
};

static void simulate_worker(const LDPCCode& sim_code, int info_k, int tx_len, double rate,
                            const vector<int>* repeat_cnt, int frames, double EbN0_dB,
                            int max_iter, double alpha, uint64_t seed, Counters& out) {
    std::mt19937_64 rng(seed);
    double ebn0 = std::pow(10.0, EbN0_dB / 10.0);
    double sigma = std::sqrt(1.0 / (2.0 * rate * ebn0));
    double inv_sigma2 = 1.0 / (sigma * sigma);
    std::normal_distribution<double> noise(0.0, sigma);

    Decoder dec(sim_code);
    vector<double> L(sim_code.n);

    for (int f = 0; f < frames; ++f) {
        if (repeat_cnt) {
            std::fill(L.begin(), L.end(), 0.0);
            for (int v = 0; v < sim_code.n; ++v) {
                int copies = (*repeat_cnt)[v];
                double llr = 0.0;
                for (int c = 0; c < copies; ++c) {
                    double y = 1.0 + noise(rng); // all-zero BPSK symbol
                    llr += 2.0 * y * inv_sigma2;
                }
                L[v] = llr;
            }
        } else {
            for (int v = 0; v < sim_code.n; ++v) {
                double y = 1.0 + noise(rng);
                L[v] = 2.0 * y * inv_sigma2;
            }
        }

        int iters = dec.decode(L, max_iter, alpha);
        long long berr = 0, ierr = 0;
        for (int v = 0; v < sim_code.n; ++v) if (L[v] < 0.0) berr++;
        for (int v = 0; v < info_k; ++v) if (L[v] < 0.0) ierr++;

        out.bit_errors += berr;
        out.info_bit_errors += ierr;
        out.frame_errors += (ierr > 0);
        out.code_frame_errors += (berr > 0);
        out.total_bits += sim_code.n;
        out.total_info_bits += info_k;
        out.total_iters += iters;
        (void)tx_len;
    }
}

static ResultRow run_code(const string& exp, const LDPCCode& base, const string& scheme,
                          const string& selected_mode, const LDPCCode& sim_code,
                          int tx_len, double rate, int frames, double eb, int max_iter,
                          double alpha, uint64_t seed, const vector<int>* repeat_cnt,
                          const StructDiag& diag) {
    Counters total;

#ifdef _OPENMP
    int threads = omp_get_max_threads();
    vector<Counters> local(threads);
#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int nthreads = omp_get_num_threads();
        int start = (frames * tid) / nthreads;
        int end = (frames * (tid + 1)) / nthreads;
        simulate_worker(sim_code, base.k, tx_len, rate, repeat_cnt, end - start, eb,
                        max_iter, alpha, seed + 1000003ULL * tid, local[tid]);
    }
    for (const auto& c : local) {
        total.bit_errors += c.bit_errors;
        total.info_bit_errors += c.info_bit_errors;
        total.frame_errors += c.frame_errors;
        total.code_frame_errors += c.code_frame_errors;
        total.total_bits += c.total_bits;
        total.total_info_bits += c.total_info_bits;
        total.total_iters += c.total_iters;
    }
#else
    simulate_worker(sim_code, base.k, tx_len, rate, repeat_cnt, frames, eb,
                    max_iter, alpha, seed, total);
#endif

    ResultRow r;
    r.experiment = exp;
    r.matrix = base.name;
    r.scheme = scheme;
    r.selected_mode = selected_mode;
    r.frames = frames;
    r.n = sim_code.n;
    r.k = base.k;
    r.tx_len = tx_len;
    r.effective_rate = rate;
    r.EbN0_dB = eb;
    r.bit_errors = total.bit_errors;
    r.info_bit_errors = total.info_bit_errors;
    r.frame_errors = total.frame_errors;
    r.code_frame_errors = total.code_frame_errors;
    r.BER = (double)total.bit_errors / std::max(1LL, total.total_bits);
    r.BER_info = (double)total.info_bit_errors / std::max(1LL, total.total_info_bits);
    r.FER_info = (double)total.frame_errors / std::max(1, frames);
    r.FER_code = (double)total.code_frame_errors / std::max(1, frames);
    r.FER = r.FER_info;
    r.AvgIter = (double)total.total_iters / std::max(1, frames);
    r.upper95_if_zero_info = (total.info_bit_errors == 0) ? 3.0 / std::max(1LL, total.total_info_bits) : 0.0;
    r.weak_concentration = diag.weak_concentration;
    r.block_imbalance = diag.block_imbalance;
    r.coupling_risk = diag.coupling_risk;
    r.rate_zone = diag.rate_zone;
    r.hot_coverage_ratio = diag.hot_coverage_ratio;
    r.hot_top10_edge_share = diag.hot_top10_edge_share;
    r.hot_duplicate_pattern_ratio = diag.hot_duplicate_pattern_ratio;
    r.hot_max_use_count = diag.hot_max_use_count;
    r.hot_pair_reuse_max = diag.hot_pair_reuse_max;
    r.hot_quality_score = diag.hot_quality_score;
    r.ext_coverage_ratio = diag.ext_coverage_ratio;
    r.ext_top10_edge_share = diag.ext_top10_edge_share;
    r.ext_duplicate_pattern_ratio = diag.ext_duplicate_pattern_ratio;
    r.ext_max_use_count = diag.ext_max_use_count;
    r.ext_pair_reuse_max = diag.ext_pair_reuse_max;
    r.ext_quality_score = diag.ext_quality_score;
    r.adaptive_reason = diag.adaptive_reason;
    return r;
}

static ResultRow run_scheme(const string& exp, const LDPCCode& base, const string& scheme,
                            double target_rate, int frames, double eb, int max_iter,
                            double alpha, uint64_t seed, int serial) {
    StructDiag diag = diagnose_structure(base, target_rate);
    string actual_mode = scheme;
    string selected_mode = scheme;

    if (scheme == "NoRepeat") {
        double r0 = (double)base.k / base.n;
        StructDiag d0 = diagnose_structure(base, r0);
        return run_code(exp, base, scheme, "NoRepeat", base, base.n, r0, frames, eb,
                        max_iter, alpha, make_seed(seed, exp + scheme + base.name, serial), nullptr, d0);
    }

    if (scheme == "UniformRepeat" || scheme == "RepeatKeep") {
        int tx = target_tx_len(base.k, base.n, target_rate);
        double rate = (double)base.k / tx;
        vector<int> cnt = repeat_counts(base, target_rate, scheme);
        return run_code(exp, base, scheme, scheme, base, tx, rate, frames, eb,
                        max_iter, alpha, make_seed(seed, exp + scheme + base.name, serial), &cnt, diag);
    }

    if (scheme == "AdaptiveStructV2") {
        actual_mode = choose_adaptive_struct_v2(base, target_rate, &diag);
        selected_mode = actual_mode;
    }

    if (scheme.rfind("AdaptiveSelectV8_", 0) == 0) {
        actual_mode = scheme.substr(string("AdaptiveSelectV8_").size());
        selected_mode = actual_mode;
    }

    LDPCCode ext = make_extended_code(base, target_rate, actual_mode, 5);
    int tx = ext.n;
    double rate = (double)base.k / tx;
    return run_code(exp, base, scheme, selected_mode, ext, tx, rate, frames, eb,
                    max_iter, alpha, make_seed(seed, exp + scheme + base.name, serial), nullptr, diag);
}

static string short_sim_select(const string& exp, const LDPCCode& base, double target_rate,
                               int short_frames, double eb, int max_iter, double alpha,
                               uint64_t seed, int serial) {
    ResultRow h = run_scheme(exp + "_short_hot", base, "HotspotOnlyParity", target_rate,
                             short_frames, eb, max_iter, alpha, seed, serial + 101);
    ResultRow e = run_scheme(exp + "_short_ext", base, "ExtParityV2", target_rate,
                             short_frames, eb, max_iter, alpha, seed, serial + 202);
    return (h.BER_info <= e.BER_info) ? "HotspotOnlyParity" : "ExtParityV2";
}

static ResultRow run_adaptive_select_v8(const string& exp, const LDPCCode& base, double target_rate,
                                        int frames, double eb, int max_iter, double alpha,
                                        uint64_t seed, int serial) {
    int short_frames = std::max(200, std::min(1000, frames / 20));
    string chosen = short_sim_select(exp, base, target_rate, short_frames, eb, max_iter, alpha, seed, serial);
    ResultRow r = run_scheme(exp, base, string("AdaptiveSelectV8_") + chosen, target_rate,
                             frames, eb, max_iter, alpha, seed, serial + 303);
    r.scheme = "AdaptiveSelectV8";
    r.selected_mode = chosen;
    r.selection_short_frames = short_frames;
    r.selection_extra_decodes = 2 * short_frames;
    return r;
}

static void write_csv(const string& path, const vector<ResultRow>& rows) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Cannot open output: " + path);
    out << "experiment,matrix,scheme,selected_mode,frames,n,k,tx_len,effective_rate,EbN0_dB,"
        << "BER,BER_info,FER,FER_info,FER_code,AvgIter,bit_errors,info_bit_errors,frame_errors,code_frame_errors,upper95_if_zero_info,"
        << "weak_concentration,block_imbalance,coupling_risk,rate_zone,"
        << "hot_coverage_ratio,hot_top10_edge_share,hot_duplicate_pattern_ratio,hot_max_use_count,hot_pair_reuse_max,hot_quality_score,"
        << "ext_coverage_ratio,ext_top10_edge_share,ext_duplicate_pattern_ratio,ext_max_use_count,ext_pair_reuse_max,ext_quality_score,"
        << "adaptive_reason,selection_short_frames,selection_extra_decodes\n";
    out << std::setprecision(12);
    for (const auto& r : rows) {
        out << r.experiment << ','
            << r.matrix << ','
            << r.scheme << ','
            << r.selected_mode << ','
            << r.frames << ','
            << r.n << ','
            << r.k << ','
            << r.tx_len << ','
            << r.effective_rate << ','
            << r.EbN0_dB << ','
            << r.BER << ','
            << r.BER_info << ','
            << r.FER << ','
            << r.FER_info << ','
            << r.FER_code << ','
            << r.AvgIter << ','
            << r.bit_errors << ','
            << r.info_bit_errors << ','
            << r.frame_errors << ','
            << r.code_frame_errors << ','
            << r.upper95_if_zero_info << ','
            << r.weak_concentration << ','
            << r.block_imbalance << ','
            << r.coupling_risk << ','
            << r.rate_zone << ','
            << r.hot_coverage_ratio << ','
            << r.hot_top10_edge_share << ','
            << r.hot_duplicate_pattern_ratio << ','
            << r.hot_max_use_count << ','
            << r.hot_pair_reuse_max << ','
            << r.hot_quality_score << ','
            << r.ext_coverage_ratio << ','
            << r.ext_top10_edge_share << ','
            << r.ext_duplicate_pattern_ratio << ','
            << r.ext_max_use_count << ','
            << r.ext_pair_reuse_max << ','
            << r.ext_quality_score << ','
            << r.adaptive_reason << ','
            << r.selection_short_frames << ','
            << r.selection_extra_decodes << '\n';
    }
}

static int frames_arg(int argc, char** argv, int def = 20000) {
    return (argc > 1) ? std::max(1, std::atoi(argv[1])) : def;
}

static string output_arg(int argc, char** argv, const string& def) {
    return (argc > 2) ? string(argv[2]) : def;
}

static uint64_t seed_arg(int argc, char** argv, uint64_t def = 20260428ULL) {
    return (argc > 3) ? std::stoull(argv[3]) : def;
}

static void push_methods(vector<ResultRow>& rows, const string& exp, const LDPCCode& mat,
                         const vector<string>& methods, double rate, const vector<double>& ebn0s,
                         int frames, int max_iter, double alpha, uint64_t seed, int& serial) {
    for (const auto& m : methods) {
        for (double eb : ebn0s) {
            if (m == "AdaptiveSelectV8") {
                rows.push_back(run_adaptive_select_v8(exp, mat, rate, frames, eb, max_iter, alpha, seed, serial++));
            } else {
                rows.push_back(run_scheme(exp, mat, m, rate, frames, eb, max_iter, alpha, seed, serial++));
            }
            std::cerr << "[done] " << exp << " | " << mat.name << " | " << m << " | EbN0=" << eb << "\n";
        }
    }
}

#endif
