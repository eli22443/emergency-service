#pragma once
#include <string>
#include <iostream>
#include <queue>
#include <map>
#include <vector>
#include <atomic>
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <fstream>
#include <algorithm>
#include "ConnectionHandler.h"  // Include your ConnectionHandler header here
#include "event.h"              // Include your Event header here

class StompProtocol {
private:
    std::atomic<bool> shouldTerminate;
    std::atomic<bool> isConnected;
    std::atomic<int> disconnectReceiptId;
    std::atomic<int> subId;
    std::atomic<int> receiptId;
    ConnectionHandler* connectionHandler;
    std::string currentUser;
    std::map<std::string, int> subscriptionIds;  
    std::map<std::string, std::map<std::string, std::vector<Event>>> userEvents; 
    
public:
    std::string getNextMessageId();
    bool isLoggedIn() const;
    void processCommand(const std::string& command);
    void handleLogin(const std::string& command);
    void handleJoin(const std::string& command);
    void handleExit(const std::string& command);
    void handleReport(const std::string& command);
    std::string createReportFrame(const Event& event, const std::string& channel);
    void handleSummary(const std::string& command);
    void writeSummary(std::ofstream& out, const std::string& channel, const std::vector<Event>& events);
    void handleLogout();
    void processServerMessage(const std::string& message);
    void dissconnect();
    std::string epoch_to_date(std::time_t timestamp);
    StompProtocol();
    ~StompProtocol();
    void start();
};
