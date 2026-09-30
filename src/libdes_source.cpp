#include "libdes_source.hpp"

namespace des
{
	template <typename TT, template <typename> typename T>
	source<TT, T>::source(vector<shared_ptr<T<TT>>> rand_dist, string description) : sourcesink(description, rand_dist.size()),
		rng(rand_dist)
	{
		vector<pair<int, vector<int>>> s_map;
		q.clear();
		q_map.clear();
		for(unsigned int i = 0; i < rand_dist.size(); i++)
		{
			// Setup the mapping between event class and queue where jobs are enqueued
			// q_map.push_back(make_pair<int, vector<int>>(0, vector<int>(rand_dist.size(), 1)));
			// Setup the mapping between event class and server where jobs are served
			s_map.push_back(make_pair<int, vector<int>>(0, vector<int>(rand_dist.size(), 1)));
			// Setup a queue for serve jobs of a given class
			s.push_back(shared_ptr<queue>(shared_ptr<queue>(new queue(numeric_limits<int>::max(), shared_ptr<policy>(new fifo())))));
		}
		// q.push_back(shared_ptr<queue>(shared_ptr<queue>(new queue(numeric_limits<int>::max(), shared_ptr<policy>(new fifo())))));
	}

	template <typename TT, template <typename> typename T>
	source<TT, T>::source(string description) : sourcesink::sourcesink(description),
		rng()
	{
		set_sid(description);
		vector<pair<int, vector<int>>> s_map;
		// Setup the mapping between event class and queue where jobs are enqueued
		// q_map.push_back(make_pair<int, vector<int>>(0, vector<int>(1, 1)));
		// Setup the mapping between event class and server where jobs are served
		s_map.push_back(make_pair<int, vector<int>>(0, vector<int>(1, 1)));
		s.push_back(shared_ptr<queue>(shared_ptr<queue>(new queue(numeric_limits<int>::max(), shared_ptr<policy>(new fifo())))));
		q.clear();
		q_map.clear();
		// q.push_back(shared_ptr<queue>(shared_ptr<queue>(new queue(numeric_limits<int>::max(), shared_ptr<policy>(new fifo())))));
	}

	template <typename TT, template <typename> typename T>
	double source<TT, T>::get_service(unsigned int& cls, unsigned int&)
	{
		return (*(rng.at(cls)))(service_stream(cls));
	}

	template <typename TT, template <typename> typename T>
	int source<TT, T>::enqueue(const shared_ptr<event>&, const vector<vector<int>>&)
	{
		// object::TraceLoc(source_location::current(), std::to_string(e->get_id()), " time ",
		// 				 e->get_time(), " node ", std::to_string(get_id()));
		return 0;
	}

	template <typename TT, template <typename> typename T>
	int source<TT, T>::dequeue(const shared_ptr<event>& e, const vector<vector<int>>&)
	{
		// object::TraceLoc(source_location::current(), std::to_string(e->get_id()), " time ",
		// 				 e->get_time(), " node ", std::to_string(get_id()));
		shared_ptr<event> ev = sourcesink::get_event();
		ev -> clone(*e);
		ev -> set_time(ev -> get_time());// + get_service(cls));
		arrival(ev);
		return 0;
	}

	template <typename TT, template <typename> typename T>
	int source<TT, T>::schedule(const shared_ptr<event>&, const vector<vector<int>>&)
	{
		return 0;
	}

	template <typename TT, template <typename> typename T>
	string source<TT, T>::to_string() const
	{
		return "node::source::" + node::to_string();
	}
}

// Same distributions as des::station, see libdes_station.cpp

//		Uniform distribution
template class des::source<int, uniform_int_distribution>;
template class des::source<float, uniform_real_distribution>;
template class des::source<double, uniform_real_distribution>;

// Related to Bernoulli (yes/no) trials:

//		Binomial distribution
template class des::source<int, binomial_distribution>;

//		Geometric distribution (class template )
template class des::source<int, geometric_distribution>;

//		Negative binomial distribution (class template )
template class des::source<int, negative_binomial_distribution>;

// Rate-based distributions:

//     Poisson distribution (class template )
template class des::source<int, poisson_distribution>;

//     Exponential distribution (class template )
template class des::source<float, exponential_distribution>;
template class des::source<double, exponential_distribution>;

//		Gamma distribution (class template )
template class des::source<float, gamma_distribution>;
template class des::source<double, gamma_distribution>;

//     Weibull distribution (class template )
template class des::source<float, weibull_distribution>;
template class des::source<double, weibull_distribution>;

//     Extreme Value distribution (class template )
template class des::source<float, extreme_value_distribution>;
template class des::source<double, extreme_value_distribution>;

// Related to Normal distribution:

//     Normal distribution (class template )
template class des::source<float, normal_distribution>;
template class des::source<double, normal_distribution>;

//     Lognormal distribution (class template )
template class des::source<float, lognormal_distribution>;
template class des::source<double, lognormal_distribution>;

//     Chi-squared distribution (class template )
template class des::source<float, chi_squared_distribution>;
template class des::source<double, chi_squared_distribution>;

//     Cauchy distribution (class template )
template class des::source<float, cauchy_distribution>;
template class des::source<double, cauchy_distribution>;

//     Fisher F-distribution (class template )
template class des::source<float, fisher_f_distribution>;
template class des::source<double, fisher_f_distribution>;

//     Student T-Distribution (class template )
template class des::source<float, student_t_distribution>;
template class des::source<double, student_t_distribution>;

// Piecewise distributions:

//     Discrete distribution (class template )
template class des::source<int, discrete_distribution>;

//     Piecewise constant distribution (class template )
template class des::source<float, piecewise_constant_distribution>;
template class des::source<double, piecewise_constant_distribution>;

//     Piecewise linear distribution (class template )
template class des::source<float, piecewise_linear_distribution>;
template class des::source<double, piecewise_linear_distribution>;
