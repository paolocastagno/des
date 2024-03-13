#include "libdes_station.hpp"

namespace des
{
	template <typename TT, template <typename> typename T>
	station<TT, T>::station(vector<vector<shared_ptr<T<TT>>>> rand_dist,
							unsigned int nserver,
							unsigned int s_places,
							unsigned int nqueue,
							unsigned int q_places,
							shared_ptr<policy> p_queue,
							shared_ptr<policy> p_service,
							string description,
							shared_ptr<mt19937_64> gen) : 
					node::node(rand_dist.at(0).size(),
							description, gen),
		rng(rand_dist)
	{
		q_map.clear();
		for(unsigned int i = 0; i < nqueue; i++)
		{
			// Setup the mapping between event class and queue where jobs are enqueued
			q_map.push_back(vector<int>(rand_dist.at(0).size(), 1));
			q.push_back(shared_ptr<queue>(new queue(q_places, p_queue)));
		}
		s_map.clear();
		for(unsigned int i = 0; i < nserver; i++)
		{
			// Setup the mapping between event class and server where jobs are served
			s_map.push_back(vector<int>(rand_dist.at(0).size(), 1));
			s.push_back(shared_ptr<queue>(new queue(s_places, p_service)));
		}
		measure_s = false;
	}

	template<typename TT, template <typename> typename T>
	station<TT, T>::station(vector<vector<shared_ptr<T<TT>>>> rand_dist,
							unsigned int nserver,
							unsigned int s_places,
							unsigned int nqueue,
							unsigned int q_places,
							string description,
							shared_ptr<mt19937_64> gen) : 
					station<TT, T>::station(rand_dist,
										nserver,
										s_places,
										nqueue,
										q_places,
										shared_ptr<policy>(new fifo()),
										shared_ptr<policy>(new fifo()),
										description,
										gen)
	{}

	template<typename TT, template <typename> typename T>
	station<TT, T>::station(vector<vector<shared_ptr<T<TT>>>> rand_dist,
							unsigned int nserver,
							unsigned int s_places,
							shared_ptr<policy> p_service,
							string description,
							shared_ptr<mt19937_64> gen) : 
					node::node(rand_dist.at(0).size(),
							description, gen),
		rng(rand_dist)
	{
		q_map.clear();
		s_map.clear();
		for(unsigned int i = 0; i < nserver; i++)
		{
			// Setup the mapping between event class and server where jobs are served
			s_map.push_back(vector<int>(rand_dist.at(0).size(), 1));
			s.push_back(shared_ptr<queue>(new queue(s_places, p_service)));
		}
		measure_s = false;
	}

	template<typename TT, template <typename> typename T>
	station<TT, T>::station(vector<vector<shared_ptr<T<TT>>>> rand_dist,
							unsigned int nserver,
							unsigned int s_places,
							string description,
							shared_ptr<mt19937_64> gen) : 
					station<TT, T>::station(rand_dist,
											nserver,
											s_places,
											shared_ptr<is>(new is()),
											description,
											gen)
	{}

	template<typename TT, template <typename> typename T>
	station<TT, T>::station(vector<vector<shared_ptr<T<TT>>>> rand_dist,
							string description,
							shared_ptr<mt19937_64> gen) : 
					station<TT, T>::station(rand_dist,
											numeric_limits<int>::max(),
											1,
											description,
											gen)
	{};

	// template<typename T, typename S> station<T,S>::~station()
	// {
	// }
	
	template<typename TT, template <typename> typename T>
	double station<TT, T>::get_service(unsigned int& cls, unsigned int& idx)
	{
		return (*(rng.at(idx).at(cls)))(*gen.get());
	}

	template<typename TT, template <typename> typename T>
	shared_ptr<T<TT>> station<TT, T>::get_rng(unsigned int& cls, unsigned int& idx)
	{
		return rng.at(idx).at(cls);
	}

	template<typename TT, template <typename> typename T>
	int station<TT, T>::enqueue(shared_ptr<event>& e, vector<vector<int>> q_map)
	{
		if(q_map.size() > 0)
		{ 
			unsigned int last = 0;
			vector<unsigned int> indexes({});
			for(unsigned int i = 0; i < q_map.size(); i++)
			{
				if(q_map.at(i).at(e->get_cls()) != 0)
				{
					last = i;
					if(!q.at(i) -> is_full())
					{
						indexes.push_back(i);
					}
				}
			}
			if(indexes.size() == 0)
			{
				return static_cast<int>(last);
			}
			else
			{
				uniform_int_distribution<int> idx(0,indexes.size()-1);
				return indexes.at(idx(*gen.get()));
			}
		}
		return -1;
	}

	template<typename TT, template <typename> typename T>
	int station<TT, T>::dequeue(shared_ptr<event>& e, vector<vector<int>> q_map)
	{
		unsigned int last = 0;
		if(q_map.size() > 0)
		{
			vector<unsigned int> indexes({});
			for(unsigned int i = 0; i < q_map.size(); i++)
			{
				if(q_map.at(i).at(e->get_cls()) != 0)
				{
					last = i;
					if(q.at(i) -> in_queue() != 0)
					{
						indexes.push_back(i);
					}
				} 
			}
			if(indexes.size() == 0)
			{
				return static_cast<int>(last);
			}
			else
			{
				uniform_int_distribution<int> idx(0,indexes.size()-1);
				return indexes.at(idx(*gen.get()));
			}
		}
		return -1;
	}

	template<typename TT, template <typename> typename T>
	int station<TT, T>::schedule(shared_ptr<event>& e, vector<vector<int>> s_map)
	{
		if(s_map.size() > 0)
		{
			unsigned int last = 0;
			vector<unsigned int> indexes({});
			for(unsigned int i = 0; i < s_map.size(); i++)
			{
				if(s_map.at(i).at(e->get_cls()) != 0)
				{
					last = i;
					if(!s.at(i) -> is_full())
					{
						indexes.push_back(i);
					}
				}
			}
			if(indexes.size() == 0)
			{
				return static_cast<int>(last);
			}
			else
			{
				uniform_int_distribution<int> idx(0,indexes.size()-1);
				return indexes.at(idx(*gen.get()));
			}
		}
		return -1;
	}

	template<typename TT, template <typename> typename T>
	string station<TT, T>::to_string() const
	{
		return "node::station::" + node::to_string();
	}
}

//		Uniform distribution
template class des::station<int, uniform_int_distribution>;
template class des::station<float, uniform_real_distribution>;
template class des::station<double, uniform_real_distribution>;

// Related to Bernoulli (yes/no) trials:

//		Bernoulli distribution (class )

//		Binomial distribution 
template class des::station<int, binomial_distribution>;

//		Geometric distribution (class template )
template class des::station<int, geometric_distribution>;

//		Negative binomial distribution (class template )
template class des::station<int, negative_binomial_distribution>;

// Rate-based distributions:

//     Poisson distribution (class template )
template class des::station<int, poisson_distribution>;

//     Exponential distribution (class template )
template class des::station<float, exponential_distribution>;
template class des::station<double, exponential_distribution>;

//		Gamma distribution (class template )
template class des::station<float, gamma_distribution>;
template class des::station<double, gamma_distribution>;

//     Weibull distribution (class template )
template class des::station<float, weibull_distribution>;
template class des::station<double, weibull_distribution>;

//     Extreme Value distribution (class template )
template class des::station<float, extreme_value_distribution>;
template class des::station<double, extreme_value_distribution>;

// Related to Normal distribution:

//     Normal distribution (class template )
template class des::station<float, normal_distribution>;
template class des::station<double, normal_distribution>;

//     Lognormal distribution (class template )
template class des::station<float, lognormal_distribution>;
template class des::station<double, lognormal_distribution>;

//     Chi-squared distribution (class template )
template class des::station<float, chi_squared_distribution>;
template class des::station<double, chi_squared_distribution>;

//     Cauchy distribution (class template )
template class des::station<float, cauchy_distribution>;
template class des::station<double, cauchy_distribution>;

//     Fisher F-distribution (class template )
template class des::station<float, fisher_f_distribution>;
template class des::station<double, fisher_f_distribution>;

//     Student T-Distribution (class template )
template class des::station<float, student_t_distribution>;
template class des::station<double, student_t_distribution>;

// Piecewise distributions:

//     Discrete distribution (class template )
template class des::station<int, discrete_distribution>;

//     Piecewise constant distribution (class template )
template class des::station<float, piecewise_constant_distribution>;
template class des::station<double, piecewise_constant_distribution>;

//     Piecewise linear distribution (class template )
template class des::station<float, piecewise_linear_distribution>;
template class des::station<double, piecewise_linear_distribution>;