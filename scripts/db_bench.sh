#!/bin/bash
 
BENCH="/home/hustzyw/xiaohu/doubleBufferV2/build/db_bench"

NVMDB="/pmem/xiaohu/rocksdb_dir"

DB="/mnt/ssd/xiaohu/rocksdb_dir"

MAPFILE=$NVMDB"/pmem.map"
BENCH_SETTING=" --benchmarks=fillrandom,stats,readrandom,stats,readrandom,stats \
                --key_size=16 \
                --reads=1000000 \
                --write_buffer_size=`expr 64 \* 1024 \* 1024` \
                --max_bytes_for_level_base=`expr  8 \* 1024  \* 1024 \* 1024` \
                --use_existing_db=0 \
                --num_high_pri_threads=1 \
                --num_low_pri_threads=2 \
                --max_background_compactions=1 \
                --compression_type=none \
                --wal_dir=$NVMDB \
                --use_nvm_write_cache=1 \
                --pmem_path=${MAPFILE} \
                --reset_nvm_write_cache=0 \
                --chunk_bloom_bits=16 \
                --range_num=64 \
                --range_size=128 \
                --prefix_bits=16 \
                --compression_ratio=1
"

#CLEAR_FILE="rm -r /pmem/xiaohu/rocksdb_dir/* && rm -r /mnt/ssd/xiaohu/rocksdb_dir/*"

#DROP_CACHE="echo > 3 /proc/sys/vm/drop_caches"

#VALUE_SIZE_SET=(65536 16384 4096 1024 256 )
VALUE_SIZE_SET=(4096)


for VALUE in ${VALUE_SIZE_SET[@]}; do
#${CLEAR_FILE}
rm -f /pmem/xiaohu/rocksdb_dir/*
rm -f /mnt/ssd/xiaohu/rocksdb_dir/*                                                                                                                                                            
#    ${DROP_CACHE}
echo > 3 /proc/sys/vm/drop_caches
sync
    sleep 1
    let NUMKEYS=200000
    let stat=10000
    let test_date=$(date +"%Y%m%d")
#echo ${CLEAR_FILE}
#   echo ${DROP_CACHE}
#   echo ${NUMKEYS}
#   echo ${date}
#${BENCH} ${BENCH_SETTING} --db=${DB} --value_size=${VALUE} --num=${NUMKEYS} --num_stat=${stat}| tee rangekv-log/rangekv-V2-randread-${VALUE}-${test_date}.log
${BENCH} ${BENCH_SETTING} --db=${DB} --value_size=${VALUE} --num=${NUMKEYS} --num_stat=${stat}

--benchmarks=fillrandom,stats,readrandom,stats,readrandom,stats \
--key_size=16 \
--reads=1000000 \
--write_buffer_size=`expr 64 \* 1024 \* 1024` \
--max_bytes_for_level_base=`expr  8 \* 1024  \* 1024 \* 1024` \
--use_existing_db=0 \
--num_high_pri_threads=1 \
--num_low_pri_threads=2 \
--max_background_compactions=1 \
--compression_type=none \
--wal_dir=/pmem/xiaohu/rocksdb_dir \
--use_nvm_write_cache=1 \
--pmem_path=/pmem/xiaohu/rocksdb_dir/pmem.map \
--reset_nvm_write_cache=0 \
--chunk_bloom_bits=16 \
--range_num=64 \
--range_size=128 \
--prefix_bits=16 \
--compression_ratio=1 \
--db=/mnt/ssd/xiaohu/rocksdb_dir --value_size=4096 --num=200000 --num_stat=10000