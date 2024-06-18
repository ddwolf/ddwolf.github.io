---
layout: post
title: "glibc-tcmalloc-笔记"
date: 2024-06-18
---

glibc tcmalloc 笔记
max_bins 64
max_fastbin 128
max_fast_size 160, 80 * SIZE_SZ / 4
unsorted_chunk  bin_at(1)
global_max_fast 128
min_large_size 1024
chunk_size 即实际大小
TCACHE_MAX_BINS 64 每个线程可以拥有多少个 thread local 的 chunk，is a arbitrary limit
checked_request2size  == req + sizeof(mchunk_size) 并按 MALIGNMENT 做对齐。这里表示：申请 req 的空间，需要增加一个 sizeof(mchunk_size) 的位置保存 chunk size，chunk size 位置前一个位置实际还有一个字段 prev_size，但是chunk处于used 状态时，这个prev_size可以用于used data的。所以，这里，前面需要一个 sizeof(prev_size)，但是这个chunk的后面是下一个chunk的prev_size，可以直接拿来当useddata用，所以两相抵销，只需要一个额外的sizeof(mchunk_size) 空间就可以了

CHUNK_HDR_SZ = 16
#define mem2chunk(mem) ((mchunkptr)tag_at (((char*)(mem) - CHUNK_HDR_SZ))) 向后偏移 16 个字节
#define chunk2mem(chk) 向前偏移 16 个字节
arena_for_chunk 先检查 head 标记位（chunsize的第3位），如果是main_arena，直接返回；再通过 64M 取余，即可拿到 heap 的地址。通过 heap->ar_ptr 即可拿到相应的arena

tcache->entries[x] 里面保存的是 chunk2mem 的结果。chunk2mem 的结果会被构造成一个tcache_entry 结构体。key可以用于double check 校验，next值是啥意思？