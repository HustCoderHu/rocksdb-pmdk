#!/bin/bash
#set -x

BIN=bin/ycsb

# aep
WORKLOAD_ARRAY=(c d b a f e)
# DB=/pmem/ycsb-rocksdb-xiaohu
DB=/pmem/xiaohu/fb-rocksdb-rid-merge
LOG_DIR=log/fb/rid_merge_aep

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
-P our-nvm-workloads/workload$WORKLOAD \
-p rocksdb.dir=$DB \
2>&1 | tee $LOG_DIR/$WORKLOAD.log
  echo ''
  # sleep 1s
done

# ssd
DB=/mnt/ssd/xiaohu/fb-rocksdb-rid-merge
LOG_DIR=log/fb/rid_merge_ssd

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
-P our-nvm-workloads/workload$WORKLOAD \
-p rocksdb.dir=$DB \
2>&1 | tee $LOG_DIR/$WORKLOAD.log
  echo ''
  # sleep 1s
done