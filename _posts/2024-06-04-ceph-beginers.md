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
- paxos begin 调用栈
```
#3  0x000055e7cd4dbe08 in Paxos::begin(ceph::buffer::v15_2_0::list&) ()
#4  0x000055e7cd4e33de in Paxos::propose_pending() ()
#5  0x000055e7cd4e772d in Paxos::trigger_propose() ()
#6  0x000055e7cd4f1aa6 in PaxosService::propose_pending() ()
#7  0x000055e7cd4f381b in PaxosService::_active() ()
#8  0x000055e7cd4f61ee in PaxosService::_active()::C_Active::finish(int) ()
#9  0x000055e7cd260c2f in Context::complete(int) ()
#10 0x000055e7cd21768c in void finish_contexts<std::__cxx11::list<Context*, std::allocator<Context*> > >(ceph::common::CephContext*, std::__cxx11::list<Context*, std::allocator<Context*> >&, int) ()
#11 0x000055e7cd4ddd5d in Paxos::finish_round() ()
#12 0x000055e7cd4db9af in Paxos::handle_last(boost::intrusive_ptr<MonOpRequest>) ()
#13 0x000055e7cd4e6db1 in Paxos::dispatch(boost::intrusive_ptr<MonOpRequest>) ()
#14 0x000055e7cd1f8d5f in Monitor::dispatch_op(boost::intrusive_ptr<MonOpRequest>) ()
#15 0x000055e7cd1f45e0 in Monitor::_ms_dispatch(Message*) ()
#16 0x000055e7cd2265e5 in Monitor::ms_dispatch(Message*) ()
#17 0x000055e7cd226662 in Dispatcher::ms_dispatch2(boost::intrusive_ptr<Message> const&) ()
#18 0x00007fb4bfba95aa in Messenger::ms_deliver_dispatch(boost::intrusive_ptr<Message> const&) ()
   from /home/todd/kata/src/ceph/build/lib/libceph-common.so.2
#19 0x00007fb4bfba775d in DispatchQueue::entry() () from /home/todd/kata/src/ceph/build/lib/libceph-common.so.2
#20 0x00007fb4bfd3cf09 in DispatchQueue::DispatchThread::entry() () from /home/todd/kata/src/ceph/build/lib/libceph-common.so.2
#21 0x00007fb4bf92b3a4 in Thread::entry_wrapper() () from /home/todd/kata/src/ceph/build/lib/libceph-common.so.2
#22 0x00007fb4bf92b305 in Thread::_entry_func(void*) () from /home/todd/kata/src/ceph/build/lib/libceph-common.so.2
#23 0x00007fb4bdc9dea7 in start_thread (arg=<optimized out>) at pthread_create.c:477
#24 0x00007fb4bd828a6f in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
```

- 开始选举
```cpp
void Monitor::start_election()
```