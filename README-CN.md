# Hybrid ECS

Hybrid ECS 是 ECS（实体组件系统）的 C++23 header-only 库，具有高性能和易用性。

'Hybrid' 表示此库采用基于 `chunk` 的存储和基于 `sparse set` 的存储的混合存储，这使得 ECS 能够保持复杂组合和大规模应用的性能

并且存储是自动控制的。因此无需手动管理数据存储。

**注意：目前此库仍然不完整，对多线程和任务编排支持还在进行中**

# Overview

ECS (Entity Component System) 作为一种架构设计模式，近年来在游戏开发和高性能计算领域受到了越来越多的关注。
特别是 Unity 的 ECS 实现虽然在性能上展现了强大的潜力，但在开发体验上却显得较为笨拙。本库是突破ECS设计限制的一个尝试。

本文旨在总结现有 ECS 的优缺点，并探讨如何通过混合存储与模块化分组的策略，构建一种更加高效的 ECS 解决方案。


## ECS (Entity Component System)

对于ECS的，已经有相当多的资料和项目可参考

可参见下面的链接

- [Unity 官方 ECS 文档](https://docs.unity3d.com/Packages/com.unity.entities@0.50/manual/ecs_core.html)

- [GDC 演讲：ECS 架构介绍](https://www.youtube.com/watch?v=W3aieHjyNvw&ab_channel=GDC)

- [游戏架构设计：面向数据编程 (DOP) - KillerAery](https://www.cnblogs.com/KillerAery/p/11746639.html)

- [博客：ECS 与面向数据的设计](https://neil3d.github.io/3dengine/why-ecs.html)

- [ECS back and forth - entt](https://skypjack.github.io/2021-08-29-ecs-baf-part-12/)

- [开源 C++ ECS 框架：UECS](https://zhuanlan.zhihu.com/p/141255752)

FLECS 的作者对当前的 ECS 发展有很全面的概述

- [ECS-FAQ](https://github.com/SanderMertens/ecs-faq)


## ECS 要解决什么问题

### DOP(Data Oriented Programming)

ECS 的设计哲学深刻体现了数据导向编程 (Data Oriented Programming) 的思想，其追求的目标是让程序的性能和扩展性最大化。这种理念强调：

1. **缓存友好的数据布局**：通过将关联数据紧密排列，优化 CPU 缓存命中率，从而减少访问延迟。
2. **逻辑与数据的解耦**：借助独立的系统处理方式实现逻辑的高效并行化。

#### 数据结构（Component）

组件（Component）的核心不在于它们存储了什么，而在于它们如何被存储。高效的数据布局是 ECS 思想的起点。

#### 数据操作（System）

1. 组件的组合本质上是集合的划分，不同的组件定义了不同行为的实体
System 的意义不仅在于计算逻辑的实现，更在于如何最大化利用硬件资源。ECS 的哲学在此体现为：

- 将系统视作数据的处理者而非数据的拥有者。
- 通过模块化逻辑设计，增强任务调度的并行性。

这种设计使得 ECS 不仅仅是一个架构模式，而是一种从硬件底层到程序逻辑的全局优化思维的体现。

## Component存储

当前ECS的存储实现方式主要分为两大流派

## Archetype(异构数组)

具有相同组件集的实体被分组到同一个数组中，这个数组称为一个“Archetype Array”

诸如 Unity、Unreal、flecs 等ECS的存储方案都是采用Archetype的方式

使用Archetype的一大好处在于对于使用了相同Component的实体，他们的数据能被按顺序紧密排列在一起，从而能在遍历时更加缓存友好

但是使用Archetype会带来一些问题

1. 当实体的组件发生改变时，需要将其所有的数据转移到新的Archetype中，带来高拷贝的开销
2. 只有当实体的组件完全相同才会被放入同一个Archetype中
   1. 当Archetype中的实体数量较少时会造成较多的内存浪费
   2. 实体的Component组合具有多样性时，会导致存储碎片化，最糟糕的情况是每个实体都在不同的Archetype中，不仅不能提高缓存命中率，加上间接组件寻址的开销其性能变得相当糟糕
3. 组件的随机访问寻址开销更高

## SparseSet(稀疏集)

与其称之为SparseSet，其实叫组件表Component Tables，可能更加合适。在这种结构中，每个组件类型都有自己的表，实体通过索引映射到各个组件表中的数据。

所谓SparseSet是针对Entity映射以及遍历进行优化的映射表容器，功能与哈希表相同。

`EnTT` 是使用这种结构的代表

这种结构可以避免使用Archetype而出现的问题，能做到灵活地添加和删除组件，以及更好的随机访问访问性能

此外能带来一个独有的有优点：可以对某个组件进行排序

使用这种数据结构的 trade-off 是
1. 对于Component的组合查询，只能对其中一个Component进行有序访问，其它Component需要进行随机访问，但这也属于是Archetype的优势应用场景
2. Query更加复杂：使用Archetype时，Query只要知道有哪些Archetype是查询目标即可；但是对于SparseSet，Query需要通过与其有关的组件表追踪实体的组件状态，考虑是否要将实体加入到访问列表中。相当于对于SparseSet中的Query而言，每个一个实体都是一个Archetype

---

## Hydrid Method



其实Archetype方法与SparseSet方法都有其最适用的场景，属于是互补的关系。如果能将这两种方法结合起来，就能在更多的场景下得到更好的性能表现：Archetype适用于有大量相同的实体的场景，而SparseSet则能更好应对碎片化的场景

通常对于一个实体来说，其上会挂载各个模块的组件

例如，一个entity上可能有关于渲染、物理、AI、以及其它Gameplay逻辑的模块，各个模块中都有不同的组件组合

如果有$M$个模块，每个模块中有$N_i$个组件组合，最坏的情况是在全局中出现$\prod_{i=1}^{M}N_i$个组合，这样就会出现碎片化问题

### Component Group

![image-20250102225937285](https://cdn.jsdelivr.net/gh/StellarWarp/StellarWarp.github.io@main/img/image-20250102225937285.png)

Component Group 这是混合方法的核心

将Component按照功能划分进入不同的Group中，在Group中才会形成Archetype，这样在全局中的组件组合数可以被极大降低，变成了$\sum_{i=1}^{M}N_i$

每个Group中的Archetype都是独立的，能大幅度避免碎片化的问题，当需要跨Group进行组件查询时，需要随机访问

因为主要的计算和数据交换都集中在某一个模块内部，将模块中适用的Component划分到一个Group中可以保证在模块内部的热点计算上有足够好的缓存命中率

（其实entt中也有Group的思考在，但其只能指定组件形成一个对应排序，相当于一个Group中只有一个Archetype）

### Hydrid Storage

![image-20250102225914513](https://cdn.jsdelivr.net/gh/StellarWarp/StellarWarp.github.io@main/img/image-20250102225914513.png)

因为Archetype在组合中Entity较少时不仅会产生内存浪费，也会带来碎片化的问题，所以这里还采用一个混合存储方法：Hydrid Storage

当Archetype的内存使用低于一个阈值，其会采用 SparseSet 存储，当 Archetype 的内存使用高于一个阈值，其会采用 Archetype 的 Chunk 进行存储，降低组件随机访问的寻址开销；在一些情况下因为 SparseSet 每个 Component 都是紧密排布的，能增加一些随机访问时的缓存命中率

### Tag Component

在使用 Archetype 存储时增加或删除组件会带来的拷贝开销

组件的增删是应当尽可能避免的，但是在ECS的开发中不可避免得会通过频繁的增删组件来控制System的执行

各家都给出了些解决方案：
如 Unity 中的 `IEnableableComponent` ，其也像正常的组件一样会占用 Archetype 中的空间。通过在存储中增加一串掩码来标识当前的实体是否拥有该组件。这个方案虽然简单，但带来的问题是内存的浪费以及查询时需要遍历Archetype的开销，尤其当一个Archetype中只有一小部分实体需要这种特性时这种方案会带来不必要的开销


将需要频繁的增删组件标记为Tag Component

Tag Component 不存储在 Archetype 中，而是存储在 SparseSet 中，这样就避免了内存的浪费问题，也天然对增删友好；对于空的Component，则不需要存储

> 通常来 Tag Component 是：不携带数据的组件，一般来说是空结构体
> 因为我确实没有想到什么好名字，还是觉得 Tag Component 最适合描述这种情况

为了能对 Tag Component 进行高效的查询，这里也延伸出了 Tag Archetype ，本质上来说一个 Tag Archetype 标识了一个 Archetype 中的子集。

Tag Archetype 与 Tag Archetype 间没有交集，但 Tag Archetype 一定是某个 Archetype 的子集

在查询时也会对同一个 Archetype 下的 Tag Archetype 做聚合，这样就能有更好的缓存命中率

---

# Query

## Introduction

ECS是这样的，用户只要关心System里的逻辑和Query的数据就可以了，而ECS考虑的就很多了

Query是连接逻辑和数据的途径，它是ECS的核心概念

Query需要通过数据的描述找到对应的数据

许多ECS实现中Query的查询并不复杂，如:

- 使用 Archetype 的实现中，一般的实现是将所有的Archetype都遍历一遍来筛选需要的数据。因为对于只使用Archetype的ECS来说Query的查询并不是性能热点。
- 使用 SparseSet 的实现中，Query往往需要缓存满足条件的实体，并监听各有关组件表的增删来更新缓存

因为 HECS 的混合存储架构，以及需要聚合优化、考虑存在大量 Query 和 Archetype 下的性能等原因，Query 的算法复杂度增加不少，以至于需要独立的模块来处理这些问题

Query主要分为两部分，数据更新访问 与 数据查询

## 数据查询

Query到底在做什么

Query的条件指定了集合的查询区域

Archetype是集合中的一点

无非要做两件事
1. 新增 Query 查询匹配的 Archetype
2. 新增 Archetype 查询匹配的 Query

### In-Group Query

支持的查询类型有
- **全匹配**：`all`
- **任一匹配**：`any`
- **排除**：`none`

一个完整的条件可以被描述为
`base... tag... ((base...|tag...) ...) none(base...,tag...)`

拆分为三部分为
- **all part** `base... tag...`
- **any part** `(base...|tag...) ...`
- **none part** `none(base...,tag...)`

根据是否有 tag， Query可以被分为以下几类

- **untag query / direct query**
  - 条件不含 tag
  - 直接访问 Archetype
  - `base... (base...) none(base...)`
- **mix query**
  - 条件含 tag
  - 直接访问 Archetype 或访问 Tag Archetype
  - `base... tag... ((base...|tag...) ...) none(base...,tag...)`
- **pure tag query**
  - 全匹配的条件只有 tag
  - 只访问 Tag Archetype
  - `tag... ((base...|tag...) ...) none(base...,tag...)`


还有一类特殊的 Query
- archetype query，其用于查询有着相同arhcetype的tag archetype
- component query，只有一个Component的Query，作为基础节点


#### Subquery

Subquery 指一个 Query 的 Entity 集是另一个 Query 的子集

通过从父查询中筛选Archetype可减少查询量

### Cross-Group Query

Cross-Group Query 是 In-Group Query 的扩展，其可以跨越多个Group进行查询

## 访存模型

In-Group Query 的访存模型如下

- **query**: 一个 In-Group Query 访问存入口
- **table tag query**: 按tag筛选，访问一个table的子集，对每一个table而言，对其tag的query
- **arch storage**: 存储Archetype的存储，对每一个table而言，对其base的query
  - **sparse table**: Archetype 采用**稀疏集**存储
  - **table**: Archetype 采用**表**存储
- **component storage**: 采用**稀疏集**存储实现的组件

![image-20250102225808834](https://cdn.jsdelivr.net/gh/StellarWarp/StellarWarp.github.io@main/img/image-20250102225808834.png)

一个查询的实例可能如下

![image-20250102231558304](https://cdn.jsdelivr.net/gh/StellarWarp/StellarWarp.github.io@main/img/image-20250102231558304.png)

### Cross-Group Query & Access

Cross-Group 的 Query 则要通过潜在组合的计数来实现

一个计数表：记录Entity对应Match的Group的数量
一个Entity缓存表：记录满足条件的Entity

Cross-Group 无法进行顺序访问，只能逐组进行随机访问

![image-20250102231322481](https://cdn.jsdelivr.net/gh/StellarWarp/StellarWarp.github.io@main/img/image-20250102231322481.png)

### Random Access

![image-20250103095400482](https://cdn.jsdelivr.net/gh/StellarWarp/StellarWarp.github.io@main/img/image-20250103095400482.png)


## 查询结构

![image-20250102230117513](https://cdn.jsdelivr.net/gh/StellarWarp/StellarWarp.github.io@main/img/image-20250102230117513.png)

[todo]

### In-Group Query Matching Algorithm

[todo]

![image-20250102230357678](https://cdn.jsdelivr.net/gh/StellarWarp/StellarWarp.github.io@main/img/image-20250102230357678.png)


### Cross-Group Query Matching Algorithm

跨Group的Query则要通过潜在组合的计数来实现

一个计数表：记录Entity对应Match的Group的数量
一个Entity缓存表：记录满足条件的Entity

## 访存容器

[todo]

## API Design

cpp具有很强的类型描述能力，考虑到方便System的编写，将静态的查询条件在System的函数参数中描述是很好的选择

[todo]

```cpp
builder.register_executer([](
        const A& a,
        B& b,
        D& d,
        optional<const C&> c,
        none_of<E, F>,

        relation_ref<"o1">,
        relation_ref<"o2">,

        begin_rel_scope<relation1, "o1">,
            const A& a1,
            B& b1,
            none_of<C, D>,
            relation_ref<"o3">,
        end_rel_scope,

        begin_rel_scope<relation2, "o2">,
            const A& a2,
            const B& b2,
            const C& c2,
            relation_ref<"o3">,
        end_rel_scope,

        begin_rel_scope<relation3, "o3">,
            A& a3,
            B& b3,
        end_rel_scope,

        multi_relation<child(
                A&,
                B&
        )> rel2
)
{
  //system body...
});
```

---

# Evemt

## Introduction

事件驱动其实对于逻辑开发上必不可少，但是许多ECS框架上的还没有很完善的支持，unity的 ECS 甚至完全不对事件驱动做支持，导致一些逻辑难以通过纯粹的ECS的方式实现

事件驱动提供了及时的响应，以及可以为System的调用顺序提供保证

面向数据编程听起来和事件驱动不着边际

事件驱动在面向对象的编程中往往是一个过程的执行过程中调用另一个过程，这与ECS的设计理念相背驰

但是事件驱动在本质上来说可以看作是是对数据的改变做出响应，相比于在'一个过程中调用另一个过程'这样高耦合的方式进行事件驱动，可以把过程的调度权交给数据机制管理

ECS把事件响应视为对特定数据的写入和处理

例如对 UI 上的数值数据进行写入之后，应当有对应的 System 去处理这个数据的变化

## Event System

[todo]

---

# Relation

## Introduction

在编写逻辑时，不可能只是处理一个Entity上的数据

Entity与Entity之间往往会产生交互，在没有Relation的ECS中处理这种常见的情况却变得非常棘手

这往往需要在编写逻辑时通过一个Component中记录与之相关的Entity并通过组件的随机访问获取其它Entity中的数据，这回带来很多问题

1. 另一个Entity的生命周期不受控，处理时可能出现非法的Entity
2. 其它Entity的Component不受约束，其它Entity上是否存在某个Component完全由开发者自己在System的编写逻辑中处理，带来可靠性和可维护性上的问题
3. 不能联合访问其它Entity的一组Component，带来更多的随机访问的开销

如果没有一个Relation系统来处理这些繁琐的事情的话，ECS的开发会变得非常痛苦

## Relation Ship

### Relation Type

![image-20250102230727428](https://cdn.jsdelivr.net/gh/StellarWarp/StellarWarp.github.io@main/img/image-20250102230727428.png)

[todo]

## Relation Query

![image-20250102230811267](https://cdn.jsdelivr.net/gh/StellarWarp/StellarWarp.github.io@main/img/image-20250102230811267.png)

[todo]