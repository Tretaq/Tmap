#include <iostream>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdint>
#include <fcntl.h>// flags F_GETFL, F_SETFL, O_NONBLOCK
#include <poll.h>// pollfd
#include <string>
#include <vector>
#include <algorithm>
#include <atomic>
#include <complex>
#include <queue>
#include <stdexcept>
#include <mutex>
#include <thread>
#include <chrono>
using namespace std;



// g++ main.cpp -o Tmap

enum class PortState { Open, Closed, Filtered, Error };
struct ScanResult{
    PortState state = PortState::Error;
    string banner;
};
int toInt(const string& s) { // no copy no changes igstabilność materi
    size_t used = 0;
    int v = stoi(s, &used);
    if (used != s.size()) throw invalid_argument("not a number: " + s); // jeżeli sie nie równają to znaczy że są śmieci w środku
    return v;
};

vector<uint16_t> parsePorts(const string& spec) {
    vector<uint16_t> ports; // uint16_t maks = 65,535
    size_t start = 0;
    while (start <= spec.size()) {
        size_t comma = spec.find(',',start);// wyszukanie pierwszego , od jakiegoś startu(numeru) zacząć to wyszukiwanie
        if (comma == string::npos) comma = spec.size(); // check if there even is ,
        string token = spec.substr(start, comma - start);// wycina określony fragment i zapisuje w token
        start = comma + 1;

        if (token.empty()) throw invalid_argument("empty entry in port list");// theres no word in token

        int lo, hi; // lowest higest port
        size_t dash = token.find('-');//we look for a range :>
        if (dash == string::npos) {
            lo = hi = toInt(token);
        } else {
            lo = toInt(token.substr(0, dash));
            hi = toInt(token.substr(dash + 1));// od dash + 1 do końca
        }
        if (lo < 1 || hi > 65535 || lo > hi) {
            throw invalid_argument("Bad port or range: " + token);
        }
        for (int p = lo; p <= hi; ++p) ports.push_back(p);// add to vector each port
    }
    sort(ports.begin(), ports.end());
    ports.erase(unique(ports.begin(),ports.end()),ports.end());// unique sets an iterator to the first duplicated element than erase deletes to end
    return ports;
}


int connectTo(const sockaddr_in& base, uint16_t port , int timeoutMs , PortState& state ) {
    //
    sockaddr_in addr = base;
    addr.sin_port = htons(port);
    state = PortState::Error;

    int fd = socket(AF_INET, SOCK_STREAM, 0);// AF_INET = IPv4 SOCK_STREAM = TCP 0 = default protocol
    // create a socket
    if (fd == -1) return -1;

    int flags = fcntl(fd, F_GETFL, 0); // we take the setting (flags) for this socket and in nex we do new ones
    fcntl(fd, F_SETFL, flags | O_NONBLOCK); // normally connect() and recv() stop running program till end of op after O_NONBLOCK they stop so if no conn there is ther will be an error nad prograam can do smth else
    // it returns immidiently and dont wait for handshake


    int rc = connect(fd, (sockaddr*)&addr, sizeof(addr)); //

    if (rc == 0) {
        state = PortState::Open;
    } else if (errno == ECONNREFUSED) {
        state = PortState::Closed;
    } else if ( errno == EINPROGRESS) { // the things before this  are checking mostly on local network where it can con or err instantly
        pollfd pfd{};
        pfd.fd = fd;
        pfd.events = POLLOUT;

        int r = poll(&pfd, 1 , timeoutMs);// check the response

        if (r == 0) {
            state = PortState::Filtered;
        } else if (r > 0){
            int err = 0;
            socklen_t len = sizeof(err);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0) {
                if (err == 0) state = PortState::Open;
                else if (err == ECONNREFUSED) state = PortState::Closed;
            }
        }
    }
    if (state == PortState::Open) return fd;
    close(fd);

    return -1;

}
string ReadSome(int fd, int timeoutMs) {
    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLIN;
    if (poll(&pfd , 1 , timeoutMs) <= 0) return ""; // sprawdzamy przez sekunde/timeoutMs czy coś poszło

    char buf[512];
    ssize_t n = recv(fd,buf,sizeof(buf),0);// mówi ile bajtów wzieliśmy z serwera
    if (n <= 0) return "";
    return string(buf, n);// zwraca tylko n bajtów
}

string pickLine(const string& raw) { // just return whats on port
    if (raw.rfind("HTTP/", 0) == 0) {
        size_t pos = raw.find("\r\nServer:");
        if (pos != string::npos) { // czy pos zostało znalezione jezeli jest to sie wykona
            size_t start = pos + 2;// + 2 bo wtedy zwraca jaki software
            size_t end = raw.find("\r\n", start);
            return raw.substr(start,end == string::npos ? string::npos : end - start);
        }
    }
    return raw;
}
string cleanBanner(const string& raw) {
    string out;
    for (char c : raw) {
        if (c == '\r' || c == '\n') {
            break;
        }
        out += (c >= 32 && c < 126 ? c : '.');
    }
    if (out.size() > 80) out.resize(80); // it cant be that big copium
    return out;
}
string whatService(const string& b) { // string::npos nie znaleziono
    if (b.rfind("SSH-", 0) == 0) return "SSH";
    if (b.rfind("HTTP/",0) == 0 || b.rfind("Server:" , 0) == 0) return  "HTTP";
    if (b.rfind("220",0) == 0) {
        if (b.find("FTP") != string::npos) return "FTP";
        if (b.find("SMTP") != string::npos) return "SMTP";
    }
    return "";
}

PortState scanPort(const sockaddr_in& base, uint16_t port, int timeoutMs) { // sockaddr_in& base host addres
    PortState state;
    int k = connectTo(base,port,timeoutMs,state); // create socket
    if (k != -1) close(k); // close socket if isint opem
    return state;

}
string grabBaner() {

}
int main(int argc, char* argv[]) {
    string portSpec = "1-1024";
    int timeoutMs = 1000;
    string host;
    vector<uint16_t> ports;
    int num_threads = 100;
    bool grabBanner = false;

    try {
        for (int i = 1 ; i < argc; ++i) {
            string argument = argv[i];
            if (argument == "-p" || argument == "-t" || argument == "-T") {
                if (i + 1 >= argc) {
                    throw invalid_argument(argument + " needs a value");
                }
                string value = argv[++i];
                if (argument == "-p") {
                    portSpec = value;
                }else if (argument == "-T") {
                    num_threads = toInt(value);
                }
                else {
                    timeoutMs = toInt(value);
                }
            } else if (argument[0] == '-') {
                throw invalid_argument("Unknown option: " + argument);
            } else {
                host = argument;
            }
        }
        if (host.empty()) throw invalid_argument("missing host");
        if (num_threads < 1) throw invalid_argument("number of threads must be at least 1");
        if (timeoutMs < 1) throw invalid_argument("timeout must be at least 1 ms");
        ports = parsePorts(portSpec);
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n" << "Usage: " << argv[0] << " -p ports -t timeout_ms <host> \n";
        return 1;
    }

    addrinfo hints{}; // addrinfo = to typ który przchowuje teczke informacji o danym hoscie a hnints mówi co my tylko chcemy {} resetuje całą pamięć bez tego będzie miała losowe śmieci z pamięci bo nie bedzie zainicjonowana
    hints.ai_family = AF_INET; // only ipv4
    hints.ai_socktype = SOCK_STREAM; // only tcp    tcp udp is sock_dgram

    addrinfo* res = nullptr; // pointer that shoud fill in addrinfo
    int rc = getaddrinfo(host.c_str(), nullptr, &hints, &res); // we check what ip is behind eg google.com also it only can use c string that ends in a 0
    if (rc != 0) { // check if no internet or typo in hostname
        cerr << "Resolve failed: " << gai_strerror(rc) << "\n";
        return 1;
    }
    sockaddr_in base = *(sockaddr_in*)res->ai_addr; // wskaźnik typu sockaddr* ponieważ wcześniej określiliśmy AF_INET (IPv4) rzutujemy na sockaddr_in* i pobieramy warość do zmiennej base
    freeaddrinfo(res); // zwalnia pamięć przydzieloną dla struktury res

    // atomic == single step
    vector<PortState> results(ports.size(),PortState::Error);
    atomic<size_t> nextIndex{0};

    auto worker = [&](){
        while (true) {
            size_t i = nextIndex.fetch_add(1);// fetch add = 1 operation on cpu
            if (i >= ports.size()) break;
            results[i] = scanPort(base,ports[i],timeoutMs);
        }
    };
    // worker is just what each thread do
    size_t n = min(static_cast<size_t>(num_threads),ports.size()); // optimalization so we dont need to create more threads then in need
    vector<thread> threads;
    for (size_t t = 0; t < n ; ++t) {
        threads.emplace_back(worker); // create worker
    }
    for (auto& th : threads) {
        th.join();// wait for all to end
    }
    for (size_t i = 0; i < ports.size() ; ++i) {
        if (results[i] == PortState::Open) {
            cout << "port " << ports[i] << " is OPEN\n";
        }
    }






    // for (uint16_t port: ports) {
    //     if (scanPort(base, port, timeoutMs) == PortState::Open) {
    //         cout << "port " << port << " is OPEN\n";
    //     }
    // }
    cout << "END OF PORTS\n";


    return 0;
}


// Executed in   18.73 secs      fish           external
//    usr time    4.62 millis    0.00 micros    4.62 millis
//    sys time    6.89 millis  877.00 micros    6.02 millis
//
// Executed in   99.30 secs      fish           external
//    usr time   10.01 millis    0.38 millis    9.64 millis
//    sys time   15.08 millis    1.16 millis   13.92 millis

// for (int port = 1; port <= 1024; ++port) {
//     if (scanPort(base, port, 1000) == PortState::Open) {
//         cout << "port " << port << " is OPEN\n";
//     }
// }
//
//
//
// -t = timeout -p = ports
// if (argc != 2)
// {
//     cerr << "Usage: " << argv[0] << " <host>\n";
//     return 1;
// }
// vector<uint16_t> aa =  parsePorts("10,20,30-40,100-1000");
// for (int i = 0 ; i < aa.size() ; i++) {
//     cout << aa[i] << " ";
// }

// if (argc != 3) {
//     cerr << "Usage: " << argv[0] << " <host> <port>\n";
//     return 1;
// }
// const char* host = argv[1];
// const char* port = argv[2];
// // argc  = number of command-line arguments // argc   0 = program name 1 = ip/website 2 = port num
// addrinfo hints{}; // addrinfo = to typ który przchowuje teczke informacji o danym hoscie a hnints mówi co my tylko chcemy {} resetuje całą pamięć bez tego będzie miała losowe śmieci z pamięci bo nie bedzie zainicjonowana
// hints.ai_family = AF_UNSPEC; // either ipv4 ipv6
// hints.ai_socktype = SOCK_STREAM; // tcp udp is sock_dgram
// addrinfo* result = nullptr; // pointer that shoud fill in addrinfo
// int rc = getaddrinfo(host, port, &hints, &result); // we check what ip is behind eg google.com
// if (rc != 0) {// we check if no internet and typo in hostname
//     cerr << "Resolve failed: " << gai_strerror(rc) << "\n";
//     return 1;
// }
// bool open = false;
// int lastError = 0;
// for (addrinfo* p = result ; p != nullptr ; p = p->ai_next) { // a hostname can be resolved to severall adresses result is head of linked list
//     int fd = socket(p->ai_family,p->ai_socktype,p->ai_protocol);// creates socket for example 192.168.1.1:80
//     if (fd == -1) continue;
//     if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) {// jeżeli połączenie działa
//         open = true;
//     } else {
//         lastError = errno; // errno = numer ostatniego błędu przechowywanego przez biblioteke standardową albo wywyołanie systemowe
//     }
//     close(fd);
//     if (open) break;
// }
// freeaddrinfo(result);
// if (open) {
//     cout << host << ":" << port << "is OPEN\n";
// } else {
//     cout << host << ":" << port << " is CLOSED (" << strerror(lastError) << ")\n";
// }
// class portQueue
// {
//     private: queue<uint16_t> ports;
//     mutex mtx; // mutex is just like a lock in a toilet only one person(thread) can get in and not everyone to the same toilet
//
//     public:
//     void push(int port)
//     {
//         lock_guard<mutex> lock(mtx);// closes the door from the inside (nothing can do anything else)
//         ports.push(port);
//     }// automaticly onlocks the door
//     int pop()
//     {
//         lock_guard<mutex> lock(mtx);
//         if (ports.empty()) return -1;
//         int port = ports.front();
//         ports.pop();
//         return port;
//     }
// };