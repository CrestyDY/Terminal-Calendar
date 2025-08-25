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
#include <stdexcept>

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
    
    std::cout << manager.color_text("Task Manager CLI (Type 'h' for commands, 'exit' to quit)", manager.getTextColor()) << std::endl;

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
        std::cout << manager.color_text("> ", manager.getTextColor());
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
    try {
        int day = std::stoi(cmd);
        int daysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31};
        if (day <= daysInMonth[monthNumber - 1] && day > 0){
            manager.listTasksByDay(monthNumber, day);
        }
        else {
             std::cout << "Invalid day for the current month." << std::endl;
        }
    } catch (const std::invalid_argument& e){
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
                std::cout << manager.color_text("Error: Task description cannot be empty.", manager.getTextColor()) << std::endl;
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
                std::cout << manager.color_text("Error: Invalid task ID.", manager.getTextColor()) << std::endl; 
            }
        } else if (cmd == "dt") {
            int id;
            if (iss >> id) {
                manager.deleteTask(id);
            } else {
                std::cout << manager.color_text("Error: Invalid task ID.", manager.getTextColor()) << std::endl; 
            }
        } else if (cmd == "ct") {
            std::cout << manager.color_text("Clearing all tasks...", manager.getTextColor()) << std::endl; 
            manager.clearTasks();
        } else if (cmd == "h") {
            manager.help();
        } else if (cmd == "c"){
            monthNumber = ltm->tm_mon + 1;
            manager.displayCalendar(monthNumber, true);  // Use static display
        } else if (cmd == "sh"){
            int newHeight;
            if (iss >> newHeight){
                if (newHeight >= 5 && newHeight <= 10){
                    manager.setCalendarCellHeight(newHeight);
                    cout << manager.color_text("The cell height of the calendar has been set to: ", manager.getTextColor()) << newHeight << "\n";
                } else {
                    std::cout << manager.color_text("Error: Height must be between, manager.getTextColor()) 5 and 10.\n", manager.getTextColor()); 
                }
            } else {
            std::cout << manager.color_text("Error: Invalid input.Please enter a number.", manager.getTextColor()) << std::endl; 
            }
        } else if (cmd == "sw") {
            int newWidth;
            if (iss >> newWidth) {
                if (newWidth >= 13 && newWidth <= 40) {
                    manager.setCalendarCellWidth(newWidth);
                    cout << manager.color_text("The cell width of the calendar has been set to: ", manager.getTextColor()) << newWidth << "\n";
                } else {
                    std::cout << manager.color_text("Error: Width must be between 13 and 40.", manager.getTextColor()) << std::endl; 
                }
            } else {
                std::cout << manager.color_text("Error: Invalid input. Please enter a number.", manager.getTextColor()) << std::endl; 
            }
        } else if (cmd == "scc") {
            std::string text = "Welcome to Terminal Calendar";

            std::cout << "Pick your calendar color (1-8): " << std::endl;
            std::cout << "\033[30m" << "1. " + text << "\033[0m" << " (Black)\n";
            std::cout << "\033[31m" << "2. " + text << "\033[0m" << " (Red)\n";
            std::cout << "\033[32m" << "3. " + text << "\033[0m" << " (Green)\n";
            std::cout << "\033[33m" << "4. " + text << "\033[0m" << " (Yellow)\n";
            std::cout << "\033[34m" << "5. " + text << "\033[0m" << " (Blue)\n";
            std::cout << "\033[35m" << "6. " + text << "\033[0m" << " (Magenta)\n";
            std::cout << "\033[36m" << "7. " + text << "\033[0m" << " (Cyan)\n";
            std::cout << "\033[37m" << "8. " + text << "\033[0m" << " (White)\n";

            int choice;
            std::string input;
            std::getline(std::cin, input);  
            std::istringstream inputStream(input);

            if (inputStream >> choice) {
                switch(choice){
                    case 1:
                        manager.setCalendarBorderColor("BLACK");
                        break;
                    case 2:
                        manager.setCalendarBorderColor("RED");
                        break;
                    case 3:
                        manager.setCalendarBorderColor("GREEN");
                        break;
                    case 4:
                        manager.setCalendarBorderColor("YELLOW");
                        break;
                    case 5:
                        manager.setCalendarBorderColor("BLUE");
                        break;
                    case 6:
                        manager.setCalendarBorderColor("MAGENTA");
                        break;
                    case 7:
                        manager.setCalendarBorderColor("CYAN");
                        break;
                    case 8:
                        manager.setCalendarBorderColor("WHITE");
                        break;
                    default:
                        std::cout << manager.color_text("Unknown option. Please enter a valid option (1-8)", manager.getTextColor()) << std::endl; 
                        break;
                }
            }
            else {
                std::cout << manager.color_text("Invalid input. Please enter a valid choice (1-8)", manager.getTextColor()) << std::endl; 
            }
        } else if (cmd == "stc") {
            std::string text = "Welcome to Terminal Calendar";

            std::cout << "Pick your text color (1-8):\n";
            std::cout << "\033[30m" << "1. " + text << "\033[0m" << " (Black)\n";
            std::cout << "\033[31m" << "2. " + text << "\033[0m" << " (Red)\n";
            std::cout << "\033[32m" << "3. " + text << "\033[0m" << " (Green)\n";
            std::cout << "\033[33m" << "4. " + text << "\033[0m" << " (Yellow)\n";
            std::cout << "\033[34m" << "5. " + text << "\033[0m" << " (Blue)\n";
            std::cout << "\033[35m" << "6. " + text << "\033[0m" << " (Magenta)\n";
            std::cout << "\033[36m" << "7. " + text << "\033[0m" << " (Cyan)\n";
            std::cout << "\033[37m" << "8. " + text << "\033[0m" << " (White)\n";

            int choice;
            std::string input;
            std::getline(std::cin, input);  
            std::istringstream inputStream(input);

            if (inputStream >> choice) {
                switch(choice){
                    case 1:
                        manager.setTextColor("BLACK");
                        break;
                    case 2:
                        manager.setTextColor("RED");
                        break;
                    case 3:
                        manager.setTextColor("GREEN");
                        break;
                    case 4:
                        manager.setTextColor("YELLOW");
                        break;
                    case 5:
                        manager.setTextColor("BLUE");
                        break;
                    case 6:
                        manager.setTextColor("MAGENTA");
                        break;
                    case 7:
                        manager.setTextColor("CYAN");
                        break;
                    case 8:
                        manager.setTextColor("WHITE");
                        break;
                    default:
                        std::cout << manager.color_text("Unknown option. Please enter a valid option (1-8)", manager.getTextColor()) << std::endl;
                        break;    
                }
            }
            else {
                std::cout << manager.color_text("Invalid input. Please enter a valid choice (1-8)", manager.getTextColor()) << std::endl; 
            }
        } else if (cmd == "sec"){

            std::string text = "Events: ";

            std::cout << "Pick your text color (1-8):\n";
            std::cout << "\033[30m" << "1. " + text << "\033[0m" << " (Black)\n";
            std::cout << "\033[31m" << "2. " + text << "\033[0m" << " (Red)\n";
            std::cout << "\033[32m" << "3. " + text << "\033[0m" << " (Green)\n";
            std::cout << "\033[33m" << "4. " + text << "\033[0m" << " (Yellow)\n";
            std::cout << "\033[34m" << "5. " + text << "\033[0m" << " (Blue)\n";
            std::cout << "\033[35m" << "6. " + text << "\033[0m" << " (Magenta)\n";
            std::cout << "\033[36m" << "7. " + text << "\033[0m" << " (Cyan)\n";
            std::cout << "\033[37m" << "8. " + text << "\033[0m" << " (White)\n";

            int choice;
            std::string input;
            std::getline(std::cin, input);  
            std::istringstream inputStream(input);

            if (inputStream >> choice) {
                switch(choice){
                    case 1:
                        manager.setEventsColor("BLACK");
                        break;
                    case 2:
                        manager.setEventsColor("RED");
                        break;
                    case 3:
                        manager.setEventsColor("GREEN");
                        break;
                    case 4:
                        manager.setEventsColor("YELLOW");
                        break;
                    case 5:
                        manager.setEventsColor("BLUE");
                        break;
                    case 6:
                        manager.setEventsColor("MAGENTA");
                        break;
                    case 7:
                        manager.setEventsColor("CYAN");
                        break;
                    case 8:
                        manager.setEventsColor("WHITE");
                        break;
                    default:
                        std::cout << manager.color_text("Unknown option. Please enter a valid option (1-8)", manager.getTextColor()) << std::endl;
                        break;    
                }
            }
            else {
                std::cout << manager.color_text("Invalid input. Please enter a valid choice (1-8)", manager.getTextColor()) << std::endl; 
            }
        } else if (cmd == "fetch"){
            manager.displaySummary(); 
        } else if (cmd == "scb"){
            std::string text = "***********************";

            std::cout << manager.color_text("Do you want to toggle the boldness of your calendar borders? (y/n): \n", manager.getTextColor());
            std::cout << manager.color_text("Your current calendar border: ", manager.getTextColor()) << std::string(11, ' ') << manager.color_text(text, manager.getCalendarBorderColor(), manager.getCalendarBorderBold()) << std::endl;
            std::cout << manager.color_text("Your calendar border after the change: ", manager.getTextColor()) << "  " << manager.color_text(text, manager.getCalendarBorderColor(), manager.getCalendarBorderBold() ^ 1) << std::endl;
            char choice;
            std::string input;
            std::getline(std::cin, input);  
            std::istringstream inputStream(input);
            if (inputStream >> choice) {
                switch(choice){
                    case 'y':
                        manager.toggleCalendarBorderBold();
                        break;
                    case 'n':
                        break;
                    default:
                        std::cout << manager.color_text("Unknown option. Please enter a valid option (y/n)", manager.getTextColor()) << std::endl;
                        break;
                }
            }
            else {
                std::cout << manager.color_text("Invalid input. Please enter a valid choice (y/n)", manager.getTextColor()) << std::endl;
            }
        } else if (cmd == "stb"){
            std::string text = "Welcome to Terminal Calendar !";
            std::cout << manager.color_text("Do you want to toggle the boldness of Terminal Calendar's text ? (y/n): \n", manager.getTextColor());
            std::cout << manager.color_text("Your current text: ", manager.getTextColor()) << std::string(11, ' ') << manager.color_text(text, manager.getTextColor(), manager.getTextBold()) << std::endl;
            std::cout << manager.color_text("Your text after the change: ", manager.getTextColor()) << "  " << manager.color_text(text, manager.getTextColor(), manager.getTextBold() ^ 1) << std::endl;
            char choice;
            std::string input;
            std::getline(std::cin, input);  
            std::istringstream inputStream(input);
            if (inputStream >> choice) {
                switch(choice){
                    case 'y':
                        manager.toggleTextBold();
                        break;
                    case 'n':
                        break;
                    default:
                        std::cout << manager.color_text("Unknown option. Please enter a valid option (y/n)", manager.getTextColor()) << std::endl;
                        break;
                }
            }
            else {
                std::cout << manager.color_text("Invalid input. Please enter a valid choice (y/n)", manager.getTextColor()) << std::endl;
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
                manager.displayCalendar(monthNumber, true);  // Use static display
            } else {
                std::cout << manager.color_text("Invalid month. Please enter a number (1-12) or a valid month name.", manager.getTextColor()) << std::endl; 
            }
        } else if (cmd == "n") {
            monthNumber ++;
            manager.displayCalendar(monthNumber, true);  // Use static display for navigation
        } else if (cmd == "p") {
            monthNumber --;
            manager.displayCalendar(monthNumber, true);  // Use static display for navigation
        } else if (cmd == "t"){
            int val = manager.getICSVal();
            if (val == 0){
                std::cout << manager.color_text("Terminal Calendar is currently configured to not open your calendar app upon adding a new task. \nDo you want to configure it so that it opens your calendar app when adding a new task? (y/n)", manager.getTextColor()) << std::endl;
                char choice;
                std::string input;
                std::getline(std::cin, input);
                std::istringstream inputStream(input);
                if (inputStream >> choice) {
                    switch (choice){
                        case 'y':
                            manager.toggleICS();
                            std::cout << manager.color_text("Terminal Calendar has been configured to open your calendar app upon adding a new task!", manager.getTextColor()) << std::endl;
                            break;
                        case 'n':
                            std::cout << manager.color_text("No changes were made.", manager.getTextColor()) << std::endl;
                            break;
                        default:
                            std::cout << manager.color_text("Unknown input. Please enter a valid input (y/n).", manager.getTextColor()) << std::endl;
                            break;
                    }
                }
            }
            else if (val == 1){
                std::cout << manager.color_text("Terminal Calendar is currently configured to open your calendar app upon adding a new task. \nDo you want to configure it so that it does not open your calendar app when adding a new task? (y/n)", manager.getTextColor()) << std::endl;
                char choice;
                std::string input;
                std::getline(std::cin, input);
                std::istringstream inputStream(input);
                if (inputStream >> choice) {
                    switch (choice){
                        case 'y':
                            manager.toggleICS();
                            std::cout << manager.color_text("Terminal Calendar has been configured to not open your calendar app upon adding a new task!", manager.getTextColor()) << std::endl;
                            break;
                        case 'n':
                            std::cout << manager.color_text("No changes were made.", manager.getTextColor()) << std::endl;
                            break;
                        default:
                            std::cout << manager.color_text("Unknown input. Please enter a valid input (y/n).", manager.getTextColor()) << std::endl;
                            break;
                    }
                }
            }
            else {
                std::cerr << manager.color_text("Error reading config file. Detected an invalid value.", manager.getTextColor()) << std::endl;
            }
        } else if (cmd == "display"){
            int val = manager.getEventDisplay();
            if (val == 0){
                std::cout << manager.color_text("Terminal Calendar is currently configured to display a summary of events for the day on the calendar. \nDo you want to configure it so that it lists the event descriptions on your calendar instead? (y/n)", manager.getTextColor()) << std::endl;
                char choice;
                std::string input;
                std::getline(std::cin, input);
                std::istringstream inputStream(input);
                if (inputStream >> choice) {
                    switch (choice){
                        case 'y':
                            manager.toggleEventDisplay();
                            std::cout << manager.color_text("Terminal Calendar has been configured to list task descriptions on your calendar!", manager.getTextColor()) << std::endl;
                            break;
                        case 'n':
                            std::cout << manager.color_text("No changes were made.", manager.getTextColor()) << std::endl;
                            break;
                        default:
                            std::cout << manager.color_text("Unknown input. Please enter a valid input (y/n).", manager.getTextColor()) << std::endl;
                            break;
                    }
                }
            }
            else if (val == 1){
                std::cout << manager.color_text("Terminal Calendar is currently configured to list task descriptions on the calendar. \nDo you want to configure it so that it does displays a summary of events for the day instead? (y/n)", manager.getTextColor()) << std::endl;
                char choice;
                std::string input;
                std::getline(std::cin, input);
                std::istringstream inputStream(input);
                if (inputStream >> choice) {
                    switch (choice){
                        case 'y':
                            manager.toggleEventDisplay();
                            std::cout << manager.color_text("Terminal Calendar has been configured to display a summary of events for the day!", manager.getTextColor()) << std::endl;
                            break;
                        case 'n':
                            std::cout << manager.color_text("No changes were made.", manager.getTextColor()) << std::endl;
                            break;
                        default:
                            std::cout << manager.color_text("Unknown input. Please enter a valid input (y/n).", manager.getTextColor()) << std::endl;
                            break;
                    }
                }
            }
            else {
                std::cerr << manager.color_text("Error reading config file. Detected an invalid value.", manager.getTextColor()) << std::endl;
            }
        } else if (cmd == "sort"){
            std::cout << manager.color_text("   Select how you want events to be sorted (1-3): ", manager.getTextColor()) << manager.color_text("\n   1. By ID", manager.getTextColor()) << manager.color_text("\n   2. By nearest", manager.getTextColor()) << manager.color_text("\n   3. By furthest", manager.getTextColor()) << manager.color_text("\n Your choice: ", manager.getTextColor()) ;
            int choice;
            std::string input;
            std::getline(std::cin, input);  
            std::istringstream inputStream(input);

            if (inputStream >> choice) {
                switch (choice){
                    case 1:
                        manager.sortByID();
                        std::cout << manager.color_text("Your events have successfully been sorted by their ID!", manager.getTextColor()) << std::endl; 
                        break;
                    case 2:
                        manager.sortByDeadlineAscending();
                        std::cout << manager.color_text("Your events have successfully been sorted by the nearest due date!", manager.getTextColor()) << std::endl; 
                        break;
                    case 3:
                        manager.sortByDeadlineDescending();
                        std::cout << manager.color_text("Your events have successfully been sorted by the furthest due date!", manager.getTextColor()) << std::endl; 
                        break;
                    default:
                        std::cout << manager.color_text("Unknown option. Please enter a valid choice (1-3)", manager.getTextColor()) << std::endl; 
                        break;
                }
            }
            else{
                std::cout << manager.color_text("Invalid input. Please enter a valid choice (1-8)", manager.getTextColor()) << endl; 
            }
        } else {
            std::cout << manager.color_text("Unknown command. Type 'h' for available commands.", manager.getTextColor()) << std::endl; 
        }
    }
}
