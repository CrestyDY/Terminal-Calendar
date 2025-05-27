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
    int year;
};

class TaskManager {
private:
    std::map<int, std::vector<Task>> tasks;
    std::vector<Task> taskList;
    std::string filename;
    int nextId;
    static int CELL_WIDTH;
    static int CELL_HEIGHT;
    static int ICS_VALUE;
    static std::string CALENDAR_BORDER_COLOR;
    static std::string TEXT_COLOR;
    static std::string EVENTS_COLOR;
    static std::string SORT_METHOD;
    static int CALENDAR_BORDER_BOLD;
    static int TEXT_BOLD;
    static json configFile;
    void loadTasks();
    void saveTasks();
    void loadConfigs();
    std::string getCurrentDateTime();
    std::string getExecutableDirectory();
    bool isValidDateTime(const std::string& dateTime);
    
public:
    TaskManager(const std::string& file);
    std::string color_text(const std::string& text, const std::string& color, const int bold = TaskManager::TEXT_BOLD);
    void addTask(const std::string& description, const std::string& deadline);
    void listTasks(bool all = true);
    void completeTask(int id);
    void deleteTask(int id);
    void clearTasks();
    void help();

    // Getters 
    static int getCalendarCellWidth();
    static int getCalendarCellHeight();
    static int getICSVal();
    static std::string getCalendarBorderColor();
    static int getCalendarBorderBold();
    static std::string getTextColor();
    static std::string getEventsColor();
    static int getTextBold();
    std::vector<Task> getMonthTask(int month);
    int getMonthOfTask(std::string& deadline);
    int getDayOfTask(std::string& deadline);
    int getYearOfTask(std::string& deadline);
    
    // Setters
    void setCalendarCellWidth(int newWidth);
    void setCalendarCellHeight(int newHeight);
    void toggleICS();
    void setCalendarBorderColor(std::string color);
    void setTextColor(std::string color);
    void setEventsColor(std::string color);
    void toggleCalendarBorderBold();
    void toggleTextBold();
    
    void displayCalendar(int month);
    void displayCalendar(const std::string& month);
    void displaySummary();
    void sortByID();
    void sortByDeadlineAscending();
    void sortByDeadlineDescending();
};

#endif 
