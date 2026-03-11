---
layout: post
title: "Haskell: EitherT Monad 的实现"
date: 2026-03-11
---
# 类型声明
```hs
newtype EitherT e m a = EitherT { runExceptT :: m (Either e a)}
instance Show (m (Either e a)) => Show (Either e m a) where
    show (EitherT ema) = "EitherT ( ++ show ema ++ ")"
```
## Functor 的实现
```hs
instance Functor m => Functor (EitherT e m) where
    fmap f (EitherT ema) = EitherT $ fmap (fmap f) ema
```

## Applicative 的实现
```hs
instance Applicative m => Applicative (EitherT e m) where
    pure = EitherT $ pure . Right
    (<*>) (EitherT fmea) (EitherT mea) = EitherT $ (<*>) <$> fmea <*> mea
```
## Monad 的实现
```hs
instance Monad m => Monad (EitherT e m) where
    return = pure
    (>>=) (EitherT mea) f = EitherT $ do
        ea <- mea
        case ea of
            Left err -> return $ Left err
            Right a -> runExceptT (f a)
```

## 在实现 Monad 的时候，遇到一个问题。
```hs
        case ea of
            Left err -> return ea
```

这里出问题了，虽然 ea 的值是 Left err，但是它的类型是 Either e a。而箭头右边的值的类型要求是 Either e b。所以，尽管值的形式都是 Left err，但是箭头右边一定要写成 `return $ Left err`。

相关报错如下：
```
    • Couldn't match type ‘a’ with ‘b’
      Expected: Either e b
        Actual: Either e a
      ‘a’ is a rigid type variable bound by
        the type signature for:
          (>>=) :: forall a b.
                   EitherT e m a -> (a -> EitherT e m b) -> EitherT e m b
        at monad_transformers.hs:41:19-21
      ‘b’ is a rigid type variable bound by
        the type signature for:
          (>>=) :: forall a b.
                   EitherT e m a -> (a -> EitherT e m b) -> EitherT e m b
        at monad_transformers.hs:41:19-21
    • In the first argument of ‘pure’, namely ‘e’
      In the expression: pure e
      In a case alternative: Left err -> pure e
    • Relevant bindings include
        e :: Either e a (bound at monad_transformers.hs:42:9)
        f :: a -> EitherT e m b (bound at monad_transformers.hs:41:23)
        mea :: m (Either e a) (bound at monad_transformers.hs:41:14)
        (>>=) :: EitherT e m a -> (a -> EitherT e m b) -> EitherT e m b
          (bound at monad_transformers.hs:41:19)
   |
45 |             Left err -> pure e
   |                              ^
```
