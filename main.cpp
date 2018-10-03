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
#include <signal.h>

typedef std::tuple<pid_t, pid_t, std::string> process;
typedef std::vector<process> process_list;


/**
 * Use regex to search for the pid within the output of the bash command ps.
 *
 * @param ps_output output of the bash ps command.
 * @param pid process id to search for.
 * @return the process tuple of matched pid, if no match was found all contents of the process tuple will be empty.
 */
process get_target(const std::string &ps_output,  pid_t pid) {
    std::string str = "(\\S+)\\s+("+std::to_string(pid)+")\\s+([0-9]+) ([a-bA-Z]) [0-9]{2}:[0-9]{2}:[0-9]{2}\\s+(\\S+)";
    std::regex rgx(str.c_str());

    std::match_results< std::string::const_iterator > matches;

    std::regex_match(ps_output, matches, rgx);
    for( std::size_t index = 1; index < matches.size(); ++index ){
        std::cout << matches[ index ] << '\n';
    }

    if(std::regex_search(ps_output, matches, rgx)) {
        return std::make_tuple(std::stoi(matches[2], nullptr, 10), std::stoi(matches[3], nullptr, 10), matches[5]);
    } else {
        // we failed to find the head process pid set errno to 1 to notify the loop to quit.
        errno=1;
    }
}


/**
 * Use regex to search for child processes of the given ppid within the output of the bash command ps.
 *
 * @param ps_output output of the bash ps command.
 * @param ppid process parent id to match for children.
 * @return a process_list of all the children of the ppid.
 */
process_list get_childs(const std::string &ps_output, const pid_t &ppid) {
    process_list child_pl;

    std::string regexStr =
            "(\\S+)\\s+([0-9]+)\\s+(" + std::to_string(ppid) + ") ([a-bA-Z]) [0-9]{2}:[0-9]{2}:[0-9]{2}\\s+(\\S+)";
    std::regex e(regexStr.c_str());

    std::sregex_iterator iter(ps_output.begin(), ps_output.end(), e);
    std::sregex_iterator end;

    while (iter != end) {
        process process = std::make_tuple(std::stoi((*iter)[2], nullptr, 10), std::stoi((*iter)[3], nullptr, 10),
                                          (*iter)[5]);
        child_pl.push_back(process);
        process_list child_child_pl = get_childs(ps_output, std::stoi((*iter)[2], nullptr, 10));
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
    FILE *stream;
    const int max_buffer = 264;
    char buffer[max_buffer];
    stream = popen("ps -u $USER -o user,pid,ppid,state,start,cmd --sort start", "r");
    if (stream) {
        while (!feof(stream))
            if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
        pclose(stream);
    }
    std::cout << data;
    return data;
}


/**
 * Set the cpu time limit to 10 mins for safe development.
 */
void set_cpu_safety() {
    rlimit rilimit{};
    rilimit.rlim_cur = 600;
    rilimit.rlim_max = 600;
    setrlimit(RLIMIT_CPU, &rilimit);
}


int main(int argc, char **argv) {
    set_cpu_safety();

    // parse command line args
    unsigned int interval;
//    char *pid = nullptr;
    pid_t  pid;
    if (argc <= 1) {
        printf("ERROR: missing arguments\n");
        return 1;
    } else {
//        pid = argv[1];
        pid = std::stoi(argv[1], nullptr, 10);
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

    u_int loop_count = 0;
    process_list child_pl;
    for (;;) {
        printf("a1mon [counter=%2d, pid=%5u, target_pid=%5u, interval=%2d sec]:\n", loop_count, getpid(), pid,
               interval);

        // run ps and concentrate its output
        std::string ps_output = run_ps();
        printf("--------------------\n");

        process head_process = get_target(ps_output, pid);
        if (errno==1) {
            printf("a1mon: target appears to have terminated; cleaning\n");
            for (auto it = child_pl.rbegin(); it != child_pl.rend(); ++it) {
                printf("terminating [%u, %s]\n", std::get<0>(*it), std::get<2>(*it).c_str());
                kill(std::get<0>(*it), SIGKILL);
            }
            printf("exiting a1mon\n");
            return 0;
        }

        // get all children of head process
        child_pl = get_childs(ps_output, std::get<0>(head_process));
        printf("List of monitored processes:\n");
        // print head process
        printf("[0:[%u,%s]", std::get<0>(head_process), std::get<2>(head_process).c_str());
        // iterate through child processes
        for (process_list::size_type i = 0; i != child_pl.size(); i++) {
            printf(", %lu:[%u,%s]", i, std::get<0>(child_pl[i]), std::get<2>(child_pl[i]).c_str());
        }
        printf("]\n");

        // sleep to avoid resource hogging
        sleep(interval);

        // iterate the loop counter
        ++loop_count;
    }
}