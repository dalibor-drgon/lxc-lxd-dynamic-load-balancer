
#include "balancer.hpp"
#include "pstream.h"

#include <iostream>
#include <cstdarg>
#include <unordered_map>
#include <cstdlib>


static void ssystem(const char *format, ...) {
    va_list argp;
    va_start(argp, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer)-1, format, argp);
    printf("> %s\n", buffer);
    if(system(buffer) == -1) {
        perror("system()");
        exit(1);
    }
}

static int get_thread_count() {
    redi::ipstream in("nproc");
    int count = 1;
    in >> count;
    return count;
}

int main() {
    // Get usage
    std::vector<BalancerUsage> vect = lxc_balancer_usage();
    
    /*
    for(BalancerUsage &usage : vect) {
        std::cout << usage.container_name << std::endl;
        std::cout << "ThreadsCount:\t" << usage.threads << std::endl;
        std::cout << "LastLoad:\t" << usage.last_load << std::endl;
        std::cout << "Performance:\t" << (int) usage.dedicated_performance_percetange << std::endl;
        std::cout << "PerfIsLim:\t" << usage.performance_is_limited << std::endl;
    }
    */
    
    std::vector<BalancerOutput> vect_out = lxc_balance(vect, get_thread_count());
    for(auto &out : vect_out) {
        std::string name = out.container_name;
        std::string cpus = encode_cpu_list(out.used_cores);
        ssystem("lxc config set %s limits.cpu %s", name.c_str(), cpus.c_str());
    }

}