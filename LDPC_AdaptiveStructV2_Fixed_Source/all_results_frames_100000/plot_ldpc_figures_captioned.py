# -*- coding: utf-8 -*-
"""
最终论文图形绘制脚本（中文标题版）
生成图4-1到图4-8，与第四章终稿完全对应。

运行方式：
python .\plot_final_paper_figures_cn.py

输出目录：
figures_final_cn/
"""

from pathlib import Path
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


# =========================
# 0. 路径与全局设置
# =========================

BASE_DIR = Path(__file__).resolve().parent
OUT_DIR = BASE_DIR / "figures_final_cn"
OUT_DIR.mkdir(exist_ok=True)

# Windows中文字体设置
plt.rcParams["font.sans-serif"] = [
    "Microsoft YaHei",
    "SimHei",
    "Arial Unicode MS",
    "DejaVu Sans"
]
plt.rcParams["axes.unicode_minus"] = False
plt.rcParams["figure.dpi"] = 120
plt.rcParams["savefig.dpi"] = 300

# 统一颜色
COLORS = {
    "NoRepeat": "#222222",
    "UniformRepeat": "#4C78A8",
    "RepeatKeep": "#F58518",
    "UniformParity": "#4C78A8",
    "HotspotOnlyParity": "#E45756",
    "ExtParityV2": "#54A24B",
    "AdaptiveStructV2": "#7B61FF",
    "AdaptiveSelectV8": "#B279A2",
}

# 统一标记
MARKERS = {
    "NoRepeat": "o",
    "UniformRepeat": "s",
    "RepeatKeep": "D",
    "UniformParity": "s",
    "HotspotOnlyParity": "^",
    "ExtParityV2": "D",
    "AdaptiveStructV2": "*",
    "AdaptiveSelectV8": "P",
}

# 图例显示名
LABELS = {
    "NoRepeat": "NoRepeat",
    "UniformRepeat": "UniformRepeat",
    "RepeatKeep": "RepeatKeep",
    "UniformParity": "UniformParity",
    "HotspotOnlyParity": "Hotspot",
    "ExtParityV2": "ExtParityV2",
    "AdaptiveStructV2": "AdaptiveStructV2",
    "AdaptiveSelectV8": "AdaptiveSelectV8",
}


# =========================
# 1. 工具函数
# =========================

def load_csv(filename):
    path = BASE_DIR / filename
    if not path.exists():
        raise FileNotFoundError(f"找不到文件：{path}")
    return pd.read_csv(path)


def save_fig(fig, name):
    png_path = OUT_DIR / f"{name}.png"
    pdf_path = OUT_DIR / f"{name}.pdf"

    fig.savefig(png_path, bbox_inches="tight", dpi=300)
    fig.savefig(pdf_path, bbox_inches="tight")

    print(f"已保存：{png_path}")
    print(f"已保存：{pdf_path}")


def style_axis(ax, xlabel="Eb/N0 (dB)", ylabel="BER_info", logy=True):
    ax.set_xlabel(xlabel, fontsize=11)
    ax.set_ylabel(ylabel, fontsize=11)

    if logy:
        ax.set_yscale("log")

    ax.grid(True, which="both", linestyle="--", linewidth=0.6, alpha=0.55)
    ax.tick_params(axis="both", labelsize=10)


def plot_ber_lines(ax, df, schemes, title=None, ylim=None):
    """
    绘制BER_info曲线图
    """
    for scheme in schemes:
        sub = df[df["scheme"] == scheme].sort_values("EbN0_dB")

        if sub.empty:
            continue

        ax.plot(
            sub["EbN0_dB"],
            sub["BER_info"],
            label=LABELS.get(scheme, scheme),
            color=COLORS.get(scheme, None),
            marker=MARKERS.get(scheme, "o"),
            linewidth=1.8,
            markersize=5
        )

    if title:
        ax.set_title(title, fontsize=12)

    style_axis(ax, xlabel="Eb/N0 (dB)", ylabel="BER_info", logy=True)

    if ylim is not None:
        ax.set_ylim(*ylim)

    ax.legend(fontsize=9, frameon=True)


def select_row(df, matrix, scheme, ebn0, rate_target=None, rate_tol=0.02):
    """
    按矩阵、方案、Eb/N0和目标码率选取对应行
    """
    sub = df[(df["matrix"] == matrix) & (df["scheme"] == scheme)].copy()

    if rate_target is not None:
        sub = sub[(sub["effective_rate"] - rate_target).abs() <= rate_tol]

    if sub.empty:
        raise ValueError(
            f"找不到数据：matrix={matrix}, scheme={scheme}, "
            f"ebn0={ebn0}, rate={rate_target}"
        )

    sub["ebn0_diff"] = (sub["EbN0_dB"] - ebn0).abs()
    sub = sub.sort_values(["ebn0_diff", "effective_rate"])

    return sub.iloc[0]


def get_metric(df, matrix, scheme, ebn0, rate_target=None, col="BER_info"):
    row = select_row(df, matrix, scheme, ebn0, rate_target=rate_target)
    return float(row[col])


def grouped_bar(ax, categories, series_dict, title=None, ylabel="BER_info", logy=True, rotation=0):
    """
    绘制分组柱状图
    """
    names = list(series_dict.keys())
    n_group = len(categories)
    n_series = len(names)

    x = np.arange(n_group)
    width = 0.78 / n_series

    for i, name in enumerate(names):
        vals = series_dict[name]
        pos = x - 0.39 + width / 2 + i * width

        ax.bar(
            pos,
            vals,
            width=width,
            color=COLORS.get(name, None),
            label=LABELS.get(name, name),
            alpha=0.92
        )

    ax.set_xticks(x)
    ax.set_xticklabels(categories, fontsize=10, rotation=rotation, ha="center")

    if title:
        ax.set_title(title, fontsize=12)

    style_axis(ax, xlabel="", ylabel=ylabel, logy=logy)

    ax.legend(fontsize=9, frameon=True)


# =========================
# 2. 读取数据
# =========================

df1 = load_csv("Experiment1_Baseline_RouteValidation_exp1_results.csv")
df2 = load_csv("Experiment2_Hotspot_vs_UniformParity_exp2_results.csv")
df3 = load_csv("Experiment3_Hotspot_vs_ExtParityV2_exp3_results.csv")
df4 = load_csv("Experiment4_AdaptiveStructV2_Core_exp4_results.csv")
df5 = load_csv("Experiment5_MainResult_R0348_exp5_results.csv")
df6 = load_csv("Experiment6_MainResult_R04_exp6_results.csv")
df7 = load_csv("Experiment7_Generalization_MultiMatrix_exp7_results.csv")
df8 = load_csv("Experiment8_ThresholdScan_exp8_results.csv")
df9 = load_csv("Experiment9_Engineering_AdaptiveSelect_vs_Struct_exp9_results.csv")


# =========================
# 图4-1 不同保护路线
# =========================

fig, ax = plt.subplots(figsize=(7.5, 4.8))

sub = df6[df6["scheme"].isin([
    "NoRepeat",
    "UniformParity",
    "HotspotOnlyParity"
])]

plot_ber_lines(
    ax,
    sub,
    ["NoRepeat", "UniformParity", "HotspotOnlyParity"],
    title="不同保护路线在主矩阵A上的性能对比（R=0.400）",
    ylim=(1e-3, 1e-1)
)

save_fig(fig, "fig4_1_route_validation")
plt.close(fig)


# =========================
# 图4-2 Hotspot vs Uniform
# =========================

fig, ax = plt.subplots(figsize=(7.5, 4.8))

sub = df2[
    (df2["scheme"].isin(["UniformParity", "HotspotOnlyParity"])) &
    (df2["effective_rate"] > 0.34) &
    (df2["effective_rate"] < 0.36)
]

plot_ber_lines(
    ax,
    sub,
    ["UniformParity", "HotspotOnlyParity"],
    title="热点优先新增校验与均匀新增校验对比（R≈0.348）",
    ylim=(5e-4, 3e-2)
)

save_fig(fig, "fig4_2_hotspot_vs_uniform")
plt.close(fig)


# =========================
# 图4-3 Hotspot vs ExtParityV2
# =========================

fig, ax = plt.subplots(figsize=(7.5, 4.8))

sub = df3[
    (df3["matrix"] == "A_main_QC_12x24_Z32") &
    (df3["scheme"].isin(["HotspotOnlyParity", "ExtParityV2"])) &
    (df3["effective_rate"] > 0.34) &
    (df3["effective_rate"] < 0.36)
]

plot_ber_lines(
    ax,
    sub,
    ["HotspotOnlyParity", "ExtParityV2"],
    title="HotspotOnlyParity与ExtParityV2性能对比（R≈0.348）",
    ylim=(5e-4, 8e-3)
)

save_fig(fig, "fig4_3_hotspot_vs_ext")
plt.close(fig)


# =========================
# 图4-4 AdaptiveStructV2 核心对比
# =========================

matrices = [
    "A_main_QC_12x24_Z32",
    "B_balanced_QC_12x24_Z32",
    "C_long_QC_14x28_Z32",
    "D_short_hotspot_QC_8x16_Z32",
]

matrix_short = [
    "A_main",
    "B_balanced",
    "C_long",
    "D_short"
]

methods = [
    "HotspotOnlyParity",
    "ExtParityV2",
    "AdaptiveStructV2"
]

rates = [
    0.348141432457,
    0.4
]

rate_titles = [
    "R≈0.348",
    "R=0.400"
]

fig, axes = plt.subplots(1, 2, figsize=(13.5, 4.8), sharey=True)

for ax, rate, rate_title in zip(axes, rates, rate_titles):

    series = {m: [] for m in methods}

    for mat in matrices:
        for method in methods:
            val = get_metric(
                df4,
                mat,
                method,
                2.0,
                rate_target=rate,
                col="BER_info"
            )
            series[method].append(val)

    grouped_bar(
        ax,
        matrix_short,
        series,
        title=f"AdaptiveStructV2核心对比（{rate_title}, Eb/N0=2.0 dB）",
        ylabel="BER_info",
        logy=True,
        rotation=0
    )

    ax.set_ylim(5e-5, 1.2e-2)

save_fig(fig, "fig4_4_adaptive_core")
plt.close(fig)


# =========================
# 图4-5 主结果 R≈0.348
# =========================

fig, ax = plt.subplots(figsize=(8.2, 5.0))

sub = df5[df5["scheme"].isin([
    "NoRepeat",
    "RepeatKeep",
    "UniformParity",
    "HotspotOnlyParity",
    "ExtParityV2",
    "AdaptiveStructV2"
])]

plot_ber_lines(
    ax,
    sub,
    [
        "NoRepeat",
        "RepeatKeep",
        "UniformParity",
        "HotspotOnlyParity",
        "ExtParityV2",
        "AdaptiveStructV2"
    ],
    title="主矩阵A在R≈0.348条件下的BER_info对比",
    ylim=(5e-4, 8e-2)
)

save_fig(fig, "fig4_5_main_r0348")
plt.close(fig)


# =========================
# 图4-6 主结果 R=0.400
# =========================

fig, ax = plt.subplots(figsize=(8.2, 5.0))

sub = df6[df6["scheme"].isin([
    "NoRepeat",
    "RepeatKeep",
    "UniformParity",
    "HotspotOnlyParity",
    "ExtParityV2",
    "AdaptiveStructV2"
])]

plot_ber_lines(
    ax,
    sub,
    [
        "NoRepeat",
        "RepeatKeep",
        "UniformParity",
        "HotspotOnlyParity",
        "ExtParityV2",
        "AdaptiveStructV2"
    ],
    title="主矩阵A在R=0.400条件下的BER_info对比",
    ylim=(2e-3, 8e-2)
)

save_fig(fig, "fig4_6_main_r04")
plt.close(fig)


# =========================
# 图4-7 多矩阵泛化
# =========================

matrices_gen = [
    "B_balanced_QC_12x24_Z32",
    "C_long_QC_14x28_Z32",
    "D_short_hotspot_QC_8x16_Z32",
]

matrix_gen_short = [
    "B_balanced",
    "C_long",
    "D_short"
]

methods_gen = [
    "UniformParity",
    "HotspotOnlyParity",
    "ExtParityV2",
    "AdaptiveStructV2"
]

series = {m: [] for m in methods_gen}

for mat in matrices_gen:
    for method in methods_gen:
        val = get_metric(
            df7,
            mat,
            method,
            2.0,
            rate_target=0.4,
            col="BER_info"
        )
        series[method].append(val)

fig, ax = plt.subplots(figsize=(8.8, 5.0))

grouped_bar(
    ax,
    matrix_gen_short,
    series,
    title="多矩阵泛化中不同方法的BER_info对比",
    ylabel="BER_info",
    logy=True
)

ax.set_ylim(1e-4, 3e-2)

save_fig(fig, "fig4_7_generalization")
plt.close(fig)


# =========================
# 图4-8 阈值扫描与工程化选择对比
# =========================

fig, axes = plt.subplots(1, 2, figsize=(13.5, 4.8))

# 左图：阈值扫描
sub = df8[df8["scheme"].isin([
    "NoRepeat",
    "HotspotOnlyParity",
    "AdaptiveStructV2"
])]

plot_ber_lines(
    axes[0],
    sub,
    [
        "NoRepeat",
        "HotspotOnlyParity",
        "AdaptiveStructV2"
    ],
    title="阈值扫描：BER_info随Eb/N0变化",
    ylim=(1e-4, 8e-2)
)

# 右图：工程化选择对比
eng_matrices = [
    "A_main_QC_12x24_Z32",
    "B_balanced_QC_12x24_Z32"
]

eng_short = [
    "A_main",
    "B_balanced"
]

eng_methods = [
    "AdaptiveStructV2",
    "AdaptiveSelectV8"
]

series = {m: [] for m in eng_methods}

for mat in eng_matrices:
    for method in eng_methods:
        val = get_metric(
            df9,
            mat,
            method,
            2.0,
            rate_target=0.4,
            col="BER_info"
        )
        series[method].append(val)

grouped_bar(
    axes[1],
    eng_short,
    series,
    title="工程化选择对比（Eb/N0=2.0 dB）",
    ylabel="BER_info",
    logy=True
)

axes[1].set_ylim(1e-4, 1e-2)

# 中文工程开销说明
axes[1].text(
    0.03,
    0.04,
    "选择开销：\n"
    "AdaptiveStructV2：0帧短仿真，0次额外译码\n"
    "AdaptiveSelectV8：1000帧短仿真，2000次额外译码",
    transform=axes[1].transAxes,
    fontsize=8.5,
    va="bottom",
    ha="left",
    bbox=dict(
        boxstyle="round,pad=0.35",
        fc="white",
        ec="gray",
        alpha=0.9
    )
)

save_fig(fig, "fig4_8_threshold_engineering")
plt.close(fig)


print("\n全部论文终稿图形已绘制完成。")
print(f"输出目录：{OUT_DIR}")
