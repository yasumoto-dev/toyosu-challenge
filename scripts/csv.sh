#!/bin/bash
# odom_loggerのCSVをlogs/<日時>_csv/odom.csvに保存するsh

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

filename="$(date +%Y.%m%d.%H%M)_csv"
echo ${filename}
mkdir -p "${ROOT}/logs/${filename}"
cd "${ROOT}/logs/${filename}" || exit 1
pwd

echo "recording... press CTRL-C to stop"
"${ROOT}/bin/odom_logger" >odom.csv
