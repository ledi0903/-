# AdaptiveStructV2 论文定稿版实验代码

## 这版代码按上传的《毕设实验路径与最终自适应算法实施方案》重写

AdaptiveStructV2 的正式定义：

1. 输入基础 QC-LDPC 矩阵、Z、目标冗余预算、目标有效码率。
2. 根据矩阵结构估计变量节点弱点评分。
3. 构造两套候选新增校验：
   - HotspotOnlyParity：关键弱点优先新增校验；
   - ExtParityV2：弱点—覆盖平衡新增校验。
4. 提取结构诊断量：
   - weak_concentration：弱点集中程度；
   - block_imbalance：块间不均衡程度；
   - coupling_risk：局部耦合风险；
   - rate_zone：目标码率区间。
5. 使用结构诊断 + 码率门控做静态选择。
6. 一旦选择完成，固定扩展矩阵，正式通信阶段不再短仿真、不反馈、不额外发送。

## 重要修正

- AdaptiveStructV2 不是写死成 ExtParityV2。
- AdaptiveStructV2 也不是简单固定比例混合。
- AdaptiveStructV2 是按结构诊断在 HotspotOnlyParity 和 ExtParityV2 中静态选择。
- CSV 中包含 selected_mode，可查看最终选择。
- AdaptiveSelectV8 保留为短仿真自适应对照，只用于实验9。

## 运行

解压到英文路径，例如：

```powershell
cd E:\code\LDPC_AdaptiveStructV2_Final
powershell -ExecutionPolicy Bypass -File .\run_all_experiments.ps1 -Frames 100 -Seed 20260428
```

测试通过后：

```powershell
powershell -ExecutionPolicy Bypass -File .\run_all_experiments.ps1 -Frames 20000 -Seed 20260428
```

20 万帧：

```powershell
powershell -ExecutionPolicy Bypass -File .\run_all_experiments.ps1 -Frames 200000 -Seed 20260428
```

## 输出

结果位于：

```text
all_results_frames_20000
all_results_frames_200000
```

CSV 重点查看：

- scheme
- selected_mode
- BER_info
- FER
- AvgIter
- weak_concentration
- block_imbalance
- coupling_risk
- rate_zone


## 本次算法修复说明

本修复版不是只改 CSV 或论文表述，而是改了公共算法 `common/ldpc_final_common.hpp`：

- `HotspotOnlyParity`：保留“关键弱点优先”，但新增覆盖率、节点最大使用次数、块使用、pair 复用和重复信息子集约束，避免冗余过度集中。
- `ExtParityV2`：实现为“弱点-覆盖-重复惩罚”平衡构造，覆盖约束更强。
- `AdaptiveStructV2`：仍然只在 `HotspotOnlyParity` 和 `ExtParityV2` 之间静态选择；选择依据是基础矩阵结构诊断、目标码率门控和候选扩展图的结构诊断，不使用短仿真。
- `FER`：保留旧字段，但其含义为 `FER_info`；新增 `FER_code` 用于完整 decoded codeword 帧错误率。
- `AdaptiveSelectV8`：新增 `selection_short_frames` 和 `selection_extra_decodes`，便于实验 9 体现短仿真额外开销。
- 实验 8 的门槛扫描范围扩展到 1.6-4.8 dB，以更符合 BER_info < 1e-3 crossing 的实验要求。

建议重新跑正式 20000 帧结果，旧压缩包内的历史 CSV 不再作为修复后算法结论使用。
