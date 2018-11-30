#include "../Headers/Client.h"

using namespace std;

Client::Client() {
    // Initialization
    this->state = 0;
    this->PID = getpid();
    this->sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&(this->followerAdd), 0, sizeof(this->followerAdd));
    this->followerAdd.sin_family = AF_INET;
    this->followerPort = FOLLOWER_PORT;   
}

Client::~Client() {
    // TODO
}

int Client::SendRequest(int key, int request) {
    // TOTEST
    string message = to_string(request) + to_string(key) + to_string(this->PID);

    write(this->sock, message.c_str(), sizeof(message.c_str()));

    return 0;
}

int Client::OccupyLock(int key, int time) {

    sleep(time);    
    return 0;
}

int Client::FormMessage(int key, int request, string& message) {
    // TOTEST
    assert(request == PREEMPTE || request == DELETE);
    message = to_string(request) + '/' + to_string(key) + "/" + to_string(this->PID);

    return 0;
}

int Client::AddLock(int key) {
    // TOTEST
    map<int, int>::iterator iter = this->lockOwned.find(key);
    if(iter != this->lockOwned.end()) {
        this->lockOwned[key] = OWNED;
    }
    else {
        this->lockOwned.insert(pair<int, int>(key, OWNED));
    }

    return 0;
}

int Client::DeleteLock(int key) {
    // TOTEST
    map<int, int>::iterator iter = this->lockOwned.find(key);
    if(iter != this->lockOwned.end()) {
        this->lockOwned[key] = NOT_OWNED;
    }
    else {
        this->lockOwned.insert(pair<int, int>(key, NOT_OWNED));
    }

    return 0;
}

int Client::ListLockOwned() {
    // TOTEST
    cout << "(0_0) Lock owned\n";
    cout << "(0_0) ";
    map<int, int>::iterator iter;
    for(iter = this->lockOwned.begin(); iter != this->lockOwned.end(); iter++) {
        if(iter->second == OWNED)
            cout << "|" << iter->first << "| ";
    }
    cout << "\n";

    return 0;
}

int Client::Connect2Follower() {
    // TOTEST
    char* IP = new char;
    strcpy(IP, this->followerIP.c_str());
    this->followerAdd.sin_family = AF_INET;
    this->followerAdd.sin_addr.s_addr = inet_addr(IP);
    this->followerAdd.sin_port = htons(this->followerPort);

    return connect(this->sock, (struct sockaddr*)&(this->followerAdd), sizeof(this->followerAdd));
}

bool Client::IfConnected() {
    // TODO
    return 0;
}

int Client::GetUUID() {
    // TOTEST
    return this->PID;
}

void Client::ThRead() {
    // TOTEST
    char buf[MESSAGE_SIZE];
    while(true) {        
        memset(buf, 0, sizeof(char));
        read(this->sock, buf, sizeof(buf) - 1);
        if(strcmp(buf, "") == 0) continue;

        string message(buf);
        cout << "(0_0) Receive from follower " << message << "\n";
        vector<string> splited;
        SplitBySlash(message, splited);
        int requestCode = atoi(Parse(splited, MODE_RA).c_str());
        int answerCode = atoi(Parse(splited, MODE_FA).c_str());
        int key = atoi(Parse(splited, MODE_KEY).c_str());
        string request = RA2Text(requestCode);
        string answer = FA2Text(answerCode);

        cout << "(0_0) Answer from " << this->followerIP << ": " << this->followerPort << "\n";
        cout << "(0_0) " << answer << ": " << request << " for lock of " << key << ".\n";

        if(answerCode == SUCCESS) {
            if(requestCode == PREEMPTE) {
                this->AddLock(key);
            }
            else if(requestCode == DELETE)
            {
                this->DeleteLock(key);
            }
            this->ListLockOwned();            
        }
    }
}

void Client::ThWrite() {
    // TOTEST
    char buf[MESSAGE_SIZE];
    while(true) {
        sleep(1);
        getchar();
        cout << "(0_0) Type lock to request (preempte(0) or delete(1)): \n";
        cout << ">> ";
        int key, request;
        cin >> request >> key;
        string message;
        this->FormMessage(key, request, message);
        memset(buf, 0, sizeof(buf));
        strcpy(buf, message.c_str());
        cout << "(0_0) Trying to send message: " << buf << "\n";

        write(this->sock, buf, sizeof(buf));
        cout << "(0_0) Send complete.\n";
    }
}

int Client::MakeUI() {
    // TOTEST
    cout << "(0_0) Type IP address of a follwer: \n";
    cout << ">> ";
    cin >> this->followerIP;
    cout << "(0_0) Connecting to " << this->followerIP << '\n';

    while(this->Connect2Follower() != 0);

    cout << "(0_0) Connected!\n";
    cout << "(0_0) Start read and write thread.\n";

    thread readThread(&Client::ThRead, this);
    thread writeThread(&Client::ThWrite, this);

    readThread.join();
    writeThread.join();

    close(this->sock);

    return 0;
}


