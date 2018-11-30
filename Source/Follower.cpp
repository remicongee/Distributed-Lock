#include "../Headers/Follower.h"

using namespace std;

Follower::Follower() {
    // TOTEST
    this->state = 0;
    this->clientNumber = 0;
    this->port = FOLLOWER_PORT;
    this->leaderPort = LEADER_PORT;
    this->lockNumber = LOCK_NUMBER;
    this->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    this->leaderSock = socket(AF_INET, SOCK_STREAM, 0);

    for(int idx = 0;idx < this->lockNumber;idx++) {
        this->lockMap.insert(pair<int, string>(idx, ""));
    }
    memset(&(this->leaderAdd), 0, sizeof(this->leaderAdd));
    memset(&(this->followerAdd), 0, sizeof(this->followerAdd));
    this->IP = "127.0.0.1";
    this->port = FOLLOWER_PORT;
    this->leaderIP = "127.0.0.1";
    this->leaderPort = LEADER_PORT;
    FD_ZERO(&this->clientSockSet);
    for(int idx = 0;idx < LOCK_NUMBER;idx++) {
        this->lockMap.insert(pair<int, string>(idx, string("")));
    }
    this->interval.tv_sec = WAIT_MESSAGE_TIME;
    this->interval.tv_usec = 0;
}

Follower::~Follower() {
    // TODO
}

void Follower::ThListen() {
    // TOTEST
    int flag = 0;
    this->followerAdd.sin_family = AF_INET;
    this->followerAdd.sin_addr.s_addr = inet_addr(this->IP.c_str());
    this->followerAdd.sin_port = htons(this->port);
    bind(this->sock, (struct sockaddr*)&(this->followerAdd), sizeof(this->followerAdd));
    // FD_SET(this->leaderSock, &this->followerSockSet);
    listen(this->sock, 20);
    cout << "(0.0) Listen thread initialized.\n";
    while(true) {
        struct sockaddr_in clientAdd;
        socklen_t clientAddSize = sizeof(clientAdd);
        int clientSock = accept(this->sock, (struct sockaddr*)&clientAdd, &clientAddSize);
        this->sock2IPMap.insert(pair<int, string>(clientSock, string(inet_ntoa(clientAdd.sin_addr))));
        this->IP2SockMap.insert(pair<string, int>(string(inet_ntoa(clientAdd.sin_addr)), clientSock));

        FD_SET(clientSock, &this->clientSockSet);
        // Test print
        cout << "(0.0) Accept connection from " << inet_ntoa(clientAdd.sin_addr) << "\n";
        // thread followerRead(&Leader::ThRead, this, followerSock);
        // followerRead.detach();
        if(flag == 0) {            
            thread clientRead(&Follower::ThReadClient, this);            
            clientRead.detach();
            thread backC2L(&Follower::ThBackC2L, this);
            backC2L.detach();
            thread write2C(&Follower::ThWrite2Client, this);
            write2C.detach();
            flag = 1;
        }
    }
}

void Follower::ThReadClient() {
    // TOTEST
    vector<pair<int, string>> messageBuf;
    vector<pair<int, string>>::iterator bufIter;
    map<int, string>::iterator sockMapIter;
    unique_lock<mutex> lck(this->clientMessageMutex, defer_lock);
    char buf[MESSAGE_SIZE];
    char wbuf[MESSAGE_SIZE];
    fd_set set;
    cout << "(C.C) Read thread by select initialized.\n"; 
    while(true) {
        set = this->clientSockSet;

        int readyNumber = select(FD_SETSIZE, &set, NULL, NULL, &this->interval);
        if(readyNumber != -1 && readyNumber != 0) {
            for(sockMapIter = this->sock2IPMap.begin();sockMapIter != this->sock2IPMap.end();sockMapIter++) {
                int sock = sockMapIter->first;
                if(FD_ISSET(sock, &set)) {
                    memset(buf, 0, sizeof(buf));
                    read(sock, buf, sizeof(buf) - 1);
                    if(strcmp(buf, "") != 0)
                        messageBuf.emplace_back(sock, buf);
                }
            }
        }
        if(!messageBuf.empty()) {
            try {
                lck.try_lock();
            }
            catch (const std::exception&) {
                // cout << "(0.0) Tried to get an owned lock.\n";
            }
        }
        else {
            continue;
        }
        
        if(lck.owns_lock()) {
            for(bufIter = messageBuf.begin();bufIter != messageBuf.end();bufIter++) {
                cout << "(C.C) Receive from " << bufIter->first << " " << bufIter->second << "\n";
                this->clientMessage.emplace_back(bufIter->first, bufIter->second);
            }            
            if(!messageBuf.empty()) {
                lck.unlock();
                this->clientMessageCondition.notify_one();
                messageBuf.clear();
            }
            else {
                continue;
            }            
        }
        else {
            vector<string>* splited = new vector<string>();
            string* pending = new string();
            for(bufIter = messageBuf.begin();bufIter != messageBuf.end();bufIter++) {
                SplitBySlash(bufIter->second, *splited);
                *pending = string(bufIter->second + "/" + to_string(PENDING));
                memset(wbuf, 0, sizeof(wbuf));
                strcpy(wbuf, pending->c_str());
                write(bufIter->first, wbuf, sizeof(wbuf));
            }
            delete pending;
        }
    }
    cout << "(0.0) Read terminated.\n";
}

void Follower::ThReadLeader() {
    vector<string> messageBuf;
    vector<string>::iterator bufIter;
    unique_lock<mutex> lck(this->leaderMessageMutex, defer_lock);
    char buf[MESSAGE_SIZE];
    cout << "(L.L) Read thread by select initialized.\n"; 
    while(true) {
        memset(buf, 0, sizeof(buf));
        read(this->leaderSock, buf, sizeof(buf) - 1);
        if(strcmp(buf, "") != 0)
            messageBuf.emplace_back(buf);
        else {
            continue;
        }
        try {
            lck.try_lock();
        }
        catch (const std::exception&) {
            // cout << "(L.L) Tried to get an owned lock.\n";
        }
        if(lck.owns_lock()) {
            for(bufIter = messageBuf.begin();bufIter != messageBuf.end();bufIter++) {
                cout << "(L.L) Receive from leader " << *bufIter << "\n";
                this->leaderMessage.emplace_back(*bufIter);
            }
            if(!messageBuf.empty()) {
                lck.unlock();
                cout << "(L.L) Notify L2C back.\n";
                this->leaderMessageCondition.notify_one();
                messageBuf.clear();
            }
            else {
                continue;
            }            
        }
    }
    cout << "(0.0) Read terminated.\n";
}

void Follower::ThWrite2Client() {
    // TOTEST
    char buf[MESSAGE_SIZE];
    unique_lock<mutex> lck(this->answerMutex, defer_lock);
    vector<pair<int, string>>::iterator iter;
    cout << "(C.C) Write thread initialized.\n";
    while(true) {
        try {
            while(!lck.try_lock());
        }
        catch (const std::exception&) {
            // cout << "(C.C) Tried to get an owned lock.\n";
        }
        if(this->answer.empty()) {
            cout << "(C.C) Write waits.\n";
            this->answerCondition.wait(lck);
        }
        for(iter = this->answer.begin();iter != this->answer.end();iter++) {
            cout << "(C.C) Write2C handles " << iter->second << "\n";
            memset(buf, 0, sizeof(buf));
            strcpy(buf, iter->second.c_str());
            write(iter->first, buf, sizeof(buf));
        }
        this->answer.clear();

        lck.unlock();

        this->answerCondition.notify_one();
    }
    cout << "(C.C) Write terminated.\n";
}

void Follower::ThWrite2Leader() {
    // TOTEST
    char buf[MESSAGE_SIZE];
    unique_lock<mutex> lck(this->requestMutex, defer_lock);
    vector<string>::iterator iter;
    cout << "(L.L) Write thread initialized.\n";
    while(true) {
        try {
            while(!lck.try_lock());
        }
        catch (const std::exception&) {
            // cout << "(L.L) Tried to get an owned lock.\n";
        }
        if(this->answer.empty()) {
            cout << "(L.L) Write waits.\n";
            this->requestCondition.wait(lck);
        }
        for(iter = this->request.begin();iter != this->request.end();iter++) {
            cout << "(L.L) Write2L handles " << *iter << "\n";        
            memset(buf, 0, sizeof(buf));
            strcpy(buf, (*iter).c_str());
            write(this->leaderSock, buf, sizeof(buf));

            usleep(100);
        }
        this->request.clear();

        lck.unlock();

        this->requestCondition.notify_one();
    }
    cout << "(L.L) Write terminated.\n";
}

void Follower::ThBackC2L() {
    // TOTEST
    vector<pair<int, string>> messageBuf;
    // vector<pair<int, string>>::iterator bufIter;
    vector<pair<int, string>>::iterator iter;
    unique_lock<mutex> lckC(this->clientMessageMutex, defer_lock);
    unique_lock<mutex> lckR(this->requestMutex, defer_lock);
    char buf[MESSAGE_SIZE];
    cout << "(C.L) Back thread initialized.\n";
    while(true) {
        try {
            while(!lckC.try_lock());
        }
        catch (const std::exception&) {
            // cout << "(>.C) Tried to get an owned lock.\n";
        }
        if(this->clientMessage.empty()) {
            this->clientMessageCondition.wait(lckC);
            cout << "(C.L) Back waits.\n";
        }
        for(iter = this->clientMessage.begin();iter != this->clientMessage.end();iter++) {
            messageBuf.emplace_back(iter->first, iter->second);
        }
        this->clientMessage.clear();
        lckC.unlock();

        vector<string>* splited = new vector<string>();
        string* UUID = new string();
        for(iter = messageBuf.begin();iter != messageBuf.end();iter++) {
            cout << "(C.L) Back C2L handles " << iter->second << "\n";
            SplitBySlash(iter->second, *splited);
            int key = stoi(Parse(*splited, MODE_KEY));
            *UUID = Parse(*splited, MODE_PID) + "/" + this->sock2IPMap[iter->first];
            cout << "(C.L) from " << *UUID << "\n";
            if(this->UUID2SockMap.find(*UUID) == this->UUID2SockMap.end()) {
                this->UUID2SockMap.emplace(*UUID, iter->first);
            }
            if(!this->CheckLockOwned(key, *UUID)) {         
                int request = stoi(Parse(*splited, MODE_RA));       
                string* fail = new string(to_string(request) + "/" + to_string(key) + "/" + to_string(FAIL));
                memset(buf, 0, sizeof(buf));
                strcpy(buf, fail->c_str());
                delete fail;
                write(iter->first, buf, sizeof(buf));
                messageBuf.erase(iter);
                iter--;
            }
            else {
                iter->second += string("/" + this->sock2IPMap[iter->first]);
            }
        }     
        if(!messageBuf.empty()) {
            try {
                while(!lckR.try_lock());
            }
            catch (const std::exception&) {
                cout << "(C.L) Back waits.\n";
            }
        }
        else {
            continue;
        }  
        // if(!this->request.empty()) {
        //     this->requestCondition.wait(lckR);
        // }
        for(iter = messageBuf.begin();iter != messageBuf.end();iter++) {
            this->request.emplace_back(iter->second);
        }
        lckR.unlock();
        this->requestCondition.notify_one();

        messageBuf.clear();
        delete splited;
        delete UUID;
    }
}

void Follower::ThBackL2C() {
    // TOTEST
    vector<pair<int, string>> messageBuf;
    vector<pair<int, string>>::iterator bufIter;
    vector<string>::iterator iter;
    map<int, string>::iterator mapIter;
    unique_lock<mutex> lckL(this->leaderMessageMutex, defer_lock);
    unique_lock<mutex> lckA(this->answerMutex, defer_lock);
    char buf[MESSAGE_SIZE];
    cout << "(L.C) Back thread initialized.\n";
    while(true) {
        try {
            while(!lckL.try_lock());
            cout << "(L.C) Back locks.\n";
        }
        catch (const std::exception&) {
            // cout << "(>.L) Tried to get an owned lock.\n";
        }
        if(this->leaderMessage.empty()) {
            cout << "(L.C) Back waits.\n";
            this->leaderMessageCondition.wait(lckL);            
        }
        for(iter = this->leaderMessage.begin();iter != this->leaderMessage.end();iter++) {
            messageBuf.emplace_back(0, *iter);
        }
        leaderMessage.clear();
        lckL.unlock();
        // this->leaderMessageCondition.notify_one();

        vector<string>* splited = new vector<string>;
        string* UUID = new string();
        for(bufIter = messageBuf.begin();bufIter != messageBuf.end();bufIter++) {
            cout << "(L.C) Back L2C handles " << bufIter->second << "\n";
            SplitBySlash(bufIter->second, *splited);
            int request = stoi(Parse(*splited, MODE_RA));
            int key = stoi(Parse(*splited, MODE_KEY));
            int answer = stoi(Parse(*splited, MODE_LA));
            *UUID = string(Parse(*splited, MODE_PID) + "/" + Parse(*splited, MODE_CIP));
            switch(answer) {
                case(ADD):
                    mapIter = this->lockMap.find(key);
                    mapIter->second = string(UUID->c_str());
                    messageBuf.erase(bufIter);
                    bufIter--;
                    break;
                case(ELIMINATE):
                    mapIter = this->lockMap.find(key);
                    mapIter->second = "";
                    messageBuf.erase(bufIter);
                    bufIter--;
                    break;
                default:
                    FormAnswer(request, key, answer, buf);
                    bufIter->first = this->UUID2SockMap[*UUID];
                    bufIter->second = string(buf);
                    cout << "(L.C) Form answer " << buf << "\n";
                    break;
            }
        }
        if(!messageBuf.empty()) {
            try {
                while(!lckA.try_lock());
            }
            catch (const std::exception&) {
                // cout << "(>.L) Tried to get an owned lock.\n";
            }
        }
        else {
            continue;
        }        
        // if(!this->answer.empty()) {
        //     this->answerCondition.wait(lckA);
        // }
        for(bufIter = messageBuf.begin();bufIter != messageBuf.end();bufIter++) {
            this->answer.emplace_back(bufIter->first, bufIter->second);
        }
        lckA.unlock();
        this->answerCondition.notify_one();

        messageBuf.clear();
        delete splited;
        delete UUID;        
    }
}

void Follower::RunFollower() {
    // TOTEST
    while(this->Connect2Leader() != 0);

    cout << "(0_0) Connected!\n";
    cout << "(0_0) Start read and write thread.\n";

    thread listen(&Follower::ThListen, this); 
    thread readLeader(&Follower::ThReadLeader, this);       
    thread backL2C(&Follower::ThBackL2C, this);
    thread write2L(&Follower::ThWrite2Leader, this);     

    listen.join();    
    readLeader.join();    
    backL2C.join();
    write2L.join();
}

bool Follower::CheckLockOwned(int key, string UUID) {
    // TOTEST
    map<int, string>::iterator iter = this->lockMap.find(key);
    if(iter == this->lockMap.end()) return false;
    bool owned =  (strcmp(iter->second.c_str(), UUID.c_str()) == 0);
    bool vacant = (strcmp(iter->second.c_str(), "") == 0);
    return (owned || vacant);
}

int Follower::AddLock(int key, string UUID) {
    // TOTEST
    map<int, string>::iterator iter = this->lockMap.find(key);
    cout << "(>.<) Transfer lock on " << key << " from " << iter->second << " to ";
    iter->second = string(UUID.c_str());
    // Test print
    cout << iter->second << "\n";

    return 0;
}

int Follower::DelLock(int key) {
    // TOTEST
    map<int, string>::iterator iter = this->lockMap.find(key);
    iter->second = string("");

    return 0;
}

void Follower::FormAnswer(int request, int key, int answer, char* buf) {
    // TOTEST
    string* message = new string(to_string(request) + "/" + to_string(key) + "/" + to_string(answer));
    memset(buf, 0, sizeof(buf));
    strcpy(buf, message->c_str());
    delete message;
}

void Follower::FormRequest(std::string request, std::string IP, char* buf) {
    // TOTEST
    string* message = new string(request + "/" + IP);
    memset(buf, 0, sizeof(buf));
    strcpy(buf, message->c_str());
    delete message;
}

int Follower::Connect2Leader() {
    // TOTEST
    char* IP = new char;
    strcpy(IP, this->leaderIP.c_str());
    this->leaderAdd.sin_family = AF_INET;
    this->leaderAdd.sin_addr.s_addr = inet_addr(IP);
    this->leaderAdd.sin_port = htons(this->leaderPort);

    return connect(this->leaderSock, (struct sockaddr*)&(this->leaderAdd), sizeof(this->leaderAdd));
}
