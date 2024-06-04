---
layout: post
title: "usefull commands"
date: 2024-06-05
---

# gstack
`gdb -q -p $1 -iex 'set pagination off' -ex="set confirm off" -ex 't a a bt' -ex quit`

>> -q quiet 模式，不打印version
>> -p 进程id
>> -iex 在读取进程信息之前执行的命令
>> -ex 在读取进程信息之后执行的命令，可以指定多次，按顺序执行
>> set pagination off 关闭分页功能
>> set confirm off 关闭退出时的确认询问
>> t a a bt 是 thread apply all bt 的简写
>> quit 退出
