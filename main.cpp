#include <cassert>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

#include <readline/history.h>
#include <readline/readline.h>

using namespace std;
namespace fs = std::filesystem;

static string const default_text = "\033[0m";

static string const bold_text = "\033[1m";

static string const red_text = "\033[31m";
static string const green_text = "\033[32m";
static string const yellow_text = "\033[33m";
static string const blue_text = "\033[34m";
static string const magenta_text = "\033[35m";
static string const cyan_text = "\033[36m";

static string const bright_red_text = "\033[91m";
static string const bright_green_text = "\033[92m";
static string const bright_yellow_text = "\033[93m";
static string const bright_blue_text = "\033[94m";
static string const bright_magenta_text = "\033[95m";
static string const bright_cyan_text = "\033[96m";

struct state_t {
    bool exit = false;
    fs::path base{"."};
    bool input_good = true;
    string prompt;
};

void
printIntro() {
    cout << "easy shell 0.1.0 (" << __DATE__ << ", " << __TIME__ << ")" << endl;
    cout << "Enter 'help' for more information about the usage of this program" << endl;
}

void
updatePrompt(state_t &state) {
    stringstream out;
    out << bold_text << bright_green_text << fs::current_path().string() << default_text << ": > ";
    state.prompt = out.str();
}

void
printOutro(state_t const &state) {
    if (!state.input_good) {
        cout << endl;
    }
    cout << "Au revoir, mes amis!" << endl;
}

bool
is_accessible(fs::path base, fs::path path) {
    base = fs::canonical(base);
    path = fs::weakly_canonical(path);
    while (true) {
        if (path == base) {
            return true;
        }

        fs::path newPath = path.parent_path();
        if (path != newPath) {
            path = newPath;
        } else {
            break;
        }
    }

    return false;
}

void
cmd_cat(vector<string> args, state_t &state) {
    if (args.size() != 1) {
        cout << "cat";
        for (string const &arg : args) {
            cout << " <" << arg << ">";
        }
        cout << endl;
        cout << "Invalid syntax! Usage: cat <file>" << endl;
        return;
    }

    fs::path file{args[0]};

    if (!is_accessible(state.base, file)) {
        cout << "cat: " << file << " is not accessible" << endl;
        return;
    }

    if (!fs::exists(file)) {
        cout << "cat: " << file << " doesn't exist" << endl;
        return;
    }

    ifstream file_stream(file);
    char c;
    while (file_stream.get(c)) {
        cout << c;
    }
}

void
cmd_cd(vector<string> args, state_t &state) {
    if (args.size() != 1) {
        cout << "cd";
        for (string const &arg : args) {
            cout << " <" << arg << ">";
        }
        cout << endl;
        cout << "Invalid syntax! Usage: cd <directory>" << endl;
        return;
    }

    fs::path new_working_directory{args[0]};

    if (!is_accessible(state.base, new_working_directory)) {
        cout << "cat: " << new_working_directory << " is not accessible" << endl;
        return;
    }

    try {
        fs::current_path(new_working_directory);
    } catch (fs::filesystem_error err) {
        cout << "cd: " << err.what() << endl;
    }
}

void
cmd_exit(vector<string> /*args*/, state_t &state) {
    state.exit = true;
}

void
cmd_help(vector<string> /*args*/, state_t & /*state*/) {
    cout << left;
    cout << "Available commands:" << endl;
    cout << setw(45) << "  cat <file>"
         << "Shows the content of a <file> as text." << endl;
    cout << setw(45) << "  cd <directory>"
         << "Changes the working directory to <directory>." << endl;
    cout << setw(45) << "  exit"
         << "Leaves the current shell program." << endl;
    cout << setw(45) << "  grep [-r] <pattern> <file/directory...>"
         << "Shows the lines in a given <file> that match a given <pattern>." << endl
         << setw(45) << ""
         << "-r: All files from a given <directory> and its subdirectories are considered." << endl;
    cout << setw(45) << "  help"
         << "Shows this information text." << endl;
    cout << setw(45) << "  ls"
         << "Lists the contents of the working directory." << endl;
    cout << setw(45) << "  ls <directory>"
         << "Lists the contents of the given <directory>." << endl;
}

void
cmd_ls(vector<string> args, state_t &state) {
    if (args.size() >= 2) {
        cout << "ls";
        for (string const &arg : args) {
            cout << " <" << arg << ">";
        }
        cout << endl;
        cout << "Invalid syntax! Usage: ls or ls <directory>" << endl;
        return;
    }

    fs::path target_dir = fs::current_path();
    if (args.size() == 1) {
        target_dir = args[0];
    }

    if (!is_accessible(state.base, target_dir)) {
        cout << "ls: " << target_dir << " is not accessible" << endl;
        return;
    }

    if (!fs::exists(target_dir)) {
        cout << "ls: " << target_dir << " doesn't exist" << endl;
    }

    if (!fs::is_directory(target_dir)) {
        cout << "ls: " << target_dir << " is not a directory" << endl;
    }

    try {
        std::vector<fs::directory_entry> entries;
        for (fs::directory_entry entry : fs::directory_iterator(target_dir)) {
            entries.push_back(entry);
        }

        std::sort(entries.begin(), entries.end());

        for (fs::directory_entry entry : entries) {
            string filename = entry.path().filename();
            if (entry.is_directory()) {
                cout << bold_text << bright_blue_text << filename << "/" << default_text << endl;
            } else {
                cout << filename << endl;
            }
        }
    } catch (fs::filesystem_error err) {
        cout << "ls: " << err.what() << endl;
    }
}

void
cmd_grep(vector<string> args, state_t &state) {
    if (args.size() < 2) {
        cout << "grep";
        for (string const &arg : args) {
            cout << " <" << arg << ">";
        }
        cout << endl;
        cout << "Invalid syntax! Usage: grep [-r] <pattern> <file...>" << endl;
    }

    size_t idx = 0;
    bool recursive = false;
    if (args.at(0) == "-r") {
        recursive = true;
        idx++;
    }

    regex pattern;
    try {
        pattern = regex(args.at(idx++));
    } catch (regex_error err) {
        cout << "grep: " << err.what() << endl;
    }

    vector<fs::path> paths;
    for (size_t i = idx; i < args.size(); ++i) {
        fs::path path = args[i];
        if (!is_accessible(state.base, path)) {
            cout << "grep: " << path << " is not accessible" << endl;
            continue;
        }

        if (!fs::exists(path)) {
            cout << "grep: " << path << " doesn't exist" << endl;
            continue;
        }

        path = fs::relative(path);
        paths.push_back(path);
    }

    bool multiple_paths = paths.size() > 1;

    vector<fs::path> file_paths;
    for (fs::path const &path : paths) {
        try {
            if (fs::is_regular_file(path)) {
                file_paths.push_back(path);
            } else if (fs::is_directory(path)) {
                if (recursive) {
                    for (fs::directory_entry const &entry :
                         fs::recursive_directory_iterator(path)) {
                        if (entry.is_regular_file() && !entry.is_symlink()) {
                            file_paths.push_back(entry.path());
                        }
                    }
                } else {
                    cout << "grep: " << path << " is a directory" << endl;
                }
            }
        } catch (fs::filesystem_error err) {
            cout << "grep: " << err.what() << endl;
        }
    }

    sort(file_paths.begin(), file_paths.end());
    unique(file_paths.begin(), file_paths.end());
    for (fs::path const &file_path : file_paths) {
        ifstream file{file_path};
        if (!file.good()) {
            cout << "grep: Couldn't open " << file_path << endl;
        }

        string line, line_part;
        while (getline(file, line)) {
            smatch match;
            if (regex_search(line, match, pattern)) {
                if (multiple_paths || recursive) {
                    cout << magenta_text << fs::relative(file_path).string() << cyan_text << ":"
                         << default_text;
                }
                string suffix;
                do {
                    cout << match.prefix() << bold_text << bright_red_text << match.str()
                         << default_text;
                    suffix = match.suffix();
                } while (regex_search(suffix, match, pattern) && !suffix.empty());
                cout << suffix << endl;
            }
        }
    }
}

static map<string, function<void(vector<string>, state_t &)>> commands{{"cd", cmd_cd},
                                                                       {"cat", cmd_cat},
                                                                       {"exit", cmd_exit},
                                                                       {"help", cmd_help},
                                                                       {"ls", cmd_ls},
                                                                       {"grep", cmd_grep}};

void
executeCommand(string const &command_line, state_t &state) {
    stringstream command_parser{command_line};
    string command, arg;
    vector<string> args;
    command_parser >> skipws >> command;
    while (true) {
        if (command_parser >> quoted(arg)) {
            args.push_back(arg);
        } else {
            break;
        }
    }

    if (command.empty()) {
        return;
    }

    auto command_it = commands.find(command);
    if (command_it != commands.end()) {
        auto command_func = command_it->second;
        command_func(args, state);
    } else {
        cout << "easy shell: command not found '" << command << "'" << endl;
    }
}

bool
getCommand(string &command, state_t &state) {
    char *input = readline(state.prompt.c_str());
    if (input == nullptr) {
        state.input_good = false;
        return false;
    }

    command = std::string(input);
    if (!command.empty()) {
        add_history(input);
    }

    free(input);

    return true;
}

int
main(int argc, char **args) {
    rl_bind_key('\t', rl_complete);

    state_t state;

    if (argc >= 2) {
        try {
            fs::path new_working_directory{args[1]};
            fs::current_path(new_working_directory);
            state.base = new_working_directory;
        } catch (fs::filesystem_error err) {
            cout << "cd: " << err.what() << endl;
        }
    }

    printIntro();

    while (!state.exit) {
        string command;
        updatePrompt(state);
        if (!getCommand(command, state)) {
            state.exit = true;
            break;
        }
        executeCommand(command, state);
    }

    printOutro(state);

    return 0;
}
