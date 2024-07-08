---
layout: post
title: "mockcpp代码分析"
date: 2024-05-15
---

# 重点
mockcpp实现方法重写的原理是覆盖原函数的字节码为一个jmp指令，具体可参考 JmpCodeImpl 

ParameterizedApiHookHolder   --   只保存一个函数指针及相应的类型
ApiHookGenerator/ApiHookFunctor 都只有十个类型。每个类型里面都有静态apiAddress。也就是说，同样函数签名的函数，只允许有十个mock。静态refCount就是引用计数，同一时间只允许有十个对象引用这个ApiHook(特定函数签名，同样地址)

ChainableMockObjectBase 表示 mock对象本身，可以在其上面链式调用mock相关的方法
实现类：  ChainableMockObjectBaseImpl* This;

ChainableMockMethodContainer： 实现类：ChainableMockMethodContainerImpl* This;
    getMethod
    addMethod

ChainableMockMethodKey： 只提供equals方法
ChainableMockMethodCore：提供链式调用的关键类，继承Method,继承Invokable
    Method 提供 getName和getNameSpace方法
    Invokable 提供invoke方法
    继承  InvocationMockerContainer：提供如下方法
        addInvocationMocker(InvocationMocker* mocker) 
        addDefaultInvocationMocker(InvocationMocker* mocker) 
        getInvocationMocker(const std::string& id) 

InvocationMocker：继承 SelfDescribe，SelfDescribe提供toString方法
    提供如下方法：

        Method* getMethod() const;
        void addStub(Stub* stub);
        void addMatcher(Matcher* matcher);
        bool hasBeenInvoked(void) const ;

        void setId(InvocationId* id);
        const InvocationId* const getId(void) const;

        bool matches(const Invocation& inv) const;
        Any& invoke(const Invocation& inv);

        void verify();

        std::string toString() const;

HookMockObject 继承 ChainableMockObjectBase
    ChainableMockMethod 继承  ChainableMockMethodBase，ChainableMockMethodBase 提供 operator() 方法，getResult方法

    Result 只提供 getResult方法
InvocationMockBuilderGetter 提供如下方法
    StubsBuilder stubs();
    MockBuilder expects(Matcher* matcher); 
    DefaultBuilder defaults();
    这三种类型都是InvocationMockBuilder的别名，并带有不同的模板参数
    InvocationBuilder是一个继承体系：都有getMocker 方法
        MoreStubBuilder 提供 then 方法，和getMocker方法
        StubBuilder提供 will 方法
        ArgumentsMatchBuilder 提供 with 和 which 方法
        InvocationMockBuilder 没有自己的方法

struct Matcher
{
    virtual bool matches(const Invocation& inv) const = 0;
    virtual void increaseInvoked(const Invocation& inv) = 0;
    virtual void verify() = 0;
    virtual std::string toString() const = 0;

    virtual ~Matcher() {}
};

TypelessConstraintAdapterImpl
TypelessConstraint 无类型信息 eval(void)
struct Constraint 包含类型信息 eval(RefAny)

## 链接：下面是一个轻量的mock实现，可以mock虚方法。
https://github.com/wangyongfeng5/lmock
