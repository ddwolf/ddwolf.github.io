---
layout: post
title: "usefull commands"
date: 2024-06-05
---

# gstack
`gdb -q -p $1 -iex 'set pagination off' -ex="set confirm off" -ex 't a a bt' -ex quit`
```bash
 -q quiet 模式，不打印version
 -p 进程id
 -iex 在读取进程信息之前执行的命令
 -ex 在读取进程信息之后执行的命令，可以指定多次，按顺序执行
 set pagination off 关闭分页功能
 set confirm off 关闭退出时的确认询问
 t a a bt 是 thread apply all bt 的简写
 quit 退出
```

# gdb
`x/8gx c._vptr$Base-0x2` 8个一组打印内存值
```gdb
set print demangle on
set print asm-demangle on
```
自动解析c++的符号名称，输出更友好。not `_ZTV7Derived`, but` vtable for Derived`。


