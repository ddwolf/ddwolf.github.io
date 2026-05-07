---
layout: post
title: "Python: 为什么 import 后再 dlclose 会导致 PyType 崩溃"
date: 2026-05-06
---

# Python Embedding + dlopen/dlclose 的一个大坑：为什么 import 后再 dlclose 会导致 PyType 崩溃

最近排查了一个非常隐蔽、而且极具迷惑性的 CPython 崩溃问题。
现象包括：

* `PyType_Ready: Assertion "type->tp_vectorcall_offset > 0" failed`
* `tp_dict == NULL`
* `munmap_chunk(): invalid pointer`
* `stack smashing detected`
* GC 阶段随机崩溃
* `PyDict_GetItemWithError(op=0x0, ...)`

最终发现：

> 根因并不是 Python 本身的问题，而是：
>
> **对一个已经参与 Python Runtime 的 so 调用了 `dlclose()`。**

这个坑非常深，尤其在：

* Python Embedding
* pybind11
* Cython
* NumPy dtype
* 动态插件系统

中非常容易踩。

这篇文章把整个问题完整梳理一下。

---

# 一、问题场景

系统结构大概如下：

```text
strategy.c
    └── dlopen(bootstrap.so)
              └── bootstrap.cpp
                        └── pybind11::module_::import(...)
```

然后：

```c
dlclose(bootstrap.so);
```

理论上看：

* strategy 不再调用 bootstrap
* bootstrap 似乎也已经“没用了”

于是觉得：

> dlclose 应该是安全的。

但实际上，这是未定义行为。

---

# 二、现象：Import 阶段直接崩溃

最开始以为：

* 是 NumPy
* 是 GIL
* 是 PyArg_ParseTuple
* 是内存踩踏

但后来在 Debug Python 下，出现了：

```text
Objects/typeobject.c:5259:
PyType_Ready:
Assertion "type->tp_vectorcall_offset > 0" failed
```

并且：

```gdb
(gdb) p type->tp_name
$1 = "_cython_3_2_4amsendbackport.cython_function_or_method"
```

更离谱的是：

```gdb
(gdb) p type->tp_dict
$2 = 0x0
```

也就是说：

> 一个 Python Type：
>
> * 名字还在
> * 但内部结构已经坏了

---

# 三、为什么会这样？

关键理解：

> Python Object 生命周期 ≠ so 生命周期

很多人（包括我一开始）会误以为：

```text
dlclose(bootstrap.so)
```

意味着：

```text
bootstrap.so 创建的一切对象都会消失
```

这是错误的。

---

# 四、Python Import 到底做了什么？

## 1. import Python 源文件

```python
import foo
```

结果：

* `foo` 被放入：

```python
sys.modules
```

* module object 生命周期由 Python 管理

---

## 2. import C-extension so

```python
import xxx.so
```

Python 会：

```text
dlopen(xxx.so)
调用 PyInit_xxx
```

但：

> Python 几乎永远不会主动 dlclose extension module。

原因是：

* extension module 里有：

  * PyTypeObject
  * PyCFunction
  * 全局静态状态
  * vectorcall
  * tp_dealloc
  * NumPy dtype callback

Python 无法保证安全卸载。

所以：

> Python 默认假设：
>
> “extension module 一旦 import，就常驻进程。”

---

# 五、真正的坑：PyTypeObject / callback / function pointer

这是整个问题的核心。

当一个 so：

* 注册了 `PyTypeObject`
* 暴露了 `PyCFunction`
* 注册了 NumPy dtype
* 使用 pybind11 / Cython

那么：

> Python Runtime 内部会长期保存大量指向该 so 的函数指针。

例如：

```c
tp_dealloc
tp_call
tp_traverse
tp_repr
vectorcall
```

这些函数指针：

```text
都指向 bootstrap.so 的代码段
```

---

# 六、问题就出在这里

当我们：

```c
dlclose(bootstrap.so)
```

之后：

* bootstrap.so 的代码段被卸载
* 但 Python Runtime：

  * 仍然持有 Type
  * 仍然持有 callback
  * 仍然持有 function object

于是：

```text
对象还活着
代码已经死了
```

随后：

* import
* GC
* interpreter shutdown
* vectorcall
* type check

任意一个动作：

都会跳转到已经被卸载的地址。

结果：

```text
随机崩溃
```

---

# 七、为什么 import Python 源文件更容易炸？

一个很迷惑的现象：

```text
import 编译后的 so 没问题
import Python 源文件会炸
```

原因是：

## import so

Python 自己：

```text
dlopen(so)
```

并负责整个生命周期。

so 不会被卸载。

---

## import py

Python 源文件里的：

* function
* closure
* decorator
* class

会捕获：

```text
bootstrap.so 中的 C 函数指针
```

于是：

```text
Python module 活着
bootstrap.so 已 dlclose
```

崩。

---

# 八、Debug Python 为什么能精准抓到？

Debug Python 会检查：

```c
if (tp_flags & _Py_TPFLAGS_HAVE_VECTORCALL) {
    assert(tp_vectorcall_offset > 0);
}
```

而我们的 Type：

* flags 还在
* 但内存已经被破坏

于是：

```text
tp_vectorcall_offset == 0
```

Debug Python 直接当场抓获。

Release 模式通常只是：

```text
随机 segfault
```

更难排查。

---

# 九、一个典型误区

很多人会觉得：

> “strategy.c 已经不再调用 bootstrap.so 了”

但真正的问题是：

> 不是 strategy 在调用 bootstrap
>
> 而是 Python Runtime 在调用 bootstrap

例如：

* GC
* tp_dealloc
* vectorcall
* Cython function
* NumPy dtype cleanup

这些调用：

```text
根本不经过 strategy.c
```

---

# 十、正确做法

## 原则

> 只要一个 so 曾经参与 Python Runtime：
>
> * 定义过 PyTypeObject
> * 创建过 Python Object
> * 注册过 callback
>
> 那么：
>
> 👉 它必须活到 Python 解释器结束。

---

## 最简单修复

### 不要 dlclose

```c
dlopen(...)
...
// 不要 dlclose
```

让 bootstrap.so 常驻。

这是最稳、最符合 CPython 设计的做法。

---

# 十一、什么时候才能安全 dlclose？

只有当：

* so 不创建任何 Python Object
* 不注册任何 Type
* 不暴露 callback
* 不 import Python module

也就是说：

> 它必须只是一个“纯 C 工具库”。

---

# 十二、一些调试技巧

## 1. 使用 Debug Python

强烈推荐：

```bash
./configure --with-pydebug
```

很多隐藏问题会直接 assert。

---

## 2. 查看 Type 是否损坏

```gdb
p type->tp_name
p type->tp_dict
p type->tp_flags
p type->tp_vectorcall_offset
```

---

## 3. 查看对象是否已经被释放

```gdb
p PyUnicode_AsUTF8(obj)
```

如果直接炸：

```text
对象已经损坏
```

---

## 4. 使用 ASAN

非常有帮助：

```bash
-fsanitize=address
```

---

# 十三、总结

这个问题本质上是：

> “动态链接生命周期” 与 “Python Runtime 生命周期” 的冲突。

一句话总结：

> **Python Object 可以比创建它的 so 活得更久。**

因此：

> **不要 dlclose 一个曾经参与 Python Runtime 的 so。**

尤其是：

* pybind11
* Cython
* NumPy dtype
* 自定义 PyTypeObject

否则：

```text
对象活着
代码死了
```

最终一定会以：

* GC crash
* vectorcall assert
* 随机 segfault

的形式爆炸。

