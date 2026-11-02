---
title: projects
description: Systems and GPU projects, with benchmarks.
---

# Projects

Things I've built to understand how systems behave under the hood. Each
links to source with a README covering the design and how it was measured.

## Limit Order Book & Matching Engine

Price-time priority matching engine in C++20. TODO: orders/sec, p50/p99
add/cancel latency, and what made the difference (e.g. intrusive lists,
preallocated pools, no allocation on the hot path).

[source](https://github.com/sharmakrishna276) · [writeup](posts/TODO.html)

## SPSC lock-free Ring Buffer

Single-producer/single-consumer queue using acquire/release atomics, with
head and tail on separate cache lines. TODO: throughput and latency against
a mutex-based queue.

[source](https://github.com/sharmakrishna276)

## CUDA SGEMM, from naive to near-cuBLAS

Matrix multiply optimised one step at a time: coalesced loads, shared-memory
tiling, register blocking, vectorised loads. TODO: GFLOPS at each step as a
percentage of cuBLAS on TODO GPU.

[source](https://github.com/sharmakrishna276)

## This website

A dependency-free static site generator in C++20, with a hand-written
Markdown parser, templates and an RSS feed, deployed with GitHub Actions.

[source](https://github.com/sharmakrishna276/ksharma.github.io)
