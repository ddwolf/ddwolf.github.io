[C++ ABI链接](http://itanium-cxx-abi.github.io/cxx-abi/)

可以用下面的命令来输出对象布局：
```bash
clang -Xclang -fdump-record-layouts abi_example_1.cpp
```

计算primary base class的算法
1. 第一个非虚继承的动态（直接）基类，如果不存在，则
2. 第一个nearly empty proper base class，且不能是indirect primary base；
3. 如果所有的nearly empty base class都是indirect primary base，则选择其中第一个作为primary base class

示例1:
```cpp
// A是B的第一个非虚继承的动态类，所以A是B的primary (virtual) base。对应算法中的第一个条件。
struct A { virtual void f()};
struct B : public A {}
```

示例2:
```cpp
// A不是B的第一个非虚继承的动态类，但是是第一个nearly empty base class，又因为B没有其它基类，所以A不是任何其它基类的primary base，所以A是B的primary (virtual) base。对应算法中的第二个条件。
struct A {virtual void f()};
struct B : public virtual A {};
```

示例3:
```cpp
// C没有非虚继承的动态类，A和B都是nearly empty class，但是A是C的一个基类B的primary base，也就是文档中所说的indirect primary base class，所以A不是C的primary base class，B不是indirect primary base class，所以B是C的primary (virtual) base。对应算法中的第二个条件。
struct A {virtual void f()};
struct B : public virtual A {};
struct C : public virtual B {};
```

示例4:
```cpp
// 与示例3类似，但是此例中，B不是nearly empty，所以B不能是primary base，对应算法中的第三个条件：所有的nearly empty virtual base class都是indirect primary base，那么选择其中的第一个。在此例中，A是唯一一个满足条件的nearly empty virtual base class，所以选择A
struct A {virtual void f();}
struct B : public virtual A {int i;};
struct C : public virutal B {};
```

示例5:
```cpp
// 与示例一类似，但B对A只是普通的继承，对应算法中的第二个条件，B是第一个nearly empty virtual base class，所以B是C的primary (virtual) base
struct A {virtual void f();};
struct B : public A {};
struct C : public virtual B {};
```
