#ifndef __LEADER_H
#define __LEADER_H

#include "Common.h"

class Leader {
    public:
        int state;
    private:
        std::map<int, std::string> lockMap;
        std::map<int, std::string> followerSockIPMap;
        std::vector<std::pair<int, std::string>> followerMessage;
        std::mutex followerMessageMutex;
        std::condition_variable followerMessageCondition;
        std::vector<std::pair<int, std::string>> leaderMessage;
        std::mutex leaderMessageMutex;
        std::condition_variable leaderMessageCondition;
        fd_set followerSockSet;
        struct timeval interval;
        int followerNumber;
        int lockNumber;
        std::string IP;
        int port;
        int leaderSock;
        struct sockaddr_in leaderAdd;
    public:
        Leader();
        ~Leader();

        void ThListen();
        void ThReadBySock(int sock);
        void ThRead();
        void ThWrite();
        void ThBack();

        void FormMessage(int answer, std::string followerMessage, char* buf);
        bool CheckLockOwned(int key, std::string UUID);
        int AddLock(int key, std::string UUID);
        int DelLock(int key);
        // Broadcast request to all followers: ADD, DELETE
        int BroadRequest(int key, std::string message);
        // Send answer to the follower sending request: ADD, DELETE, PENDING
        int SendAnswer(int sock, std::string message);
        int RunLeader();

        int GetLockNumber(); // Not used
        int GetFollowerNumber(); // Not used
};



#endif