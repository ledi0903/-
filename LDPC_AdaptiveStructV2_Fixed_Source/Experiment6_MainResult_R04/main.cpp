#include "../common/ldpc_final_common.hpp"
int main(int argc, char** argv) {
    int frames = frames_arg(argc, argv);
    string out = output_arg(argc, argv, "exp6_results.csv");
    uint64_t seed = seed_arg(argc, argv);
    int max_iter = 50;
    double alpha = 0.80;
    LDPCCode A = make_matrix_A();
    vector<ResultRow> rows;
    int serial = 1;

    push_methods(rows, "Exp6_Main_R04", A,
                 {"NoRepeat","RepeatKeep","UniformParity","HotspotOnlyParity","ExtParityV2","AdaptiveStructV2"},
                 0.4, {1.50,1.75,2.00,2.25}, frames, max_iter, alpha, seed, serial);

    write_csv(out, rows);
    std::cerr << "saved " << out << " rows=" << rows.size() << "\n";
    return 0;
}
