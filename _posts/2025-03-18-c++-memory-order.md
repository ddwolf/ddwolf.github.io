---
layout: post
title: "内存序：acquire, release 介绍"
date: 2025-03-18
---

# 标题：图文解释 acquire, release 语义
内存序之所以是个问题，原因主要在于 cpu 的乱序执行和指令多发射。

在内存序领域，有两个操作比较常见，一个是 acquire, 一个是 release，acquire 主要是用于读的场景，release 主要用于定局场景。
在 C++ 中，有两个 memory_order 与之分别对应，分别是 std::memory_order_acquire, std::memory_order_release。两个 `memory_order` 主要与原子变量的操作有关。其中，acquire 类似于获取资源的原语， release 类似于释放资源的原语。我们假设线程1对原子变量执行 `A: store(memory_order_release)` 操作，线程2对同样的原子变量执行 B: load(memory_order_acquire)` 操作。假设在时间序上，A 操作先发生，当程调2用 acquire 后，**排在 A 操作之前的代码对内存所做的所有修改，对排在 B 操作之后的代码可见**。
下面我们使用代码来解释。

```
atomic<int> x{0};
int data = 0;

线程1                               | 线程2
------------------------------------|-------------------------------------
1: data = 42;                       | 
2: x.store(1, memory_order_release);|
3:                                  | int y = x.load(memory_order_acquire);
4:                                  | printf("x=%d, data=%d\n", y, data);
```

线程1执行了release，线程2执行了 acquire 操作。这两个操作保证了在执行完第 3 行后，第 4 行一定会看到 data=42。

# 详解
根据 c++ 手册的描述，store(release) 操作表示，在执行完 store(release) 后，其它线程在同一个原子变量上执行 load(acquire) 操作，会看到修改之后的值。且：
 1. load(acquire) 后面的代码不允许被重排序到 load(acquire) 的前面
 2. store(release) 前面的代码不允许重排序到 store(release) 的后面。
下面使用示例中的代码分别说明这两个限制条件。
 - 对于条件1，如果 load(acquire) 之后的 printf 被重排序到 load 之前，则虽然线程1已经修改了 data 的值为42，但 printf 有可能看不到这个最新的值，读到的可能仍然是旧的值。
 - 对于条件2，如果 store(release) 前面的 data=42 被重排序到了 store 之后，则虽然这个修改发生在线程2的printf之前，printf 仍然有可能看不到这个最新的值。

# 肋记图
可以把 acquire 和 release 想象成一个水桶的上下两个盖子。水桶里面的水（代码）不允许从上面流出去（acquire 后面的代码不允许重排到前面去），也不允许从下面漏出去（release 前面的代码不允许重排到后面去）
```
   ____ acquire
  /    \
 /~~~~~~\  <- 水(代码）
 \      /
  \____/ release
```
