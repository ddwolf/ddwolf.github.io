# 构建ceph可参考 https://drunkard.github.io/install/build-ceph/
pybind报错可参考这里 [https://tracker.ceph.com/issues/62140]
报错信息如下： 
```
Error compiling Cython file:
------------------------------------------------------------
...
        """
        name = cstr(name, 'name')
        cdef:
            rados_ioctx_t _ioctx = convert_ioctx(ioctx)
            char *_name = name
            librbd_progress_fn_t _prog_cb = &no_op_progress_callback
                                            ^
------------------------------------------------------------

rbd.pyx:760:44: Cannot assign type 'int (*)(uint64_t, uint64_t, void *) except? -1' to 'librbd_progress_fn_t' (alias of 'int (*)(uint64_t, uint64_t, void *) noexcept'). Exception values are incompatible. Suggest adding 'noexcept' to the type of the value being assigned.
```

## RADOS: A Scalable, Reliable Storage Service for Petabyte-scale Storage Clusters
### Cluster Map 在存储节点和客户端之间互相复制。客户端也需要复制。Cluster Map 代表整个集群的数据分布，客户端可以在逻辑上把整个集群当成一个单一的存储对象

* PG: a logical collection of objects that are replicated by the same set of devices
* Each object’s PG is determined by a hash of the object name o, the desired level of replication r, and a bit mask m that controls the total number of placement groups in the system. That is, pgid = (r, hash(o)&m), where & is a bit-wise AND and the mask m = $2^k−1$, constraining the number of PGs by a power of two
* Each PG is mapped to an ordered list of `r` OSDs. Our implementation utilizes `CRUSH`, a robust replica distribution algorithm that calculates a stable, pseudo-random mapping
* If the active list is currently empty, PG data is temporarily unavailable, and pending I/O is blocked.
### Cluster Map
* includes the current network address of all OSDs that are currently online and reachable (up)
* not online or not reachable (down)
* OSD liveness:
  - `in` devices are included in the mapping and assigned placement groups
  - `out` devices are not `in`
* `in` and `down` means OSD is in the mapping but not reachable and PG data not remapped yet
* `out` and `up` means OSD is online but idle

### Map Propagation
* RADOS cluster may include many thousands of devices or more, it is not practical to simply broadcast map updates to all parties.
* differences in map epochs are significant only when they vary between two communicating OSDs
* This property allows RADOS to distribute map updates lazily by combining them with existing inter-OSD messages
* effectively shifting the distribution burden to OSDs.
* Each OSD maintains a history of past incremental map updates, tags all messages with its latest epoch, and keeps track of the most recent epoch observed to be present at each peer.

### INTELLIGENT STORAGE DEVICES
* RADOS currently implements 
  - n-way replication combined with -
  - per-object versions and -
  - short-term logs
  for each PG.
  procedure: 
  1. client --- single write operation ---> first primary OSD
  2. first primary OSD, update and replicates all replicas
  This shifts replication-related bandwidth to the storage cluster’s internal network and simplifies client design

#### Replica Schemes
1. Primary Copy - updates all replicas in parallel, and processes both reads and writes at the primary OSD
2. Chain - updates replicas in series: writes are sent to the primary (head), and reads to the tail, ensuring that reads always reflect fully replicated updates
3. Habrid, Splay - combines the parallel updates of primary-copy replication with the read/write role separation of chain replication

#### Strong Consistency
  All RADOS messages—both those originating from clients and from other OSDs—are tagged with the sender’s map epoch to ensure that all update operations are applied in a fully consistent fashion
- If a client sends an I/O to the wrong OSD due to an out of data map, the OSD will respond with the appropriate incrementals so that the client can redirect the request
- This avoids the need proactively share map updates with clients: they will learn about them as they interact with the storage cluster. In most cases, they will learn about updates that do not affect the current operation, but allow future I/Os to be directed accurately

#### Failure Detection
- RADOS employs an asynchronous, ordered point to point message passing library for communication
- A failure on the TCP socket results in a limited number of reconnect attempts before a failure is reported to the monitor cluster

#### Data Migration and Failure Recovery
- RADOS data migration and failure recovery are driven entirely by cluster map updates and subsequent changes in the mapping placement groups to OSDs. 
- PG recovery in RADOS is coordinated by the primary
- Thus, in the aggregate, every rereplicated object is read only once

### MONITORS
  A small cluster of monitors are collectively responsible for managing the storage system by storing the master copy of the cluster map and making periodic updates in response to configuration changes or changes in OSD state.
  The cluster, which is based in part on the Paxos part-time parliament algorithm, is designed to favor **consistency** and **durability** over __availability__ and __update__ latency
```dot
Store Object --1:1--> PG(Placement Group)
```
