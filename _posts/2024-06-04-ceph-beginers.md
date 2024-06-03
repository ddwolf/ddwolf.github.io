---
layout: post  
title: "ceph beginers document"
date: 2024-06-04
---       

### ceph 有三种存储接口
 - CephFS (a file system)
 - RBD (block devices)
 - RADOS (an object store).

### ceph 是什么？
 - 一个存储管理器。协助存储资源存储数据。存储资源包括磁带，磁鼓，硬盘，SSD 等
 - 一个集群存储管理器。ceph 可以被安装到多台节点上，组成一个存储集群系统
 - 一个分布式存储管理器。ceph 是去中心化的。只要有多数节点的状态是 up，则系统仍然可以对外提供服务
 - 提供数据冗余

### 节点角色
 - ceph monitor: 大意是管理配置信息，包括数据的位置，cluster state，osd map, mds map 等静态信息和状态信息
 - manager: 确保整个系统负载均衡，避免出现少数热点节点。跟踪系统统计数据，如cpu,内存等使用情况
 - osd: object storage device. 是一个进程，管理单存储资源，通常是一个磁盘或ssd
 - pools: 一个存储池可以使用复制（replication）或纠删码（erasure coded）来保护数据。
 - placement groups: 是 ceph 的核心技术，主要用于数据分布算法的实现。
 - mds: metadata server. ceph-fs 的核心组件。
