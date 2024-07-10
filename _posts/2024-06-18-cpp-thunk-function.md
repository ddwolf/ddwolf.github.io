---
layout: post
title: "cpp-thunk-function"
date: 2024-06-18
---

#### Thunk函数生成

**类对象的布局**
##### 输出vtable布局的命令及参数如下
`clang++ -c virtual.cc -Xclang -fdump-vtable-layouts`

一个类对象在内存中由以下三部分顺序组成

1. 非虚直接基类（同时要排除这些直接基类里面的虚基类）
2. 类自身的数据成员
3. 虚基类（直接或间接）

对于下面三个类组成的继承关系

```cpp
struct V {double v; virtual void f() {}};
struct A {double a;};
struct B : public A, public virtual V {double j; virtual void f() override {}};
```

则B类的对象布局为：

     16          0 | struct B
     17          0 |   (B vtable pointer)
     18          8 |   struct A (base)
     19          8 |     double a
     20         16 |   double j
     21         24 |   struct V (virtual base)
     22         24 |     (V vtable pointer)
     23         32 |     double v
     24            | [sizeof=40, dsize=40, align=8,
     25            |  nvsize=24, nvalign=8]
其中，`B`作为most-derived类，`V`subobject 相对于`B`的偏移是24

假设另一个类C继承了B, `class C : public B {double m;};`，则C的内存布局为：

     28          0 | class C
     29          0 |   struct B (primary base)
     30          0 |     (B vtable pointer)
     31          8 |     struct A (base)
     32          8 |       double a
     33         16 |     double j
     34         24 |   double m
     35         32 |   struct V (virtual base)
     36         32 |     (V vtable pointer)
     37         40 |     double v
     38            | [sizeof=48, dsize=48, align=8,
     39            |  nvsize=32, nvalign=8]
其中，V subobject 相对于B的偏移是32，与B作为most-derived时不同 -- 也就是说，虚基类相对于子类的位置不是固定的。在做涉及到指针的类型转换时,如果source type和target type之间有虚继承，如：V\*->B\*或B\*->V\*，则转换的时候就需要查表(virtual table)来确定虚偏移；如果没有虚继承，如：B\*->A\*，则只需要考虑一个确定的偏移（编译时可以确定）；如果在source type和target type之间既有非虚继承，又有虚继承，则需要同时考虑这两种偏移。



**虚表**（virtual table）：

虚表是一个指针数组，包含四种类型的指针。如下图：

```llvm
177 Vtable for 'C' (8 entries).
178    0 | vbase_offset (32)
179    1 | offset_to_top (0)
180    2 | C RTTI
181        -- (B, 0) vtable address --
182        -- (C, 0) vtable address --
183    3 | void B::f()
184    4 | vcall_offset (-32)
185    5 | offset_to_top (-32)
186    6 | C RTTI
187        -- (V, 32) vtable address --
188    7 | void B::f()
189        [this adjustment: 0 non-virtual, -24 vcall offset offset]
190
191 Virtual base offset offsets for 'C' (1 entry).
192    V | -24
```

这里只介绍与`thunk`函数相关的两个概念

1. vbase_offset

   如果一个类A有直接或间接虚基类B，则，在A的vtable中，会有一项用来保存B相对于A的偏移。知道了A或B的地址，且知道B的vbase_offset，那么，通过这个vbase_offset就可以方便的在`A*`与`B*`之间做类型转换。在上图中的第178行可以看到时 vbase_offset，即C类型对象的第32个字节是V对象的开始。

   - 一个类有虚基类的情况下才会有vbase_offset。

2. vcall_offset

   `V`作为`C`的基类，出现在`C`类的第32个字节处。通过`V*`或`V&`调用f的时候，实际应该调用的是`B::f`，因为`B`重写了`f`函数。如果是通过`V* v`调用`f`，而最终实际调用的是`B::f`，那就意味着，需要把`V* v`转换成`B*`。V是被虚继承的，因此，需要通过查虚表的方式拿到`V`相对于`B`的偏移，即两个指针之间的距离。这个vcall_offset保存的就是这个偏移

   - 只有一个虚函数A::f被C::f覆盖，且A与C之间的有虚继承的情况下才会有vcall_offset

   - vcall_offset出现在A到C的继承路径中第一个虚基类相应的vtable中

以此图中的184行的 `vcall_offset` 为例，与此 `vcall_offset` 对应的函数是第188行的`B::f`，187行显示，188行的`B::f`位于类 `V` 的虚表中。`B::f` 需要的参数是 `B*`，因此，在由 `V*` 调用`f`时，需要给`V*`加上一个`vcall_offset(-32)`，再将结果传给`B::f`

**thunk函数**

假设有一个类叫`A`，`A`类声明了一个虚函数`void f()`，在经过编译器编译后，生成的函数会是`void f(A*)`。通过一个指向A对象的指针`A* ap`来调用f：`ap->f()`，在经过编译器编译后，生成的代码是`f(ap)`。

由于`f`是虚函数，所以，需要通过查找vtable的方式来确定最终需要调用的函数，我们假设，最终需要调用的函数是C::f(C\*)，即C类里面最终定义了f的实现，且C的任何子类没有继续重写f函数。C可能是A本身，也可能是A的一个直接或间接子类。不失一般性，我们可以认为C可能不是most derived class，不防假设X是most derived class。因此，最终的继承关系是`A <--- C <--- X`。注意，这只是局部的继承关系，即，C除了继承自A以外，还有可能继承了其它类；X除了继承C以外，还有可能继承其它类；且这里说的继承关系不一定是直接继承，也有可能是间接继承。

画一个图来展示：
```
┌──────┐◄──────────────X::vptr      
│      │                            
│      │                            
│      ├────┐◄─────────C::vptr      
│      │    │                       
│      │    ├────┐◄────A::vptr◄───ap
│  X   │ C  │    │                  
│      │    │ A  │                  
│      │    ├────┘                  
│      ├────┘                       
│      │                            
└──────┘                                      
```
**this指针调整**

通过`ap`调用 `f` 的时候，最终实际调用的是`func=C::f(C*)`。`func`的参数是`C*`，但`ap`其实是`A*`，所以，不能直接传给`func`，而是需要先将`ap`转换成`C*`。转换的过程写在另一个函数`th_f`里面，这个函数对`ap`做相应的调整后，将调整后的ap传给真正需要调用的`C::f(C*)`，这个`th_f`就是`thunk`函数。通过`A::vptr`指向的虚表查到的f函数实际上就是这个 `th_f`。

由`A*`到`C*`的转换过程：

1. 如果`A`和`C`之间没有任何虚继承，则，根据对象布局的原则可知，无论完整的继承关系是什么样的，`A`在`C`中的偏移是确定的，这个偏移在编译时已知。th_f的伪代码为：

   ```mpl
   C* cp = (void*)ap + OFFSET_OF_A_IN_C // 编译时已知
   return C::f(cp)
   ```

   

2. 如果`A`本身是被虚继承的，则A相对于C的偏移是不确定的，这个偏移保存在vtable中，与f对应的vcall_offset中。此`vcall_offset`在`vtalbe`中的相对索引vcall_offset_offset在编译时是已知的。th_f的伪代码为：

   ```mpl
   vcall_offset_offset =  VCALL_OFSET_OFFSET_OF_IN_B(C::f) // 编译时已知
   vcall_offset_addr = (void*)ap + vcall_offset_offset
   vcall_offset = *vcall_offset_addr
   C *cp = (void*)ap + vcall_offset
   return C::f(cp)
   ```

   

3. 如果`A`本身不是被虚继承的，但是在由`A`到`C`的继承关系中，有一个被虚继承的`B`，即`A <-- B <-- C <-- X`。我们让`B`是由`A`到`C`的继承关系中遇到的第一个虚继承（即`B`与`C`之间可能还有其它虚基类，但A与B之间没有其它虚继承）。我们可以得到如下结论：

   1. 因为`A`与`B`之间没有直接或间接虚继承关系，因此`A`相对于`B`的偏移是确定的
   2. 因为`B`是第一个虚基类，因此，B的vtable中会有与f相对应的vcall_offset，此vcall_offset会记录由`B*`到`C*`的偏移

   因此，由`A*`到`C*`的转换可以结过两步

   1. 将`A*`转换到继承体系中第一个虚基类B，只需要给`A*`加一个固定的偏移，转换后得到`B*`
   2. 拿到`f`在`B`中的`vcall_offset`，将此`vcall_offset`加到`B*`上，得到最终的`C*`

   th_f的伪代码为：

   ```mpl
   vcall_offset_offset = VCALL_OFFSET_OFFSET_OF_IN_B(C::f)  // 编译时已知
   vcall_offset_addr = (void *)ap + vcall_offset_offset
   vcall_offset = *vcall_offset_addr
   C *cp = (void*)ap
           + OFFSET_OF_A_IN_B // 编译时已知
           + vcall_offset
   return C::f(cp)
   ```

   

**返回值调整**

虚函数的函数签名必须要一样，同时，他们的返回值可以不完全一样，如基类中声明的虚函数A::f的返值是指向基类`A`的指针`A*`，则子类C中重写的函数`C::f`可以返回指向类`C`的指针`C*`，且仍然符合函数重载的定义。当然，覆盖函数和被覆盖函数的返回值可以是任何其它类型，只要它们一样或者是分别指向基类和子类的指针即可。对于后一种情况，除了需要对`this`指针做调整以外，还需要在`thunk`函数中对函数的返回值做调整。

```cpp
A* A::f() { return new A();}
C* C::f() { return new C();}
A *ap = new C();
A *a = ap->f();
```

_代码中的`ap->f`会最终调用`C::f`。`C::f`返回的是`C*`，但最终返回值被赋值给了`A* a`。由`C*`到`A*`的转换是在thunk函数中完成的。_

与`this`指针调整是由**基类到子类**不同，返回值调整是由**子类到基类**的调整。这里只介绍第三种情况，在source type和target type之间，存在虚继承，且基类不是虚基类。过程与`this`调整类似，但顺序相反。

我们还是利用`this`指针调整中的例子。假设需要调整的类型为由`C*`到`A*`的调整，且由`A`到`C`的继承路径中，第一个虚基类是`B`。我们有以下结论：

1. B是虚基类，其相对于子类C的偏移在运行时是不确定的，此值保存在`vtable`中，叫作`vbase offset`。此`vbase offset`在虚表中的索引是可以在编译期间确定的。
2. `A`与`B`之间没有任何虚继基类出现（因为`B`是继承路径中第一个虚基类），因此，A相对于B的偏移是固定的，在编译时可知。

因此，由`C*`到`A*`的转换可以通过下面两步来解决：

1. 通过vbase offset offset，拿到B在C中的偏移，将此偏移加到`C*`上，得到`B*`
2. A相对于B的偏移是一个固定值，将其加到上一步的`B*`上，最终得到`A*`

生成的thunk函数中，返回值调整部分的伪代码如下：

```mpl
C* cp = call C::f(adjusted_this) // adjusted_this是经过`this`指针调整后的值
vbase_offset_offset = VBASE_OFFSET_OFFSET_OF_B_IN_C // 编译时已知
vbase_offset_ptr = (void*)cp + vbase_offset_offset
vbase_offset = *vbase_offset_ptr
A* ap = (A*)(
    (void*)cp
    + vbase_offset
    + OFFSET_OF_A_IN_B
)
```



_此文中说到的`vbase_offset_offset`和`vcall_offset_offset`两个值已经被clang封装到`clang::ThunkInfo`中_
