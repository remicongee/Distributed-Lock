#include "../Headers/Leader.h"

using namespace std;

Leader::Leader() {
    // TOTEST
    this->state = 0;
    this->followerNumber = 0;
    this->lockNumber = LOCK_NUMBER;

    for(int idx = 0;idx < this->lockNumber;idx++) {
        this->lockMap.insert(pair<int, string>(idx, ""));
    }
    memset(&(this->leaderAdd), 0, sizeof(this->leaderAdd));
    this->IP = "127.0.0.1";
    this->port = LEADER_PORT;
    FD_ZERO(&this->followerSockSet);
    for(int idx = 0;idx < LOCK_NUMBER;idx++) {
        this->lockMap.insert(pair<int, string>(idx, string("")));
    }
    this->interval.tv_sec = WAIT_MESSAGE_TIME;
    this->interval.tv_usec = 0;
}

Leader::~Leader() {
    // TODO
}

void Leader::ThListen() {
    // TOTEST
    int flag = 0;
    this->leaderSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    this->leaderAdd.sin_family = AF_INET;
    this->leaderAdd.sin_addr.s_addr = inet_addr(this->IP.c_str());
    this->leaderAdd.sin_port = htons(this->port);
    bind(this->leaderSock, (struct sockaddr*)&(this->leaderAdd), sizeof(this->leaderAdd));
    // FD_SET(this->leaderSock, &this->followerSockSet);
    listen(this->leaderSock, 20);
    cout << "(0.0) Listen thread initialized.\n";
    while(true) {
        struct sockaddr_in followerAdd;
        socklen_t followerAddSize = sizeof(followerAdd);
        int followerSock = accept(this->leaderSock, (struct sockaddr*)&followerAdd, &followerAddSize);
        this->followerSockIPMap.insert(pair<int, string>(followerSock, string(inet_ntoa(followerAdd.sin_addr)) + ":" + to_string(ntohs(followerAdd.sin_port))));        

        FD_SET(followerSock, &this->followerSockSet);
        // Test print
        cout << "(0.0) Accept connection from " << inet_ntoa(followerAdd.sin_addr) << "\n";
        // thread followerRead(&Leader::ThRead, this, followerSock);
        // followerRead.detach();
        if(flag == 0) {
            thread followerRead(&Leader::ThRead, this);
            followerRead.detach();
            flag += 1;
        }
    }
}

void Leader::ThReadBySock(int sock) {
    // TOTEST    
    vector<string> messageBuf;
    vector<string>::iterator iter;
    // vector<string> splited;
    char buf[MESSAGE_SIZE];
    char wbuf[MESSAGE_SIZE];
    unique_lock<mutex> lck(this->followerMessageMutex, defer_lock);
    cout << "(0.0) Read thread for " << this->followerSockIPMap.find(sock)->second << " initialized.\n";
    while(true) {
        memset(buf, 0, sizeof(buf));
        read(sock, buf, sizeof(buf) - 1);
        if(strcmp(buf, "") == 0) continue;
        // Test print
        cout << "(0.0) Receive message from " << this->followerSockIPMap.find(sock)->second << ": " << buf << "\n";

        try
        {
            lck.try_lock();
        }
        catch (const std::exception&)
        {
            cout << "(0.0) Tried to get an owned lock.\n";
        }

        if(lck.owns_lock()) {
            // Test print
            // cout << "(0.0) Read locked.\n";

            for(iter = messageBuf.begin();iter != messageBuf.end();iter++) {
                this->followerMessage.emplace_back(sock, string(*iter));
            }
            messageBuf.clear();
            this->followerMessage.emplace_back(sock, string(buf));
            lck.unlock();
            // Test print
            // cout << "(0.0) Read unlocked.\n";

            this->followerMessageCondition.notify_one();
        }
        else {
            // Test print
            // cout << "(0.0) Read pending to buf.\n";

            messageBuf.emplace_back(buf);
            // splited.clear();
            memset(wbuf, 0, sizeof(wbuf));
            // SplitBySlash(string(buf), splited);
            string pendingAnswer = string(buf) + "/" + to_string(PENDING);
            strcpy(wbuf, pendingAnswer.c_str());
            write(sock, wbuf, sizeof(wbuf));
        }        
    }
}

void Leader::ThRead() {
    // TOTEST
    vector<pair<int, string>> messageBuf;
    vector<pair<int, string>>::iterator bufIter;
    map<int, string>::iterator sockMapIter;
    // vector<string> splited;
    char buf[MESSAGE_SIZE];
    char wbuf[MESSAGE_SIZE];
    unique_lock<mutex> lck(this->followerMessageMutex, defer_lock);
    fd_set set;
    cout << "(0.0) Read thread by select initialized.\n";    
    while(true) {
        set = this->followerSockSet;
        // Test print
        cout << "(0.0) Start select.\n";
        
        int readyNumber = select(FD_SETSIZE, &set, NULL, NULL, &this->interval);
        if(readyNumber != -1 && readyNumber != 0) {
            // Test print
            // cout << "(0.0) Get messages.\n";

            for(sockMapIter = this->followerSockIPMap.begin();sockMapIter != this->followerSockIPMap.end();sockMapIter++) {
                int sock = sockMapIter->first;
                if(FD_ISSET(sock, &set)) {
                    memset(buf, 0, sizeof(buf));
                    read(sock, buf, sizeof(buf) - 1);
                    if(strcmp(buf, "") != 0)
                        messageBuf.emplace_back(sock, buf);
                }
            }
            try {
                lck.try_lock();
            }
            catch (const std::exception&) {
                // cout << "(0.0) Tried to get an owned lock.\n";
            }
            if(lck.owns_lock()) {
                for(bufIter = messageBuf.begin();bufIter != messageBuf.end();bufIter++) {
                    this->followerMessage.emplace_back(bufIter->first, bufIter->second);
                    // Test print
                    // cout << "(0.0) Put message " << bufIter->second << "\n";
                }
                if(!messageBuf.empty()) {
                    lck.unlock();
                    this->followerMessageCondition.notify_one();
                    messageBuf.clear();
                }
                else {
                    continue;
                }                
            }
            else {
                string* pending = new string();
                for(bufIter = messageBuf.begin();bufIter != messageBuf.end();bufIter++) {
                    *pending = string(bufIter->second + "/" + to_string(PENDING));
                    memset(wbuf, 0, sizeof(wbuf));
                    strcpy(wbuf, pending->c_str());
                    write(bufIter->first, wbuf, sizeof(buf));                    
                }
                delete pending;
            }
        }
    }
    cout << "(0.0) Read terminated.\n";
}

void Leader::ThWrite() {
    // TOTEST
    char buf[MESSAGE_SIZE];
    unique_lock<mutex> lck(this->leaderMessageMutex, defer_lock);
    vector<pair<int, string>>::iterator iter;
    cout << "(x.x) Write thread initialized.\n";
    while(true) {
        try
        {
            while(!lck.try_lock());
        }
        catch (const std::exception&)
        {
            // cout << "(x.x) Tried to get an owned lock.\n";
        }        
        if(this->leaderMessage.empty()) {
            cout << "(x.x) Write waits.\n";
            this->leaderMessageCondition.wait(lck);
        }
        for(iter = this->leaderMessage.begin();iter != this->leaderMessage.end();iter++) {
            memset(buf, 0, sizeof(buf));
            strcpy(buf, (*iter).second.c_str());
            write((*iter).first, buf, sizeof(buf));
            // Test print
            cout << "(x.x) Write message " << buf << " to " << this->followerSockIPMap.find((*iter).first)->second << "\n";

            usleep(100);
        }
        this->leaderMessage.clear();
        // Test print
        // cout << "(x.x) Leader messages cleared.\n";

        lck.unlock();
        // Test print
        // cout << "(x.x) Leader messages unlocked.\n";

        this->leaderMessageCondition.notify_one();
        // Test print
        // cout << "(x.x) Leader messages notified to back.\n";

        // Test print
        // cout << "(z.z) Write thread sleep.\n";
        // sleep(3);
    }
    cout << "(x.x) Write terminated.\n";
}

void Leader::ThBack() {
    // TOTEST    
    unique_lock<mutex> lckF(this->followerMessageMutex, defer_lock);
    unique_lock<mutex> lckL(this->leaderMessageMutex, defer_lock);
    vector<pair<int, string>> messageBuf;
    vector<pair<int, string>>::iterator iter;
    map<int, string>::iterator sockIter;
    vector<string> splited;
    char buf[MESSAGE_SIZE];
    cout << "(>.<) Back thread initialized.\n";
    while(true) {
        try
        {
            while(!lckF.try_lock());
            // if(!lckF.owns_lock()) continue;
        }
        catch (const std::exception&)
        {
            // cout << "(>.<) Tried to get an owned follower lock.\n";
        }
        if(this->followerMessage.empty()) {
            cout << "(>.<) Back waits.\n";
            this->followerMessageCondition.wait(lckF);
        }            
        // Test print
        // cout << "(>.<) Get follower messages " << this->followerMessage.size() << ".\n";

        for(iter = this->followerMessage.begin();iter != this->followerMessage.end();iter++) {
            messageBuf.emplace_back((*iter).first, (*iter).second);
        }
        this->followerMessage.clear();
        lckF.unlock();

        try
        {
            while(!lckL.try_lock());            
        }
        catch (const std::exception&)
        {
            cout << "(>.<) Tried to get an owned leader lock.\n";
        }
        if(!this->leaderMessage.empty()) {
            this->leaderMessageCondition.wait(lckL);
            // Test print
            // cout << "(>.<) Back notified by write.\n";
        }
        for(iter = messageBuf.begin();iter != messageBuf.end();iter++) {
            // Test print
            cout << "(>.<) Start processing request from " << this->followerSockIPMap.find((*iter).first)->second << ".\n"; 

            SplitBySlash((*iter).second, splited);
            int key = atoi(Parse(splited, MODE_KEY).c_str());
            int request = atoi(Parse(splited, MODE_RA).c_str());
            string UUID = Parse(splited, MODE_PID) + "/" + Parse(splited, MODE_CIP);
            bool owned = this->CheckLockOwned(key, UUID);
            if(!owned) {
                this->FormMessage(FAIL, (*iter).second, buf);
                this->leaderMessage.emplace_back((*iter).first, buf);
            }
            else {
                switch(request) {
                    case(PREEMPTE):
                        // Test print
                        cout << "(>.<) Preempte case from " << UUID << "\n";

                        this->AddLock(key, UUID);
                        this->FormMessage(SUCCESS, (*iter).second, buf);
                        // Test print
                        // cout << "(>.<) Form message " << buf << "\n";

                        this->leaderMessage.emplace_back((*iter).first, buf);
                        for(sockIter = this->followerSockIPMap.begin();sockIter != this->followerSockIPMap.end();sockIter++) {
                            this->FormMessage(ADD, (*iter).second, buf);
                            this->leaderMessage.emplace_back(sockIter->first, buf);
                        }
                        break;
                    case(DELETE):
                        // Test print
                        cout << "(>.<) Delete case " << UUID << "\n";

                        this->DelLock(key);
                        this->FormMessage(SUCCESS, (*iter).second, buf);
                        // Test print
                        // cout << "(>.<) Form message " << buf << "\n";

                        this->leaderMessage.emplace_back((*iter).first, buf);
                        for(sockIter = this->followerSockIPMap.begin();sockIter != this->followerSockIPMap.end();sockIter++) {
                            this->FormMessage(ELIMINATE, (*iter).second, buf);
                            this->leaderMessage.emplace_back(sockIter->first, buf);
                        }
                        break;
                    default:
                        // Test print
                        cout << "(>.<) Wrong case " << UUID << "\n";

                        this->FormMessage(FAIL, (*iter).second, buf);
                        this->leaderMessage.emplace_back((*iter).first, buf);
                }
            }
        }
        lckL.unlock();
        // Test print
        // cout << "(>.<) Notify write thread.\n";
        this->leaderMessageCondition.notify_one();
        messageBuf.clear();
        // Test print
        // cout << "(>.<) Back complete one turn.\n";
    }
    cout << "(>.<) Back terminated.\n";
}

void Leader::FormMessage(int answer, string followerMessage, char* buf) {
    // TOTEST
    string message = followerMessage + "/" + to_string(answer);
    memset(buf, 0, sizeof(buf));
    strcpy(buf, message.c_str());
}

bool Leader::CheckLockOwned(int key, string UUID) {
    // TOTEST
    map<int, string>::iterator iter = this->lockMap.find(key);
    if(iter == this->lockMap.end()) return false;
    bool owned =  (strcmp(iter->second.c_str(), UUID.c_str()) == 0);
    bool vacant = (strcmp(iter->second.c_str(), "") == 0);
    return (owned || vacant);
}

int Leader::AddLock(int key, string UUID) {
    // TOTEST
    // assert(this->CheckLockOwned(key) != 1);
    map<int, string>::iterator iter = this->lockMap.find(key);
    cout << "(>.<) Transfer lock on " << key << " from " << iter->second << " to ";
    iter->second = UUID;
    // Test print
    cout << iter->second << "\n";

    return 0;
}

int Leader::DelLock(int key) {
    // TOTEST
    map<int, string>::iterator iter = this->lockMap.find(key);
    iter->second = string("");

    return 0;
}

int Leader::RunLeader() {
    // TOTEST    
    thread writeThread(&Leader::ThWrite, this);
    thread backThread(&Leader::ThBack, this);
    thread listenThread(&Leader::ThListen, this);
    
    writeThread.join();
    backThread.join();
    listenThread.join();

    map<int, string>::iterator iter;
    for(iter = this->followerSockIPMap.begin();iter != this->followerSockIPMap.end();iter++) {
        close(iter->first);
    }

    return 0;
}

int Leader::BroadRequest(int key, string message) {
    // TOTEST
    char buf[MESSAGE_SIZE];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, message.c_str());
    for(map<int, string>::iterator iter = this->followerSockIPMap.begin();iter != this->followerSockIPMap.end();iter++) {
        write(iter->first, buf, sizeof(buf));
    }

    return 0;
}

int Leader::SendAnswer(int socket, string message) {
    // TOTEST
    char buf[MESSAGE_SIZE];
    memset(buf, 0, sizeof(buf));
    strcpy(buf, message.c_str());
    write(socket, buf, sizeof(buf));

    return 0;
}

int Leader::GetLockNumber() {
    // TOTEST
    return this->lockNumber;
}

int Leader::GetFollowerNumber() {
    // TOTEST
    return this->followerNumber;
}