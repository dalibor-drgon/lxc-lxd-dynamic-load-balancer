
#include "balancer.hpp"
#include "json.hpp"
#include "pstream.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

using nlohmann::json;


static int count_cpus(std::string str) {
    if(str.find('-') == -1 && str.find(',') == -1) {
        // Just single number, represents number of CPUs
        int num = 1;
        std::stringstream stream(str);
        stream >> num;
        return num;
    } 
    return parse_cpu_list(str).size();
}

// TODO save the interval from <x>ms/<interval>ms and use it later.
static void parse_allowance(BalancerUsage &usage, std::string str) {
    std::stringstream stream(str);
    if(str[str.length()-1] == '%') {
        // allowance: x%
        int num;
        stream >> num;
        //std::cout << "Allowance: " << str << ", num: " << num << std::endl;
        usage.dedicated_performance_percetange = num;
        usage.performance_is_limited = false;
    } else if(str.find('/') != -1) {
        // allowance: <dedicated>ms/<slice>ms
        std::size_t index = str.find('/');
        int dedicated, slice;
        stream >> dedicated;
        stream.seekg(index+1);
        stream >> slice;
        usage.dedicated_performance_percetange = 100 * dedicated / slice / usage.threads;
        usage.performance_is_limited = true;
    } else {
        int num;
        stream >> num;
        usage.dedicated_performance_percetange = 100 * usage.threads / num;
        usage.performance_is_limited = true;
    }
}

/// BalancerUsage->last_load is absolute value, difference
/// has to be calculated.
static std::vector<BalancerUsage> lxc_current() {
    redi::ipstream in("lxc list --format json");
    json json;
    in >> json;

    std::vector<BalancerUsage> vect;
    
    for(auto instance : json) {
        BalancerUsage usage;
        if(instance["status"] != "Running") {
            // Ignore
            continue;
        }

        //std::cout << "1" << std::endl;
        usage.container_name = instance["name"];
        usage.last_load = instance["state"]["cpu"]["usage"];
        
        auto config = instance["config"];
        if(config["limits.cpu"] == NULL) {
            // Given container uses all CPUs, ignore it.
            continue;
        }

        // Counts CPUS
        std::string cpus = config["limits.cpu"];
        usage.threads = count_cpus(cpus);
        
        // Parse allowance
        if(config["limits.cpus.allowance"] != NULL) {
            parse_allowance(usage, config["limits.cpu.allowance"]);
        } else {
            usage.dedicated_performance_percetange = 100;
            usage.performance_is_limited = false;
        }

        // Ship it!
        vect.push_back(usage);
    }
    
    return vect;
}

static bool process_state(BalancerUsage &usage, json &state) {
    for(auto &container : state) {
        if(container["name"] == usage.container_name) {
            usage.last_load = usage.last_load - (uint64_t) container["load"];
            return true;
        }
    }
    return false;
}

std::vector<BalancerUsage> lxc_balancer_usage() {
    // Parse state
    json state = "[]"_json;
    std::ifstream state_file("state.json");
    if(state_file) {
        state_file >> state;
    }

    // Get current data
    std::vector<BalancerUsage> vect = lxc_current();
    
    // Calculate differences
    json state_out;
    for(BalancerUsage &usage : vect) {
        json cur_state;
        cur_state["name"] = usage.container_name;
        cur_state["load"] = usage.last_load;
        state_out.push_back(cur_state);

        if(!process_state(usage, state)) {
            usage.last_load = 1;
        }
    }
    
    // Write the state into file
    std::ofstream out("state.json");
    out << state_out.dump(4) << std::endl;

    // Sort to Max-min
    std::sort(vect.begin(), vect.end(), std::greater<BalancerUsage>());

    // Return
    return vect;
}


std::vector<BalancerOutput> lxc_balance(std::vector<BalancerUsage> vect, int threads) {
    std::unordered_map<unsigned, unsigned> load_per_thread;
    for(unsigned i = 0; i < threads; i++) {
        load_per_thread[i] = 0;
    }
    
    std::vector<BalancerOutput> vect_out;
    for(auto &usage : vect) {
        BalancerOutput out;
        out.container_name = usage.container_name;
        std::unordered_map<unsigned, unsigned> load_per_thread_copy(load_per_thread);

        for(unsigned thread = 0; thread < usage.threads; thread++) {
            // TODO could be edited to current thread's load
            unsigned cur_load = usage.last_load / usage.threads;

            // Find the least used cpu
            unsigned min_load_thread = 0;
            unsigned min_load_value = -1;
            for(auto pair : load_per_thread_copy) {
                if(pair.second < min_load_value) {
                    min_load_thread = pair.first;
                    min_load_value = pair.second;
                }
            }
            // Assign the container's thread to this cpu
            out.used_cores.push_back(min_load_thread);
            load_per_thread[min_load_thread] = min_load_value + cur_load;
            load_per_thread_copy.erase(min_load_thread);
            
        }
        vect_out.push_back(out);
    }
    return vect_out;
}
