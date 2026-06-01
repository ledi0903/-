#include "../common/ldpc_final_common.hpp"

int main(int argc, char** argv) {
    int frames = frames_arg(argc, argv);
    string out = output_arg(argc, argv, "exp10_results.csv");
    uint64_t seed = seed_arg(argc, argv);

    int max_iter = 50;
    double alpha = 0.80;

    vector<LDPCCode> mats = {
        make_matrix_A(),
        make_matrix_B(),
        make_matrix_C(),
        make_matrix_D()
    };

    vector<ResultRow> rows;
    int serial = 1;

    /*
     * Experiment10: Fair-rate comparison
     *
     * Fair comparison principle:
     * 1. All compared methods use the same target effective code rate.
     * 2. All compared methods use the same Eb/N0 points.
     * 3. All compared methods use the same LDPC matrix and decoder parameters.
     * 4. Only the redundancy construction strategy is changed.
     *
     * NoRepeat is not included because its effective rate is the original
     * LDPC rate, while the protected schemes operate at lower target rates.
     *
     * Method hierarchy:
     * - UniformRepeat     : naive uniform repetition redundancy
     * - RepeatKeep        : selected repetition protection
     * - UniformParity     : uniform added parity redundancy
     * - HotspotOnlyParity : hotspot-oriented fixed parity construction
     * - ExtParityV2       : coverage-balanced fixed parity construction
     * - AdaptiveStructV2  : proposed structure-adaptive construction
     */

    vector<string> fair_methods = {
        "UniformRepeat",
        "RepeatKeep",
        "UniformParity",
        "HotspotOnlyParity",
        "ExtParityV2",
        "AdaptiveStructV2"
    };

    /*
     * R ~= 0.348:
     * Redundancy-sufficient case.
     */
    for (const auto& M : mats) {
        push_methods(rows,
                     "Exp10_FairRate_R0348",
                     M,
                     fair_methods,
                     0.348,
                     {2.00, 2.25},
                     frames,
                     max_iter,
                     alpha,
                     seed,
                     serial);
    }

    /*
     * R = 0.400:
     * Redundancy-limited case.
     */
    for (const auto& M : mats) {
        push_methods(rows,
                     "Exp10_FairRate_R04",
                     M,
                     fair_methods,
                     0.400,
                     {2.00, 2.25},
                     frames,
                     max_iter,
                     alpha,
                     seed,
                     serial);
    }

    write_csv(out, rows);
    std::cerr << "saved " << out << " rows=" << rows.size() << "\n";
    return 0;
}
