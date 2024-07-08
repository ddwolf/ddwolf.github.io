---
layout: post
title: "cpp-类型转换"
date: 2024-07-09
---
##### dynamic class
    A class requiring a virtual table pointer (because it or its bases have one or more virtual member functions or virtual base classes). 

在指向复杂对象的指针间做类型转换的时候，涉及到this指针的偏移。this指针偏移后指向的虚表，与那个虚表对应的类型直接生成的对象对应的虚表不是同一个虚表。类型转换后指向的虚表，是原 `most derived class` 大虚表中的某一个位置。在 `abi` 文档中，应该叫做 `secondary virtual table`