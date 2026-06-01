# -*- coding: utf-8 -*-
"""
绘制论文图4-8和图4-9

图4-8：阈值扫描实验结果
图4-9：AdaptiveStructV2 与 AdaptiveSelectV8 的工程化选择对比

运行方式：
python .\plot_fig4_8_4_9.py
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

plt.rcParams["font.sans-serif"] = [
    "Microsoft YaHei",
    "SimHei",
    "Arial Unicode MS",
    "DejaVu Sans"
]
plt.rcParams["axes.unicode_minus"] = False
plt.rcParams["figure.dpi"] = 120
plt.rcParams["savefig.dpi"] = 300


COLORS = {
    "NoRepeat": "#222222",
    "HotspotOnlyParity": "#E45756",
    "AdaptiveStructV2": "#7B61FF",
    "AdaptiveSelectV8": "#B279A2",
    "short_frames": "#4C78A8",
    "extra_decodes": "#F58518",
}

MARKERS = {
    "NoRepeat": "o",
    "HotspotOnlyParity": "^",
    "AdaptiveStructV2": "*",
    "AdaptiveSelectV8": "P",
}

LABELS = {
    "NoRepeat": "NoRepeat",
    "HotspotOnlyParity": "Hotspot",
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


def style_axis(ax, xlabel="", ylabel="", logy=False):
    ax.set_xlabel(xlabel, fontsize=11)
    ax.set_ylabel(ylabel, fontsize=11)

    if logy:
        ax.set_yscale("log")

    ax.grid(True, which="both", linestyle="--", linewidth=0.6, alpha=0.55)
    ax.tick_params(axis="both", labelsize=10)


def plot_ber_lines(ax, df, schemes, title=None, ylim=None):
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


def select_row(df, matrix, scheme, ebn0, rate_target=0.4, rate_tol=0.02):
    sub = df[(df["matrix"] == matrix) & (df["scheme"] == scheme)].copy()

    if rate_target is not None:
        sub = sub[(sub["effective_rate"] - rate_target).abs() <= rate_tol]

    if sub.empty:
        raise ValueError(
            f"找不到数据：matrix={matrix}, scheme={scheme}, "
            f"EbN0={ebn0}, rate={rate_target}"
        )

    sub["ebn0_diff"] = (sub["EbN0_dB"] - ebn0).abs()
    sub = sub.sort_values("ebn0_diff")

    return sub.iloc[0]


def add_bar_labels(ax, bars, fmt="{:.3g}", y_offset=3):
    for bar in bars:
        height = bar.get_height()
        ax.annotate(
            fmt.format(height),
            xy=(bar.get_x() + bar.get_width() / 2, height),
            xytext=(0, y_offset),
            textcoords="offset points",
            ha="center",
            va="bottom",
            fontsize=8
        )


# =========================
# 2. 读取数据
# =========================

df8 = load_csv("Experiment8_ThresholdScan_exp8_results.csv")
df9 = load_csv("Experiment9_Engineering_AdaptiveSelect_vs_Struct_exp9_results.csv")


# ============================================================
# 图4-8 阈值扫描实验结果
# ============================================================

fig, ax = plt.subplots(figsize=(7.8, 5.0))

sub = df8[df8["scheme"].isin([
    "NoRepeat",
    "HotspotOnlyParity",
    "AdaptiveStructV2"
])]

plot_ber_lines(
    ax,
    sub,
    ["NoRepeat", "HotspotOnlyParity", "AdaptiveStructV2"],
    title="阈值扫描：BER_info随Eb/N0变化",
    ylim=(1e-4, 8e-2)
)

save_fig(fig, "fig4_8_threshold_scan")
plt.close(fig)


# ============================================================
# 图4-9 工程化选择对比
# ============================================================

# -------------------------
# 左图数据：BER_info对比
# -------------------------

conditions = [
    ("A_main_QC_12x24_Z32", 2.0, "A_main\n2.0 dB"),
    ("A_main_QC_12x24_Z32", 2.25, "A_main\n2.25 dB"),
    ("B_balanced_QC_12x24_Z32", 2.0, "B_balanced\n2.0 dB"),
    ("B_balanced_QC_12x24_Z32", 2.25, "B_balanced\n2.25 dB"),
]

methods = ["AdaptiveStructV2", "AdaptiveSelectV8"]

ber_data = {m: [] for m in methods}

for matrix, ebn0, label in conditions:
    for method in methods:
        row = select_row(
            df9,
            matrix=matrix,
            scheme=method,
            ebn0=ebn0,
            rate_target=0.4
        )
        ber_data[method].append(float(row["BER_info"]))

condition_labels = [c[2] for c in conditions]


# -------------------------
# 右图数据：选择开销
# -------------------------

overhead_methods = ["AdaptiveStructV2", "AdaptiveSelectV8"]
short_frames = [0, 1000]
extra_decodes = [0, 2000]


# -------------------------
# 绘制图4-9
# -------------------------

fig, axes = plt.subplots(1, 2, figsize=(13.8, 5.0))

# 左侧：BER_info对比
ax = axes[0]

x = np.arange(len(condition_labels))
width = 0.36

bars1 = ax.bar(
    x - width / 2,
    ber_data["AdaptiveStructV2"],
    width,
    label="AdaptiveStructV2",
    color=COLORS["AdaptiveStructV2"],
    alpha=0.92
)

bars2 = ax.bar(
    x + width / 2,
    ber_data["AdaptiveSelectV8"],
    width,
    label="AdaptiveSelectV8",
    color=COLORS["AdaptiveSelectV8"],
    alpha=0.92
)

ax.set_xticks(x)
ax.set_xticklabels(condition_labels, fontsize=9)
ax.set_title("工程化选择性能对比", fontsize=12)
style_axis(ax, xlabel="", ylabel="BER_info", logy=True)
ax.set_ylim(1e-4, 1e-2)
ax.legend(fontsize=9, frameon=True)

# 不强制给所有柱加标签，避免拥挤；
# 如果需要标签，可取消下面两行注释
# add_bar_labels(ax, bars1, fmt="{:.2g}")
# add_bar_labels(ax, bars2, fmt="{:.2g}")


# 右侧：选择开销对比
ax = axes[1]

x2 = np.arange(len(overhead_methods))
width2 = 0.36

bars3 = ax.bar(
    x2 - width2 / 2,
    short_frames,
    width2,
    label="短仿真帧数",
    color=COLORS["short_frames"],
    alpha=0.92
)

bars4 = ax.bar(
    x2 + width2 / 2,
    extra_decodes,
    width2,
    label="额外译码次数",
    color=COLORS["extra_decodes"],
    alpha=0.92
)

ax.set_xticks(x2)
ax.set_xticklabels(overhead_methods, fontsize=9)
ax.set_title("选择阶段开销对比", fontsize=12)
style_axis(ax, xlabel="", ylabel="数量", logy=False)
ax.set_ylim(0, 2300)
ax.legend(fontsize=9, frameon=True)

add_bar_labels(ax, bars3, fmt="{:.0f}")
add_bar_labels(ax, bars4, fmt="{:.0f}")

# 添加说明文字
ax.text(
    0.02,
    0.95,
    "AdaptiveStructV2：结构诊断静态选择\n"
    "AdaptiveSelectV8：短仿真辅助选择",
    transform=ax.transAxes,
    fontsize=8.5,
    va="top",
    ha="left",
    bbox=dict(
        boxstyle="round,pad=0.35",
        fc="white",
        ec="gray",
        alpha=0.9
    )
)

fig.suptitle(
    "AdaptiveStructV2与AdaptiveSelectV8的工程化选择对比",
    fontsize=13,
    y=1.02
)

plt.tight_layout()

save_fig(fig, "fig4_9_engineering_selection")
plt.close(fig)


print("\n图4-8和图4-9已绘制完成。")
print(f"输出目录：{OUT_DIR}")
