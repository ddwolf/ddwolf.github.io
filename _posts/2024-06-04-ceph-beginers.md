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
# 开启日志
For each subsystem, there is a logging level for its output logs (a so-called “log level”) and a logging level for its in-memory logs (a so-called “memory level”). Different values may be set for these two logging levels in each subsystem. Ceph’s logging levels operate on a scale of 1 to 20, where 1 is terse and 20 is verbose. In certain rare cases, there are logging levels that can take a value greater than 20. The resulting logs are extremely verbose.

## 日志使用
```cpp
dout(10) << "xxx";
```
`dout` 的定义在`debug.h`中，代码如下：
  ```cpp
  #define dout(v) ldout((dout_context), (v))
  ```
`ldout` 的定义在 `dout.h` 中，代码如下：
```cpp
#define dout_prefix *_dout
#define ldout(cct, v)  dout_impl(cct, dout_subsys, v) dout_prefix
```
也就是说，`dout` 会先调用 `dout_prefix`。`dout_prefix` 的返回值是一个 `stream&` 类型，后面可以接 `<<`。 `dout_prefix` 有默认的定义，如果想在某个文件的日志中输出自己的前缀，可以参考 `Elector.cc`：
```cpp
#undef dout_prefix
#define dout_prefix _prefix(_dout, mon, get_epoch())
static ostream& _prefix(std::ostream *_dout, Monitor *mon, epoch_t epoch) {
  return *_dout << "mon." << mon->name << "@" << mon->rank
		<< "(" << mon->get_state_name()
		<< ").elector(" << epoch << ") ";
}
```

## vstart 启动时的输出，可以看到很多默认的配置信息
```
** going verbose **
rm -f core*
hostname DESKTOP-DONHL05
ip x.x.x.x
port 40112
CEPHSRC/build/bin/ceph-authtool --create-keyring --gen-key --name=mon. out2/keyring --cap mon 'allow *'
creating out2/keyring
CEPHSRC/build/bin/ceph-authtool --gen-key --name=client.admin --cap mon 'allow *' --cap osd 'allow *' --cap mds 'allow *' --cap mgr 'allow *' out2/keyring
CEPHSRC/build/bin/monmaptool --create --clobber --addv a [v2:x.x.x.x:40112,v1:x.x.x.x:40113] --addv b [v2:x.x.x.x:40114,v1:x.x.x.x:40115] --addv c [v2:x.x.x.x:40116,v1:x.x.x.x:40117] --print /tmp/ceph_monmap.32370
CEPHSRC/build/bin/monmaptool: monmap file /tmp/ceph_monmap.32370
CEPHSRC/build/bin/monmaptool: generated fsid xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
setting min_mon_release = pacific
epoch 0
fsid xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
last_changed 2024-06-21T02:03:44.581412+0800
created 2024-06-21T02:03:44.581412+0800
min_mon_release 16 (pacific)
election_strategy: 1
0: [v2:x.x.x.x:40112/0,v1:x.x.x.x:40113/0] mon.a
1: [v2:x.x.x.x:40114/0,v1:x.x.x.x:40115/0] mon.b
2: [v2:x.x.x.x:40116/0,v1:x.x.x.x:40117/0] mon.c
CEPHSRC/build/bin/monmaptool: writing epoch 0 to /tmp/ceph_monmap.32370 (3 monitors)
rm -rf -- out2/dev/mon.a
mkdir -p out2/dev/mon.a
CEPHSRC/build/bin/ceph-mon --mkfs -c out2/ceph.conf -i a --monmap=/tmp/ceph_monmap.32370 --keyring=out2/keyring
rm -rf -- out2/dev/mon.b
mkdir -p out2/dev/mon.b
CEPHSRC/build/bin/ceph-mon --mkfs -c out2/ceph.conf -i b --monmap=/tmp/ceph_monmap.32370 --keyring=out2/keyring
rm -rf -- out2/dev/mon.c
mkdir -p out2/dev/mon.c
CEPHSRC/build/bin/ceph-mon --mkfs -c out2/ceph.conf -i c --monmap=/tmp/ceph_monmap.32370 --keyring=out2/keyring
rm -- /tmp/ceph_monmap.32370
CEPHSRC/build/bin/ceph-mon -i a -c out2/ceph.conf
CEPHSRC/build/bin/ceph-mon -i b -c out2/ceph.conf
CEPHSRC/build/bin/ceph-mon -i c -c out2/ceph.conf
Populating config ...

[mgr]
        mgr/telemetry/enable = false
        mgr/telemetry/nag = false
Setting debug configs ...
creating out2/dev/mgr.x/keyring
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring -i out2/dev/mgr.x/keyring auth add mgr.x mon 'allow profile mgr' mds 'allow *' osd 'allow *'
added key for mgr.x
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring config set mgr mgr/dashboard/x/ssl_server_port 41112 --force
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring config set mgr mgr/prometheus/x/server_port 9283 --force
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring config set mgr mgr/restful/x/server_port 42112 --force
Starting mgr.x
CEPHSRC/build/bin/ceph-mgr -i x -c out2/ceph.conf
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring mgr stat
false
waiting for mgr to become available
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring mgr stat
false
waiting for mgr to become available
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring mgr stat
false
waiting for mgr to become available
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring mgr stat
false
waiting for mgr to become available
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring mgr stat
true
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring -h
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring dashboard ac-user-create admin -i out2/dashboard-admin-secret.txt administrator --force-password
{"username": "admin", "password": "$2b$12$kpVzCWhc.0UWc0I9PNdogOiD4k5eoJpeONB4R1F7mxhOnAbYyrY0.", "roles": ["administrator"], "name": null, "email": null, "lastUpdate": 1718906634, "enabled": true, "pwdExpirationDate": null, "pwdUpdateRequired": false}
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring dashboard create-self-signed-cert
Self-signed certificate created
add osd0 600533b1-b136-43f9-a846-eacaf5ec162d
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring osd new 600533b1-b136-43f9-a846-eacaf5ec162d -i out2/dev/osd0/new.json
0
CEPHSRC/build/bin/ceph-osd -i 0 -c out2/ceph.conf --mkfs --key AQAKb3Rm6iiXJhAA2/z1toU52gTWlRz47eJOug== --osd-uuid 600533b1-b136-43f9-a846-eacaf5ec162d
2024-06-21T02:03:55.233+0800 7f9a587baf80 -1 bluestore(out2/dev/osd0/block) _read_bdev_label failed to open out2/dev/osd0/block: (2) No such file or directory
2024-06-21T02:03:55.233+0800 7f9a587baf80 -1 bluestore(out2/dev/osd0/block) _read_bdev_label failed to open out2/dev/osd0/block: (2) No such file or directory
2024-06-21T02:03:55.233+0800 7f9a587baf80 -1 bluestore(out2/dev/osd0/block) _read_bdev_label failed to open out2/dev/osd0/block: (2) No such file or directory
2024-06-21T02:03:55.243+0800 7f9a587baf80 -1 bluestore(out2/dev/osd0) _read_fsid unparsable uuid
2024-06-21T02:03:55.243+0800 7f9a587baf80 -1 bluestore(out2/dev/osd0) _minimal_open_bluefs out2/dev/osd0/block.db symlink exists but target unusable: (2) No such file or directory
2024-06-21T02:03:55.243+0800 7f9a587baf80 -1 bluestore(out2/dev/osd0) _open_db failed to prepare db environment:
2024-06-21T02:03:55.593+0800 7f9a587baf80 -1 bluestore(out2/dev/osd0) mkfs failed, (5) Input/output error
2024-06-21T02:03:55.593+0800 7f9a587baf80 -1 OSD::mkfs: ObjectStore::mkfs failed with error (5) Input/output error
2024-06-21T02:03:55.593+0800 7f9a587baf80 -1  ** ERROR: error creating empty object store in out2/dev/osd0: (5) Input/output error
start osd.0
osd 0 CEPHSRC/build/bin/ceph-osd -i 0 -c out2/ceph.conf
CEPHSRC/build/bin/ceph-osd -i 0 -c out2/ceph.conf
add osd1 52e715ab-4e4b-48cd-8f43-84eb705afca2
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring osd new 52e715ab-4e4b-48cd-8f43-84eb705afca2 -i out2/dev/osd1/new.json
2024-06-21T02:03:55.753+0800 7fceb1010f80 -1  ** ERROR: unable to open OSD superblock on out2/dev/osd0: (2) No such file or directory
1
CEPHSRC/build/bin/ceph-osd -i 1 -c out2/ceph.conf --mkfs --key AQALb3RmTiU4KRAAEXvOXFSWY91+3uuo2+bgIg== --osd-uuid 52e715ab-4e4b-48cd-8f43-84eb705afca2
2024-06-21T02:03:56.143+0800 7fbb1de9ef80 -1 bluestore(out2/dev/osd1/block) _read_bdev_label failed to open out2/dev/osd1/block: (2) No such file or directory
2024-06-21T02:03:56.143+0800 7fbb1de9ef80 -1 bluestore(out2/dev/osd1/block) _read_bdev_label failed to open out2/dev/osd1/block: (2) No such file or directory
2024-06-21T02:03:56.143+0800 7fbb1de9ef80 -1 bluestore(out2/dev/osd1/block) _read_bdev_label failed to open out2/dev/osd1/block: (2) No such file or directory
2024-06-21T02:03:56.143+0800 7fbb1de9ef80 -1 bluestore(out2/dev/osd1) _read_fsid unparsable uuid
2024-06-21T02:03:56.153+0800 7fbb1de9ef80 -1 bluestore(out2/dev/osd1) _minimal_open_bluefs out2/dev/osd1/block.db symlink exists but target unusable: (2) No such file or directory
2024-06-21T02:03:56.153+0800 7fbb1de9ef80 -1 bluestore(out2/dev/osd1) _open_db failed to prepare db environment:
2024-06-21T02:03:56.493+0800 7fbb1de9ef80 -1 bluestore(out2/dev/osd1) mkfs failed, (5) Input/output error
2024-06-21T02:03:56.493+0800 7fbb1de9ef80 -1 OSD::mkfs: ObjectStore::mkfs failed with error (5) Input/output error
2024-06-21T02:03:56.493+0800 7fbb1de9ef80 -1  ** ERROR: error creating empty object store in out2/dev/osd1: (5) Input/output error
start osd.1
osd 1 CEPHSRC/build/bin/ceph-osd -i 1 -c out2/ceph.conf
CEPHSRC/build/bin/ceph-osd -i 1 -c out2/ceph.conf
add osd2 7f38089f-aa3a-43f0-8cd9-066eda9e7191
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring osd new 7f38089f-aa3a-43f0-8cd9-066eda9e7191 -i out2/dev/osd2/new.json
2024-06-21T02:03:56.593+0800 7fa21a4aff80 -1  ** ERROR: unable to open OSD superblock on out2/dev/osd1: (2) No such file or directory
2
CEPHSRC/build/bin/ceph-osd -i 2 -c out2/ceph.conf --mkfs --key AQAMb3RmPNEcIxAAKKEBiS7Q7ofGAt05WPhmoA== --osd-uuid 7f38089f-aa3a-43f0-8cd9-066eda9e7191
2024-06-21T02:03:57.053+0800 7fc9c5951f80 -1 bluestore(out2/dev/osd2/block) _read_bdev_label failed to open out2/dev/osd2/block: (2) No such file or directory
2024-06-21T02:03:57.053+0800 7fc9c5951f80 -1 bluestore(out2/dev/osd2/block) _read_bdev_label failed to open out2/dev/osd2/block: (2) No such file or directory
2024-06-21T02:03:57.053+0800 7fc9c5951f80 -1 bluestore(out2/dev/osd2/block) _read_bdev_label failed to open out2/dev/osd2/block: (2) No such file or directory
2024-06-21T02:03:57.063+0800 7fc9c5951f80 -1 bluestore(out2/dev/osd2) _read_fsid unparsable uuid
2024-06-21T02:03:57.073+0800 7fc9c5951f80 -1 bluestore(out2/dev/osd2) _minimal_open_bluefs out2/dev/osd2/block.db symlink exists but target unusable: (2) No such file or directory
2024-06-21T02:03:57.073+0800 7fc9c5951f80 -1 bluestore(out2/dev/osd2) _open_db failed to prepare db environment:
2024-06-21T02:03:57.393+0800 7fc9c5951f80 -1 bluestore(out2/dev/osd2) mkfs failed, (5) Input/output error
2024-06-21T02:03:57.393+0800 7fc9c5951f80 -1 OSD::mkfs: ObjectStore::mkfs failed with error (5) Input/output error
2024-06-21T02:03:57.393+0800 7fc9c5951f80 -1  ** ERROR: error creating empty object store in out2/dev/osd2: (5) Input/output error
start osd.2
osd 2 CEPHSRC/build/bin/ceph-osd -i 2 -c out2/ceph.conf
CEPHSRC/build/bin/ceph-osd -i 2 -c out2/ceph.conf
add osd3 d1a2b089-f171-44e8-8ac2-1b142ddfd8c3
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring osd new d1a2b089-f171-44e8-8ac2-1b142ddfd8c3 -i out2/dev/osd3/new.json
2024-06-21T02:03:57.493+0800 7f2417f9ff80 -1  ** ERROR: unable to open OSD superblock on out2/dev/osd2: (2) No such file or directory
3
CEPHSRC/build/bin/ceph-osd -i 3 -c out2/ceph.conf --mkfs --key AQANb3RmJrO7HBAA0n1K6O46QArnjd/T7ggGIg== --osd-uuid d1a2b089-f171-44e8-8ac2-1b142ddfd8c3
2024-06-21T02:03:57.933+0800 7f5e2f04af80 -1 bluestore(out2/dev/osd3/block) _read_bdev_label failed to open out2/dev/osd3/block: (2) No such file or directory
2024-06-21T02:03:57.933+0800 7f5e2f04af80 -1 bluestore(out2/dev/osd3/block) _read_bdev_label failed to open out2/dev/osd3/block: (2) No such file or directory
2024-06-21T02:03:57.933+0800 7f5e2f04af80 -1 bluestore(out2/dev/osd3/block) _read_bdev_label failed to open out2/dev/osd3/block: (2) No such file or directory
2024-06-21T02:03:57.943+0800 7f5e2f04af80 -1 bluestore(out2/dev/osd3) _read_fsid unparsable uuid
2024-06-21T02:03:57.953+0800 7f5e2f04af80 -1 bluestore(out2/dev/osd3) _minimal_open_bluefs out2/dev/osd3/block.db symlink exists but target unusable: (2) No such file or directory
2024-06-21T02:03:57.953+0800 7f5e2f04af80 -1 bluestore(out2/dev/osd3) _open_db failed to prepare db environment:
2024-06-21T02:03:58.283+0800 7f5e2f04af80 -1 bluestore(out2/dev/osd3) mkfs failed, (5) Input/output error
2024-06-21T02:03:58.283+0800 7f5e2f04af80 -1 OSD::mkfs: ObjectStore::mkfs failed with error (5) Input/output error
2024-06-21T02:03:58.283+0800 7f5e2f04af80 -1  ** ERROR: error creating empty object store in out2/dev/osd3: (5) Input/output error
start osd.3
osd 3 CEPHSRC/build/bin/ceph-osd -i 3 -c out2/ceph.conf
CEPHSRC/build/bin/ceph-osd -i 3 -c out2/ceph.conf
add osd4 77bc39e7-57bc-4ef9-b9ad-707079aaf2c5
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring osd new 77bc39e7-57bc-4ef9-b9ad-707079aaf2c5 -i out2/dev/osd4/new.json
2024-06-21T02:03:58.373+0800 7fb06fa4ef80 -1  ** ERROR: unable to open OSD superblock on out2/dev/osd3: (2) No such file or directory
4
CEPHSRC/build/bin/ceph-osd -i 4 -c out2/ceph.conf --mkfs --key AQAOb3RmOLGoFRAALIUGujhk62RLkMYlzvzMBg== --osd-uuid 77bc39e7-57bc-4ef9-b9ad-707079aaf2c5
2024-06-21T02:03:58.833+0800 7f4516c31f80 -1 bluestore(out2/dev/osd4/block) _read_bdev_label failed to open out2/dev/osd4/block: (2) No such file or directory
2024-06-21T02:03:58.833+0800 7f4516c31f80 -1 bluestore(out2/dev/osd4/block) _read_bdev_label failed to open out2/dev/osd4/block: (2) No such file or directory
2024-06-21T02:03:58.833+0800 7f4516c31f80 -1 bluestore(out2/dev/osd4/block) _read_bdev_label failed to open out2/dev/osd4/block: (2) No such file or directory
2024-06-21T02:03:58.843+0800 7f4516c31f80 -1 bluestore(out2/dev/osd4) _read_fsid unparsable uuid
2024-06-21T02:03:58.843+0800 7f4516c31f80 -1 bluestore(out2/dev/osd4) _minimal_open_bluefs out2/dev/osd4/block.db symlink exists but target unusable: (2) No such file or directory
2024-06-21T02:03:58.843+0800 7f4516c31f80 -1 bluestore(out2/dev/osd4) _open_db failed to prepare db environment:
2024-06-21T02:03:59.193+0800 7f4516c31f80 -1 bluestore(out2/dev/osd4) mkfs failed, (5) Input/output error
2024-06-21T02:03:59.193+0800 7f4516c31f80 -1 OSD::mkfs: ObjectStore::mkfs failed with error (5) Input/output error
2024-06-21T02:03:59.193+0800 7f4516c31f80 -1  ** ERROR: error creating empty object store in out2/dev/osd4: (5) Input/output error
start osd.4
osd 4 CEPHSRC/build/bin/ceph-osd -i 4 -c out2/ceph.conf
CEPHSRC/build/bin/ceph-osd -i 4 -c out2/ceph.conf
add osd5 7e4eb6b1-e77d-437f-9337-ee0b286b2ca2
CEPHSRC/build/bin/ceph -c out2/ceph.conf -k out2/keyring osd new 7e4eb6b1-e77d-437f-9337-ee0b286b2ca2 -i out2/dev/osd5/new.json
2024-06-21T02:03:59.283+0800 7f6c70863f80 -1  ** ERROR: unable to open OSD superblock on out2/dev/osd4: (2) No such file or directory
5
CEPHSRC/build/bin/ceph-osd -i 5 -c out2/ceph.conf --mkfs --key AQAPb3Rml82/EBAAH0rTDI/fZvC2frIlLma1Xw== --osd-uuid 7e4eb6b1-e77d-437f-9337-ee0b286b2ca2
2024-06-21T02:03:59.713+0800 7f38cc457f80 -1 bluestore(out2/dev/osd5/block) _read_bdev_label failed to open out2/dev/osd5/block: (2) No such file or directory
2024-06-21T02:03:59.713+0800 7f38cc457f80 -1 bluestore(out2/dev/osd5/block) _read_bdev_label failed to open out2/dev/osd5/block: (2) No such file or directory
2024-06-21T02:03:59.713+0800 7f38cc457f80 -1 bluestore(out2/dev/osd5/block) _read_bdev_label failed to open out2/dev/osd5/block: (2) No such file or directory
2024-06-21T02:03:59.723+0800 7f38cc457f80 -1 bluestore(out2/dev/osd5) _read_fsid unparsable uuid
2024-06-21T02:03:59.723+0800 7f38cc457f80 -1 bluestore(out2/dev/osd5) _minimal_open_bluefs out2/dev/osd5/block.db symlink exists but target unusable: (2) No such file or directory
2024-06-21T02:03:59.723+0800 7f38cc457f80 -1 bluestore(out2/dev/osd5) _open_db failed to prepare db environment:
2024-06-21T02:04:00.053+0800 7f38cc457f80 -1 bluestore(out2/dev/osd5) mkfs failed, (5) Input/output error
2024-06-21T02:04:00.053+0800 7f38cc457f80 -1 OSD::mkfs: ObjectStore::mkfs failed with error (5) Input/output error
2024-06-21T02:04:00.053+0800 7f38cc457f80 -1  ** ERROR: error creating empty object store in out2/dev/osd5: (5) Input/output error
start osd.5
osd 5 CEPHSRC/build/bin/ceph-osd -i 5 -c out2/ceph.conf
CEPHSRC/build/bin/ceph-osd -i 5 -c out2/ceph.conf
2024-06-21T02:04:00.133+0800 7f3e7d952f80 -1  ** ERROR: unable to open OSD superblock on out2/dev/osd5: (2) No such file or directory
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

- ceph-monitor 入口：ceph_mon.cc

- 开始选举
```cpp
void Monitor::start_election()
// 选举发送的消息：`MMonElection::OP_PROPOSE`，通过 `Elector::propose_to_peers` 来触发。
// 接收选举的代码：`Elector::dispatch` -> `handle_propose` -> `logic.receive_propose`
// 发送响应的代码：`propose_classic_handler` -> `ElectionLogic::defer` -> `elector->_defer_to`，发送的消息为`new MMonElection(MMonElection::OP_ACK, ...`
// 如果接收到`OP_PROPOSE`后，发现本节点的rank小于发起选举的节点，则不需要送响应，并决定是否由自己发起新的选举
// 接收响应的代码：`handle_ack`。更新 peer_info[from]，并调用 `ElectionLogic::receive_ack`。
void ElectionLogic::receive_ack(int from, epoch_t from_epoch)
{
  ceph_assert(from_epoch % 2 == 1); // sender in an election epoch. 单数的epoch是选举中，一旦选举成功，epoch会在bump_epoch方法中自增
  if (from_epoch > epoch) {
    ldout(cct, 5) << "woah, that's a newer epoch, i must have rebooted.  bumping and re-starting!" << dendl;
    bump_epoch(from_epoch);
    start();
    return;
  }
  // is that _everyone_?
  if (electing_me) {
    acked_me.insert(from);
    if (acked_me.size() == elector->paxos_size()) {
      // if yes, shortcut to election finish
      declare_victory();
    }
  } else {
    // ignore, i'm deferring already.
    ceph_assert(leader_acked >= 0);
  }
}
void ElectionLogic::declare_victory()
{
  ldout(cct, 5) << "I win! acked_me=" << acked_me << dendl;
  last_election_winner = elector->get_my_rank();
  last_voted_for = last_election_winner;
  clear_live_election_state();

  set<int> new_quorum;
  new_quorum.swap(acked_me);
  
  ceph_assert(epoch % 2 == 1);  // election
  bump_epoch(epoch+1);     // is over!

  elector->message_victory(new_quorum);
}
message_victory中会调用向每个节点发送 `MMonElection::OP_VICTORY` 消息 
同时会知会monitor。`mon->win_election`

Paxos 请求发送
1. Paxos::collect {send MMonPaxos::OP_COLLECT}
2. handle_collect receive OP_COLLECT {send MMonPaxos::OP_LAST}
3. handle_last receive OP_LAST {optionally send MMonPaxos::OP_COMMIT}
4. handle_commit receive OP_COMMIT {store_commit}
--------
5. Paxos::begin {send MMonPaxos::OP_BEGIN}
6. Paxos::handle_begin {send MMonPaxos::OP_ACCEPT}
7. Paxos::handle_accept {
  commit_start
}

peon节点和主节点必须有相同的last_commit才能返回OP_LEASE_OK，否则直接忽略OP_LEASE请求
