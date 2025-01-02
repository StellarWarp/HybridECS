Here’s the complete translation of the document:

------

# Hybrid ECS

Hybrid ECS is a C++23 header-only library for ECS (Entity Component System) offering high performance and ease of use.

'Hybrid' refers to the library's mixed storage approach, combining chunk-based and sparse set-based storage, which ensures ECS maintains performance in complex compositions and large-scale applications.

Additionally, the storage is automatically managed, eliminating the need for manual data storage management.

**Note: This library is currently incomplete, and support for multithreading and task orchestration is in progress.**

# Overview

ECS (Entity Component System), as an architectural design pattern, has garnered increasing attention in game development and high-performance computing in recent years. While Unity's ECS implementation has demonstrated strong potential in performance, it has been criticized for a cumbersome development experience. This library is an attempt to break through ECS design limitations.

This document aims to summarize the advantages and disadvantages of existing ECS systems and explore how a hybrid storage and modular grouping strategy can build a more efficient ECS solution.

## ECS (Entity Component System)

There are numerous resources and projects about ECS for reference.

Refer to the following links:

- [Unity's official ECS documentation](https://docs.unity3d.com/Packages/com.unity.entities@0.50/manual/ecs_core.html)
- [GDC Talk: Introduction to ECS Architecture](https://www.youtube.com/watch?v=W3aieHjyNvw&ab_channel=GDC)
- [Game Architecture Design: Data-Oriented Programming (DOP) - KillerAery](https://www.cnblogs.com/KillerAery/p/11746639.html)
- [Blog: ECS and Data-Oriented Design](https://neil3d.github.io/3dengine/why-ecs.html)
- [ECS Back and Forth - EnTT](https://skypjack.github.io/2021-08-29-ecs-baf-part-12/)
- [Open Source C++ ECS Framework: UECS](https://zhuanlan.zhihu.com/p/141255752)

The creator of Flecs has a comprehensive overview of the current state of ECS development:

- [ECS FAQ](https://github.com/SanderMertens/ecs-faq)

## Problems ECS Addresses

### DOP (Data-Oriented Programming)

ECS design philosophy deeply embodies the principles of Data-Oriented Programming, aiming to maximize program performance and scalability. This philosophy emphasizes:

1. **Cache-Friendly Data Layouts:** Optimizing CPU cache hit rates by tightly packing related data, thereby reducing access latency.
2. **Decoupling Logic from Data:** Implementing high-efficiency parallelism in logic through independent system processing.

#### Data Structures (Component)

The essence of components lies not in what they store but in how they are stored. Efficient data layout is the starting point of ECS philosophy.

#### Data Operations (System)

1. Component composition essentially partitions sets, where different components define the behaviors of entities. The significance of systems lies not only in implementing computational logic but also in maximizing hardware resource utilization. ECS philosophy here reflects:

- Treating systems as data processors rather than data owners.
- Enhancing task scheduling parallelism through modular logic design.

This design positions ECS not just as an architectural pattern but as a holistic optimization mindset from hardware to program logic.

Certainly! Continuing from where we left off:

------

## Component Storage

The current ECS storage implementations are primarily divided into two main approaches:

### Archetype (Heterogeneous Arrays)

Entities with identical sets of components are grouped into the same array, referred to as an "Archetype Array." Storage schemes of ECS systems like Unity, Unreal, and flecs utilize this approach.

The primary advantage of using Archetype is that data for entities with the same components can be tightly packed together in sequence, making traversal more cache-friendly.

However, the Archetype approach introduces some issues:

1. When an entity's components change, all its data must be transferred to a new Archetype, incurring high copying costs.
2. Only entities with identical components are placed in the same Archetype:
   - When an Archetype contains only a few entities, it can lead to significant memory wastage.
   - With diverse component combinations, storage fragmentation can occur. In the worst-case scenario, every entity ends up in a different Archetype, which not only fails to improve cache hit rates but also incurs significant overhead due to indirect component addressing.
3. Random access to components has higher addressing overhead.

### Sparse Set

Rather than calling it a Sparse Set, it might be more appropriate to call it a Component Table. In this structure, each component type has its own table, and entities map to the data within each component table via an index.

A Sparse Set is essentially an optimized mapping table container for entity indexing and traversal, functioning similarly to a hash table.

`EnTT` is a notable example of an ECS implementation using this structure.

This structure avoids many of the issues associated with Archetype-based systems, enabling flexible addition and removal of components and better random access performance.

Additionally, it offers a unique advantage: the ability to sort entities by specific components.

The trade-offs of using this structure include:

1. For component combination queries, only one component can be accessed sequentially, while others require random access. This is an area where Archetypes have an advantage.
2. Queries are more complex: With Archetypes, queries only need to identify the relevant Archetypes. In contrast, Sparse Set queries must track entity component states across multiple component tables to determine whether entities should be included in the query results. In essence, with Sparse Sets, each entity is equivalent to an Archetype in an Archetype-based system.

------

## Hybrid Method

Archetype and Sparse Set methods each excel in different scenarios, making them complementary. Combining these two methods can yield better performance in a wider range of situations: Archetype is ideal for scenarios with a large number of similar entities, while Sparse Set handles fragmented scenarios more effectively.

Typically, an entity may have components from various modules, such as rendering, physics, AI, and other gameplay logic. Each module may contain different component combinations.

If there are $M$ modules, each with $N_i$ component combinations, the worst-case scenario would result in $\prod_{i=1}^{M}N_i$ combinations, leading to severe fragmentation.

### Component Group

![Component Group Diagram](https://cdn.jsdelivr.net/gh/StellarWarp/StellarWarp.github.io@main/img/image-20250102225937285.png)

The Component Group is at the core of the hybrid method.

Components are divided into groups based on functionality, and Archetypes are only formed within groups. This significantly reduces the number of component combinations globally to $\sum_{i=1}^{M}N_i$.

Archetypes within each group are independent, largely avoiding fragmentation. Cross-group component queries require random access.

Since most computation and data exchange occur within individual modules, grouping components according to module functionality ensures sufficient cache hit rates during hotspot computations within the module.

(Note: EnTT also has a concept of groups, but it only allows specifying components to form a single sorted group, effectively creating just one Archetype per group.)

### Hybrid Storage

![Hybrid Storage Diagram](https://cdn.jsdelivr.net/gh/StellarWarp/StellarWarp.github.io@main/img/image-20250102225914513.png)

Because Archetypes with few entities not only waste memory but also exacerbate fragmentation, this hybrid storage method is employed:

- When an Archetype's memory usage is below a threshold, Sparse Set storage is used.
- When an Archetype's memory usage exceeds a threshold, chunk-based Archetype storage is used to reduce the addressing overhead for random component access.

In some cases, Sparse Sets, with their tightly packed components, can improve cache hit rates during random access.
