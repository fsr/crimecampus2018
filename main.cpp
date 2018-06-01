#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <readline/readline.h>
#include <readline/history.h>

using namespace std;
namespace fs = std::filesystem;

struct state_t {
    bool exit = false;
    fs::path base {"."};
    std::string prompt;
    bool input_good = true;
};

void
printIntro() {
    cout << "easy shell 0.1.0 (" << __DATE__ << ", " << __TIME__ << ")" << endl;
    cout << "Enter 'help' for more information about the usage of this program" << endl;
}

void
updatePrompt(state_t &state) {
    std::stringstream out;
    out << "[" << fs::current_path().string() << "] > " << flush;
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
cmd_cat(vector<string> const &args, state_t & state) {
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
cmd_cd(vector<string> const &args, state_t & state) {
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
cmd_exit(vector<string> const & /*args*/, state_t &state) {
    state.exit = true;
}

void
cmd_help(vector<string> const & /*args*/, state_t & /*state*/) {
    cout << left;
    cout << "Available commands:" << endl;
    cout << setw(35) << "  cat <file>"
         << "Shows the content of a <file> as text." << endl;
    cout << setw(35) << "  cd <directory>"
         << "Changes the working directory to <directory>." << endl;
    cout << setw(35) << "  exit"
         << "Leaves the current shell program." << endl;
    cout << setw(35) << "  help"
         << "Shows this information text." << endl;
    cout << setw(35) << "  ls"
         << "Lists the contents of the working directory." << endl;
    cout << setw(35) << "  ls <directory>"
         << "Lists the contents of the given <directory>." << endl;
    cout << setw(35) << "  mail <file> <email address>"
         << "Sends a <file> to an <email adress>." << endl;
}

void
cmd_ls(vector<string> const &args, state_t & state) {
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
            cout << entry.path().filename().string() << (entry.is_directory() ? "/" : "") << endl;
        }
    } catch (fs::filesystem_error err) {
        cout << "ls: " << err.what() << endl;
    }
}

void
cmd_mail(vector<string> const &args, state_t & state) {
    if (args.size() != 2) {
        cout << "mail";
        for (string const &arg : args) {
            cout << " <" << arg << ">";
        }
        cout << endl;
        cout << "Invalid syntax! Usage: mail <file> <email address>" << endl;
        return;
    }

    fs::path file = args[0];
    string mail_address = args[1];

    if (!is_accessible(state.base, file)) {
        cout << "mail: " << file << " is not accessible" << endl;
        return;
    }

    if (!fs::exists(file)) {
        cout << "mail: " << file << " doesn't exist" << endl;
        return;
    }

    fs::path mail_tasks_file = fs::temp_directory_path() / "mail_tasks";
    ofstream mail_tasks {mail_tasks_file, ios_base::app};
    mail_tasks << fs::absolute(file) << " " << file.filename() << " " << quoted(mail_address) << endl;
    mail_tasks.close();
}

static map<string, function<void(vector<string> const &, state_t &)>> commands{{"cd", cmd_cd},
                                                                               {"cat", cmd_cat},
                                                                               {"exit", cmd_exit},
                                                                               {"help", cmd_help},
                                                                               {"ls", cmd_ls},
                                                                               {"mail", cmd_mail}};

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
    rl_bind_key('\t', rl_insert);

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
