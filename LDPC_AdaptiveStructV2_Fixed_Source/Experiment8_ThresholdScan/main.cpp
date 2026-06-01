#include "../common/ldpc_final_common.hpp"
int main(int argc, char** argv) {
    int frames = frames_arg(argc, argv);
    string out = output_arg(argc, argv, "exp8_results.csv");
    uint64_t seed = seed_arg(argc, argv);
    int max_iter = 50;
    double alpha = 0.80;
    LDPCCode A = make_matrix_A();
    vector<ResultRow> rows;
    int serial = 1;
    vector<double> eb;
    for (int i = 16; i <= 48; ++i) eb.push_back(i / 10.0); // 1.6 to 4.8 dB, dense threshold scan

    push_methods(rows, "Exp8_ThresholdScan_R04", A,
                 {"NoRepeat","HotspotOnlyParity","AdaptiveStructV2"},
                 0.4, eb, frames, max_iter, alpha, seed, serial);

    write_csv(out, rows);
    std::cerr << "saved " << out << " rows=" << rows.size() << "\n";
    return 0;
}
