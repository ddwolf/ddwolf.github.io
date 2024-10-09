---
layout: post
title: "pNFS and Linux:Working Towards a Heterogeneous Future"
date: 2024-10-08
---

# 标题：pNFS 和 Linux: 为异构未来而奋斗
  pNFS 是一个异构元数据协议（heterogeneous metadata protocol）, 它把 `NFSv4.1` 拆分成两部分，一部分是数据路径，一部分是控制路径。`NFSv4.1` 存在于控制路径。在数据路径中的存储协议提供直接和并行的数据访问。一个管理协议将存储设备和元数据绑定到一起。
  ```
                   NFSv4.1(control)
  pNFS Client <------------------------> pNFS Server
  ^^^                                         ^
  |||                                         |
  |||                                         |
  Storage Protocol(data)              Management Protocol
  |||                                         |
  |||-------------> Storage                   |
  ||----------------> Nodes ------------------|
  |--------------------> ...
  ```
## pNFS protocol extensions
  这部分描述 `NFSv4.1` 扩展协议对 `pNFS` 的支持
   - `LAYOUTGET operation`: 此操作用于从底层存储获取对一个文件基于范围的访问信息。此操作发生成 `open()` 之后，数据访问之前。  请求的频率和范围大小由具体的实现决定。`LAYOUTGET` 返回请求的布局，更像一个透明对象，使 `pNFS` 能够支持任意布局。`pNFS` 永远不需要解析这个对象。这个对象只是一个管道，连接存储系统和布局驱动（Layout Driver）
   - `LAYOUTCOMMIT operation`: 提交对 `layout` 所作的修改。具体的修改包含：提交或丢弃预先申请的空间；更新文件结尾的位置；填充文件空洞等
   - `LAYOUTRETURN operation`: 通知 `NFSv4` server，先前获取的布局信息不再需要了。客户端可以主动返回布局信息，或在服务器端请求的时候响应布局信息。（为何客户端会有服务端不知道的布局信息？）
   - `CB_LAYOUTRECALL operation`: 如果布局信息在某个客户端是排他性占用，这时其它客户端请求访问，服务端可以从客户端收回相应的布局信息。客户端需要在返回相应的布局之前，使用要召回的布局完成所有在途请求，写回所有的buffer 脏数据。或者后面通过普通的 `NFSv4` 来完成写操作。
   - `GETDEVINFO/GETDEVLIST operations`: 遍历所有存储节点的信息。典型场景：客户端在挂载的时候通过 `GETDEVLIST` 获取所有的存储节点

## Linux pNFS
  尽管并行文件系统把控制和数据流分开，但它们之间存在着紧密的联系。用户必须适配不同的一致性和安全语义。使用 `pNFS` 协议，可以作为统一的元数据管理协议，帮应用实现跨不同存储的一致的语义接口。在现有的 `NFSv4.1` 控制协议和所有存储协议之间提供了一个框架。
### pNFS 在传统的 NFSv4 架构的基础上增了 `layout driver` 和 `transport driver`
   - `Layout Driver` 解析和使用从服务端返回的透明布局对象。`Layout` 信息包含访问一个文件任何 `Range` 所需要的所有信息。
   直接并发I/O操作包含如下步骤：
    1. `pNFS` 客户端向 `pNFS` 服务端请求布局信息
    2. `pNFS` 客户端的 `Layout Driver` 解析布局信息，把相应的I/O操作翻译为对存储介质的直接读写。
  `pNFS` 服务器可以产生布局信息，或请求底层介质协助产生布局信息。
   - `Transport Driver` 完成具体的 I/O 操作，比如利用 iSCSI 协议或 ONC RPC 直接操作存储介质。

## pNFS 客户端扩展：可插拨的存储协议
  `Layout Drivers` 是可插拨的，使用的是一组标准的接口。
   - `I/O` 接口
   - `Policy` 接口
  `I/O` 接口
    当前的 `Linux` `pNFS` 客户端协议可以通过 `O_DIRECT` 访问页缓存（`Page Cache`）; 或者通过 bypassing 的方式传递所有 Page Cache 或 NFSv4 请求。
     - page cache 通过一块缓存区域来收集或拆分写操作，整理成 `block-size` 大小的块，再发送给存储层。
     - `O_DIRECT` 方式也会使用缓存区域，但不是 Page Cache。 `O_DIRECT` 方式适用于小 I/O 的场景，如数据库应用，它通常会有自己的缓存。
     - `direct` 访问方式转发 `Linux Page Cache` 和 `NFS` 加写缓存，将 I/O 请求直接发送给 `Layout Driver`。一些科学类的软件，如 `MPI-IO` 使用这种方式。
  `Policy` 接口
    `Layout Driver` 的功能取决于具体的应用，支持的存储协议，以及底层的并发文件系统。举例：很多科学计算软件不需要数据缓存（或关联开销）。因此更倾向于使用 `O_DIRECT` 或 `direct` 的方式。其它的 `policy` 比如文件系统条带化配置，块大小，何时访问布局信息，I/O 请求的大小限制等。
    其它的 `policy` ，告诉 `layout driver` 是是否使用 `linux` 提供的服务，或者使用自我实现的服务。这些服务包括：
      - Linux Page Cache
      - NFSv4 writeback cache
      - Linux kernel readahead
    `layout driver` 可以自定义 `policy` 或者使用由 `pNFS server` 提供的 `policy`。

## pNFS 服务端扩展
  `pNFS server` 管理 `NFSv4.1` 的状态，扩展现有的 `Linux NFS server` 接口。`pNFS server` 不提供管理接口，管理接口由 `pNFS capable file system` 提供。
  `pNFS server` 不感知具体的存储协议。它通过底层文件系统获取到存储设备及布局信息。

## Evaluation(评估)
  评估三种访问方式:`page cache`, `O_DIRECT`, `direct access`在 `PVFS2` 和 `GPFS` 下的表现。
  共三组实验：前两组使用 `PVFS2` 文件系统测试 I/O 㖔吐。变量是客户端的个数。第三组使用一个 10 Gbps 的客户端，评估 `PVFS2` 和 `GPFS` 的数据传输性能。所有节点使用 Linux 2.6.17
  O_DIRECT 访问方式可以更好的利用 disk 的带宽。O_DIRECT 方式相对 NFSv4 减少了 SYNC 操作的次数。
