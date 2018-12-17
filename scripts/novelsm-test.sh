#!/bin/bash
#set -x

# YOU DO
TEST_TMPDIR=
VALUSESZ_ARRAY=(1024 4096 16384 65536 512 64)


NUMTHREAD=1
BENCHMARKS="fillrandom,readrandom"
NUMKEYS="160000000"
#NoveLSM specific parameters
#NoveLSM uses memtable levels, always set to num_levels 2
#write_buffer_size DRAM memtable size in MBs
#write_buffer_size_2 specifies NVM memtable size; set it in few GBs for perfomance;
OTHERPARAMS="--num_levels=7 --write_buffer_size=$DRAMBUFFSZ --nvm_buffer_size=$NVMBUFFSZ"
NUMREADTHREADS="0"
VALUSESZ=512

SETUP() {
  if [ -z "$TEST_TMPDIR" ]
  then
        echo "DB path empty. Run source scripts/setvars.sh from source parent dir"
        exit
  fi
  rm -rf $TEST_TMPDIR/*
  mkdir -p $TEST_TMPDIR
}

MAKE() {
  cd $NOVELSMSRC
  #make clean
  make -j8
}


cd $NOVELSMSRC
SETUP
$APP_PREFIX $DBBENCH/db_bench --threads=$NUMTHREAD --num=$NUMKEYS \
--benchmarks=$BENCHMARKS --value_size=$VALUSESZ $OTHERPARAMS --num_read_threads=$NUMREADTHREADS
SETUP

for VALUSESZ in xx; do
  echo ${VALUSESZ}
  NUMKEYS='expr $a + $b'
  # rm -rf $TEST_TMPDIR/*
  # mkdir -p $TEST_TMPDIR
  
  # $APP_PREFIX $DBBENCH/db_bench --threads=$NUMTHREAD --num=$NUMKEYS \
    # --benchmarks=$BENCHMARKS --value_size=$VALUSESZ $OTHERPARAMS --num_read_threads=$NUMREADTHREADS \
    # > value_size-${VALUSESZ}.log 2>&1
  sleep 1s
done

#Run all benchmarks
#$APP_PREFIX $DBBENCH/db_bench --threads=$NUMTHREAD --num=$NUMKEYS --value_size=$VALUSESZ \
#$OTHERPARAMS --num_read_threads=$NUMREADTHREADS