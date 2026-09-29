#ifndef RANDOM_H
#define RANDOM_H

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>

using namespace std;

namespace des
{
	class random_engine;
	class stream_block;
}

/**
 * @brief xoshiro256** pseudo-random generator, period 2^256 - 1.
 *
 * D. Blackman, S. Vigna, "Scrambled Linear Pseudorandom Number Generators", ACM TOMS 47(4), 2021.
 * It is a UniformRandomBitGenerator, so every <random> distribution accepts it.
 *
 * The state is expanded from the 64-bit seed with SplitMix64, as the authors recommend: a
 * generator of a different nature avoids correlated streams for similar seeds.
 * jump() and long_jump() advance the state by 2^128 and 2^192 draws, to split the period into
 * disjoint streams (see des::stream_block).
 */
class des::random_engine
{
	public:
		typedef uint64_t result_type;
		static constexpr result_type default_seed = 0;
		/**
		 * @brief Construct an engine seeded with default_seed
		 */
		random_engine() : random_engine(default_seed)
		{}
		/**
		 * @brief Construct an engine seeded with @p value
		 */
		explicit random_engine(result_type value)
		{
			seed(value);
		}
		/**
		 * @brief Set the state from @p value through SplitMix64
		 */
		void seed(result_type value);
		static constexpr result_type min()
		{
			return 0;
		}
		static constexpr result_type max()
		{
			return numeric_limits<result_type>::max();
		}
		/**
		 * @brief Next 64-bit draw
		 */
		inline result_type operator()()
		{
			const uint64_t result = rotl(s[1] * 5, 7) * 9;
			const uint64_t t = s[1] << 17;
			s[2] ^= s[0];
			s[3] ^= s[1];
			s[1] ^= s[2];
			s[0] ^= s[3];
			s[2] ^= t;
			s[3] = rotl(s[3], 45);
			return result;
		}
		/**
		 * @brief Advance the state by 2^128 draws
		 */
		void jump();
		/**
		 * @brief Advance the state by 2^192 draws
		 */
		void long_jump();
		bool operator==(const random_engine&) const = default;
	private:
		array<uint64_t, 4> s;
		/**
		 * @brief Advance the state by the number of draws encoded by the jump polynomial @p poly
		 */
		void advance(const array<uint64_t, 4>& poly);
};

/**
 * @brief A block of up to 2^64 disjoint random streams of 2^128 draws each.
 *
 * Stream k starts k * 2^128 draws after the start of the block. Streams are created on first use,
 * and stream k starts from the same state whatever the order in which the streams are requested
 * and however much the other streams have been used. References returned by at() stay valid as
 * long as the block.
 */
class des::stream_block
{
	public:
		/**
		 * @brief Construct the block starting at the state of @p start
		 */
		explicit stream_block(const random_engine& start = random_engine()) :
			next_start(start)
		{}
		/**
		 * @brief Stream @p k of the block
		 */
		inline random_engine& at(size_t k)
		{
			if(k >= streams.size())
			{
				extend(k);
			}
			return streams[k];
		}
	private:
		/**
		 * @brief streams[k] is stream k; a deque keeps references valid while it grows
		 */
		deque<random_engine> streams;
		/**
		 * @brief Start of stream streams.size()
		 */
		random_engine next_start;
		/**
		 * @brief Create the streams up to stream @p k
		 */
		void extend(size_t k);
};
#endif
