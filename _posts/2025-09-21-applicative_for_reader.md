---
layout: post
title: "Haskell: 对 function 本身作为 Applicative 的理解"
date: 2025-09-21
---
# 对 function 本身作为 Haskell 的 Applicative 的理解

## Applicative 的定义
```haskell
class Functor f => Applicative f where
pure a :: f a
<*> :: f (a -> b) -> f a -> f b
```
## 函数 `((->) r)` 作为 Applicative
先看一下它应该有的类型是什么样子的

```haskell
pure a :: (-> r a) -- r -> a
<*> :: ((->) r) a -> b -> (((->) r) a) -> (((->r) b))
 -- :: (r -> a -> b) -> (r -> a) -> (r -> b)
```

## pure a :: (r -> a)
这个函数的类型是 `r -> a`，即 `pure` 接收一个 `a` 值，生成一个 `r -> a` 的函数。这里对 `r` 没有任何限制，对 `a` 也没有任何限制，根据类型越宽泛，实现越有限，可知，这里的实现只有一种，即：`pure a = \r -> a`，

## 重点：<*> 的实现
`<*>` 的类型是 `(r -> a -> b) -> (r -> a) -> (r -> b)`，可以写成这样一种形式
```haskell
f :: r -> a -> b
g :: r -> a
f <*> g = ???
```

首先，`f <*> g` 的类型是 `:: r -> b`，所以，上面的形式也可以写成
```haskell
(f <*> g) r = (??? :: b)
```

即，`f <*> g` 接收一个 `r` 类型的参数，返回一个 `b` 类型的值，它的实现只能有一种，即：

```haskell
f <*> g x = f x (g x)
```

入参 `x` 需要同时作为 `f` 和 `g` 的参数，才能产生一个新的 `b` 类型的值。如果不能同时把 `x` 作用于 `f` 与 `g`，则 `f` 和 `g` 需要两个不同的 `r` 类型变量作为参数。但是，`f <*> g` 只有一个 r 参数，因为 `f <*> g` 的类型是 `r -> b`