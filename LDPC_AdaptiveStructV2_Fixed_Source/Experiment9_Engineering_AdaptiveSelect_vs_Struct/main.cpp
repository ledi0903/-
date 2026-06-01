#include "../common/ldpc_final_common.hpp"
int main(int argc, char** argv) {
    int frames = frames_arg(argc, argv);
    string out = output_arg(argc, argv, "exp9_results.csv");
    uint64_t seed = seed_arg(argc, argv);
    int max_iter = 50;
    double alpha = 0.80;
    vector<LDPCCode> mats = {make_matrix_A(), make_matrix_B()};
    vector<ResultRow> rows;
    int serial = 1;

    for (const auto& M : mats) {
        push_methods(rows, "Exp9_Engineering_R04", M,
                     {"AdaptiveStructV2","AdaptiveSelectV8","ExtParityV2","HotspotOnlyParity"},
                     0.4, {1.75,2.00,2.25}, frames, max_iter, alpha, seed, serial);
    }

    write_csv(out, rows);
    std::cerr << "saved " << out << " rows=" << rows.size() << "\n";
    return 0;
}
