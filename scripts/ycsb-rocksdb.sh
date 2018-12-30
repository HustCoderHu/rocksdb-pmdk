#!/bin/bash
#set -x

BIN=bin/ycsb
# DB=/pmem/ycsb-rocksdb-xiaohu
DB=/mnt/ssd/ycsb-rocksdb-xiaohu

# workloadc workloadd workloadb workloada workloadf tsworkloada workloade
# WORKLOAD_ARRAY=(workloadc workloadd workloadb workloada workloadf tsworkloada workloade)
# WORKLOAD_ARRAY=(workloada workloadb workloadc workloadd workloade workloadf tsworkloada)
# LOG_DIR=log/fb/aep

# aep
WORKLOAD_ARRAY=(workloade)
DB=/pmem/ycsb-rocksdb-xiaohu
LOG_DIR=log/fb/aep

if [ ! -d $LOG_DIR ]; then
  echo dir not exists: $LOG_DIR
  mkdir -p $LOG_DIR
fi

for WORKLOAD in ${WORKLOAD_ARRAY[@]}; do
  echo 'next'
  echo WORKLOAD = ${WORKLOAD}

  echo 3 > /proc/sys/vm/drop_caches
  sync
  $BIN run rocksdb -s \
-P our-nvm-workloads/$WORKLOAD \
-p rocksdb.dir=$DB \
2>&1 | tee $LOG_DIR/$WORKLOAD.log
  echo ''
  # sleep 1s
done

# ssd
WORKLOAD_ARRAY=(workloadc workloadd workloadb workloada workloadf tsworkloada workloade)
DB=/mnt/ssd/ycsb-rocksdb-xiaohu
LOG_DIR=log/fb/ssd

for WORKLOAD in ${WORKLOAD_ARRAY[@]}; do
  echo 'next'
  echo WORKLOAD = ${WORKLOAD}

  echo 3 > /proc/sys/vm/drop_caches
  sync
  $BIN run rocksdb -s \
-P our-nvm-workloads/$WORKLOAD \
-p rocksdb.dir=$DB \
2>&1 | tee $LOG_DIR/$WORKLOAD.log
  echo ''
  # sleep 1s
done