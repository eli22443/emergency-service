#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <cstring>
#include "../include/json.hpp"
#include "../include/StompProtocol.h"

StompProtocol::StompProtocol()
    : connectionHandler(nullptr), subId(0), receiptId(0), shouldTerminate(false) {}

std::string StompProtocol::getNextMessageId()
{
    return std::to_string(subId++);
}

bool StompProtocol::isLoggedIn() const
{
    return connectionHandler != nullptr && !currentUser.empty();
}

void StompProtocol::processCommand(const std::string &command)
{
    if (command.empty())
        return;

    std::string cmd = command.substr(0, command.find(' '));

    if (cmd == "login")
    {
        handleLogin(command);
    }
    else if (!StompProtocol::isLoggedIn())
    {
        std::cout << "Client is not logged in" << std::endl;
        return;
    }
    else if (cmd == "join")
    {
        StompProtocol::handleJoin(command);
    }
    else if (cmd == "exit")
    {
        StompProtocol::handleExit(command);
    }
    else if (cmd == "report")
    {
        StompProtocol::handleReport(command);
    }
    else if (cmd == "summary")
    {
        StompProtocol::handleSummary(command);
    }
    else if (cmd == "logout")
    {
        StompProtocol::handleLogout();
    }
    else
    {
        std::cout << "Illegal command, please try a different one" << std::endl;
    }
}

void StompProtocol::handleLogin(const std::string &command)
{
    // if(StompProtocol::isLoggedIn()) {
    //     std::cout << "The client is already logged in, log out before trying again" << std::endl;
    //     return;
    // }

    std::istringstream iss(command);
    std::string cmd, hostPort, username, password;
    iss >> cmd >> hostPort >> username >> password;

    size_t colonPos = hostPort.find(':');

    if(isConnected) {
        std::cout << "user already logged in" << std::endl;
        return;
    }

    if (password.empty())
    {
        std::cout << "login command needs 3 args: {host:port} {username} {password}" << std::endl;
        return;
    }
    if (colonPos == std::string::npos)
    {
        std::cout << "Invalid host:port format" << std::endl;
        return;
    }

    std::string host = hostPort.substr(0, colonPos);
    short port;
    try
    {
        port = std::stoi(hostPort.substr(colonPos + 1));
    }
    catch (...)
    {
        std::cout << "Invalid port number" << std::endl;
        return;
    }

    connectionHandler = new ConnectionHandler(host, port);
    if (!connectionHandler->connect())
    {
        std::cout << "Could not connect to server" << std::endl;
        delete connectionHandler;
        connectionHandler = nullptr;
        return;
    }

    std::string frame = "CONNECT\naccept-version:1.2\nhost:stomp.cs.bgu.ac.il\n"
                        "login:" +
                        username + "\npasscode:" + password + "\n\n" + '\0';

    if (!connectionHandler->sendFrame(frame))
    {
        std::cout << "Error sending login request" << std::endl;
        delete connectionHandler;
        connectionHandler = nullptr;
        return;
    }
    isConnected = true;
    currentUser = username;
}

void StompProtocol::handleJoin(const std::string &command)
{
    std::istringstream iss(command);
    std::string cmd, channel;
    iss >> cmd >> channel;

    if (channel.empty())
    {
        std::cout << "join command needs 1 args: {channel_name}" << std::endl;
        return;
    }

    int subscriptionId = subId++;
    subscriptionIds[channel] = subscriptionId;

    std::string frame = "SUBSCRIBE\ndestination:" + channel +
                        "\nid:" + std::to_string(subscriptionId) +
                        "\nreceipt:" + std::to_string(receiptId++) + "\n\n" + '\0';

    if (!connectionHandler->sendFrame(frame))
    {
        std::cout << "Error sending subscribe request" << std::endl;
        return;
    }
    std::cout << "Joined channel " + channel << std::endl;
}

void StompProtocol::handleExit(const std::string &command)
{
    std::istringstream iss(command);
    std::string cmd, channel;
    iss >> cmd >> channel;

    auto it = subscriptionIds.find(channel);
    if (channel.empty())
    {
        std::cout << "exit command needs 1 args: {channel_name}" << std::endl;
        return;
    }
    if (it == subscriptionIds.end())
    {
        std::cout << "Not subscribed to channel " << channel << std::endl;
        return;
    }

    std::string frame = "UNSUBSCRIBE\nid:" + std::to_string(it->second) +
                        "\nreceipt:" + std::to_string(receiptId++) + "\n\n" + '\0';

    if (!connectionHandler->sendFrame(frame))
    {
        std::cout << "Error sending unsubscribe request" << std::endl;
        return;
    }

    subscriptionIds.erase(it);
    std::cout << "Exited channel " + channel << std::endl;
}

void StompProtocol::handleReport(const std::string &command)
{
    std::istringstream iss(command);
    std::string cmd, filename;
    iss >> cmd >> filename;

    if (filename.empty())
    {
        std::cout << "report command needs 1 args: {file_name}" << std::endl;
        return;
    }

    try
    {
        names_and_events events = parseEventsFile(filename);

        if (subscriptionIds.find(events.channel_name) == subscriptionIds.end())
        {
            std::cout << "You are not registered to channel " << events.channel_name << std::endl;
            return;
        }
        std::sort(events.events.begin(), events.events.end(), [](const Event &a, const Event &b)
                  { return a.get_date_time() < b.get_date_time(); });

        for (Event &event : events.events)
        {
            event.setEventOwnerUser(currentUser);
            std::string frame = createReportFrame(event, event.get_channel_name());
            if (!connectionHandler->sendFrame(frame))
            {
                std::cout << "Error sending report" << std::endl;
                return;
            }
            userEvents[currentUser][event.get_channel_name()].push_back(event);
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "Error processing report file: " << e.what() << std::endl;
    }
}

std::string StompProtocol::createReportFrame(const Event &event, const std::string &channel)
{
    std::stringstream frame;
    frame << "SEND\ndestination:" << channel << "\n\n"
          << "user: " << currentUser << "\n"
          << "city: " << event.get_city() << "\n"
          << "event name: " << event.get_name() << "\n"
          << "date time: " << event.get_date_time() << "\n"
          << "general information:\n"
          << "\tactive: " << (event.get_general_information().at("active"))<< "\n"
          << "\tforces_arrival_at_scene: " << (event.get_general_information().at("forces_arrival_at_scene")) << "\n"
          << "description:" << event.get_description() << "\n"
          << '\0';
    return frame.str();
}

void StompProtocol::handleSummary(const std::string &command)
{
    std::istringstream iss(command);
    std::string cmd, channel, user, filename;
    iss >> cmd >> channel >> user >> filename;

    auto userIt = userEvents.find(user);

    if (filename.empty())
    {
        std::cout << "summary command needs 3 args: {channel_name} {user_name} {file_name}" << std::endl;
        return;
    }

    if(subscriptionIds.find(channel) == subscriptionIds.end()) {
        std::cout << "you are not subscribed to channel " << channel << std::endl;
        return;
    }
    // if (userIt == userEvents.end())
    // {
    //     std::cout << "No events found for user " << user << std::endl;
    //     return;
    // }

    auto channelIt = userIt->second.find(channel);
    // if (channelIt == userIt->second.end())
    // {
    //     std::cout << "No events found for channel " << channel << " for user " << user << std::endl;
    //     return;
    // }

    std::ofstream outFile(filename);
    if (!outFile)
    {
        std::cout << "Error opening output file" << std::endl;
        return;
    }

    writeSummary(outFile, channel, channelIt->second);
    outFile.close();
}

void StompProtocol::writeSummary(std::ofstream &out, const std::string &channel, const std::vector<Event> &events)
{
    out << "Channel " << channel << std::endl;

    int totalEvents = events.size();
    int activeEvents = 0;
    int forceArrivals = 0;
    for (const Event &event : events)
    {
        if (event.get_general_information().at("active") == "true")
        {
            activeEvents++;
        }
        if (event.get_general_information().at("forces_arrival_at_scene") == "true")
        {
            forceArrivals++;
        }
    }

    out << "Stats:" << std::endl;
    out << "Total: " << totalEvents << std::endl;
    out << "active: " << activeEvents << std::endl;
    out << "forces arrival at scene: " << forceArrivals << std::endl << std::endl;

    int numReport = 1;
    out << "Event Reports:" << std::endl << std::endl << std::endl;
    for (const Event &event : events)
    {
        out << "Report_" << numReport++ << ":" << std::endl;
        out << "city: " << event.get_city() << std::endl;
        out << "date time: " << epoch_to_date(event.get_date_time()) << std::endl;
        out << "event name: " << event.get_name() << std::endl;
        out << "summary: " << event.get_description().substr(0,27)<< "..." << std::endl << std::endl;
    }
}

std::string StompProtocol::epoch_to_date(std::time_t timestamp) {
    // Convert timestamp to std::tm
    std::tm* tm = std::localtime(&timestamp);
    
    // Create a string stream to format the time as needed
    std::ostringstream oss;
    oss << std::put_time(tm, "%d/%m/%Y %H:%M:%S"); // Format: DD/MM/YYYY HH:MM:SS
    
    return oss.str();
}


void StompProtocol::handleLogout()
{
    if (!isLoggedIn())
    {
        std::cout << "Client is not logged in" << std::endl;
        return;
    }

    disconnectReceiptId = receiptId++;
    std::string frame = "DISCONNECT\nreceipt:" + std::to_string(disconnectReceiptId) + "\n\n" + '\0';

    if (!connectionHandler->sendFrame(frame))
    {
        std::cout << "Error sending disconnect request" << std::endl;
        return;
    }
}

void StompProtocol::processServerMessage(const std::string &message)
{
    if (message.find("CONNECTED") == 0)
    {
        std::cout << "Login successful" << std::endl;
    }
    else if (message.find("ERROR") == 0)
    {
        size_t start = message.find("message: ");
        size_t end = message.find("\n", start);
        std::cout << message.substr(start, end - start) << std::endl;
        delete (connectionHandler);
        connectionHandler = nullptr;
    }
    else if (message.find("MESSAGE") == 0)
    {
        // size_t start = message.find("\n\n");
        // size_t end = message.size() - start;
        // std::string messageBody = message.substr(start + 2, 5);
        Event newEvent = Event(message);
        // std::istringstream stream(message);
        // std::string line;
        // std::string subscriptionId;
        // while (std::getline(stream, line))
        // {
        //     if (line.find("subscription:") == 0)
        //     {
        //         subscriptionId = line.substr(std::string("subscription:").length());
        //         break;
        //     }
        // }
        // for (const auto &pair : subscriptionIds)
        // {
        //     if (std::to_string(pair.second) == subscriptionId)
        //     {
        //         newEvent.set_channel_name(pair.first);
        //         break;
        //     }
        // }
        userEvents[newEvent.getEventOwnerUser()][newEvent.get_channel_name()].push_back(newEvent);
    }
    else if (message.find("RECEIPT") == 0)
    {
        std::cout << "Receipt received" << std::endl;
        if (message.find(std::to_string(disconnectReceiptId)) != std::string::npos)
        {
            isConnected = false;
            delete connectionHandler;
            connectionHandler = nullptr;
            currentUser.clear();
            subscriptionIds.clear();
            std::cout << "Logged out successfully" << std::endl;
        }
    }
    else
    {
        std::cout << "Unknown message: " << message << std::endl;
    }
}

void StompProtocol::start()
{
    std::thread keyboardThread([this]()
                               {
            while(1) {
                std::string command;
                std::getline(std::cin, command);
                processCommand(command);
            } });

    while (!shouldTerminate)
    {
        if (connectionHandler != nullptr && isConnected)
        {
            std::string answer;
            if (!connectionHandler->getFrame(answer))
            {
                std::cout << "Disconnected. Exiting...\n"
                          << std::endl;
                break;
            }
            // std::cout << "Reply: " << answer << std::endl;
            processServerMessage(answer);
        }
    }

    // keyboardThread.join();
}

StompProtocol::~StompProtocol()
{
    shouldTerminate = true;
    if (connectionHandler != nullptr)
    {
        delete connectionHandler;
    }
}
