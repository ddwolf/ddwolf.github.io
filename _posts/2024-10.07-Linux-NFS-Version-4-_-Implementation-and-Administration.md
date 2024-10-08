---
layout: post
title: "Linux NFS Version 4: 实现和管理"
date: 2024-10-07
---

### 论文来源：[Linux NFS Version 4: Implementation and Administration](https://www.kernel.org/doc/ols/2001/nfsv4_ols.pdf#:~:text=In%20this%20paper%20we%20detail%20CITI%E2%80%99s%20refer-%20ence%20implementation%20of)

## 概述
  NFS Version 4 有许多新的特性，用于解决先前版本的问题，如安全性和 `WAN` 环境下的性能。这篇文章介绍了 `CITI` 在 `kernel` 中对 `NFS v4` 的实现。这篇文章面向的是文件系统开发者和网络管理人员。
## 介绍
  分布式文件系统的优势：
    中心化管理的共享文件空间；面向地埋空间上随机分布的用户
  劣势：
    性能，安全性，跨平台行为一致性，管理成本
  成熟产品：
    `NFS`，`AFS`，`CIFS`，`Novell`，`AppleTalk`。每种产品都有各自的取舍
  NFS v1:
    面向 `LAN`，强调简单的服务，快速恢复，良好的读写㖔吐。安全不是重点，因为网络基本都是企业的内网
  NFS v2 & v3:
    不适用于高性能长距离安全文件访问需求（requirements for high
performance long haul secure file access）
  NFS v4:
   - 解决 v2 和 v3 中不曾遇到的新问题，与前面的版本差异巨大。此协议声称要通过新的聚合 `RPC` 降低网络延迟；提供客户端缓存一致性保障（通过文件代理：File Delegation）；提供共享锁和基于范围的锁。使用 `RPCSEC_GSS` 协议提供强安全性；在文件级别提供 `ACL`。
   - 最后，`NFS v4` 是 `IETF` 协议，有相应的扩展路径。
   - 可以参考 `RFC3010` 来了解文章中涉及的词汇。
## 命名空间的问题
  `NFS v4` 保留了高层级的命名空间的概念。通过在 `/etc/exports` 中暴露本地文件系统，并通过 mount 来加载到本地路径。
   - 服务端伪文件系统
     所有暴露出来的文件系统子树被连接到一个只读的伪文件系统中（Pseudo FS）。用户通过 `LOOKUP` 来浏览所有文件。客户端可以通过 `mount` 来挂载这些文件。文件权限由用户的 `rpc` 密钥和文件系统的权限来控制。区别于之前版本的 `nfs`，只基于 `ip` 来做权限控制。
   - 伪文件系统的特性
     - 如果客户端通过 `/etc/fstab` 来挂载 `Pseudo FS`，当服务暴露的文件系统子树出现变化时，`fstab` 不会受影响。
     - 服务端的本地命名空间和暴露的命名空间是独立的，互相可以做到独立变更。
     - 服务端把所有的 exported 的文件融合到一个一致的，可浏览的位置。
## 属主，属组，文件权限
   - `user@domain-name`
   - `group@domain-name`
  `@domain-name` 部分是可选的。
  解析 `domain-name` 到 `uid/gid` 的服务在内核之外。最简单的实现，可以利用 `/etc/passwd` 和 `/etc/group`。 `NIS` 和 `LDAP` 也可以提供类似的功能。作者提供了一个叫 `GSSD` 的服务来提供此功能。底层利用的是 `/etc/passwd` 和 `/etc/group`。

## 聚合 `RPC`(Compound RPC)
  `top-down` 模式，不是原子的，在第一个出错的 `rpc` 的位置停止处理。
## 状态管理
  `NFS v4` 是有状态的，与它的早期版本 `stateless` 不同。需要 `NLM` 协议来完成 locking 操作。不再需要 `rpc.statd` 和 `rpc.lockd` 服务。
  - 首先通过 `SETCLIENTID` 来初始化，`64` 位 `clientid`；
  - 所有状态都依赖租约；一定时间内如果客户端没有继续操作，则相关状态会过期失效；
  - 有一个特别的操作：`RENEW`，只用于更新租约
  - `clientid` 是最高级别的状态，每个客户端都有自己的 `clientid`。
  - 次高级的状态是 `lockowner`。每次打开文件的时候都会尝试初始化。
  - 最低层的状态是 `stateid`。由 `OPEN` 创建，由 `CLOSE` 使用。每个 `stateid` 关联一个 `lockowner` 和一个打开的文件。
### lockowner的三种含义
  - 锁之间的竞态关系
  - 序列化的基本单元。想完成第 `N＋1` 个 `lockowner` 相关的操作，要等待第 `N` 个操作同样 `lockeowner` 的操作返回。
  - 每个文件只能被一个 `lockowner` 打开一次。
## Linux 与 NFS v4 的 lockowners
#### 客户端如何关联 lockowner?
  两种实现方式：
   - One lockowner per pid
     NFS v4 的 RFC 中建议一个 `lockowner` 关联一个唯一的标识，如 `process id`， `thread id`。但是在 `Unix` 客户端实现时遇到问题。确切的说是在实现 `Unix file descriptor`s 与 `NFS v4 stateid`s 时。原因是：`Unix` 可以通过不同的 `File Descriptors` 多次打开同一个文件；另一个问题是，`Unix` 下面，父进程可能和子进程共享同一个 `File Descriptor`。导致 `File Descriptor` 和 `NFS v4 stateid` 之间需要一种多对多的关系。实现起来，需要和很多其它的子系统交互，比如 `mmap` 等。这就导致实现起来超复杂。因此 `NFS v4` 的实现者考虑下面第二种方案。
   - One lockowner per inode
     为每一个 `inode` 定义一个 `lockowner`。调用第一个 `OPEN` 的时候发送给 `server`，调用最后一个 `CLOSE` 的时候关闭它。带来一点复杂性：客户端需要负责检测锁冲突。
  除了实现的复杂性，性能方面也有一些取舍需要考虑。
## Linux and NFS v4 OPEN
  另一个客户端需要考虑的难点是 `open()`。
  `NFS v4 OPEN` 与 `POSIX` 语义类似
   - `OPEN` 可以同时用于打开文件或创建并打开文件
   - 必须通过文件名和相应的目录句柄来打开文件。
  `linux` 环境下，`open()` 有三步：
   - `VFS` 调用文件系统的 `lookup()`，通过文件名查找文件，并为其初始化 `inode`
   - 如果文件不存在，`open()` 中的 `O_CREAT` 被用于创建文件
   - 调用文件系统的 `open()` 来打开这个文件
  分布式场景下，这种实现方式可能引入多个竞态条件。作者实现了一个叫作 `lookup_open` 的方法来处理此问题。
## Linux and NFS v4 Locks
  `NFS v4` 定义了一个 `LOCK` 操作，用于实现基于范围的文件锁。`v3` 中是通过 `NLM` 协议和 `lockd` 实例来实现类似的操作。`v4` 和 `v3` 的区别主要是：`v4` 的锁不需要回调机制。`v4` 主要通过轮询来不断的查询锁是否可用。尽管性能不好，但是这个方案只需要单向的网络通路即可，可以工作在防火墙，NAT例子等场景下
  基于范围的文件锁主要用于弥合 `Unix/Win32` 环境的差异。但是两个平台上有一个较大的不同是，是否支持子范围（sub range）。`Unix` 支持，但 `Win32` 不支持。
## 文件代理
  文件代理是 `NFS v4` 的一个特性，用于代替轮询，在客户端提供缓存一致性。
  在缓存一致性方面，代理与锁的差异：
   - 文件锁被授予一个特定客户端的一个具体的进程；文件代理作为一个整体授予缎带一个客户端。
   - 文件锁由客户端申请，文件代理由服务端主动授予。目的是用于提高性能。
   - 文件锁可以一直持有，但代理可以由服务端在任何时候回收。
## RPCSEC_GSS 实现
  `NFS v4` 需要通过 `RPCSEC_GSS` 协议实现强安全性。
   - Mandated Use: 强制使用。`GSS-API` 有两种类型: `Kerberos V5` 或 基于 `SPKM-3(RFC2847)` 的 `LIPKEY(FSC2847)`
   - 机器验证： `SETCLIENTID` 需要在 Kerberos V5 或 LIPKEY 互相认证。
   - 灵活多机制：每个文件可以有不同的安全机制
   - 安全协商：`SECINFO` 操作用于协调安全机制
   `Kerberos` 很有名气，使用广泛；`SPKM` 和 `LIPKEY` 目前只存在于 `RFC` 中。`SPKM3` 严重依赖于 `SPKM2(RFC2025)` 和 `SPKM1`。
   `SPKM3` 与先前版本（2，1）的主要区别是，它不需要initiator 的 X509 凭据。在 `SPKM3` 中使用 `initiator credentials` 也是允许的，可以避免 `man-in-the-middle` 攻击。
   `LIPKEY` 基于 `SPKM3`，它把用户名和密码加载后放在 `SPKM3` 的协议中。
### 用户态的实现
  作者直接使用 `MIT Kerberos v5 GSS-API` 来实现。
### 内核实现
## ACL(Access Control Lists)
  `Linux ACL` 实现了 `Posix` 的 `ACL`。`NFS v4` 有独立的 `ACL` 模型：丰富，细粒度。
## 管理
### 服务器端
  所有配置写在 `/etc/exports` 里面。`Linux server Pseudo filesystem` 作为一个只读文件系统被暴露出来。使用 `AUTH_UNIX` 风格的安全配置。格式如下：
    ```
    (pseudo path) (export path) [ro] [sec=[:]]
    ```
  安全相关的选项，目前只有 `krb5` 是已经实现的
    1. none (AUTH NONE)
    2. sys (AUTH SYS)
    3. dh (AUTH DES, old diffe-hellman)
    4. krb5 (RPCSEC GSS Krb5 authentication)
    5. krb5i (RPCSEC GSS Krb5 integrity)
    6. krb5p (RPCSEC GSS Krb5 protection)
    7. spkm3 (SPKM3 authentication)
    8. spkm3i (SPKM3 integrity)
    9. spkm3p (SPKM3 protection)
    10. lkey (LIPKEY authentication)
    11. lkeyi (LIPKEY integrity)
    12. lkeyp (LIPKEY protection)
  一个 `/etc/exports` 示例
    /foo /export
    /goo/dir1 /usr/local ro
    /goo/dir2 /usr/share ro sec=dh:lkeyi
    /goo/dir3 /usr/man sec=sys
    /goo/dir4 /usr/doc sec=krb5:spkm3
# a comment
#### 未完待续
