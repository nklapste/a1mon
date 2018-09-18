#include <iostream>
#include <sys/resource.h>
#include <regex>
#include <zconf.h>


std::tuple<std::string, std::string> get_target(std::string ps_output, std::string pid){
    std::string str = "(\\S+)\\s+("+pid+")\\s+([0-9]+) ([a-bA-Z]) [0-9]{2}:[0-9]{2}:[0-9]{2}\\s+(\\S+)";
    std::regex rgx(str.c_str());
    printf("%s", str.c_str());

    std::match_results< std::string::const_iterator > matches;
    std::regex_match(ps_output, matches, rgx);
    for( std::size_t index = 1; index < matches.size(); ++index ){
        std::cout << matches[ index ] << '\n';
    }

    if(std::regex_search(ps_output, matches, rgx)) {
        std::cout << "Match found\n";
        for (size_t i = 0; i < matches.size(); ++i) {
            std::cout << i << ": '" << matches[i].str() << "'\n";
        }
    }
    return std::make_tuple(matches[2], matches[5]);
}


std::tuple<std::string, std::string> get_child(std::string ps_output, std::string ppid){
    std::string str = "(\\S+)\\s+([0-9]+)\\s+("+ppid+") ([a-bA-Z]) [0-9]{2}:[0-9]{2}:[0-9]{2}\\s+(\\S+)";
    std::regex rgx(str.c_str());
    printf("%s", str.c_str());

    std::match_results< std::string::const_iterator > matches;
    std::regex_match(ps_output, matches, rgx);
    for( std::size_t index = 1; index < matches.size(); ++index ){
        std::cout << matches[ index ] << '\n';
    }

    if(std::regex_search(ps_output, matches, rgx)) {
        std::cout << "Match found\n";
        for (size_t i = 0; i < matches.size(); ++i) {
            std::cout << i << ": '" << matches[i].str() << "'\n";
        }
    }
    return std::make_tuple(matches[2], matches[5]);
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
    return data;
}
typedef std::vector< std::tuple<std::string, std::string> > process_list;

int main(int argc, char **argv) {
    unsigned int interval = 0;
    char *pid = nullptr;
    if (argc < 0){
        printf("missing arguments\n");
        return 1;
    }
    if (argc >= 1){
        pid = argv[1];
    }
    if (argc == 3) {
        interval = (unsigned int) strtol(argv[2], (char **) NULL, 10);

    } else {
        interval = 3;
    }
    if (argc > 3) {
        printf("unsupported arguments\n");
        return 1;
    }

    struct rlimit rilimit;
    rilimit.rlim_cur = 600;
    rilimit.rlim_max = 600;
    setrlimit(RLIMIT_CPU, &rilimit);

    std::string ps_output = run_ps();
    process_list pl;
    std::tuple<std::string, std::string> process;
    process = get_target(ps_output, pid);

    for (;;){
        std::tuple<std::string, std::string> process;
        process = get_target(ps_output, pid);

        pl.push_back(process);

        sleep(interval);
    }
}