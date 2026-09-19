#!/bin/bash

filename="$(date +%Y.%m%d.%H%M)"
echo ${filename}
mkdir ${filename}
cd ${filename}
pwd

# 起動中のProcess IDを格納する配列
pids=()

# log を保存
ssm-logger -l config.log -n toyosu_config -i 0 &
pids+=($!)
ssm-logger -l spur_odometry.log -n spur_odometry -i 0 &
pids+=($!)
ssm-logger -l odom_gl.log -n odom_gl -i 0 &
pids+=($!)
# ssm-logger -l urg_fs.log        -n urg_fs        -i 0 &
# pids+=($!)
# ssm-logger -l ndt_gl.log        -n ndt_gl        -i 0 &
# pids+=($!)
# ssm-logger -l estim_gl.log      -n localizer     -i 0 &
# pids+=($!)
# ssm-logger -l wp_gl.log         -n wp_gl         -i 0 &
# pids+=($!)

# Ctrl-C で全て終了
trap 'kill "${pids[@]}" 2>/dev/null' INT TERM

echo "recording... press CTRL-C to stop"
wait
