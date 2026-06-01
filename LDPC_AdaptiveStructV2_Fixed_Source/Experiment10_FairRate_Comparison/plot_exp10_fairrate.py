import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter


# =========================
# 1. 全局绘图参数
# =========================

plt.rcParams["font.sans-serif"] = ["SimHei", "Microsoft YaHei", "DejaVu Sans"]
plt.rcParams["font.family"] = "sans-serif"
plt.rcParams["axes.unicode_minus"] = False

plt.rcParams["font.size"] = 16
plt.rcParams["axes.labelsize"] = 18
plt.rcParams["xtick.labelsize"] = 16
plt.rcParams["ytick.labelsize"] = 16
plt.rcParams["legend.fontsize"] = 16


# =========================
# 2. 文件路径设置
# =========================

csv_file = "exp10_results.csv"


# =========================
# 3. 方法名称与中文图例
# =========================

methods = [
    "UniformRepeat",
    "RepeatKeep",
    "UniformParity",
    "HotspotOnlyParity",
    "ExtParityV2",
    "AdaptiveStructV2"
]

method_cn = {
    "UniformRepeat": "均匀重复",
    "RepeatKeep": "重点重复",
    "UniformParity": "均匀新增校验",
    "HotspotOnlyParity": "热点优先校验",
    "ExtParityV2": "覆盖平衡校验",
    "AdaptiveStructV2": "结构自适应"
}


# =========================
# 4. 矩阵真实名称与显示名称
# =========================
# 左边是图中显示的短名称，右边是 CSV 中真实的 matrix 名称

matrix_items = [
    ("A_main", "A_main_QC_12x24_Z32"),
    ("B_balanced", "B_balanced_QC_12x24_Z32"),
    ("C_long", "C_long_QC_14x28_Z32"),
    ("D_short", "D_short_hotspot_QC_8x16_Z32")
]

matrix_labels = [item[0] for item in matrix_items]


# =========================
# 5. 自动匹配列名
# =========================

def find_col(df, candidates):
    for c in candidates:
        if c in df.columns:
            return c

    raise ValueError(
        f"找不到列名，候选为：{candidates}\n"
        f"CSV 实际列名为：{list(df.columns)}"
    )


# =========================
# 6. 清洗字符串列
# =========================

def clean_string_columns(df):
    """
    去掉所有字符串列前后空格，避免匹配失败。
    """
    for col in df.columns:
        if df[col].dtype == object:
            df[col] = df[col].astype(str).str.strip()
    return df


# =========================
# 7. 对数坐标刻度格式
# =========================

def log_tick_formatter(y, pos):
    """
    BER 对数坐标刻度格式化。
    显示为 10^-2、10^-3、10^-4，避免 Unicode 数学负号字体警告。
    """
    if y <= 0:
        return ""

    exponent = int(np.round(np.log10(y)))

    # 只显示 10 的整数次幂
    if np.isclose(y, 10 ** exponent):
        return f"10^{exponent}"

    return ""


# =========================
# 8. BER 柱状图绘制函数
# =========================

def plot_ber_bar(df_plot, save_name, matrix_col, method_col, ber_col):
    x = np.arange(len(matrix_items))
    width = 0.13

    fig, ax = plt.subplots(figsize=(11, 6), dpi=300)

    # 对数坐标下 BER 不能为 0
    ber_floor = 1e-7

    for i, method in enumerate(methods):
        y = []

        for label, matrix_name in matrix_items:
            row = df_plot[
                (df_plot[matrix_col] == matrix_name) &
                (df_plot[method_col] == method)
            ]

            if len(row) == 0:
                print(f"警告：缺少数据 matrix={matrix_name}, method={method}")
                y.append(np.nan)
            else:
                val = float(row[ber_col].iloc[0])

                if val <= 0:
                    val = ber_floor

                y.append(val)

        positions = x + (i - 2.5) * width

        ax.bar(
            positions,
            y,
            width=width,
            label=method_cn[method]
        )

    ax.set_yscale("log")
    ax.yaxis.set_major_formatter(FuncFormatter(log_tick_formatter))

    ax.set_xticks(x)
    ax.set_xticklabels(matrix_labels)

    ax.set_xlabel("LDPC 校验矩阵", fontsize=18)
    ax.set_ylabel(r"信息位误码率 $BER_{info}$", fontsize=18)

    ax.tick_params(axis="both", labelsize=16)

    ax.grid(
        True,
        which="both",
        axis="y",
        linestyle="--",
        alpha=0.35
    )

    # 图例放到图外上方
    ax.legend(
        loc="lower center",
        bbox_to_anchor=(0.5, 1.02),
        ncol=3,
        fontsize=16,
        frameon=True
    )

    # 给上方图例留空间
    fig.subplots_adjust(top=0.82)

    plt.savefig(
        save_name,
        dpi=300,
        bbox_inches="tight"
    )

    plt.close()


# =========================
# 9. 平均迭代次数柱状图绘制函数
# =========================

def plot_iter_bar(df_plot, save_name, matrix_col, method_col, iter_col):
    x = np.arange(len(matrix_items))
    width = 0.13

    fig, ax = plt.subplots(figsize=(11, 6), dpi=300)

    for i, method in enumerate(methods):
        y = []

        for label, matrix_name in matrix_items:
            row = df_plot[
                (df_plot[matrix_col] == matrix_name) &
                (df_plot[method_col] == method)
            ]

            if len(row) == 0:
                print(f"警告：缺少数据 matrix={matrix_name}, method={method}")
                y.append(np.nan)
            else:
                val = float(row[iter_col].iloc[0])
                y.append(val)

        positions = x + (i - 2.5) * width

        ax.bar(
            positions,
            y,
            width=width,
            label=method_cn[method]
        )

    ax.set_xticks(x)
    ax.set_xticklabels(matrix_labels)

    ax.set_xlabel("LDPC 校验矩阵", fontsize=18)
    ax.set_ylabel("平均迭代次数", fontsize=18)

    ax.tick_params(axis="both", labelsize=16)

    ax.grid(
        True,
        axis="y",
        linestyle="--",
        alpha=0.35
    )

    # 图例放到图外上方
    ax.legend(
        loc="lower center",
        bbox_to_anchor=(0.5, 1.02),
        ncol=3,
        fontsize=16,
        frameon=True
    )

    # 给上方图例留空间
    fig.subplots_adjust(top=0.82)

    plt.savefig(
        save_name,
        dpi=300,
        bbox_inches="tight"
    )

    plt.close()


# =========================
# 10. 主程序
# =========================

def main():
    df = pd.read_csv(csv_file)

    # 清洗列名和字符串内容
    df.columns = df.columns.str.strip()
    df = clean_string_columns(df)

    print("CSV 列名如下：")
    print(list(df.columns))

    exp_col = find_col(df, ["experiment", "exp", "tag", "group"])
    matrix_col = find_col(df, ["matrix", "matrix_name", "mat", "code", "code_name"])
    method_col = find_col(df, ["method", "scheme", "algo", "name"])
    ebn0_col = find_col(df, ["EbN0_dB", "ebn0", "EbN0", "snr", "snr_db"])
    ber_col = find_col(df, ["BER_info", "ber_info", "info_ber", "BERinfo"])
    iter_col = find_col(df, ["AvgIter", "avg_iter", "iter_avg", "avg_iterations"])

    df[ebn0_col] = df[ebn0_col].astype(float)

    print("\n实际识别到的列名：")
    print(f"实验列：{exp_col}")
    print(f"矩阵列：{matrix_col}")
    print(f"方法列：{method_col}")
    print(f"Eb/N0列：{ebn0_col}")
    print(f"BER_info列：{ber_col}")
    print(f"AvgIter列：{iter_col}")

    # 只绘制 Eb/N0 = 2.25 dB 的结果
    ebn0_target = 2.25
    df_225 = df[np.isclose(df[ebn0_col], ebn0_target)]

    print(f"\n筛选 Eb/N0={ebn0_target} dB 后，数据行数：{len(df_225)}")

    # R≈0.348
    df_r0348 = df_225[df_225[exp_col] == "Exp10_FairRate_R0348"]

    # R=0.400
    df_r0400 = df_225[df_225[exp_col] == "Exp10_FairRate_R04"]

    print(f"R≈0.348 数据行数：{len(df_r0348)}")
    print(f"R=0.400 数据行数：{len(df_r0400)}")

    if df_r0348.empty:
        raise ValueError(
            "R≈0.348 数据为空，请检查 experiment 字段是否为 Exp10_FairRate_R0348"
        )

    if df_r0400.empty:
        raise ValueError(
            "R=0.400 数据为空，请检查 experiment 字段是否为 Exp10_FairRate_R04"
        )

    # 图 4-12
    plot_ber_bar(
        df_r0348,
        "图4-12_R0348_公平口径信息位误码率对比_无标题大字号.png",
        matrix_col,
        method_col,
        ber_col
    )

    # 图 4-13
    plot_ber_bar(
        df_r0400,
        "图4-13_R0400_公平口径信息位误码率对比_无标题大字号.png",
        matrix_col,
        method_col,
        ber_col
    )

    # 图 4-14
    plot_iter_bar(
        df_r0348,
        "图4-14_R0348_公平口径平均迭代次数对比_无标题大字号.png",
        matrix_col,
        method_col,
        iter_col
    )

    # 图 4-15
    plot_iter_bar(
        df_r0400,
        "图4-15_R0400_公平口径平均迭代次数对比_无标题大字号.png",
        matrix_col,
        method_col,
        iter_col
    )

    print("\n绘图完成，已生成以下文件：")
    print("图4-12_R0348_公平口径信息位误码率对比_无标题大字号.png")
    print("图4-13_R0400_公平口径信息位误码率对比_无标题大字号.png")
    print("图4-14_R0348_公平口径平均迭代次数对比_无标题大字号.png")
    print("图4-15_R0400_公平口径平均迭代次数对比_无标题大字号.png")


if __name__ == "__main__":
    main()
