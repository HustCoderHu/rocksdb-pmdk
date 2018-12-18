#!/bin/bash
#set -x

BIN=/home/hustzyw/rocksdb/rocksdb/Release/db_bench
DB=/pmem/rocksdb_nvm
VALUSESZ_ARRAY=(65536 16384 1024 512 128 64)

# SETUP
for VALUSESZ in ${VALUSESZ_ARRAY[@]}; do
  echo 'next'
  echo VALUSESZ = ${VALUSESZ}
  let num=81920000000/${VALUSESZ}
  echo num = $num
  
  rm -rf ${DB}/*
  $BIN \
--benchmarks="fillrandom,readrandom" \
--db="${DB}" \
--key_size=16 \
--reads=100000 \
--write_buffer_size=`expr 64 \* 1048576` \
--max_bytes_for_level_base=`expr 4 \* 1048576 \* 1024` \
--use_existing_db=0 \
--max_background_compactions=16 \
--value_size=$VALUSESZ \
--num=$num 2>&1 | tee $VALUSESZ.log
  echo ''
  sleep 1s
done