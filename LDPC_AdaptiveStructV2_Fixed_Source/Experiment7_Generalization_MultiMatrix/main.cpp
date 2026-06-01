#include "../common/ldpc_final_common.hpp"
int main(int argc, char** argv) {
    int frames = frames_arg(argc, argv);
    string out = output_arg(argc, argv, "exp7_results.csv");
    uint64_t seed = seed_arg(argc, argv);
    int max_iter = 50;
    double alpha = 0.80;
    vector<LDPCCode> mats = {make_matrix_B(), make_matrix_C(), make_matrix_D()};
    vector<ResultRow> rows;
    int serial = 1;

    for (const auto& M : mats) {
        push_methods(rows, "Exp7_Generalization_R04", M,
                     {"UniformParity","HotspotOnlyParity","ExtParityV2","AdaptiveStructV2"},
                     0.4, {1.75,2.00,2.25}, frames, max_iter, alpha, seed, serial);
    }

    write_csv(out, rows);
    std::cerr << "saved " << out << " rows=" << rows.size() << "\n";
    return 0;
}
