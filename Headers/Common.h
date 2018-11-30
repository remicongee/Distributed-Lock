#ifndef __COMMON_H
#define __COMMON_H

// Request from client
#define PREEMPTE 0
#define DELETE 1
// Request from leader
#define ADD 2
#define ELIMINATE 3
// Answer from leader
#define PREEMPTE_PENDING 2
#define PREEMPTE_SUCCESS 3
#define PREEMPTE_FAIL 4
#define DELETE_PENDING 5
#define DELETE_SUCCESS 6
#define DELETE_FAIL 7
// Broadcast or not
#define BROADCAST 9
#define SINGLE 10
// Answer
#define SUCCESS 0
#define FAIL 1
#define PENDING 2

#define FOLLOWER_PORT 2333
#define LEADER_PORT 3333
#define MESSAGE_SIZE 128
#define LOCK_NUMBER 10

#define WAIT_MESSAGE_TIME 5

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <memory.h>
#include <assert.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <exception>
#include <sys/select.h>
#include <sys/time.h>

/* Protocol
* message: default size 128 bytes
*   Client -> Follewer: request/key/PID
*   Follower -> Leader: request/key/PID/clientIP
*   Follower -> Client: request/key/answer
*   Leader -> Follower: request/key/PID/clientIP/answer
* LockMap:
*   +-----+---------------------+
*   | key | UUID = PID/clientIP |
*   +-----+---------------------+
*/
#define MODE_RA 0
#define MODE_KEY 1
#define MODE_PID 2
#define MODE_CIP 3
#define MODE_LA 4
#define MODE_FA 2


// #include "../Tools/ReSocket.h"
// #include "../Tools/ReThread.h"

// Split message by '/'
static void SplitBySlash(const std::string message, std::vector<std::string>& splited) {
    // TOTEST
    splited.clear();
    std::string slash = "/";
    std::string::size_type pos1, pos2;
    pos2 = message.find(slash);
    pos1 = 0;
    while(std::string::npos != pos2)
    {
        splited.push_back(message.substr(pos1, pos2 - pos1));
         
        pos1 = pos2 + slash.size();
        pos2 = message.find(slash, pos1);
    }
    if(pos1 != message.length())
        splited.push_back(message.substr(pos1));
}

// Parse request or answer
static std::string Parse(std::vector<std::string>& splited, int mode) {
    // TOTEST
    assert(mode == MODE_RA || mode == MODE_KEY || mode == MODE_PID || mode == MODE_CIP || MODE_LA || MODE_FA);
    return splited[mode];
}

static std::string RA2Text(int request) {
    std::string text;
    switch(request) {
        case(PREEMPTE):
            text = "PREEMPTE";
            break;
        case(DELETE):
            text = "DELETE";
            break;
        default:
            std::cout << "Unknown answer: code " << request << "\n";
            break;
    }
    return text;
}

static std::string FA2Text(int answer) {
    std::string text;
    switch(answer) {
        case(SUCCESS):
            text = "SUCESS";
            break;
        case(FAIL):
            text = "FAIL";
            break;
        default:
            std::cout << "Unknown answer: code " << answer << "\n";
            break;
    }
    return text;
}

#endif