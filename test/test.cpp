#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <random>
#include <list>
#include <algorithm>

#include <libdes_const.hpp>
#include <libdes_event.hpp>
#include <libdes_station.hpp>
#include <libdes_source.hpp>
#include <libdes_sink.hpp>
#include <libdes_network.hpp>
#include <libdes_scalar.hpp>
#include <libdes_vector.hpp>
#include <libdes_histogram.hpp>

using namespace std;

pair<bool, int> handle_block(shared_ptr<des::event>, int destination, const vector<vector<vector<double>>>& route, shared_ptr<mt19937_64>& g)
{
    return make_pair<bool, int>(true, 3);
}

int main(int argc, char** argv)
{
    shared_ptr<des::event> e(new des::event);
    shared_ptr<std::mt19937_64> gen64ptr(new std::mt19937_64());
    gen64ptr -> seed(1);
    
    shared_ptr<exponential_distribution<double>> exp = shared_ptr<exponential_distribution<>>(new exponential_distribution<>(0.005));
    vector<double> interval({0.5, 0.5});
    vector<double> weight({1.0});
    shared_ptr<piecewise_constant_distribution<double>> constant = shared_ptr<piecewise_constant_distribution<double>>(new piecewise_constant_distribution<double>(interval.begin(),interval.end(),weight.begin()));
    shared_ptr<des::source> source = shared_ptr<des::source>(new des::source(vector<double>({0.05}), "Source", gen64ptr));
    shared_ptr<des::station<double, exponential_distribution>> q(new des::station<double, exponential_distribution>({{exp},{exp}}, 2, 1, 1, INT_MAX,"M/M/2s/infinity", gen64ptr));
    shared_ptr<des::station<double, piecewise_constant_distribution>> d(new des::station<double, piecewise_constant_distribution>({{constant}}, 1, INT_MAX, "IS", gen64ptr));
    shared_ptr<des::sink> sink = shared_ptr<des::sink>(new des::sink("Sink"));
    
    double p = 1.0, ci = 0;
    int run = 0;
    // std::vector<std::vector<std::vector<double>>> routing = { {{0},{1},{0},{0}},{{0},{0},{p},{1-p}}, {{0},{0},{0},{1}}, {{0},{0},{0},{0}} };
    std::vector<std::vector<std::vector<double>>> routing = { {{0},{1},{0}},{{0},{0},{1}}, {{0},{0},{0}} };
    e = shared_ptr<des::event>(new des::event());
    e -> set_cls(0);
    e -> set_time(0);
    e -> set_info(EVENT_NODE, 0);
    // cout << "\tResetting node..\n\t" << mm1 -> to_string() << endl;
    cout << "Setting up network...\n" << endl;
    // vector<shared_ptr<des::node>> nodes{source, d, q, sink};
    vector<shared_ptr<des::node>> nodes{source, q, sink};
    source = nullptr; d = nullptr; q = nullptr; sink = nullptr;
    // vector<shared_ptr<des::scalar>> arrivals;
    // vector<shared_ptr<des::scalar>> departures;
    // for(shared_ptr<des::node> n: nodes)
    // {
    //     string nm = "count_arrivals";
    //     shared_ptr<des::scalar> s_a(new des::scalar(nm, 1)); 
    //     n ->attach(nm, SIGNAL_NODE_ARRIVAL, s_a);
    //     arrivals.push_back(s_a);
    //     nm = "count_departures";
    //     shared_ptr<des::scalar> s_d(new des::scalar(nm,1));
    //     n -> attach(nm, SIGNAL_NODE_DEPARTURE, s_d);
    //     departures.push_back(s_d);
    //     // if(n -> get_id() != nodes.size()-1)
    //     // {
    //     //     nm = NODE_SOJOURN;
    //     //     shared_ptr<des::vector> v_s(new des::vector(nm, 1));
    //     //     n -> attach(nm, SIGNAL_NODE_DEPARTURE, v_s);
    //     //     // shared_ptr<des::vector> v_s(new des::vector(nm, SIGNAL_NODE_DEPARTURE, 1, n));
    //     //     // n -> attach(nm, SIGNAL_NODE_DEPARTURE, v_s);
    //     //     // vectors.push_back(v_s);
    //     // }
    // }

    des::histogram hst("histogram",1);
    hst.set_binsize(1e-3);

    des::network net(nodes, routing, handle_block, gen64ptr);
    cout << "\t\t Displaying event...\n\t\t\t" << e->to_string() << endl;
    nodes.at(0) -> arrival(e);
    nodes.clear();
    cout << "\t\t Arrival at source...\n\t\t\t" << e->to_string() << endl;
    do
    {
        double sim_time = 0.0, prev_time = 0.0;
        for(int i = 0; i < 100000; i++)
        {
            e = net.next_event();
            sim_time = e -> get_time();
            hst.update(sim_time-prev_time,0);
            if(sim_time < prev_time)
            {
                throw runtime_error("Event in the past!");
            }
            else
            {
                prev_time = sim_time;
            }
            int src = e -> get_info(EVENT_NODE).second;
            net.route(e, e -> get_time());
            // cout << "network::route\t" << e -> get_time() << endl;
            // cout << "(" << e -> get_id() << ")" << i << ":\t" << src << "\t->\t" << e -> get_info(EVENT_NODE).second << endl;
            if(i%10000 == 0)
            {
                cout << "(" << run << ")\t[" << sim_time << " - " << static_cast<double>(i)/sim_time << "]\tThr: 0 - 1: " << net.get_flow(0,1,0) << " +- " << net.get_flow_stddev(0,1,0)  << "Thr: 1 - 2: " << net.get_flow(1,2,0) << " +- " << net.get_flow_stddev(1,2,0) << endl;
            }
        }
        // pair<double,double> confidenceinterval = departures.at(2)->confidence_interval(0.05,0);
        // ci = (confidenceinterval.second - confidenceinterval.first);
        // cout << "Run " << run << "\t[" << confidenceinterval.first << " - " << confidenceinterval.second << "]" << endl;
        // departures.at(2) -> end_run(sim_time);
        net.reset(sim_time, {}, true);
        hst.reset(true);
    }
    // while(run++ < 2 || ci > 1e-1);
    while(run++ < 2);
    cout << "\t\t Network state.." << endl;
    for(shared_ptr<des::node> n: nodes)
    {
        string str = n -> to_string();
        cout << "\t\t\t\t" << str << endl;
        n = nullptr;
    }
    // for(shared_ptr<des::scalar> s: scalars)
    // {
    //     cout << "\t\t\t\t" << s ->get_id() << "\t" << to_string(s ->get().at(0)) << " (" << to_string(s ->get().at(0)/e -> get_time()) << ")" << endl;
    //     s = nullptr;
    // }
    // for(shared_ptr<des::vector> v: vectors)
    // {
    //     cout << "\t\t\t\t" << v->get_id() << "\t" << v -> to_string() << endl;
    // }

    cout << "Events distribution:\n\t-avg:\t" << hst.mean(0) << "\n\t-std_dev:\t" << hst.stddev(0);
    
    std::ofstream ofs;
    std::string basename = "prova_hst.csv";
    ofs.open(basename, std::ofstream::out | std::ofstream::trunc);
    ofs << hst.print();
    ofs.close();

    cout << "DONE" << endl; 
}