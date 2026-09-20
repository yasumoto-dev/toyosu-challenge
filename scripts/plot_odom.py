#!/usr/bin/env python3
# odom_logger が出力した CSV から x-y 軌跡を描画する
# 使い方: ./scripts/plot_odom.py [CSVパス]

import sys

import numpy as np
import matplotlib.pyplot as plt

DEFAULT_FILE = "logs/odometry.csv"

# --- 列の対応 (odom_logger.cpp の printf と一致させること) ---
# 0:time[s]  1:x[m]  2:y[m]  3:yaw[rad]  4:v[m/s]  5:w[rad/s]
COL_X = 1
COL_Y = 2


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_FILE

    # '#' 始まりのヘッダ行は comments 指定で読み飛ばされる
    data = np.loadtxt(path, comments="#")

    x = data[:, COL_X]
    y = data[:, COL_Y]

    fig, ax = plt.subplots(figsize=(9, 9))

    ax.plot(x, y, lw=2, color="#0072b2", label="path")
    ax.plot(x[0], y[0], "o", ms=8, color="#d55e00", label="start")

    # x軸とy軸を同縮尺にする。これが無いと直進区間が斜めに歪んで見える
    ax.set_aspect("equal", adjustable="datalim")

    ax.set_title(path)
    ax.set_xlabel("x [m]")
    ax.set_ylabel("y [m]")
    ax.grid(True, ls=":")
    ax.legend(loc="upper left")

    plt.show()


if __name__ == "__main__":
    main()
