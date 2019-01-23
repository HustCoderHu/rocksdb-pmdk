#!/bin/bash
#set -x

# 三台机器
# 本地, 跳板机, 执行机

# 目标: 从执行机上取回 log 到本地

# 思路: 本地通过 ssh 推送 scp 命令到跳板机
# 这条 scp 命令的作用是从执行机取 log 到跳板机
# 最后本地执行 scp 命令从跳板机取回 log

gateway=39.106.211.175
gateway_usr=pingcap
gateway_dir=/home/pingcap/forward_log

exec_machine=clx03
exec_machine_usr=root
# exec_machine_fpath=/home/hustzyw/xiaohu/ycsb-leveldbjni-binding-0.16.0
# exec_machine_fpath=/home/hustzyw/log
exec_machine_fpath=/home/hustzyw/xiaohu/ycsb-rocksdb-binding-0.15.0/log

target_file=fb
# target_file=novelsm_log
# target_file=rocksdb-4k1M-ssd.log
# target_file=hs_err_pid162479.log

CDIR="cd "${gateway_dir}
cmd_r=${CDIR}
cmd_r=${cmd_r}"; scp -r "${exec_machine_usr}@${exec_machine}:${exec_machine_fpath}/${target_file}" ."

# echo $cmd_r
# 右键粘贴跳板机密码
ssh -t -p 10622 ${gateway_usr}@${gateway} "${cmd_r}"

# 取到本地当前目录，右键粘贴跳板机密码
scp -P 10622 -r ${gateway_usr}@${gateway}:${gateway_dir}/${target_file} .