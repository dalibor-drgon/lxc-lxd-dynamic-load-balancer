#include "balancer.hpp"
#include <sstream>

std::vector<unsigned> parse_cpu_list(std::string str) {
    std::vector<unsigned> cpus;
    int start = 0;
    int dash_index = -1;
    std::stringstream stream(str);
    for(int i = 0; i <= str.length(); i++) {
        if(i == str.length() || str[i] == ',') {
            stream.seekg(start);
            if(dash_index != -1) {
                unsigned start, end;
                stream >> start;
                stream.seekg(dash_index+1);
                stream >> end;
                for(unsigned i = start; i <= end; i++) {
                    cpus.push_back(i);
                }
                dash_index = -1;
            } else {
                unsigned num;
                stream >> num;
                cpus.push_back(num);
            }
            start = i+1;
        } else if(str[i] == '-') {
            dash_index = i;
        }
    }
    return cpus;
}

std::string encode_cpu_list(std::vector<unsigned> vect) {
    std::stringstream out;
    for(unsigned i = 0; i < vect.size(); i++) {
        out << vect[i];
        if(i != vect.size()-1) out << ",";
    }
    return out.str();
}
