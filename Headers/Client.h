#ifndef __CLIENT_H
#define __CLIENT_H

#define EXPIRE_TIME 10
#define NOT_OWNED 0
#define OWNED 1

#include "Common.h"

class Client {
    public:
        int state;
    private:
        int PID;
        std::string followerIP;
        int followerPort;
        struct sockaddr_in followerAdd;
        int sock;
        std::map<int, int> lockOwned;
    public:
        Client();
        ~Client();

        // Send request to follower: PREEMPTE, DELETEs
        int SendRequest(int key, int request);
        // Occpupy lock of 'key' for 'time' seconds
        int OccupyLock(int key, int time);

        int Connect2Follower();
        // Form message: request/key/PID
        int FormMessage(int key, int request, std::string& message);
        // Add lock key to lockOwned
        int AddLock(int key);
        // Set lock of key as not owned
        int DeleteLock(int key);
        // Display lock owned
        int ListLockOwned();
        void ThRead();
        void ThWrite();
        // U_U
        int MakeUI();

        bool IfConnected(); // Not implemented
        int GetUUID(); // Not implemented        
};

#endif