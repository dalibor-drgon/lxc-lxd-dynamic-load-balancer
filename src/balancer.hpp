
#ifndef _LXC_BALANCER_H_
#define _LXC_BALANCER_H_

#include <string>
#include <cstdint>
#include <vector>

class BalancerUsage {
public:
    /// Name of the container
    std::string container_name;
    /// How much loaded was the CPU by this container 
    /// since the last run of the check.
    uint32_t last_load;
    
    /// Number of threads allocated.
    uint16_t threads;
    
    /// Dedicated performance when under load.
    /// This is the percentage value of limits.cpu.allowance.
    ///
    /// If limits.cpu.allowance is written as X/Y, then that 
    /// is converted into percetange as 
    /// (100% * limits.cpu.allowance / limits.cpu)
    /// and performance_is_limited is set to true.
    uint8_t dedicated_performance_percetange;

    /// Set if limits.cpu.allowance is written as X/Y.
    bool performance_is_limited;
    
    bool operator<(const BalancerUsage &rhs) const { 
        return last_load / threads < rhs.last_load / rhs.threads;
    }
    bool operator>(const BalancerUsage &rhs) const { 
        return last_load / threads > rhs.last_load / rhs.threads;
    }
};

class BalancerOutput {
public:
    std::string container_name;
    std::vector<unsigned> used_cores;
};


/// Parses input of format "0,2-4,5" into {0,2,3,4,5}.
std::vector<unsigned> parse_cpu_list(std::string str);

/// Encodes input of format {0,2,3,4,5} into "0,2,3,4,5"
std::string encode_cpu_list(std::vector<unsigned> vect);

/// Gets list of online cores (from lscpu)
std::vector<unsigned> get_online_cores();

std::vector<BalancerUsage> lxc_balancer_usage();

std::vector<BalancerOutput> lxc_balance(std::vector<BalancerUsage> usage, int threads);

#endif
