---
layout: post
title: "Sun 的 NFS 文件系统"
date: 2024-10-01
---

### 论文来源：[Sun’s Network File System (NFS)](https://pages.cs.wisc.edu/~remzi/OSTEP/dist-nfs.pdf#:~:text=Sun%E2%80%99s%20Network%20File%20System%20(NFS)%20One%20of%20the%20first%20uses)

# 第一种分布式计算的产物就是在分布式存储领域产生的。
# 这份论文比较老，里面说到客户端有多个，但服务端只有一个（或几个）
  - 为啥有这种需求呢？
    1. 允许在不同的客户端方便的共享文件
    2. 中心化的管理
    3. 安全。所有机器锁在一个机房里，可以避免某些安全问题的发生
# 一个基本的分布式文件系统
  - 分布式文件系统比本地文件多一些组件
  - 客户端通过客户端文件系统来访问
  - 客户应用向客户端系统发起系统调用，如 open(), read(), write(), close(), mkdir(), 等
  - 客户端看来，分布式文件系统与本地基于磁盘的文件系统，没有任何区别（除了性能）
  - 分布式文件系统提供对文件的透明访问
# 关注点
  - NFSv2，在很多年中都是标准；
  - NFSv3只有小的改动
  - MFSv4 的发动巨大
  - NFSv2 既漂亮又让人难受，所以，这里关注NFSv2

  NFSv2 的主要目标是：简单和快速的服务恢复。
  达到这一目标的手段：无状态协议。
    - 服务端不记录任何与客户端有关的信息，如：哪个客户端缓存了哪个文件？每个客户端打开了哪些文件，或者一个文件指针指向了哪个文件的哪个位置。
    - 如果服务端记录了客户端信息，则称发分布式状态
    - 分布式状态使得故障恢复变的复杂
  NFS 协议的关键是要理解文件操作符（file handle）。
    - 文件描述符有 3 个组成部分：1. volume identifier, 2. inode number, 3. 跟踪码（generation number）
      - volume identifier: 代表当前操作的是哪个文件系统。（一个NFS系统可以向外暴漏多个文件系统）
      - inode number: 表示当前操作的是哪个文件
      - generation number: 在 inode 复用的情况下，用于区分具体相同inode的不同文件，避免同时操作同一个文件
  NFS 协议的接口类型：
  |    名称    |     描述 |    返回  | 
  | --------- | ---------| ------- |
  | NFSPROC_GETATTR | file handle | returns: attributes |
  | NFSPROC_SETATTR | file handle attributes | returns: attributes |
  | NFSPROC_LOOKUP  | directory file handle, name of file/dir to look up | returns: file handle, attributes |
  | NFSPROC_READ    | file handle, offset, count, data | attributes |
  | NFSPROC_WRITE   | file handle, offset, count, data | attributes |
  | NFSPROC_CREATE  | directory file handle, name of file, attributes | file handle, attributes
  | NFSPROC_REMOVE  | directory file handle, name of file to be removed | |
  | NFSPROC_MKDIR   | directory file handle, name of directory, attributes | file handle, attributes |
  | NFSPROC_RMDIR   | directory file handle, name of directory to be removed | |
  | NFSPROC_READDIR | directory handle, count of bytes to read, cookie | directory entries, cookie (to get more entries}

 # 通过幂等请求处理服务端崩溃
 # 性能提升：客户端缓存
 # 缓存一致性问题：允许3秒的不同步，客户端要注意这种情况
 
