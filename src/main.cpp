#include "../include/task_manager.h"
#include <sstream>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;
using namespace std;

void processCommand(TaskManager& manager, const std::string& command);


time_t now = time(0);
tm *ltm = localtime(&now);
int monthNumber = 1 + ltm->tm_mon;

std::string getExecutableDirectory(){
    #ifdef _WIN32
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        return fs::path(buffer).parent_path().string();
    #elif __linux__
        try {
            return fs::canonical("/proc/self/exe").parent_path().string();
        } catch (const std::exception& e) {
            std::cerr << "Could not determine executable directory: " << e.what() << std::endl;
            return ".";
        }
    #elif __APPLE__
        char buffer[PATH_MAX];
        uint32_t size = sizeof(buffer);
        if (_NSGetExecutablePath(buffer, &size) == 0) {
            return fs::canonical(fs::path(buffer)).parent_path().string();
        }
        std::cerr << "Could not determine executable directory" << std::endl;
        return ".";
    #else
        return ".";
    #endif
}

int main(int argc, char* argv[]) {
    std::string executableDirectory = getExecutableDirectory();
    std::string dataFile = executableDirectory + "/tasks.dat";
    
    if (argc > 1 && std::strcmp(argv[1], "--file") == 0 && argc > 2) {
        dataFile = argv[2];
    }

    TaskManager manager(dataFile);

    std::cout << "Task Manager CLI (Type 'h' for commands, 'exit' to quit)" << std::endl;

    if ((argc > 1 && std::strcmp(argv[1], "--file") != 0) || argc > 3) {
        std::string fullCommand;
        for (int i = 1; i < argc; i++) {
            if (i > 1 && std::strcmp(argv[i-1], "--file") == 0) {
                continue;
            }
            if (std::strcmp(argv[i], "--file") == 0) {
                i++;
                continue;
            }
            fullCommand += std::string(argv[i]) + " ";
        }
        processCommand(manager, fullCommand);
        return 0;
    }

    std::string input;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input == "exit") break;

        processCommand(manager, input);
    }

    return 0;
}

void exportToICSFile(const std::string& description, const std::string& deadline, TaskManager& manager) {
    std::ofstream file("./task.ics");
    if (!file) {
        std::cerr << "Error: Could not create task.ics file." << std::endl;
        return;
    }

    std::tm tm = {};
    std::istringstream ss(deadline);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M");
    if (ss.fail()) {
        std::cerr << "Error: Invalid deadline format." << std::endl;
        return;
    }

    char dtStart[32];
    std::strftime(dtStart, sizeof(dtStart), "%Y%m%dT%H%M00", &tm);

    std::time_t now = std::time(nullptr);
    std::tm* now_tm = std::gmtime(&now);
    char dtStamp[32];
    std::strftime(dtStamp, sizeof(dtStamp), "%Y%m%dT%H%M00Z", now_tm);

    file << "BEGIN:VCALENDAR\n";
    file << "VERSION:2.0\n";
    file << "PRODID:-//EV+ Task Manager//EN\n";
    file << "BEGIN:VEVENT\n";
    file << "UID:" << now << "@taskmanager.local\n";
    file << "DTSTAMP:" << dtStamp << "\n";
    file << "DTSTART;TZID=America/New_York:" << dtStart << "\n";
    file << "DTEND;TZID=America/New_York:" << dtStart << "\n";
    file << "SUMMARY:" << description << "\n";
    file << "DESCRIPTION:" << description << "\n";
    file << "STATUS:CONFIRMED\n";
    file << "TRANSP:OPAQUE\n";
    file << "END:VEVENT\n";
    file << "END:VCALENDAR\n";
    file.close();

    int icsVal = manager.getICSVal();
    if (icsVal == 1){
        #ifdef _WIN32
            system("start ./task.ics"); 
        #elif __APPLE__
            system("open ./task.ics");
        #else
            system("xdg-open ./task.ics"); 
        #endif
    }
}

void processCommand(TaskManager& manager, const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    

    if (cmd == "nt") {
        std::string description, deadline;
        std::getline(iss, description);
        if (!description.empty()) {
            description = description.substr(1);

            size_t lastSpace = description.rfind(" ");
            if (lastSpace != std::string::npos) {
                std::string potentialTime = description.substr(lastSpace + 1);
                if (potentialTime.length() == 5 && potentialTime[2] == ':') {
                    size_t prevSpace = description.rfind(" ", lastSpace - 1);
                    if (prevSpace != std::string::npos) {
                        std::string potentialDate = description.substr(prevSpace + 1, lastSpace - prevSpace - 1);
                        if (potentialDate.length() == 10 &&
                            potentialDate[4] == '-' && potentialDate[7] == '-') {
                            deadline = potentialDate + " " + potentialTime;
                            description = description.substr(0, prevSpace);
                        }
                    }
                } else {
                    std::string potentialDate = description.substr(lastSpace + 1);
                    if (potentialDate.length() == 10 &&
                        potentialDate[4] == '-' && potentialDate[7] == '-') {
                        deadline = potentialDate;
                        description = description.substr(0, lastSpace);
                    }
                }
            }
            manager.addTask(description, deadline);
            if (!deadline.empty()) {
                exportToICSFile(description, deadline, manager);
            }
        } else {
            std::cout << "Error: Task description cannot be empty." << std::endl;
        }
    } else if (cmd == "ls") {
        manager.listTasks(false);
    } else if (cmd == "lsa") {
        manager.listTasks(true);
    } else if (cmd == "ft") {
        int id;
        if (iss >> id) {
            manager.completeTask(id);
        } else {
            std::cout << "Error: Invalid task ID." << std::endl;
        }
    } else if (cmd == "dt") {
        int id;
        if (iss >> id) {
            manager.deleteTask(id);
        } else {
            std::cout << "Error: Invalid task ID." << std::endl;
        }
    } else if (cmd == "ct") {
        std::cout << "Clearing all tasks..." << std::endl;
        manager.clearTasks();
    } else if (cmd == "h") {
        manager.help();
    } else if (cmd == "c"){
        monthNumber = ltm->tm_mon + 1;
        manager.displayCalendar(monthNumber);
    } else if (cmd == "sh"){
        int newHeight;
        if (iss >> newHeight){
            if (newHeight >= 5 && newHeight <= 10){
                manager.setCalendarCellHeight(newHeight);
                cout << "The cell height of the calendar has been set to: " << newHeight << "\n";
            } else {
                std::cout << "Error: Height must be between 5 and 10.\n";
            }
        } else {
        std::cout << "Error: Invalid input.Please enter a number.\n";
        }
    } else if (cmd == "sw") {
        int newWidth;
        if (iss >> newWidth) {
            if (newWidth >= 13 && newWidth <= 40) {
                manager.setCalendarCellWidth(newWidth);
                cout << "The cell width of the calendar has been set to: " << newWidth << "\n";
            } else {
                std::cout << "Error: Width must be between 13 and 40.\n";
            }
        } else {
            std::cout << "Error: Invalid input. Please enter a number.\n";
        }
    } else if (cmd == "dc") {
        std::string inputMonth;
        iss >> inputMonth; 
        std::transform(inputMonth.begin(), inputMonth.end(), inputMonth.begin(), ::tolower);

        std::map<std::string, int> monthMap = {
            {"january", 1}, {"february", 2}, {"march", 3}, {"april", 4},
            {"may", 5}, {"june", 6}, {"july", 7}, {"august", 8},
            {"september", 9}, {"october", 10}, {"november", 11}, {"december", 12}
        };
        if (std::all_of(inputMonth.begin(), inputMonth.end(), ::isdigit)) {
            monthNumber = std::stoi(inputMonth);
        } else if (monthMap.count(inputMonth)) {
            monthNumber = monthMap[inputMonth];
        }

        if (monthNumber >= 1 && monthNumber <= 12) {
            manager.displayCalendar(monthNumber);
        } else {
            std::cout << "Invalid month. Please enter a number (1-12) or a valid month name." << std::endl;
        }
    } else if (cmd == "n") {
        monthNumber ++;
        manager.displayCalendar(monthNumber);
    } else if (cmd == "p") {
        monthNumber --;
        manager.displayCalendar(monthNumber);
    } else if (cmd == "t"){
        manager.toggleICS();
        int val = manager.getICSVal();
        if (val == 0){
            std::cout << "Terminal Calendar has successfully been configured to not open your calendar app upon adding a new task!" << std::endl;
        }
        else if(val == 1){
            std::cout << "Terminal Calendar has successfully been configured to open your calendar app upon adding a new task!" << std::endl;
        }
    } else if (cmd == "sort"){
        cout << "   Select how you want events to be sorted (1-3): " << "\n   1. By ID" << "\n   2. By nearest" << "\n   3. By furthest" << "\n Your choice: " ;
        int choice;
        std::string input;
        std::getline(std::cin, input);  
        std::istringstream inputStream(input);

        if (inputStream >> choice) {
            switch (choice){
                case 1:
                    manager.sortByID();
                    std::cout << "Your events have successfully been sorted by their ID!" << std::endl;
                    break;
                case 2:
                    manager.sortByDeadlineAscending();
                    std::cout << "Your events have successfully been sorted by the nearest due date!" << std::endl;
                    break;
                case 3:
                    manager.sortByDeadlineDescending();
                    std::cout << "Your events have successfully been sorted by the furthest due date!" << std::endl;
                    break;
                default:
                    std::cout << "Unknown option. Please enter a valid choice (1-3)" << std::endl;
                    break;
            }
        }
        else{
            std::cout << "Invalid input. Please enter a valid choice (1-3)" << endl;
        }
    } else {
        std::cout << "Unknown command. Type 'h' for available commands." << std::endl;
    }
}
