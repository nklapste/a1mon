/**
 * a1mon
 *
 * @author Nathan Klapstein (nklapste)
 * @version 0.0.0
 */

#include <iostream>
#include <sys/resource.h>
#include <regex>
#include <unistd.h>

typedef std::tuple<std::string, std::string, std::string> process;
typedef std::vector<process> process_list;


/**
 * Use regex to search for the pid within the output of the bash command ps.
 *
 * @param ps_output output of the bash ps command.
 * @param pid process id to search for.
 * @return the process tuple of matched pid, if no match was found all contents of the process tuple will be empty.
 */
process get_target(std::string ps_output, const std::string &pid){
    std::string str = "(\\S+)\\s+("+pid+")\\s+([0-9]+) ([a-bA-Z]) [0-9]{2}:[0-9]{2}:[0-9]{2}\\s+(\\S+)";
    std::regex rgx(str.c_str());

    std::match_results< std::string::const_iterator > matches;
    std::regex_match(ps_output, matches, rgx);

    if(std::regex_search(ps_output, matches, rgx)) {
        std::cout << "expression match: " << matches[0] << std::endl;
    }
    return std::make_tuple(matches[2], matches[3], matches[5]);
}


/**
 * Use regex to search for child processes of the given ppid within the output of the bash command ps.
 *
 * @param ps_output output of the bash ps command.
 * @param ppid process parent id to match for children.
 * @return a process_list of all the children of the ppid.
 */
process_list get_childs(std::string ps_output, const std::string &ppid){
    process_list child_pl;

    std::string regexStr = "(\\S+)\\s+([0-9]+)\\s+("+ppid+") ([a-bA-Z]) [0-9]{2}:[0-9]{2}:[0-9]{2}\\s+(\\S+)";
    std::regex e(regexStr.c_str());

    std::sregex_iterator iter(ps_output.begin(), ps_output.end(), e);
    std::sregex_iterator end;

    while(iter != end)
    {
        std::cout << "expression match: " << (*iter)[0] << std::endl;
        std::tuple<std::string, std::string, std::string> process = std::make_tuple((*iter)[2], (*iter)[3], (*iter)[5]);
        child_pl.push_back(process);
        process_list child_child_pl = get_childs(ps_output, (*iter)[2]);
        child_pl.insert(child_pl.end(), child_child_pl.begin(), child_child_pl.end());
        ++iter;
    }
    return child_pl;
}


/**
 * Execute the bash ps command using popen.
 *
 * @return the output of the bash ps command.
 */
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
    std::cout << data << std::endl;
    return data;
}


int main(int argc, char **argv) {
    // parse command line args
    unsigned int interval;
    char *pid;
    if (argc <= 1){
        printf("missing arguments\n");
        return 1;
    } else {
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

    // set cpu time limit as a safeguard
    struct rlimit rilimit;
    rilimit.rlim_cur = 600;
    rilimit.rlim_max = 600;
    setrlimit(RLIMIT_CPU, &rilimit);

    process_list child_pl;
    for (;;){
        // run ps and concentrate its output
        std::string ps_output = run_ps();
        // TODO: pid is getting set to null
        process head_process = get_target(ps_output, pid);
        if (std::get<0>(head_process).empty()){
            std::cout << "head process" << pid << " dead/missing" << std::endl;
            for(auto it = child_pl.rbegin(); it != child_pl.rend(); ++it) {
                std::cout << "Terminating child: "<< std::get<0>(*it) << std::endl;
                // todo if head process is dead kill head processes children
            }
            return 0;
        } else {
            std::cout << "Head process found: "<< std::get<0>(head_process) << std::endl;
        }

        // get all children of head process
        child_pl = get_childs(ps_output, std::get<0>(head_process));

        // sleep to avoid resource hogging
        sleep(interval);
    }
}