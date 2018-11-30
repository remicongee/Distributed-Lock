#ifndef __FOLLOWER_H
#define __FOLLOWER_H

#include "Common.h"

class Follower {
    public:
        int state;
    private:
        std::map<int, std::string> lockMap;
        std::vector<std::string> leaderMessage;
        std::mutex leaderMessageMutex;
        std::condition_variable leaderMessageCondition;
        std::vector<std::pair<int, std::string>> answer;
        std::mutex answerMutex;
        std::condition_variable answerCondition;
        std::vector<std::pair<int, std::string>> clientMessage;
        std::mutex clientMessageMutex;
        std::condition_variable clientMessageCondition;
        std::vector<std::string> request;
        std::mutex requestMutex;
        std::condition_variable requestCondition;
        std::map<std::string, int> IP2SockMap;
        std::map<int, std::string> sock2IPMap;
        std::map<std::string, int> UUID2SockMap;
        int clientNumber;
        int lockNumber;
        std::string IP;
        int port;
        int sock;
        std::string leaderIP;
        int leaderPort;
        int leaderSock;
        struct sockaddr_in followerAdd;
        struct sockaddr_in leaderAdd;
        fd_set clientSockSet;
        struct timeval interval;
    public:
        Follower();
        ~Follower();

        void ThListen();
        void ThReadClient();
        void ThReadLeader();
        void ThWrite2Client();
        void ThWrite2Leader();
        void ThBackC2L();
        void ThBackL2C();
        void RunFollower();

        bool CheckLockOwned(int key, std::string UUID);
        int AddLock(int key, std::string UUID);
        int DelLock(int key);
        void FormAnswer(int request, int key, int answer, char* buf);
        void FormRequest(std::string request, std::string IP, char* buf);
       
        int Connect2Leader();

        bool IfConnected();
        int GetLockNumber();
        int GetClientNumber();
        int AddClient();
};


#endif