
```bash
mkdir build -p
cd build
cmake -GNinja -DWITH_PYTHON3=3 -DCMAKE_CXX_COMPILER=clang++-16 -DWITH_MANPAGE=OFF -DWITH_LZ4=OFF -DWITH_FUSE=OFF -DWITH_LTTNG=OFF -DWITH_RDMA=OFF -DWITH_OPENLDAP=OFF -DCMAKE_C_COMPILER=clang-16 -DWITH_BABELTRACE=OFF -DWITH_JAEGER=OFF -DWITH_RADOSGW=OFF -DWITH_XFS=OFF -DWITH_BLUESTORE=OFF ..
```

### Cluster Map 在存储节点和客户端之间互相复制。客户端也需要复制。Cluster Map 代表整个集群的数据分布，客户端可以在逻辑上把整个集群当成一个单一的存储对象

* PG: a logical collection of objects that are replicated by the same set of devices
* Each object’s PG is determined by a hash of the object name o, the desired level of replication r, and a bit mask m that controls the total number of placement groups in the system. That is, pgid = (r, hash(o)&m), where & is a bit-wise AND and the mask m = $2^k−1$, constraining the number of PGs by a power of two
* Each PG is mapped to an ordered list of `r` OSDs. Our implementation utilizes `CRUSH`, a robust replica distribution algorithm that calculates a stable, pseudo-random mapping

### Cluster Map
* includes the current network address of all OSDs that are currently online and reachable (up)
* not online or not reachable (down)
* OSD liveness:
  - `in` devices are included in the mapping and assigned placement groups
  - `out` devices are not `in`
```dot
Store Object --1:1--> PG(Placement Group)
```
