---
layout: post
title: "paxos made simple 学习笔记"
date: 2024-05-16
---

** paxos made simple 学习笔记          **
分布式一致性算法的目的是在存在多个节点同时可以针对某个决议进行投票的情况下，如何就某一个决议值达成一致。





- 安全性保障（safety）            
  - 在所有被投票的值中，只有一个被最终选中     
  - 有且只能有一个被选中    
  - 不能无中生有，没被选中的，不能被变为是选中的    
- 活性保障（liveness）  
  - 整个过程不能卡死  

## 节点角色 
- proposer    
- acceptor         
- learner  

## 适用场景
- 异步
- 非拜占庭模型（non-Byzantine）不允许有被篡改的消息

## 算法
1. 首先，acceptor 需要先择一个值。如果不选，那就卡死了。因此，**acceptor 在收到第一个提议时必须要选择此值**         
2. 如果只有一个acceptor，这个acceptor挂了，整个系统就死了，所以需要有多个 `acceptor`。且只胡大多数 acceptor 选择某个值的情况下，才认为这个值是最终决议













