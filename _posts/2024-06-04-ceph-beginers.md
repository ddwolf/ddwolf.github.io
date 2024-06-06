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

### 测试环境
```bash
# 构建过程可能会执行pip install，可能需要科学方法
export http_proxy=xxx
export https_proxy=$http_proxy
# 构建 ceph 及 vstart. vstart 工具可以快速拉起一个调试环境
# 18/17/16版本在构建时会boost会报错，升级到19版本问题解决。可参考 [build-ceph](https://ddwolf.github.io/2024/05/05/build-ceph/) 这篇文章
# 如果是为了学习，记得一定要开启 Debug 模式（下面的-DCMAKE_BUILD_TYPE=Debug），在wsl2下构建一次实在太久了
rm -rf build; ./do_cmake.sh -DWITH_SYSTEM_BOOST=ON  -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_BUILD_TYPE=Debug
cd $CEPH_SRC/build
ninja vstart
# 通过 vstart 创建并启动集群。改`MON=1`为`MON=3`就可以测试三副本情况下的paxos
MON=1 OSD=6 MDS=0 MGR=1 RGW=0 ../src/vstart.sh -d -n -x
# 通过 ./bin/ceph mgr services 可以查到mgr的访问url。如：x.x.x.x:41324
# 之后访问网页就可以看到集群的情况了
```
![图片](https://github.com/ddwolf/ddwolf.github.io/assets/251396/33062916-fbff-48a8-9c37-ad902a2bfa61)

```bash
./bin/ceph osd pool create ceph-demo 64 64 # 64个PG，64个PGP，默认就会有3个副本
echo 123 > a.txt # 生成一个文件，后面我们把这个文件作为一个对象存储到资源池中
./bin/rados put abc a.txt -p ceph-demo # 上传文件a.txt，并命名其为abc
./bin/rados get abc b.txt -p ceph-demo # 下载名为abc的对象并保存在b.txt中
```

## 接下来就可以通过 gdb 去调试 paxos 了
