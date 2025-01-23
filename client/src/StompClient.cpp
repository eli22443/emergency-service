#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <atomic>
#include <vector>
#include <map>
#include <queue>
#include "../include/ConnectionHandler.h"
#include "../include/event.h"

class StompClient {
private:
    std::atomic<bool> shouldTerminate{false};
    std::atomic<int> subId{0};
    std::atomic<int> receiptId{0};
    ConnectionHandler* connectionHandler{nullptr};
    std::string currentUser;
    std::map<std::string, int> subscriptionIds;  
    std::map<std::string, std::map<std:: string, std::vector<Event>>> userEvents; 

    std::string getNextMessageId() {
        return std::to_string(subId++);
    }

    bool isLoggedIn() const {
        return connectionHandler != nullptr && !currentUser.empty();
    }

    void processCommand(const std::string& command) {
        if(command.empty()) return;
        
        std::string cmd = command.substr(0, command.find(' '));
        
        if(cmd == "login") {
            handleLogin(command);
        }
        else if(!isLoggedIn()) {
            std::cout << "Client is not logged in" << std::endl;
            return;
        }
        else if(cmd == "join") {
            handleJoin(command);
        }
        else if(cmd == "exit") {
            handleExit(command);
        }
        else if(cmd == "report") {
            handleReport(command);
        }
        else if(cmd == "summary") {
            handleSummary(command);
        }
        else if(cmd == "logout") {
            handleLogout();
        }
    }

    void handleLogin(const std::string& command) {
        if(isLoggedIn()) {
            std::cout << "The client is already logged in, log out before trying again" << std::endl;
            return;
        }

        std::istringstream iss(command);
        std::string cmd, hostPort, username, password;
        iss >> cmd >> hostPort >> username >> password;

        size_t colonPos = hostPort.find(':');
        if(colonPos == std::string::npos) {
            std::cout << "Invalid host:port format" << std::endl;
            return;
        }

        std::string host = hostPort.substr(0, colonPos);
        short port;
        try {
            port = std::stoi(hostPort.substr(colonPos + 1));
        } catch(...) {
            std::cout << "Invalid port number" << std::endl;
            return;
        }

        connectionHandler = new ConnectionHandler(host, port);
        if(!connectionHandler->connect()) {
            std::cout << "Could not connect to server" << std::endl;
            delete connectionHandler;
            connectionHandler = nullptr;
            return;
        }

        std::string frame = "CONNECT\naccept-version:1.2\nhost:stomp.cs.bgu.ac.il\n" 
                           "login:" + username + "\npasscode:" + password + "\n\n^@";
        
        if(!connectionHandler->sendLine(frame)) {
            std::cout << "Error sending login request" << std::endl;
            delete connectionHandler;
            connectionHandler = nullptr;
            return;
        }

        currentUser = username;
    }

    void handleJoin(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd, channel;
        iss >> cmd >> channel;

        int subscriptionId = subId++;
        subscriptionIds[channel] = subscriptionId;

        std::string frame = "SUBSCRIBE\ndestination:/" + channel + 
                           "\nid:" + std::to_string(subscriptionId) + 
                           "\nreceipt:" + std::to_string(receiptId++) + "\n\n^@";

        if(!connectionHandler->sendLine(frame)) {
            std::cout << "Error sending subscribe request" << std::endl;
            return;
        }
        std::cout << "Joined channel " + channel << std::endl;
    }

    void handleExit(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd, channel;
        iss >> cmd >> channel;

        auto it = subscriptionIds.find(channel);
        if(it == subscriptionIds.end()) {
            std::cout << "Not subscribed to channel " << channel << std::endl;
            return;
        }

        std::string frame = "UNSUBSCRIBE\nid:" + std::to_string(it->second) + 
                           "\nreceipt:" + std::to_string(receiptId++) + "\n\n^@";

        if(!connectionHandler->sendLine(frame)) {
            std::cout << "Error sending unsubscribe request" << std::endl;
            return;
        }

        subscriptionIds.erase(it);
        std::cout << "Exited channel " + channel << std::endl;
    }

    void handleReport(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd, filename;
        iss >> cmd >> filename;

        try {
            names_and_events events = parseEventsFile(filename);

            std::sort(events.events.begin(), events.events.end(), [](const Event& a, const Event& b) {
                return a.get_date_time() < b.get_date_time();
            });
            
            for(Event& event : events.events) {
                event.setEventOwnerUser(currentUser);
                std::string frame = createReportFrame(event, event.get_channel_name());
                if(!connectionHandler->sendLine(frame)) {
                    std::cout << "Error sending report" << std::endl;
                    return;
                }
                userEvents[currentUser][event.get_channel_name()].push_back(event);
            }
        } catch(const std::exception& e) {
            std::cout << "Error processing report file: " << e.what() << std::endl;
        }
    }

    std::string createReportFrame(const Event& event, const std::string& channel) {
        std::stringstream frame;
        frame << "SEND\ndestination:/" << channel << "\n"
              << "user: " << currentUser << "\n"
              << "city: " << event.get_city() << "\n"
              << "event name: " << event.get_name() << "\n"
              << "date time: " << event.get_date_time() << "\n"
              << "general information:\n"
              << "  active: " << (event.get_general_information().at("active") == "true") << "\n"
              << "  forces_arrival_at_scene: " << (event.get_general_information().at("forces_arrival_at_scene") == "true") << "\n"
              << "description:" << event.get_description() << "\n\n^@";
        return frame.str();
    }

    void handleSummary(const std::string& command) {
        std::istringstream iss(command);
        std::string cmd, channel, user, filename;
        iss >> cmd >> channel >> user >> filename;

        auto userIt = userEvents.find(user);
        if (userIt == userEvents.end()) {
            std::cout << "No events found for user " << user << std::endl;
            return;
        }

        auto channelIt = userIt->second.find(channel);
        if (channelIt == userIt->second.end()) {
            std::cout << "No events found for channel " << channel << " for user " << user << std::endl;
            return;
        }

        std::ofstream outFile(filename);
        if(!outFile.is_open()) {
            std::cout << "Error opening output file" << std::endl;
            return;
        }

        writeSummary(outFile, channel, channelIt->second);
        outFile.close();
    }

    void writeSummary(std::ofstream& out, const std::string& channel, const std::vector<Event>& events) {
        out << "Channel: " << channel << std::endl;
        
        int totalEvents = events.size();
        int activeEvents = 0;
        int forceArrivals = 0;
        for(const Event& event : events) {
            if(event.get_general_information().at("active") == "true") {
                activeEvents++;
            }
            if(event.get_general_information().at("forces_arrival_at_scene") == "true") {
                forceArrivals++;
            }
        }

        out << "Stats:" << std::endl;
        out << "Total: " << totalEvents << std::endl;
        out << "Active: " << activeEvents << std::endl;
        out << "Forces arrival at scene: " << forceArrivals << std::endl;

        int numReport = 0;
        out << "\nEvent Reports:" << std::endl;
        for(const Event& event : events) {
            out << "Report_" << numReport++ << std::endl;
            out << "   city: " << event.get_city() << std::endl;
            out << "   date time: " << event.get_date_time() << std::endl;
            out << "   event name: " << event.get_name() << std::endl;
            out << "   summery: " << event.get_description() << std::endl;
        }
    }

    void handleLogout() {
        if(!isLoggedIn()) {
            std::cout << "Client is not logged in" << std::endl;
            return;
        }

        int curReceiptId = receiptId++;
        std::string frame = "DISCONNECT\nreceipt:" + std::to_string(curReceiptId) + "\n\n^@";

        if(!connectionHandler->sendLine(frame)) {
            std::cout << "Error sending disconnect request" << std::endl;
            return;
        }

        std::string response;
        if(connectionHandler->getLine(response) && 
           response.find("RECEIPT") != std::string::npos && 
           response.find(curReceiptId) != std::string::npos) {
            
            connectionHandler->close();
            currentUser.clear();
            subscriptionIds.clear();
            std::cout << "Logged out successfully" << std::endl;
        }
    }

    void processServerMessage(const std::string& message) {
        if(message.find("CONNECTED") == 0) {
            std::cout << "Login successful" << std::endl;
        }
        else if(message.find("ERROR") == 0) {
            size_t start = message.find("message: ");
            size_t end = message.find("\n", start);
            std::cout << message.substr(start, end - start) << std::endl;
        }
        else if (message.find("MESSAGE") == 0) {
            Event newEvent = Event(message);
            userEvents[newEvent.getEventOwnerUser()][newEvent.get_channel_name()].push_back(newEvent);
        }
    }

public:
    StompClient() {}

    void start() {
        std::thread keyboardThread([this]() {
            while(!shouldTerminate) {
                std::string command;
                std::getline(std::cin, command);
                processCommand(command);
            }
        });

        std::thread socketThread([this]() {
            while(!shouldTerminate) {
                if(connectionHandler != nullptr) {
                    std::string message;
                    if(connectionHandler->getLine(message)) {
                        processServerMessage(message);
                    }
                }
            }
        });

        keyboardThread.join();
        socketThread.join();
    }

    ~StompClient() {
        shouldTerminate = true;
        if(connectionHandler != nullptr) {
            delete connectionHandler;
        }
    }
};

int main(int argc, char *argv[]) {
    StompClient client;
    client.start();
	return 0;
}
