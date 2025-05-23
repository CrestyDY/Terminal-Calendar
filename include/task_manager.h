#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Task {
    int id;
    std::string description;
    std::string deadline;           
    bool completed;
    int month;
    int day;
};

class TaskManager {
private:
    std::map<int, std::vector<Task>> tasks;
    std::string filename;
    int nextId;
    static int CELL_WIDTH;
    static int CELL_HEIGHT;
    static int ICS_VALUE;
    static json configFile;
    void loadTasks();
    void saveTasks();
    void loadConfigs();
    std::string getCurrentDateTime();
    std::string getExecutableDirectory();
    bool isValidDateTime(const std::string& dateTime);
    
public:
    TaskManager(const std::string& file);
    void addTask(const std::string& description, const std::string& deadline);
    void listTasks(bool all = true);
    void completeTask(int id);
    void deleteTask(int id);
    void clearTasks();
    void printTasks();
    void help();
    static int getCalendarCellWidth();
    static int getCalendarCellHeight();
    static int getICSVal();
    std::vector<Task> getMonthTask(int month);
    int getMonthOfTask(std::string& deadline);
    int getDayOfTask(std::string& deadline);
    void setCalendarCellWidth(int newWidth);
    void setCalendarCellHeight(int newHeight);
    void toggleICS();
    void displayCalendar(int month);
    void displayCalendar(const std::string& month);
    void sortByID(std::vector<int>& arr);
    void sortByDeadlineAscending(std::vector<int>& arr);
    void sortByDeadlineDescending(std::vector<int>& arr);
};

#endif 
