
构思
---

# range mem tab
限制大小，间接限制 chunk 数量
等效 L0 层

fix range 文件
```
| used_bits ... | 预设 start | 预设 end |
```

```
| chunk blmFilter | chunk len | data ...   |  不定长
| chunk blmFilter | chunk len | data ...  |  不定长
| chunk blmFilter | chunk len | data ...    |  不定长


| real_start | real_end |
```
布局思路，修改少的放在文件首部，频繁修改的放末尾，通过追加的方式修改，可能会有一点点的写放大

索引文件
```
| - - - chunk meta          - - - |
| - - - chunk meta          - - - |
...
...
| - - - Stat                - - - |
```

数据和索引更新一致性

chunk 数量 总大小
每个 chunk 的范围

## 读写竞争
比如 compact 结束时的 cleanup 和 追加，方案有很多种
比如
- 两块空间交替
- 循环空间首尾标记

## compaction 触发
rangeTab 申请的空间是开始就确定的，如果某次append达到空间上限，数据无法全部存下，被动触发 compaction  

compaction 线程主动检测 rangeTab 状态

# chunk 安排
连续写入到文件
同时记录偏移位置形成索引


## 规避小写
stat & global bloom filter 保存在内存中，避免频繁更新
数据库恢复时重新计算，因为数据量很大，可以考虑使用 GPU

# Range Mem Set
管理结构，记录的只是 range mem tab

申请一个 PMEMobjpool 其中一个 map 结构记录 key前缀 -> rangeMem 结构的映射

key 的hash参考 cityhash

# chunk 迭代
返回的指针要通过 D_RO 读取数据 ?


# reference
cityhash
<https://blog.csdn.net/yfkiss/article/details/7337382>  