---
layout: post
title: "cpp-类型转换"
date: 2024-07-08
---
##### dynamic class
    A class requiring a virtual table pointer (because it or its bases have one or more virtual member functions or virtual base classes).

在指向复杂对象的指针间做类型转换的时候，涉及到this指针的偏移。this指针偏移后指向的虚表，与那个虚表对应的类型直接生成的对象对应的虚表不是同一个虚表。类型转换后指向的虚表，是原 `most derived class` 大虚表中的某一个位置。在 `abi` 文档中，应该叫做 `secondary virtual table`
#####  secondary virtual table
    The instance of a virtual table for a base class that is embedded in the virtual table of a class derived from it.

#### 代码
```cpp
// compile with : 
// clang++ complex.cpp -g -std=c++11 -Xclang -fdump-record-layouts -Xclang -fdump-vtable-layouts
 struct A {
   int a;
   char s1[10];
   virtual void f() {}
 };
 struct E {
   int a;
   char sd[21];
   virtual void h() {}
 };
 struct B : public E {
   int b;
   char s2[20];
   virtual void g() {}
 };
 struct C : public virtual A, public virtual B {
   int c;
   char s3[30];
   virtual void f() {}
 };

 struct D : virtual public A, public C {
   int d;
   char s4[15];
   virtual void g() {}
 };
 int main() {
   D d;
   B *bd = &d;
   B *bp = new B;
 }// complex.cpp
```

- `complex.cpp` 中 `class D` 生成的虚表结构如下所示：
```
  1 Vtable for 'D' (16 entries).
  2    0 | vbase_offset (88)
  3    1 | vbase_offset (64)
  4    2 | offset_to_top (0)
  5    3 | D RTTI
  6        -- (C, 0) vtable address --
  7        -- (D, 0) vtable address --
  8    4 | void C::f()
  9    5 | void D::g()
 10    6 | vcall_offset (-64)
 11    7 | offset_to_top (-64)
 12    8 | D RTTI
 13        -- (A, 64) vtable address --
 14    9 | void C::f()
 15        [this adjustment: 0 non-virtual, -24 vcall offset offset]
 16   10 | vcall_offset (-88)
 17   11 | vcall_offset (0)
 18   12 | offset_to_top (-88)
 19   13 | D RTTI
 20        -- (B, 88) vtable address --
 21        -- (E, 88) vtable address --
 22   14 | void E::h()
 23   15 | void D::g()
 24        [this adjustment: 0 non-virtual, -32 vcall offset offset]
 25
 26 Virtual base offset offsets for 'D' (2 entries).
 27    A | -24
 28    B | -32
 29
 30 Thunks for 'void D::g()' (1 entry).
 31    0 | this adjustment: 0 non-virtual, -32 vcall offset offset
 32
 33 VTable indices for 'D' (1 entries).
 34    1 | void D::g()
```
在上面显示的结构中，第16行到第25行的范围是一个完整的 `virtual table`，这个 `virtual table` 与 `class B` 的虚表的结构是相似的，如果通过一个 `B` 类型的指针调用 `D` 类型的对象的方法时，会用到这个结构。但各种 `offset` 的值与 `class B` 的原始虚表可能是不同的。如下是 `class B` 的虚表：
```
  1 Vtable for 'B' (4 entries).
  2    0 | offset_to_top (0)
  3    1 | B RTTI
  4        -- (B, 0) vtable address --
  5        -- (E, 0) vtable address --
  6    2 | void E::h()
  7    3 | void B::g()
  8
  9 VTable indices for 'B' (1 entries).
 10    1 | void B::g()
```