#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <regex>

std::string s{R"(
tХB:Username!Username@Username.tcc.domain.com Connected
tХB:Username!Username@Username.tcc.domain.com WEBMSG #Username :this is a message
tХB:Username!Username@Username.tcc.domain.com Status: visible
)"};

bool get_target(std::string ps_output, std::string pid){
    std::string str = "(\\S+) +("+pid+") +([0-9]+) (a-bA-Z) [0-9]{2}:[0-9]{2}:[0-9]{2} (\\S+)";
//    printf("regex: %s", str.c_str());
    std::regex rgx(str.c_str());
    std::smatch matches;

    if(std::regex_search(ps_output, matches, rgx)) {
//        std::cout << "Match found\n";
        return true;

    } else {
//        std::cout << "Match not found\n";
        return false;
    }
}


std::string run_ps() {
    std::string data;
    FILE * stream;
    const int max_buffer = 256;
    char buffer[max_buffer];

    stream = popen("ps -u $USER -o user,pid,ppid,state,start,cmd --sort start", "r");
    if(stream) {
        while (!feof(stream))
            if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
        pclose(stream);
    }
    printf("ps_output:\n%s", data.c_str());
    return data;
}


int main() {

    std::string ps_output = run_ps();
    get_target(ps_output, "6742");
    return 0;
}