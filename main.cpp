#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

struct state_t {
    bool exit = false;
};

void
printIntro() {
    cout << "easy shell 0.1.0 (" << __DATE__ << ", " << __TIME__ << ")" << endl;
    cout << "Enter 'help' for more information about the usage of this program" << endl;
}

void
printPrompt(state_t const & /*state*/) {
    cout << "[" << fs::current_path().string() << "] > " << flush;
}

void
printOutro() {
    if (!cin.good()) {
        cout << endl;
    }
    cout << "Au revoir, mes amis!" << endl;
}

bool
is_parent(fs::path const &potential_child, fs::path const &potential_parent) {
    fs::path::iterator child_it = potential_child.begin();
    fs::path::iterator parent_it = potential_parent.begin();
    while (parent_it != potential_parent.end()) {
        if (*child_it != *parent_it) {
            return false;
        }

        parent_it++;
        child_it++;
    }
    return true;
}

void
cmd_cat(vector<string> const &args, state_t & /*state*/) {
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

    if (!fs::exists(file)) {
        cout << "cat: " << file << " doesn't exist";
        return;
    }

    ifstream file_stream(file);
    char c;
    while (file_stream.get(c)) {
        cout << c;
    }
}

void
cmd_cd(vector<string> const &args, state_t & /*state*/) {
    if (args.size() != 1) {
        cout << "cd";
        for (string const &arg : args) {
            cout << " <" << arg << ">";
        }
        cout << endl;
        cout << "Invalid syntax! Usage: cd <directory>" << endl;
        return;
    }

    try {
        fs::path new_working_directory{args[0]};
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
cmd_ls(vector<string> const &args, state_t & /*state*/) {
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
            cout << entry.path().filename().string() << endl;
        }
    } catch (fs::filesystem_error err) {
        cout << "ls: " << err.what() << endl;
    }
}

void
cmd_mail(vector<string> const &args, state_t & /*state*/) {
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
getCommand(string &command) {
    string current_line;
    while (getline(cin, current_line)) {
        if (!current_line.empty() && current_line.back() == '\\') {
            current_line.back() = '\n';
            command += current_line;
        } else {
            command += current_line;
            break;
        }
    }

    return cin.good();
}

int
main(int argc, char **args) {
    state_t state;

    if (argc >= 2) {
        try {
            fs::path new_working_directory{args[1]};
            fs::current_path(new_working_directory);
        } catch (fs::filesystem_error err) {
            cout << "cd: " << err.what() << endl;
        }
    }

    printIntro();

    while (!state.exit) {
        string command;
        printPrompt(state);
        if (!getCommand(command)) {
            state.exit = true;
            break;
        }
        executeCommand(command, state);
    }

    printOutro();

    return 0;
}
