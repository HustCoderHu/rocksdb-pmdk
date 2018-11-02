
构思
---

# range mem tab
限制大小，间接限制 chunk 数量
等效 L0 层

chunk 文件
```
| chunk blmFilter | chunk ...   |  不定长
| chunk blmFilter | chunk ...  |  不定长
| chunk blmFilter | chunk ...    |  不定长
...
...
```

索引文件
```
| - - - chunk meta          - - - |
| - - - chunk meta          - - - |
...
...
| - - - Stat                - - - |
```

# chunk 安排
连续写入到文件
同时记录偏移位置形成索引


## 规避小写
stat & global bloom filter 保存在内存中，避免频繁更新
数据库恢复时重新计算，因为数据量很大，可以考虑使用 GPU



# Range Mem Set
管理结构，记录的只是 range mem tab
