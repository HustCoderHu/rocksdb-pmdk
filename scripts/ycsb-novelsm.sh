#!/bin/bash
#set -x

# workloadc workloadd workloadb workloada workloadf tsworkloada workloade
# WORKLOAD_ARRAY=(workloadc workloadd workloadb workloada workloadf tsworkloada workloade)
# WORKLOAD_ARRAY=(workloada workloadb workloadc workloadd workloade workloadf tsworkloada)

WORKLOAD_ARRAY=(workloadc workloadd workloadb workloada workloadf tsworkloada workloade)
DB=/mnt/ssd/xiaohu/novelsm
LOG_DIR=novelsm_log

if [ ! -d $LOG_DIR ]; then
  echo dir not exists: $LOG_DIR
  mkdir -p $LOG_DIR
fi

for WORKLOAD in ${WORKLOAD_ARRAY[@]}; do
  echo 'next'
  echo WORKLOAD = ${WORKLOAD}

  # echo 3 > /proc/sys/vm/drop_caches
  # sync
  # bin/ycsb run leveldbjni \
# -P our-nvm-workloads/$WORKLOAD \
# -p rocksdb.dir=$DB \
# 2>&1 | tee $LOG_DIR/$WORKLOAD.log
  echo ''
  # sleep 1s
done