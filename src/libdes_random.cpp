#include "libdes_random.hpp"

namespace des
{
	void random_engine::seed(result_type value)
	{
		// SplitMix64 (S. Vigna, public domain)
		for(uint64_t& word : s)
		{
			uint64_t z = (value += 0x9e3779b97f4a7c15);
			z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
			z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
			word = z ^ (z >> 31);
		}
	}

	void random_engine::jump()
	{
		advance({0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c});
	}

	void random_engine::long_jump()
	{
		advance({0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635});
	}

	void random_engine::advance(const array<uint64_t, 4>& poly)
	{
		// Reference jump of xoshiro256** (D. Blackman, S. Vigna, public domain)
		array<uint64_t, 4> acc = {0, 0, 0, 0};
		for(uint64_t word : poly)
		{
			for(int b = 0; b < 64; b++)
			{
				if(word & (uint64_t(1) << b))
				{
					for(int i = 0; i < 4; i++)
					{
						acc[i] ^= s[i];
					}
				}
				(*this)();
			}
		}
		s = acc;
	}

	void stream_block::extend(size_t k)
	{
		while(streams.size() <= k)
		{
			streams.push_back(next_start);
			next_start.jump();
		}
	}
}
