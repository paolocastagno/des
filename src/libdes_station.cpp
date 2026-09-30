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
							string description) : 
					node::node(rand_dist.at(0).size(),
							description),
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
	}

	template <typename TT, template <typename> typename T>
	station<TT, T>::station(vector<vector<shared_ptr<T<TT>>>> rand_dist,
							unsigned int nserver,
							unsigned int s_places,
							unsigned int nqueue,
							unsigned int q_places,
							shared_ptr<policy> p_queue,
							shared_ptr<policy> p_service,
							node::service_pick_handler shfunc,
							string description) :
					station<TT, T>::station(rand_dist,
							nserver,
							s_places,
							nqueue,
							q_places,
							p_queue,
							p_service,
							description)
	{
		set_service_pick_handler(shfunc);
	}

	template<typename TT, template <typename> typename T>
	station<TT, T>::station(vector<vector<shared_ptr<T<TT>>>> rand_dist,
							unsigned int nserver,
							unsigned int s_places,
							unsigned int nqueue,
							unsigned int q_places,
							string description) : 
					station<TT, T>::station(rand_dist,
										nserver,
										s_places,
										nqueue,
										q_places,
										shared_ptr<policy>(new fifo()),
										shared_ptr<policy>(new fifo()),
										description)
	{}

	template<typename TT, template <typename> typename T>
	station<TT, T>::station(vector<vector<shared_ptr<T<TT>>>> rand_dist,
							unsigned int nserver,
							unsigned int s_places,
							shared_ptr<policy> p_service,
							string description) : 
					node::node(rand_dist.at(0).size(),
							description),
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
	}

	template<typename TT, template <typename> typename T>
	station<TT, T>::station(vector<vector<shared_ptr<T<TT>>>> rand_dist,
							unsigned int nserver,
							unsigned int s_places,
							shared_ptr<policy> p_service,
							node::service_pick_handler shfunc,
							string description) :
					station<TT, T>::station(rand_dist,
							nserver,
							s_places,
							p_service,
							description)
	{
		set_service_pick_handler(shfunc);
	}

	template<typename TT, template <typename> typename T>
	station<TT, T>::station(vector<vector<shared_ptr<T<TT>>>> rand_dist,
							unsigned int nserver,
							unsigned int s_places,
							string description) : 
					station<TT, T>::station(rand_dist,
											nserver,
											s_places,
											shared_ptr<is>(new is()),
											description)
	{}

	template<typename TT, template <typename> typename T>
	station<TT, T>::station(vector<vector<shared_ptr<T<TT>>>> rand_dist,
							string description) : 
					station<TT, T>::station(rand_dist,
											1,
											numeric_limits<int>::max(),
											description)
	{};

	// template<typename T, typename S> station<T,S>::~station()
	// {
	// }
	
	template<typename TT, template <typename> typename T>
	double station<TT, T>::get_service(unsigned int& cls, unsigned int& idx)
	{
		return (*(rng.at(idx).at(cls)))(service_stream(cls));
	}

	template<typename TT, template <typename> typename T>
	shared_ptr<T<TT>> station<TT, T>::get_rng(unsigned int& cls, unsigned int& idx)
	{
		return rng.at(idx).at(cls);
	}

	namespace
	{
		/**
		 * @brief Uniform choice among the indexes in [0, @p n) satisfying @p ok.
		 *
		 * Draws from @p g as picking from a vector of those indexes would, without allocating one.
		 *
		 * @return the chosen index, or -1 (drawing nothing) if no index satisfies @p ok
		 */
		template <typename Ok>
		int pick(unsigned int n, Ok ok, random_engine& g)
		{
			unsigned int few[16];
			vector<unsigned int> many;
			unsigned int* idx = few;
			if(n > 16)
			{
				many.resize(n);
				idx = many.data();
			}
			unsigned int k = 0;
			for(unsigned int i = 0; i < n; i++)
			{
				if(ok(i))
				{
					idx[k++] = i;
				}
			}
			if(k == 0)
			{
				return -1;
			}
			uniform_int_distribution<int> d(0, static_cast<int>(k) - 1);
			return static_cast<int>(idx[d(g)]);
		}

		/**
		 * @brief Last index in [0, @p n) satisfying @p ok, -1 if none does
		 */
		template <typename Ok>
		int last_of(unsigned int n, Ok ok)
		{
			for(unsigned int i = n; i-- > 0;)
			{
				if(ok(i))
				{
					return static_cast<int>(i);
				}
			}
			return -1;
		}
	}

	template<typename TT, template <typename> typename T>
	int station<TT, T>::enqueue(const shared_ptr<event>& e, const vector<vector<int>>& q_map)
	{
		// A random queue with room among those of the class, else the last of the class
		const unsigned int cls = e -> get_cls(), n = q_map.size();
		auto mapped = [&](unsigned int i){ return q_map.at(i).at(cls) != 0; };
		int chosen = pick(n, [&](unsigned int i){ return mapped(i) && !q.at(i) -> is_full(); }, choice_stream());
		return chosen >= 0 ? chosen : last_of(n, mapped);
	}

	template<typename TT, template <typename> typename T>
	int station<TT, T>::dequeue(const shared_ptr<event>& e, const vector<vector<int>>& q_map)
	{
		// A random non-empty queue among those of the class, else the last of the class
		const unsigned int cls = e -> get_cls(), n = q_map.size();
		auto mapped = [&](unsigned int i){ return q_map.at(i).at(cls) != 0; };
		int chosen = pick(n, [&](unsigned int i){ return mapped(i) && q.at(i) -> in_queue() != 0; }, choice_stream());
		return chosen >= 0 ? chosen : last_of(n, mapped);
	}

	template<typename TT, template <typename> typename T>
	int station<TT, T>::schedule(const shared_ptr<event>& e, const vector<vector<int>>& s_map)
	{
		// A random server with room among those of the class, else the last of the class
		const unsigned int cls = e -> get_cls(), n = s_map.size();
		auto mapped = [&](unsigned int i){ return s_map.at(i).at(cls) != 0; };
		int chosen = pick(n, [&](unsigned int i){ return mapped(i) && !s.at(i) -> is_full(); }, choice_stream());
		return chosen >= 0 ? chosen : last_of(n, mapped);
	}

	template<typename TT, template <typename> typename T>
	int station<TT, T>::shfunc(const shared_ptr<event>&, int sched, const vector<vector<int>>&, const vector<shared_ptr<queue>>& queues, random_engine& g)
	{
		// A random queue holding a job the freed server may serve
		unsigned int srv = static_cast<unsigned int>(sched);
		const function<bool(const event&)> eligible = [this, srv](const event& w){ return this -> can_serve(srv, static_cast<unsigned int>(w.get_cls())); };
		return pick(queues.size(), [&](unsigned int i){ return queues.at(i) -> has_next(eligible); }, g);
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