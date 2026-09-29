# Random streams

**Header:** `libdes_random.hpp`

The simulator draws every random number from one pseudo-random generator, split into disjoint streams: each source of randomness has its own. The [network](network.md#random-streams) seeds the generator and assigns the streams; this page describes the two classes behind them.

---

## des::random_engine

xoshiro256** (D. Blackman, S. Vigna, *Scrambled Linear Pseudorandom Number Generators*, ACM TOMS 47(4), 2021): 256 bits of state, period 2^256 − 1. It is a `UniformRandomBitGenerator`, so every `<random>` distribution accepts it.

```cpp
explicit random_engine(uint64_t seed);   // default: random_engine::default_seed
void seed(uint64_t value);
uint64_t operator()();                   // next 64-bit draw
void jump();                             // advance by 2^128 draws
void long_jump();                        // advance by 2^192 draws
bool operator==(const random_engine&) const;
```

The 64-bit seed is expanded into the state with SplitMix64, as the generator's authors recommend: a generator of a different nature avoids correlated streams for similar seeds (Matsumoto et al., *Common defects in initialization of pseudorandom number generators*, ACM TOMACS 17(4), 2007).

## des::stream_block

A block of up to 2^64 streams of 2^128 draws each: stream `k` starts `k` jumps after the start of the block.

```cpp
explicit stream_block(const random_engine& start);
random_engine& at(size_t k);             // stream k, created on first use
```

Stream `k` always starts from the same state, whatever the order in which the streams are requested and however much the others have been used. References returned by `at()` stay valid as long as the block.

## How the network uses them

| Block | Stream | Used for |
|---|---|---|
| `2i` | `0` | choices of node `i` among its queues and servers |
| `2i` | `1 + c` | service (or inter-arrival) times of class `c` at node `i` |
| `2i+1` | `c` | routing of the class-`c` events leaving node `i` |

Blocks are 2^192 draws apart (`long_jump()`), so streams never overlap. Because a stream depends only on the node index and the class, changing one node or one class leaves the random numbers of the others unchanged: model variants simulated with the same seed use common random numbers. Different seeds start at unrelated points of the period; the probability that streams of `n` seeds, each drawing `L` numbers, overlap is at most `n²L / P`, with `P = 2^256 − 1` the period (S. Vigna, *On the probability of overlap of random subsequences of pseudorandom number generators*, IPL 158, 2020).
