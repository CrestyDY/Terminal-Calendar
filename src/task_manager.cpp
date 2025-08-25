#include "../include/task_manager.h"
#include <map>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <codecvt>

using json = nlohmann::json;
namespace fs = std::filesystem;

int TaskManager::CELL_WIDTH;
int TaskManager::CELL_HEIGHT;
int TaskManager::ICS_VALUE;
std::string TaskManager::CALENDAR_BORDER_COLOR;
std::string TaskManager::TEXT_COLOR;
std::string TaskManager::EVENTS_COLOR;
std::string TaskManager::SORT_METHOD;
int TaskManager::CALENDAR_BORDER_BOLD;
int TaskManager::TEXT_BOLD;
int TaskManager::EVENT_DISPLAY;
json TaskManager::configFile;
int TaskManager::CALENDAR_HEIGHT;

static const std::unordered_map<std::string, std::string> colorCodes = {
        {"BLACK", "\033[30m"}, {"RED", "\033[31m"}, {"GREEN", "\033[32m"},
        {"YELLOW", "\033[33m"}, {"BLUE", "\033[34m"}, {"MAGENTA", "\033[35m"},
        {"CYAN", "\033[36m"}, {"WHITE", "\033[37m"},

        {"BOLD_BLACK", "\033[1;30m"}, {"BOLD_RED", "\033[1;31m"},
        {"BOLD_GREEN", "\033[1;32m"}, {"BOLD_YELLOW", "\033[1;33m"},
        {"BOLD_BLUE", "\033[1;34m"}, {"BOLD_MAGENTA", "\033[1;35m"},
        {"BOLD_CYAN", "\033[1;36m"}, {"BOLD_WHITE", "\033[1;37m"}
    };

TaskManager::TaskManager(const std::string& file) : filename(file), nextId(1) {
    loadConfigs();
    loadTasks();
    CALENDAR_HEIGHT = calculateCalendarHeight();
}


std::string TaskManager::color_text(const std::string& text, const std::string& color, const int bold) {
    auto it = colorCodes.find(color);
    if (it == colorCodes.end()) {
        return text;
    }
    std::string result;
    if (bold == 1) {
        result += "\033[1m";
    } else if (bold == 0) {
    } else {
        return "[Error: Invalid bold value. Use 0 or 1]";
    }

    result += it->second;
    result += text;
    result += "\033[0m";

    return result;
}

void TaskManager::clearScreen() {
    std::cout << "\033[2J\033[3J\033[H";
}

void TaskManager::moveCursor(int row, int col) {
    std::cout << "\033[" << row << ";" << col << "H";
}

void TaskManager::saveCursor() {
    std::cout << "\033[s";
}

void TaskManager::restoreCursor() {
    std::cout << "\033[u";
}

void TaskManager::clearLine() {
    std::cout << "\033[2K";
}

void TaskManager::clearFromCursor() {
    std::cout << "\033[J";
}

int TaskManager::calculateCalendarHeight() {
    // Month/year header: 2 lines
    // Day names header: 2 lines  
    // Border lines: 1 line
    // Maximum 6 weeks * (1 border + cell_height rows)
    // Bottom border: 1 line
    // Extra buffer: 5 lines
    return 8 + (6 * (CELL_HEIGHT + 1));
}

std::string TaskManager::getExecutableDirectory(){
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

void TaskManager::loadConfigs(){
    std::string executableDirectory = getExecutableDirectory();
    std::string configPath = executableDirectory + "/config.json";
    std::ifstream file(configPath); 
    if (!file){
        std::cerr << "Could not open the config file" << std::endl;
        return ;
    }
    file >> TaskManager::configFile;
    try {
        TaskManager::CELL_WIDTH = configFile.value("CELL_WIDTH", 28);
        TaskManager::CELL_HEIGHT = configFile.value("CELL_HEIGHT", 10);
        TaskManager::ICS_VALUE = configFile.value("ICS_VALUE", 1);
        TaskManager::CALENDAR_BORDER_COLOR = configFile.value("CALENDAR_BORDER_COLOR", "WHITE");
        TaskManager::TEXT_COLOR = configFile.value("TEXT_COLOR", "WHITE");
        TaskManager::EVENTS_COLOR = configFile.value("EVENTS_COLOR", "WHITE");
        TaskManager::CALENDAR_BORDER_BOLD = configFile.value("CALENDAR_BORDER_BOLD", 0);
        TaskManager::TEXT_BOLD = configFile.value("TEXT_BOLD", 0);
        TaskManager::EVENT_DISPLAY = configFile.value("EVENT_DISPLAY",1);
        std::string sortingMethod = configFile.value("SORTING_METHOD", "ID");
        if (sortingMethod.compare("ID") == 0){
            TaskManager::SORT_METHOD = "By ID";
        }
        else if (sortingMethod.compare("ASCENDING") == 0){
            TaskManager::SORT_METHOD = "By closest";
        }
        else if (sortingMethod.compare("DESCENDING") == 0){
            TaskManager::SORT_METHOD = "By furthest";
        }
    } catch (const json::exception& e) {
        std::cerr << "Error parsing config.json: " << e.what() << std::endl;
    }
}
void TaskManager::loadTasks() {
    std::ifstream inFile(filename);
    if (!inFile) {
        std::cout << color_text("No existing task file found. Creating a new one.", TaskManager::TEXT_COLOR) << std::endl;
        return;
    }
    
    tasks.clear();
    taskList.clear();
    std::string line;
    
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        Task task;
        char delimiter;
        
        iss >> task.id >> delimiter;
        if (delimiter != '|') continue;
        
        std::getline(iss, task.description, '|');
        std::getline(iss, task.deadline, '|');
        int month = getMonthOfTask(task.deadline);       
        int completed;
        iss >> completed;
        task.completed = completed == 1;
        task.day = getDayOfTask(task.deadline);
        task.year = getYearOfTask(task.deadline);
        task.month = month;
        tasks[month].push_back(task);
        taskList.push_back(task);
        
        if (task.id >= nextId) {
            nextId = task.id + 1;
        }
    }
    
    inFile.close();
}

void TaskManager::saveTasks() {
    std::ofstream outFile(filename);
    for (int i = 1; i < 13; i++){
        if (!tasks[i].empty()){
            for (const auto& task : tasks[i]) {
                outFile << task.id << "|" << task.description << "|" 
                        << task.deadline << "|" << (task.completed ? 1 : 0) << std::endl;
            }
        }
    }
    outFile.close();
}

std::string TaskManager::getCurrentDateTime() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

bool TaskManager::isValidDateTime(const std::string& dateTime) {
    if (dateTime.length() < 16) return false;
    
    if (dateTime[4] != '-' || dateTime[7] != '-') return false;
    
    if (dateTime[13] != ':') return false;
    
    if (dateTime[10] != ' ') return false;
    
    for (int i = 0; i < 4; i++) {
        if (!isdigit(dateTime[i])) return false;     
    }
    for (int i = 5; i < 7; i++) {
        if (!isdigit(dateTime[i])) return false;
    }
    for (int i = 8; i < 10; i++) {
        if (!isdigit(dateTime[i])) return false; 
    }
    for (int i = 11; i < 13; i++) {
        if (!isdigit(dateTime[i])) return false;
    }
    for (int i = 14; i < 16; i++) {
        if (!isdigit(dateTime[i])) return false;
    }
    
    return true;
}

void TaskManager::printYearAndMonth(int year, int month){
        std::map<int, std::vector<std::string>> monthMap = {
        {1, {
            "     ██╗ █████╗ ███╗   ██╗",
            "     ██║██╔══██╗████╗  ██║",
            "     ██║███████║██╔██╗ ██║",
            "██   ██║██╔══██║██║╚██╗██║",
            "╚█████╔╝██║  ██║██║ ╚████║",
            " ╚════╝ ╚═╝  ╚═╝╚═╝  ╚═══╝"
        }},
        {2, {
            " ███████╗███████╗██████╗ ",
            " ██╔════╝██╔════╝██╔══██╗",
            " █████╗  █████╗  ██████╔╝",
            " ██╔══╝  ██╔══╝  ██╔══██╗",
            " ██║     ███████╗██████╔╝",
            " ╚═╝     ╚══════╝╚═════╝ "
        }},
        {3, {
            " ███╗   ███╗ █████╗ ██████╗ ",
            " ████╗ ████║██╔══██╗██╔══██╗",
            " ██╔████╔██║███████║██████╔╝",
            " ██║╚██╔╝██║██╔══██║██╔══██╗",
            " ██║ ╚═╝ ██║██║  ██║██║  ██║",
            " ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝"
        }},
        {4, {
            "  █████╗ ██████╗ ██████╗ ",
            " ██╔══██╗██╔══██╗██╔══██╗",
            " ███████║██████╔╝██████╔╝",
            " ██╔══██║██╔═══╝ ██╔══██╗",
            " ██║  ██║██║     ██║  ██║",
            " ╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝"
        }},
        {5, {
            " ███╗   ███╗ █████╗ ██╗   ██╗",
            " ████╗ ████║██╔══██╗╚██╗ ██╔╝",
            " ██╔████╔██║███████║ ╚████╔╝ ",
            " ██║╚██╔╝██║██╔══██║  ╚██╔╝  ",
            " ██║ ╚═╝ ██║██║  ██║   ██║   ",
            " ╚═╝     ╚═╝╚═╝  ╚═╝   ╚═╝   "
        }},
        {6, {
            "     ██╗██╗   ██╗███╗   ██╗",
            "     ██║██║   ██║████╗  ██║",
            "     ██║██║   ██║██╔██╗ ██║",
            "██   ██║██║   ██║██║╚██╗██║",
            "╚█████╔╝╚██████╔╝██║ ╚████║",
            " ╚════╝  ╚═════╝ ╚═╝  ╚═══╝"
        }},
        {7, {
            "     ██╗██╗   ██╗██╗     ",
            "     ██║██║   ██║██║     ",
            "     ██║██║   ██║██║     ",
            "██   ██║██║   ██║██║     ",
            "╚█████╔╝╚██████╔╝███████╗",
            " ╚════╝  ╚═════╝ ╚══════╝"
        }},
        {8, {
            "  █████╗ ██╗   ██╗ ██████╗ ",
            " ██╔══██╗██║   ██║██╔════╝ ",
            " ███████║██║   ██║██║  ███╗",
            " ██╔══██║██║   ██║██║   ██║",
            " ██║  ██║╚██████╔╝╚██████╔╝",
            " ╚═╝  ╚═╝ ╚═════╝  ╚═════╝ "
        }},
        {9, {
            "███████╗███████╗██████╗ ████████╗",
            "██╔════╝██╔════╝██╔══██╗╚══██╔══╝",
            "███████╗█████╗  ██████╔╝   ██║   ",
            "╚════██║██╔══╝  ██╔═══╝    ██║   ",
            "███████║███████╗██║        ██║   ",
            "╚══════╝╚══════╝╚═╝        ╚═╝   "
        }},
        {10, {
            "  ██████╗  ██████╗████████╗",
            " ██╔═══██╗██╔════╝╚══██╔══╝",
            " ██║   ██║██║        ██║   ",
            " ██║   ██║██║        ██║   ",
            " ╚██████╔╝╚██████╗   ██║   ",
            "  ╚═════╝  ╚═════╝   ╚═╝   "
        }},
        {11, {
            " ███╗   ██╗ ██████╗ ██╗   ██╗",
            " ████╗  ██║██╔═══██╗██║   ██║",
            " ██╔██╗ ██║██║   ██║██║   ██║",
            " ██║╚██╗██║██║   ██║╚██╗ ██╔╝",
            " ██║ ╚████║╚██████╔╝ ╚████╔╝ ",
            " ╚═╝  ╚═══╝ ╚═════╝   ╚═══╝  "
        }},
        {12, {
            " ██████╗ ███████╗ ██████╗ ",
            " ██╔══██╗██╔════╝██╔════╝ ",
            " ██║  ██║█████╗  ██║      ",
            " ██║  ██║██╔══╝  ██║      ",
            " ██████╔╝███████╗╚██████╗ ",
            " ╚═════╝ ╚══════╝ ╚═════╝ "
        }}
    };
    std::map<int, std::vector<std::string>> yearMap = {
    {2025, {
            " ██████╗  ██████╗ ██████╗ ███████╗",
            " ╚════██╗██╔═══██╗╚════██╗██╔════╝",
            "  █████╔╝██║   ██║ █████╔╝███████╗",
            " ██╔═══╝ ██║   ██║██╔═══╝ ╚════██║",
            " ███████╗╚██████╔╝███████╗███████║",
            " ╚══════╝ ╚═════╝ ╚══════╝╚══════╝"       
        }},
        {2026,{
            " ██████╗  ██████╗ ██████╗  ██████╗ ",
            " ╚════██╗██╔═══██╗╚════██╗██╔════╝ ",
            "  █████╔╝██║   ██║ █████╔╝███████╗ ",
            " ██╔═══╝ ██║   ██║██╔═══╝ ██╔═══██╗",
            " ███████╗╚██████╔╝███████╗╚██████╔╝",
            " ╚══════╝ ╚═════╝ ╚══════╝ ╚═════╝ "
        }}
    };
    std::vector<std::string> monthASCII = monthMap[month];
    std::vector<std::string> yearASCII = yearMap[year];
    for (int i = 0; i < static_cast<int>(monthASCII.size()); i ++){
        std::cout << std::string(static_cast<int>((TaskManager::getCalendarCellWidth() * 7 - count_utf8_characters_wstring(monthASCII[i]) - 3 - count_utf8_characters_wstring(yearASCII[i])) / 2), ' ') << color_text(monthASCII[i], TaskManager::TEXT_COLOR, 0) << "   " << color_text(yearASCII[i], TaskManager::TEXT_COLOR, 0) << std::endl;
    }
}

size_t TaskManager::count_utf8_characters_wstring(const std::string& str) {
    size_t count = 0;
    for (size_t i = 0; i < str.size(); ) {
        unsigned char c = str[i];
        if ((c & 0x80) == 0x00) i += 1;           // 1-byte char
        else if ((c & 0xE0) == 0xC0) i += 2;      // 2-byte char
        else if ((c & 0xF0) == 0xE0) i += 3;      // 3-byte char
        else if ((c & 0xF8) == 0xF0) i += 4;      // 4-byte char
        else i += 1;  // Fallback: skip invalid byte
        count++;
    }
    return count;
}

void TaskManager::addTask(const std::string& description, const std::string& deadline) {
    Task task;
    task.id = nextId++;
    task.description = description;
    
    if (deadline.empty()) {
        task.deadline = getCurrentDateTime();
    } else if (deadline.length() == 10 && deadline[4] == '-' && deadline[7] == '-') {
        task.deadline = deadline + " 00:00";
    } else {
        task.deadline = deadline;
    }
    int month = getMonthOfTask(task.deadline);
    int day = getDayOfTask(task.deadline);
    int year = getYearOfTask(task.deadline);
    task.month = month;
    task.day = day;
    task.year = year;
    task.completed = false;
    
    tasks[task.month].push_back(task);
    taskList.push_back(task);

    std::string executableDirectory = getExecutableDirectory();
    std::ifstream file(executableDirectory + "/config.json");
    if (!file){
        std::cerr << "Could not open the config file" << std::endl;
    }
    file >> TaskManager::configFile;
    std::string sortMethod = TaskManager::configFile["EVENT_SORT"];
    if (sortMethod.compare("ID") == 0){
        sortByID();
    }
    else if (sortMethod.compare("ASCENDING") == 0){
        sortByDeadlineAscending();
    }
    else if (sortMethod.compare("DESCENDING") == 0){
        sortByDeadlineDescending();
    }
    else {
        std::cerr << "Invalid sorting method detected in config.json. \nMethod detected: " << sortMethod << std::endl;
    }
    saveTasks();
    
    std::cout << color_text("Task added with ID ", TaskManager::TEXT_COLOR) << task.id << std::endl;
}

void TaskManager::listTasks(bool all) {
    if (taskList.empty()) {
        std::cout << color_text("No tasks found.", TaskManager::TEXT_COLOR) << std::endl;
        return;
    }
    
    std::cout << std::left 
              << std::setw(5) << color_text("ID", TaskManager::TEXT_COLOR) << "   "
              << std::setw(50) << color_text("Description", TaskManager::TEXT_COLOR) 
              << std::setw(20) << color_text("Deadline", TaskManager::TEXT_COLOR) 
              << color_text("Status", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text(std::string(80, '-'), TaskManager::TEXT_COLOR) << std::endl;
    
    for (const auto& task : taskList) {
        if (all || !task.completed) {
            std::cout << std::left 
                      << std::setw(5) << color_text(std::to_string(task.id), TaskManager::TEXT_COLOR) << std::string(5 - static_cast<int>(std::to_string(task.id).size()), ' ') 
                      << std::setw(50) << color_text(task.description, TaskManager::TEXT_COLOR)
                      << std::setw(20) << color_text(task.deadline, TaskManager::TEXT_COLOR) << " " 
                      << (task.completed ? color_text("Completed", TaskManager::TEXT_COLOR) : color_text("Pending", TaskManager::TEXT_COLOR)) << std::endl;
        }
    }
}

void TaskManager::listTasksByDay(int month, int day){
    if (month < 1) {
        std::cout << color_text("Invalid month. Please enter a value between 1 and 12.\n", TaskManager::TEXT_COLOR);
        return;
    }
    
    int newYear;
    
    if (month > 12){
        newYear = static_cast<int>(month/12);
        month = month % 12;
    }
    else {
        newYear = 0;
    }
    
    time_t now = time(0);
    tm *ltm = localtime(&now);
    int year = 1900 + ltm->tm_year + newYear;

    std::vector<Task> TasksForTheDay;
    for (int i = 0; i < static_cast<int>(taskList.size()); i ++){
        if (taskList[i].day == day && taskList[i].month == month && taskList[i].year == year){
            TasksForTheDay.push_back(taskList[i]);
        }
    }

    std::map<int, std::string> monthMap = {
        {1, "January"}, {2, "February"}, {3, "March"}, {4, "April"},
        {5, "May"}, {6, "June"}, {7, "July"}, {8, "August"},
        {9, "September"}, {10, "October"}, {11, "November"}, {12, "December"}
    };

    if (TasksForTheDay.empty()){
        std::cout << "No tasks scheduled for " << monthMap[month] << " " << day << std::endl; 
        return ;
    }

    std::cout << std::left << "Events for " << monthMap[month] << " " << day << ": \n \n";
    
    std::cout << std::left 
              << std::setw(5) << color_text("ID", TaskManager::TEXT_COLOR) << "   "
              << std::setw(50) << color_text("Description", TaskManager::TEXT_COLOR) 
              << color_text("Status", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text(std::string(80, '-'), TaskManager::TEXT_COLOR) << std::endl;

    for (const auto& task : TasksForTheDay) {
        std::cout << std::left 
                  << std::setw(5) << color_text(std::to_string(task.id), TaskManager::TEXT_COLOR) << std::string(5 - static_cast<int>(std::to_string(task.id).size()), ' ') 
                  << std::setw(50) << color_text(task.description, TaskManager::TEXT_COLOR)
                  << (task.completed ? color_text("Completed", TaskManager::TEXT_COLOR) : color_text("Pending", TaskManager::TEXT_COLOR)) << std::endl;
    }
}

void TaskManager::completeTask(int id) {
    for (int i = 1; i < 13; i ++){
        if (! tasks[i].empty()){
            for (auto& task : tasks[i]) {
                if (task.id == id) {
                    task.completed = true;
                    saveTasks();
                    std::cout << color_text("Task ", TaskManager::TEXT_COLOR) << id << color_text(" marked as completed.", TaskManager::TEXT_COLOR) << std::endl;
                    return;
                }
            }
        }
    }
    for (int i = 0; i < static_cast<int>(taskList.size()); i ++){
        if (taskList[i].id == id){
                taskList[i].completed = true;
                saveTasks();
                std::cout << color_text("Task ", TaskManager::TEXT_COLOR) << id << color_text(" marked as completed.", TaskManager::TEXT_COLOR) << std::endl;
                return;
        }
    }
    std::cout << color_text("Task with ID ", TaskManager::TEXT_COLOR) << id << color_text(" not found.", TaskManager::TEXT_COLOR) << std::endl;
}

void TaskManager::deleteTask(int id) {
    for (int i = 1; i < 13; i++) {
        if (!tasks[i].empty()) {
            for (auto it = tasks[i].begin(); it != tasks[i].end(); ++it) {
                if (it->id == id) {
                    tasks[i].erase(it);
                    saveTasks();
                    std::cout << color_text("Task ", TaskManager::TEXT_COLOR) << id 
                              << color_text(" deleted.", TaskManager::TEXT_COLOR) << std::endl;
                    break;
                }
            }
        }
    }
    for (auto it = taskList.begin(); it != taskList.end(); ++it) {
        if (it->id == id) {
            taskList.erase(it);
            return;
        }
    }
    std::cout << color_text("Task with ID ", TaskManager::TEXT_COLOR) << id 
              << color_text(" not found.", TaskManager::TEXT_COLOR) << std::endl;
}

void TaskManager::clearTasks(){
    tasks.clear();
    taskList.clear();
    saveTasks();
    return;
}

void TaskManager::help() {
    std::cout << std::endl << color_text("Task Manager - General Commands:", TaskManager::TEXT_COLOR) << std::endl << std::endl;
    std::cout << color_text("  nt <description> [deadline]       - Add a new task with optional deadline (YYYY-MM-DD [HH:MM])", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  ls                                - List all pending tasks", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  lsa                               - List all tasks including completed ones", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  ft <id>                           - Mark a task as completed", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  dt <id>                           - Delete a task", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  ct                                - Clear all tasks", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  h                                 - Show this help message", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  exit                              - Exit the program", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  c                                 - Display calendar for current month", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  n                                 - Display calendar for next month", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  p                                 - Display calendar for previous month", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  dc <Month name or number (1-12)>  - Display calendar for specified month", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << std::endl << color_text("Task Manager - User-Specific Commands:", TaskManager::TEXT_COLOR) << std::endl << std::endl;
    std::cout << color_text("  fetch                             - Get your current configurations", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  sh <New cell height (5-10)>       - Set a new height for calendar cells", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  sw <New cell width (12-40)>       - Set a new width for calendar cells", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  t                                 - Toggle whether your calendar app is opened upon adding a new task", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  stc                               - Change the text color", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  scc                               - Change the calendar border color", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  sec                               - Change the color of events on the calendar", TaskManager::TEXT_COLOR) <<std::endl;
    std::cout << color_text("  stb                               - Change whether the text appears bold", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  scb                               - Change whether the calendar borders appear bold", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("  sort                              - Configure how the events are sorted upon listed", TaskManager::TEXT_COLOR) << std::endl;
}

void TaskManager::sortByID(){
    for (int i = 0; i < static_cast<int>(taskList.size()) - 1; ++i) {
        for (int j = 0; j < static_cast<int>(taskList.size()) - i - 1; ++j) {
            if (taskList[j].id > taskList[j + 1].id) {
                std::swap(taskList[j], taskList[j + 1]);
            }
        }
    }
    std::string executableDirectory = getExecutableDirectory();
    std::ifstream file(executableDirectory + "/config.json");
    if (!file){
        std::cerr << color_text("Could not open the config file", TaskManager::TEXT_COLOR) << std::endl;
    }
    file >> TaskManager::configFile;
    TaskManager::configFile["EVENT_SORT"] = "ID";
    std::ofstream outFile(executableDirectory + "/config.json");
    if (outFile.is_open()) {
        outFile << TaskManager::configFile.dump(4);
        outFile.close();
    }
    TaskManager::SORT_METHOD = "By ID";
}

void TaskManager::sortByDeadlineAscending(){
    for (int i = 0; i < static_cast<int>(taskList.size()) - 1; ++i) {
        for (int j = 0; j < static_cast<int>(taskList.size()) - i - 1; ++j) {
            if (taskList[j].year > taskList[j + 1].year){
                std::swap(taskList[j], taskList[j+1]);
            }
            else if (taskList[j].year == taskList[j + 1].year){
                if (taskList[j].month > taskList[j + 1].month) {
                    std::swap(taskList[j], taskList[j + 1]);
                }
                else if (taskList[j].month == taskList[j + 1].month){
                    if (taskList[j].day > taskList[j + 1].day){
                        std::swap(taskList[j], taskList[j+1]);
                    }
                }
            }
        }
    }
    std::string executableDirectory = getExecutableDirectory();
    std::ifstream file(executableDirectory + "/config.json");
    if (!file){
        std::cerr << color_text("Could not open the config file", TaskManager::TEXT_COLOR) << std::endl;
    }
    file >> TaskManager::configFile;
    TaskManager::configFile["EVENT_SORT"] = "ASCENDING";
    std::ofstream outFile(executableDirectory + "/config.json");
    if (outFile.is_open()) {
        outFile << TaskManager::configFile.dump(4);
        outFile.close();
    }
    TaskManager::SORT_METHOD = "By closest";
}
void TaskManager::sortByDeadlineDescending(){
    for (int i = 0; i < static_cast<int>(taskList.size()) - 1; ++i) {
        for (int j = 0; j < static_cast<int>(taskList.size()) - i - 1; ++j) {
            if (taskList[j].year < taskList[j + 1].year){
                std::swap(taskList[j], taskList[j+1]);
            }
            else if (taskList[j].year == taskList[j + 1].year){
                if (taskList[j].month < taskList[j + 1].month) {
                    std::swap(taskList[j], taskList[j + 1]);
                }
                else if (taskList[j].month == taskList[j + 1].month){
                    if (taskList[j].day < taskList[j + 1].day){
                        std::swap(taskList[j], taskList[j+1]);
                    }
                }
            }
        }
    }
    std::string executableDirectory = getExecutableDirectory();
    std::ifstream file(executableDirectory + "/config.json");
    if (!file){
        std::cerr << color_text("Could not open the config file", TaskManager::TEXT_COLOR) << std::endl;
    }
    file >> TaskManager::configFile;
    TaskManager::configFile["EVENT_SORT"] = "DESCENDING";
    std::ofstream outFile(executableDirectory + "/config.json");
    if (outFile.is_open()) {
        outFile << TaskManager::configFile.dump(4);
        outFile.close();
    }
    TaskManager::SORT_METHOD = "By furthest";
}

int TaskManager::getCalendarCellWidth(){
    return TaskManager::CELL_WIDTH;
}

int TaskManager::getCalendarCellHeight(){
    return TaskManager::CELL_HEIGHT;
}

int TaskManager::getICSVal(){
    return TaskManager::ICS_VALUE;
}

std::vector<Task> TaskManager::getMonthTask(int month){
    return tasks[month];
}

int TaskManager::getDayOfTask(std::string& deadline){
    int day = (deadline[8] - '0')*10 + (deadline[9] - '0');
    return day;
}

int TaskManager::getMonthOfTask(std::string& deadline){
    int month = (deadline[5] - '0')*10 + (deadline[6] - '0');
    return month;
}

int TaskManager::getYearOfTask(std::string& deadline){
    int year = (deadline[0] - '0')*1000 + (deadline[1] - '0')*100 + (deadline[2] - '0')*10 + (deadline[3] - '0');
    return year;
}

std::string TaskManager::getCalendarBorderColor(){
    return TaskManager::CALENDAR_BORDER_COLOR;
}

std::string TaskManager::getTextColor(){
    return TaskManager::TEXT_COLOR;
}

std::string TaskManager::getEventsColor(){
    return TaskManager::EVENTS_COLOR;
}

int TaskManager::getCalendarBorderBold(){
    return TaskManager::CALENDAR_BORDER_BOLD;
}

int TaskManager::getTextBold(){
    return TaskManager::TEXT_BOLD;
}

void TaskManager::setCalendarCellWidth(int newWidth){
    try {
        std::string executableDirectory = getExecutableDirectory();
        std::ifstream file(executableDirectory + "/config.json");
        if (!file){
            std::cerr << color_text("Could not open the config file", TaskManager::TEXT_COLOR) << std::endl;
        }
        file >> TaskManager::configFile;
        TaskManager::configFile["CELL_WIDTH"] = newWidth;
        std::ofstream outFile(executableDirectory + "/config.json");
        if (outFile.is_open()) {
            outFile << TaskManager::configFile.dump(4);
            outFile.close();
        }
        TaskManager::CELL_WIDTH = newWidth;
    } catch (const json::exception& e) {
        std::cerr << color_text("Error parsing config.json: ", TaskManager::TEXT_COLOR) << e.what() << std::endl;
    }
}

int TaskManager::getEventDisplay(){
    return TaskManager::EVENT_DISPLAY;
}

void TaskManager::setCalendarCellHeight(int newHeight){
    try {
        std::string executableDirectory = getExecutableDirectory();
        std::ifstream file(executableDirectory + "/config.json");
        if (!file){
            std::cerr << color_text("Could not open the config file", TaskManager::TEXT_COLOR) << std::endl;
        }
        file >> TaskManager::configFile;
        TaskManager::configFile["CELL_HEIGHT"] = newHeight;
        std::ofstream outFile(executableDirectory + "/config.json");
        if (outFile.is_open()) {
            outFile << TaskManager::configFile.dump(4);
            outFile.close();
        }
        TaskManager::CELL_HEIGHT = newHeight;
    } catch (const json::exception& e) {
        std::cerr << color_text("Error parsing config.json: ", TaskManager::TEXT_COLOR) << e.what() << std::endl;
    }
}

void TaskManager::toggleICS(){
    try {
        std::string executableDirectory = getExecutableDirectory();
        std::ifstream file(executableDirectory + "/config.json");
        if (!file){
            std::cerr << color_text("Could not open the config file", TaskManager::TEXT_COLOR) << std::endl;
        }
        file >> TaskManager::configFile;
        int newVal;
        if (TaskManager::configFile["ICS_VALUE"] == 1){
            newVal = 0;
            TaskManager::configFile["ICS_VALUE"] = newVal;
        }
        else{
            newVal = 1;
            TaskManager::configFile["ICS_VALUE"] = newVal;
        }
        std::ofstream outFile(executableDirectory + "/config.json");
        if (outFile.is_open()) {
            outFile << TaskManager::configFile.dump(4);
            outFile.close();
        }
        TaskManager::ICS_VALUE = newVal;
    } catch (const json::exception& e) {
        std::cerr << color_text("Error parsing config.json: ", TaskManager::TEXT_COLOR) << e.what() << std::endl;
    }
}

void TaskManager::setCalendarBorderColor(std::string color){
    try {
        auto it = colorCodes.find(color);
        if (it == colorCodes.end()){
            std::cerr << color_text("Unknown color was selected", TaskManager::TEXT_COLOR) << std::endl;
            return ;
        }
        std::string executableDirectory = getExecutableDirectory();
        std::ifstream file(executableDirectory + "/config.json");
        if (!file){
            std::cerr << color_text("Could not open the config file", TaskManager::TEXT_COLOR) << std::endl;
        }
        file >> TaskManager::configFile;
        TaskManager::configFile["CALENDAR_BORDER_COLOR"] = color;
        std::ofstream outFile(executableDirectory + "/config.json");
        if (outFile.is_open()) {
            outFile << TaskManager::configFile.dump(4);
            outFile.close();
        }
        TaskManager::CALENDAR_BORDER_COLOR = color;
    } catch (const json::exception& e) {
        std::cerr << color_text("Error parsing config.json: ", TaskManager::TEXT_COLOR) << e.what() << std::endl;
    }
}

void TaskManager::setTextColor(std::string color){
    try {
        auto it = colorCodes.find(color);
        if (it == colorCodes.end()){
            std::cerr << color_text("Unknown color was selected", TaskManager::TEXT_COLOR) << std::endl;
            return ;
        }
        std::string executableDirectory = getExecutableDirectory();
        std::ifstream file(executableDirectory + "/config.json");
        if (!file){
            std::cerr << color_text("Could not open the config file", TaskManager::TEXT_COLOR) << std::endl;
        }
        file >> TaskManager::configFile;
        TaskManager::configFile["TEXT_COLOR"] = color;
        std::ofstream outFile(executableDirectory + "/config.json");
        if (outFile.is_open()) {
            outFile << TaskManager::configFile.dump(4);
            outFile.close();
        }
        TaskManager::TEXT_COLOR = color;
    } catch (const json::exception& e) {
        std::cerr << color_text("Error parsing config.json: ", TaskManager::TEXT_COLOR) << e.what() << std::endl;
    }
}

void TaskManager::setEventsColor(std::string color){
    try {
        auto it = colorCodes.find(color);
        if (it == colorCodes.end()){
            std::cerr << color_text("Unknown color was selected", TaskManager::TEXT_COLOR) << std::endl;
            return ;
        }
        std::string executableDirectory = getExecutableDirectory();
        std::ifstream file(executableDirectory + "/config.json");
        if (!file){
            std::cerr << color_text("Could not open the config file", TaskManager::TEXT_COLOR) << std::endl;
        }
        file >> TaskManager::configFile;
        TaskManager::configFile["EVENTS_COLOR"] = color;
        std::ofstream outFile(executableDirectory + "/config.json");
        if (outFile.is_open()) {
            outFile << TaskManager::configFile.dump(4);
            outFile.close();
        }
        TaskManager::EVENTS_COLOR = color;
    } catch (const json::exception& e) {
        std::cerr << color_text("Error parsing config.json: ", TaskManager::TEXT_COLOR) << e.what() << std::endl;
    }
}

void TaskManager::toggleCalendarBorderBold(){
    try {
        std::string executableDirectory = getExecutableDirectory();
        std::ifstream file(executableDirectory + "/config.json");
        if (!file){
            std::cerr << color_text("Could not open the config file", TaskManager::TEXT_COLOR) << std::endl;
        }
        file >> TaskManager::configFile;
        int newVal;
        if (TaskManager::configFile["CALENDAR_BORDER_BOLD"] == 1){
            newVal = 0;
            TaskManager::configFile["CALENDAR_BORDER_BOLD"] = newVal;
        }
        else{
            newVal = 1;
            TaskManager::configFile["CALENDAR_BORDER_BOLD"] = newVal;
        }
        std::ofstream outFile(executableDirectory + "/config.json");
        if (outFile.is_open()) {
            outFile << TaskManager::configFile.dump(4);
            outFile.close();
        }
        TaskManager::CALENDAR_BORDER_BOLD = newVal;
    } catch (const json::exception& e) {
        std::cerr << color_text("Error parsing config.json: ", TaskManager::TEXT_COLOR) << e.what() << std::endl;
    }
}

void TaskManager::toggleTextBold(){
    try {
        std::string executableDirectory = getExecutableDirectory();
        std::ifstream file(executableDirectory + "/config.json");
        if (!file){
            std::cerr << color_text("Could not open the config file", TaskManager::TEXT_COLOR) << std::endl;
        }
        file >> TaskManager::configFile;
        int newVal;
        if (TaskManager::configFile["TEXT_BOLD"] == 1){
            newVal = 0;
            TaskManager::configFile["TEXT_BOLD"] = newVal;
        }
        else{
            newVal = 1;
            TaskManager::configFile["TEXT_BOLD"] = newVal;
        }
        std::ofstream outFile(executableDirectory + "/config.json");
        if (outFile.is_open()) {
            outFile << TaskManager::configFile.dump(4);
            outFile.close();
        }
        TaskManager::TEXT_BOLD = newVal;
    } catch (const json::exception& e) {
        std::cerr << color_text("Error parsing config.json: ", TaskManager::TEXT_COLOR) << e.what() << std::endl;
    }
}

void TaskManager::toggleEventDisplay(){
    try {
        std::string executableDirectory = getExecutableDirectory();
        std::ifstream file(executableDirectory + "/config.json");
        if (!file){
            std::cerr << color_text("Could not open the config file", TaskManager::TEXT_COLOR) << std::endl;
        }
        file >> TaskManager::configFile;
        int newVal;
        if (TaskManager::configFile["EVENT_DISPLAY"] == 1){
            newVal = 0;
            TaskManager::configFile["EVENT_DISPLAY"] = newVal;
        }
        else{
            newVal = 1;
            TaskManager::configFile["EVENT_DISPLAY"] = newVal;
        }
        std::ofstream outFile(executableDirectory + "/config.json");
        if (outFile.is_open()) {
            outFile << TaskManager::configFile.dump(4);
            outFile.close();
        }
        TaskManager::EVENT_DISPLAY = newVal;
    } catch (const json::exception& e) {
        std::cerr << color_text("Error parsing config.json: ", TaskManager::TEXT_COLOR) << e.what() << std::endl;
    }
};

void TaskManager::displayCalendar(int month, bool useStaticDisplay) {
    if (month < 1) {
        std::cout << color_text("Invalid month. Please enter a value between 1 and 12.\n", TaskManager::TEXT_COLOR);
        return;
    }

    // Static variable to track if we've displayed a calendar before
    static bool firstCalendarDisplay = true;
    static int lastCalendarHeight = 0;

    int newYear;
    
    if (month > 12){
        newYear = static_cast<int>(month/12);
        month = month % 12;
    }
    else {
        newYear = 0;
    }

    time_t now = time(0);
    tm *ltm = localtime(&now);
    int year = 1900 + ltm->tm_year + newYear;

    tm firstDay = {};
    firstDay.tm_mday = 1;
    firstDay.tm_mon = month - 1;
    firstDay.tm_year = year - 1900;
    mktime(&firstDay);
    int startWeekday = firstDay.tm_wday;

    int daysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    
    // Pre-calculate calendar grid to determine if there's a fifth week
    std::vector<int> calendarGrid(42, 0);
    for (int i = 0; i < daysInMonth[month - 1]; ++i) {
        calendarGrid[startWeekday + i] = i + 1;
    }
    bool hasFifthWeek = (calendarGrid[35] != 0);

    // Calculate actual calendar height for this display
    int actualCalendarHeight = 8; // Headers and borders
    if (hasFifthWeek) {
        actualCalendarHeight += 6 * (CELL_HEIGHT + 1); // 6 weeks
    } else {
        actualCalendarHeight += 5 * (CELL_HEIGHT + 1); // 5 weeks
    }

    // For static display mode (used with c/n/p commands), move cursor up to overwrite previous calendar
    if (useStaticDisplay && !firstCalendarDisplay) {
        // Move cursor up by the number of lines from the last calendar display
        std::cout << "\033[" << lastCalendarHeight << "A";
        // Clear from cursor to end of screen to remove any leftover content
        clearFromCursor();
    }

    // Mark that we've displayed at least one calendar
    if (useStaticDisplay) {
        firstCalendarDisplay = false;
        lastCalendarHeight = actualCalendarHeight;
    }

    // Now display the actual calendar content
    printYearAndMonth(year, month);
    std::cout << color_text(std::string(TaskManager::getCalendarCellWidth() * 7, '*'), TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD);
    std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD) << std::endl << " ";
    
    std::string weekDaysAbr[] = {"Su", "Mo", "Tu", "We", "Th", "Fr"};
    for (int i = 0; i < 6; i ++){
        std::cout << color_text(weekDaysAbr[i], TaskManager::TEXT_COLOR);
        for (int j = 0; j < TaskManager::getCalendarCellWidth() - 2; j ++){
            std::cout << " ";
        }
    }
    std::cout << color_text("Sa\n", TaskManager::TEXT_COLOR);

    // Rest of the calendar display logic remains the same
    for (int week = 0; week < 6; ++week) {
        if (week == 5 && !hasFifthWeek){
            continue;
        }
        std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD);
        for (int day = 0; day < 7; ++day) {
            std::cout << color_text(std::string(TaskManager::getCalendarCellWidth() - 1, '*'), TaskManager::CALENDAR_BORDER_COLOR);
            if (day < 6) std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR);
        }
        std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD);
        std::cout << "\n";

        for (int day = 0; day < 7; ++day) {
            int idx = week * 7 + day;
            int dayNumber = calendarGrid[idx];
            
            std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD);
            if (dayNumber != 0) {
                std::cout << " " << std::setw(2) << color_text(std::to_string(dayNumber), TaskManager::TEXT_COLOR);
                for (int i = 0; i < TaskManager::getCalendarCellWidth() - 2 - static_cast<int>(std::to_string(dayNumber).size()); i ++){
                    std::cout << " ";
                }
            } else {
                std::cout << std::string(TaskManager::getCalendarCellWidth() - 1, ' ');
            }
        }
        std::cout << color_text("*\n", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD);
        
        if (TaskManager::EVENT_DISPLAY == 0){
            for (int row = 0; row < TaskManager::getCalendarCellHeight() - 3; ++row) {
                for (int day = 0; day < 7; ++day) {
                    std::vector<Task> events = tasks[month];
                    std::vector<Task> eventsForTheDay;
                    if (row == 1){
                        int idx = week * 7 + day;
                        int dayNumber = calendarGrid[idx];
                        if (!events.empty()){
                            for (int i = 0; i < static_cast<int>(events.size()); i ++){
                                if (events[i].day == dayNumber){
                                    eventsForTheDay.push_back(events[i]);
                                }
                            }
                        }
                    }
                    int numberOfEvents = static_cast<int>(eventsForTheDay.size());
                    if (numberOfEvents > 0){
                        std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD) << color_text("📌 Events: ", TaskManager::EVENTS_COLOR) << color_text(std::to_string(numberOfEvents), TaskManager::EVENTS_COLOR) << color_text(std::string(TaskManager::getCalendarCellWidth() - 12 - std::to_string(numberOfEvents).length(), ' '), TaskManager::EVENTS_COLOR); 
                    } else {
                        std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD) << std::string(TaskManager::getCalendarCellWidth() - 1, ' ');
                    }
                }
                std::cout << color_text("*\n", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD);
            }
        }
        else {
            for (int row = 0; row < TaskManager::getCalendarCellHeight() - 3; ++row) {
                for (int day = 0; day < 7; ++day) {
                    std::vector<Task> events = tasks[month];
                    std::vector<Task> eventsForTheDay;
                    
                    int idx = week * 7 + day;
                    int dayNumber = calendarGrid[idx];
                    if (!events.empty()){
                        for (int i = 0; i < static_cast<int>(events.size()); i ++){
                            if (events[i].day == dayNumber && events[i].year == year){
                                eventsForTheDay.push_back(events[i]);
                            }
                        }
                    }
                    
                    int numberOfEvents = static_cast<int>(eventsForTheDay.size());
                    int cellWidth = TaskManager::getCalendarCellWidth();
                    int maxCellHeight = TaskManager::getCalendarCellHeight() - 3;
                    
                    if (numberOfEvents > 0 && row >= 1 && (row) <= numberOfEvents && row <= TaskManager::getCalendarCellHeight() - 4) {
                        if (row == TaskManager::getCalendarCellHeight() - 4 && numberOfEvents > row){
                            std::string moreText = "(...)";
                            int padding = cellWidth - 1 - static_cast<int>(moreText.length());
                            std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD) 
                                      << color_text(moreText, TaskManager::EVENTS_COLOR) 
                                      << std::string(padding, ' ');
                        }
                        else{
                            std::string description = "📌 " + eventsForTheDay[row - 1].description;
                            int maxDescLength = cellWidth - 2;
                            
                            if (static_cast<int>(description.length()) > maxDescLength) {
                                description = description.substr(0, maxDescLength - 3) + "...";
                            }
                            
                            int padding = cellWidth + 1 - static_cast<int>(description.length());
                            std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD) 
                                      << color_text(description, TaskManager::EVENTS_COLOR) 
                                      << std::string(padding, ' ');
                        }
                    }
                    else if (numberOfEvents > maxCellHeight && row == maxCellHeight - 1) {
                        std::string moreText = "(...)";
                        int padding = cellWidth - 1 - static_cast<int>(moreText.length());
                        std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD) 
                                  << color_text(moreText, TaskManager::EVENTS_COLOR) 
                                  << std::string(padding, ' ');
                    }
                    else {
                        std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD) 
                                  << std::string(cellWidth - 1, ' ');
                    }
                }
                std::cout << color_text("*\n", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD);
            }
        }
        if ((week == 4 && !hasFifthWeek) || (week == 5 && hasFifthWeek)) {
            std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD);
            for (int day = 0; day < 7; ++day) {
                std::cout << color_text(std::string(TaskManager::getCalendarCellWidth() - 1, '*'), TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD);
                if (day < 6) std::cout << color_text("*", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD);
            }
            std::cout << color_text("*\n", TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_BOLD);
        }
    }
    
    // No cursor restoration needed for inline display
}

void TaskManager::displayCalendar(const std::string& monthName) {
    std::map<std::string, int> monthMap = {
        {"january", 1}, {"february", 2}, {"march", 3}, {"april", 4},
        {"may", 5}, {"june", 6}, {"july", 7}, {"august", 8},
        {"september", 9}, {"october", 10}, {"november", 11}, {"december", 12}
    };

    std::string lowerMonth = monthName;
    std::transform(lowerMonth.begin(), lowerMonth.end(), lowerMonth.begin(), ::tolower);

    if (monthMap.find(lowerMonth) == monthMap.end()) {
        std::cout << "Invalid month name: " << monthName << "\n";
        return;
    }

    int month = monthMap[lowerMonth];
    // Call the int version with static display disabled (for string version)
    displayCalendar(month, false);
}

void TaskManager::displaySummary(){
    std::cout << color_text("╔════════════════════════════════════════════════════════════════════╗", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("║  ████████╗███████╗██████╗ ███╗   ███╗██╗███╗   ██╗ █████╗ ██╗      ║", TaskManager::TEXT_COLOR) << "    " << color_text("Welcome to Terminal Calendar!", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("║  ╚══██╔══╝██╔════╝██╔══██╗████╗ ████║██║████╗  ██║██╔══██╗██║      ║", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("║     ██║   █████╗  ██████╔╝██╔████╔██║██║██╔██╗ ██║███████║██║      ║", TaskManager::TEXT_COLOR) << "    " << color_text("Your current configurations: ", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("║     ██║   ██╔══╝  ██╔══██╗██║╚██╔╝██║██║██║╚██╗██║██╔══██║██║      ║", TaskManager::TEXT_COLOR) << "    " << color_text("Grid cell width: ", TaskManager::TEXT_COLOR) << color_text(std::to_string(TaskManager::CELL_WIDTH), TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("║     ██║   ███████╗██║  ██║██║ ╚═╝ ██║██║██║ ╚████║██║  ██║███████╗ ║", TaskManager::TEXT_COLOR) << "    " << color_text("Grid cell height: ", TaskManager::TEXT_COLOR) << color_text(std::to_string(TaskManager::CELL_HEIGHT), TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("║     ╚═╝   ╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝╚══════╝ ║", TaskManager::TEXT_COLOR) << "    " << color_text("Calendar Border Color: ", TaskManager::CALENDAR_BORDER_COLOR) << color_text(TaskManager::CALENDAR_BORDER_COLOR, TaskManager::CALENDAR_BORDER_COLOR) << std::endl;
    std::cout << color_text("║                                                                    ║", TaskManager::TEXT_COLOR) << "    " << color_text("Text Color: ", TaskManager::TEXT_COLOR) << color_text(TaskManager::TEXT_COLOR, TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("║  ██████╗ █████╗ ██╗     ███████╗███╗   ██╗██████╗  █████╗ ██████╗  ║", TaskManager::TEXT_COLOR) << "    " << color_text("Calendar Events Color: ", TaskManager::EVENTS_COLOR) << color_text(TaskManager::EVENTS_COLOR, TaskManager::EVENTS_COLOR) << std::endl;
    std::cout << color_text("║ ██╔════╝██╔══██╗██║     ██╔════╝████╗  ██║██╔══██╗██╔══██╗██╔══██╗ ║", TaskManager::TEXT_COLOR) << "    " << color_text("Bold Calendar Borders: ", TaskManager::TEXT_COLOR) << color_text(TaskManager::CALENDAR_BORDER_BOLD == 1 ? ("True") : ("False"), TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("║ ██║     ███████║██║     █████╗  ██╔██╗ ██║██║  ██║███████║██████╔╝ ║", TaskManager::TEXT_COLOR) << "    " << color_text("Bold Text: ", TaskManager::TEXT_COLOR) << color_text(TaskManager::TEXT_BOLD == 1 ? ("True") : ("False"), TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("║ ██║     ██╔══██║██║     ██╔══╝  ██║╚██╗██║██║  ██║██╔══██║██╔══██╗ ║", TaskManager::TEXT_COLOR) << "    " << color_text("ICS Enabled: ", TaskManager::TEXT_COLOR) << color_text(TaskManager::ICS_VALUE == 1 ? ("True") : ("False"), TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("║ ╚██████╗██║  ██║███████╗███████╗██║ ╚████║██████╔╝██║  ██║██║  ██║ ║", TaskManager::TEXT_COLOR) << "    " << color_text("Events Sorting Method: ", TaskManager::TEXT_COLOR) << color_text(TaskManager::SORT_METHOD, TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("║  ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ║", TaskManager::TEXT_COLOR) << std::endl;
    std::cout << color_text("╚════════════════════════════════════════════════════════════════════╝", TaskManager::TEXT_COLOR) << std::endl;
}
