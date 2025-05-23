#include "../include/task_manager.h"
#include <map>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <string>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


int TaskManager::CELL_WIDTH;
int TaskManager::CELL_HEIGHT;
int TaskManager::ICS_VALUE;
json TaskManager::configFile;

TaskManager::TaskManager(const std::string& file) : filename(file), nextId(1) {
    loadConfigs();
    loadTasks();
    printTasks();
}

void TaskManager::loadConfigs(){
    std::ifstream file("config.json"); 
    if (!file){
        std::cerr << "Could not open the config file" << std::endl;
        return ;
    }
    file >> TaskManager::configFile;
    try {
        this->CELL_WIDTH = configFile.value("CELL_WIDTH", 28);
        this->CELL_HEIGHT = configFile.value("CELL_HEIGHT", 10);
        this->ICS_VALUE = configFile.value("ICS_VALUE", 1);
    } catch (const json::exception& e) {
        std::cerr << "Error parsing config.json: " << e.what() << std::endl;
    }
}
void TaskManager::printTasks(){
    for (int i = 0; i < 12; i ++){
        std::vector<Task> monthTasks = tasks[i+1];
        std::cout << "Tasks for month: " << i + 1 << std::endl;
        for (int j = 0; j < static_cast<int>(monthTasks.size()); j ++){
            std::cout << "ID: " << monthTasks[j].id << " Deadline: " << monthTasks[j].deadline << " Description: " << monthTasks[j].description << " Completed: " << monthTasks[j].completed << std::endl;
        }
    }
}
void TaskManager::loadTasks() {
    std::ifstream inFile(filename);
    if (!inFile) {
        std::cout << "No existing task file found. Creating a new one." << std::endl;
        return;
    }
    
    tasks.clear();
    std::string line;
    
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        Task task;
        char delimiter;
        
        iss >> task.id >> delimiter;
        if (delimiter != '|') continue;
        
        std::getline(iss, task.description, '|');
        std::getline(iss, task.deadline, '|');
        int month = (task.deadline[5]-'0')*10 + (task.deadline[6]-'0');
        
        int completed;
        iss >> completed;
        task.completed = completed == 1;
        
        tasks[month].push_back(task);
        
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
    int month = (task.deadline[5]-'0')*10 + (task.deadline[6]-'0');
    
    task.completed = false;
    
    tasks[month].push_back(task);
    saveTasks();
    
    std::cout << "Task added with ID " << task.id << std::endl;
}

void TaskManager::listTasks(bool all) {
    if (tasks.empty()) {
        std::cout << "No tasks found." << std::endl;
        return;
    }
    
    std::cout << std::left 
              << std::setw(5) << "ID" 
              << std::setw(50) << "Description" 
              << std::setw(20) << "Deadline" 
              << "Status" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    for (int i = 1; i < 13; i ++){
        if (! tasks[i].empty()){
            for (const auto& task : tasks[i]) {
                if (all || !task.completed) {
                    std::cout << std::left 
                              << std::setw(5) << task.id 
                              << std::setw(50) << task.description 
                              << std::setw(20) << task.deadline 
                              << (task.completed ? "Completed" : "Pending") << std::endl;
                }
            }
        }
    }
}

void TaskManager::completeTask(int id) {
    for (int i = 1; i < 13; i ++){
        if (! tasks[i].empty()){
            for (auto& task : tasks[i]) {
                if (task.id == id) {
                    task.completed = true;
                    saveTasks();
                    std::cout << "Task " << id << " marked as completed." << std::endl;
                    return;
                }
            }
            
        }
    }
    std::cout << "Task with ID " << id << " not found." << std::endl;
}

void TaskManager::deleteTask(int id) {
    for (int i = 1; i < 13; i ++){
        if (!tasks[i].empty()){
            for (auto it = tasks[i].begin(); it != tasks[i].end(); ++it) {
                if (it->id == id) {
                    tasks[i].erase(it);
                    saveTasks();
                    std::cout << "Task " << id << " deleted." << std::endl;
                    return;
                }
            }
            
        }
    }
    std::cout << "Task with ID " << id << " not found." << std::endl;
}

void TaskManager::clearTasks(){
    tasks.clear();
    saveTasks();
    return;
}

void TaskManager::help() {
    std::cout << std::endl << "Task Manager - General Commands:" << std::endl << std::endl;
    std::cout << "  nt <description> [deadline]       - Add a new task with optional deadline (YYYY-MM-DD [HH:MM])" << std::endl;
    std::cout << "  ls                                - List all pending tasks" << std::endl;
    std::cout << "  lsa                               - List all tasks including completed ones" << std::endl;
    std::cout << "  ft <id>                           - Mark a task as completed" << std::endl;
    std::cout << "  dt <id>                           - Delete a task" << std::endl;
    std::cout << "  ct                                - Clear all tasks" << std::endl;
    std::cout << "  h                                 - Show this help message" << std::endl;
    std::cout << "  exit                              - Exit the program" << std::endl;
    std::cout << "  c                                 - Display calendar for current month" << std::endl;
    std::cout << "  n                                 - Display calendar for next month" << std::endl;
    std::cout << "  p                                 - Display calendar for previous month" << std::endl;
    std::cout << "  dc <Month name or number (1-12)>  - Display calendar for specified month" << std::endl;
    std::cout << std::endl << "Task Manager - User-Specific Commands:" << std::endl << std::endl;
    std::cout << "  sh <New cell height (5-10)>       - Set a new height for calendar cells" << std::endl;
    std::cout << "  sw <New cell width (10-40)>       - Set a new width for calendar cells" << std::endl;
    std::cout << "  t                                 - Toggle whether your calendar app is opened upon adding a new task" << std::endl;
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

void TaskManager::setCalendarCellWidth(int newWidth){
    try {
        std::ifstream file("config.json");
        if (!file){
            std::cerr << "Could not open the config file" << std::endl;
        }
        file >> TaskManager::configFile;
        TaskManager::configFile["CELL_WIDTH"] = newWidth;
        std::ofstream outFile("config.json");
        if (outFile.is_open()) {
            outFile << TaskManager::configFile.dump(4);
            outFile.close();
        }
        this->CELL_WIDTH = newWidth;
    } catch (const json::exception& e) {
        std::cerr << "Error parsing config.json: " << e.what() << std::endl;
    }
}

void TaskManager::setCalendarCellHeight(int newHeight){
    try {
        std::ifstream file("config.json");
        if (!file){
            std::cerr << "Could not open the config file" << std::endl;
        }
        file >> TaskManager::configFile;
        TaskManager::configFile["CELL_HEIGHT"] = newHeight;
        std::ofstream outFile("config.json");
        if (outFile.is_open()) {
            outFile << TaskManager::configFile.dump(4);
            outFile.close();
        }
        this->CELL_HEIGHT = newHeight;
    } catch (const json::exception& e) {
        std::cerr << "Error parsing config.json: " << e.what() << std::endl;
    }
}
void TaskManager::toggleICS(){
    try {
        std::ifstream file("config.json");
        if (!file){
            std::cerr << "Could not open the config file" << std::endl;
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
        std::ofstream outFile("config.json");
        if (outFile.is_open()) {
            outFile << TaskManager::configFile.dump(4);
            outFile.close();
        }
        this->ICS_VALUE = newVal;
    } catch (const json::exception& e) {
        std::cerr << "Error parsing config.json: " << e.what() << std::endl;
    }
}

void TaskManager::displayCalendar(int month) {
    if (month < 1 || month > 12) {
        std::cout << "Invalid month. Please enter a value between 1 and 12.\n";
        return;
    }

    time_t now = time(0);
    tm *ltm = localtime(&now);
    int year = 1900 + ltm->tm_year;

    tm firstDay = {};
    firstDay.tm_mday = 1;
    firstDay.tm_mon = month - 1;
    firstDay.tm_year = year - 1900;
    mktime(&firstDay);
    int startWeekday = firstDay.tm_wday;

    int daysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    std::string monthNames[] = {
        "January","February","March","April","May","June",
        "July","August","September","October","November","December"
    };

    if (month == 2 &&
        ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
        daysInMonth[1] = 29;
    }
    std::cout << "\n";
    for (int i = 0; i < static_cast<int>((this->getCalendarCellWidth() * 7 - monthNames[month - 1].length() - 5) / 2); i++) {
        std::cout << " ";
    }
    std::cout << monthNames[month - 1] << " " << year;
    for (int i = 0; i < static_cast<int>((this->getCalendarCellWidth() * 7 - monthNames[month - 1].length() - 5) / 2); i++) {
        std::cout << " ";
    }
    std::cout << std::endl;
    for (int i = 0; i < this->getCalendarCellWidth()*7; i ++){
        std::cout << "*";
    }
    std::cout << "*" << std::endl << " ";
    std::string weekDaysAbr[] = {"Su", "Mo", "Tu", "We", "Th", "Fr"};
    for (int i = 0; i < 6; i ++){
        std::cout << weekDaysAbr[i];
        for (int j = 0; j < this->getCalendarCellWidth() - 2; j ++){
            std::cout << " ";
        }
    }
    std::cout << "Sa\n";

    std::vector<int> calendarGrid(42, 0);
    for (int i = 0; i < daysInMonth[month - 1]; ++i) {
        calendarGrid[startWeekday + i] = i + 1;
    }
    bool hasFifthWeek;
    if (calendarGrid[35] == 0){
        hasFifthWeek = false;
    }
    else{
        hasFifthWeek = true;
    }
    for (int week = 0; week < 6; ++week) {
        if (week == 5 && !hasFifthWeek){
            continue;
        }
        std::cout << "*";
        for (int day = 0; day < 7; ++day) {
            std::cout << std::string(this->getCalendarCellWidth() - 1, '*');
            if (day < 6) std::cout << "*";
        }
        std::cout << "*";
        std::cout << "\n";

        for (int day = 0; day < 7; ++day) {
            int idx = week * 7 + day;
            int dayNumber = calendarGrid[idx];
            
            std::cout << "*";
            if (dayNumber != 0) {
                std::cout << " " << std::setw(2) << dayNumber;
                for (int i = 0; i < this->getCalendarCellWidth() - 4; i ++){
                    std::cout << " ";
                }
            } else {
                std::cout << std::string(this->getCalendarCellWidth() - 1, ' ');
            }
        }
        std::cout << "*\n";

        for (int row = 0; row < this->getCalendarCellHeight() - 3; ++row) {
            for (int day = 0; day < 7; ++day) {
                std::cout << "*" << std::string(this->getCalendarCellWidth() - 1, ' ');
            }
            std::cout << "*\n";
        }

        if ((week == 4 && !hasFifthWeek) || (week == 5 && hasFifthWeek)) {
            std::cout << "*";
            for (int day = 0; day < 7; ++day) {
                std::cout << std::string(this->getCalendarCellWidth() - 1, '*');
                if (day < 6) std::cout << "*";
            }
            std::cout << "*\n";
        }
        
    }
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

    time_t now = time(0);
    tm *ltm = localtime(&now);
    int year = 1900 + ltm->tm_year;

    tm firstDay = {};
    firstDay.tm_mday = 1;
    firstDay.tm_mon = month - 1;
    firstDay.tm_year = year - 1900;
    mktime(&firstDay);
    int startWeekday = firstDay.tm_wday;

    int daysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    std::string monthNames[] = {
        "January","February","March","April","May","June",
        "July","August","September","October","November","December"
    };

    if (month == 2 &&
        ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
        daysInMonth[1] = 29;
    }
    std::cout << "\n";
    for (int i = 0; i < static_cast<int>((this->getCalendarCellWidth() * 7 - monthNames[month - 1].length() - 5) / 2); i++) {
        std::cout << " ";
    }
    std::cout << monthNames[month - 1] << " " << year;
    for (int i = 0; i < static_cast<int>((this->getCalendarCellWidth()* 7 - monthNames[month - 1].length() - 5) / 2); i++) {
        std::cout << " ";
    }
    std::cout << std::endl;
    for (int i = 0; i < this->getCalendarCellWidth()*7; i ++){
        std::cout << "*";
    }
    std::cout << "*" << std::endl << " ";
    std::string weekDaysAbr[] = {"Su", "Mo", "Tu", "We", "Th", "Fr"};
    for (int i = 0; i < 6; i ++){
        std::cout << weekDaysAbr[i];
        for (int j = 0; j < this->getCalendarCellWidth() - 2; j ++){
            std::cout << " ";
        }
    }
    std::cout << "Sa\n";

    std::vector<int> calendarGrid(42, 0);
    for (int i = 0; i < daysInMonth[month - 1]; ++i) {
        calendarGrid[startWeekday + i] = i + 1;
    }
    bool hasFifthWeek;
    if (calendarGrid[35] == 0){
        hasFifthWeek = false;
    }
    else{
        hasFifthWeek = true;
    }
    for (int week = 0; week < 6; ++week) {
        if (week == 5 && !hasFifthWeek){
            continue;
        }
        std::cout << "*";
        for (int day = 0; day < 7; ++day) {
            std::cout << std::string(this->getCalendarCellWidth() - 1, '*');
            if (day < 6) std::cout << "*";
        }
        std::cout << "*";
        std::cout << "\n";

        for (int day = 0; day < 7; ++day) {
            int idx = week * 7 + day;
            int dayNumber = calendarGrid[idx];
            
            std::cout << "*";
            if (dayNumber != 0) {
                std::cout << " " << std::setw(2) << dayNumber;
                for (int i = 0; i < this->getCalendarCellWidth() - 4; i ++){
                    std::cout << " ";
                }
            } else {
                std::cout << std::string(this->getCalendarCellWidth() - 1, ' ');
            }
        }
        std::cout << "*\n";

        for (int row = 0; row < this->getCalendarCellHeight() - 3; ++row) {
            for (int day = 0; day < 7; ++day) {
                std::cout << "*" << std::string(this->getCalendarCellWidth() - 1, ' ');
            }
            std::cout << "*\n";
        }

        if ((week == 4 && !hasFifthWeek) || (week == 5 && hasFifthWeek)) {
            std::cout << "*";
            for (int day = 0; day < 7; ++day) {
                std::cout << std::string(this->getCalendarCellWidth() - 1, '*');
                if (day < 6) std::cout << "*";
            }
            std::cout << "*\n";
        }
        
    }
}

