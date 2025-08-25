#ifndef CAL_DAV_ACCOUNT_H
#define CAL_DAV_ACCOUNT_H

#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class CalDavAccount {
private:
    std::string username;
    std::string appPassword;
    std::string serverUrl;
    static json configFile;
    int authenticated = false;
    void loadConfigs();
    std::string getExecutableDirectory();

public:
    CalDavAccount();

    // Setters
    void setUsername(const std::string& user);
    void setAppPassword(const std::string& password);
    void setServerUrl(const std::string& url);

    // Getters
    std::string getUsername() const;
    std::string getAppPassword() const;
    std::string getServerUrl() const;
    int isAuthenticated() const;
    void getEvents();
    void getEvents(int time);
    // Authentication logic placeholder
    bool authenticate();
};
#endif 
