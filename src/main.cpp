
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

int main() {
    // Get usage
    std::vector<BalancerUsage> vect = lxc_balancer_usage();
    
    std::vector<BalancerOutput> vect_out = lxc_balance(vect, get_online_cores());
    for(auto &out : vect_out) {
        std::string name = out.container_name;
        std::string cpus = encode_cpu_list(out.used_cores);
        //ssystem("lxc config set %s limits.cpu %s", name.c_str(), cpus.c_str());
        ssystem("sh -c 'echo %s > /sys/fs/cgroup/cpuset/lxc.payload.%s/cpuset.cpus'",
            cpus.c_str(), name.c_str());
    }

}