#include "common/ldpc_final_common.hpp"
#include <iostream>
int main(){
    std::vector<LDPCCode> mats={make_matrix_A(),make_matrix_B(),make_matrix_C(),make_matrix_D()};
    std::vector<double> rates={0.348,0.4};
    std::cout << "matrix,rate,adaptive,reason,weak_concentration,block_imbalance,coupling_risk,"
              << "hot_cov,hot_top10,hot_dup,hot_maxuse,hot_pairmax,hot_q,"
              << "ext_cov,ext_top10,ext_dup,ext_maxuse,ext_pairmax,ext_q\n";
    for(auto &m:mats){
        for(double r:rates){
            StructDiag d; std::string sel=choose_adaptive_struct_v2(m,r,&d);
            std::cout << m.name << ',' << r << ',' << sel << ',' << d.adaptive_reason << ','
                      << d.weak_concentration << ',' << d.block_imbalance << ',' << d.coupling_risk << ','
                      << d.hot_coverage_ratio << ',' << d.hot_top10_edge_share << ',' << d.hot_duplicate_pattern_ratio << ','
                      << d.hot_max_use_count << ',' << d.hot_pair_reuse_max << ',' << d.hot_quality_score << ','
                      << d.ext_coverage_ratio << ',' << d.ext_top10_edge_share << ',' << d.ext_duplicate_pattern_ratio << ','
                      << d.ext_max_use_count << ',' << d.ext_pair_reuse_max << ',' << d.ext_quality_score << "\n";
        }
    }
}
