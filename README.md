# SPSC Queue Stages

A single-producer single-consumer queue, rebuilt step by step from my solution to
[HFTU](https://hftuniversity.com)'s Challenge 05 (Ring Buffer).

## Target machine

| | |
|---|---|
| Server | Dedicated bare-metal |
| CPU | AMD Ryzen 9 3900 (Zen 2, 12 cores / 24 threads) |
| Frequency | Fixed, `performance` governor |
| RAM | 64 GB DDR4 |
| OS | Ubuntu 24.04.4 LTS |
| Compiler | GCC 14.2.0 (`g++-14`) |
| CMake | 3.28.3 |
| Default flags | `-std=c++23 -O2 -march=native` |
| Cores | Producer and consumer pinned to separate isolated cores that share an L3 |
