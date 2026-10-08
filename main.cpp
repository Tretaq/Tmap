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
#include <complex>
#include <stdexcept>
using namespace std;


// g++ main.cpp -o Tmap
enum class PortState { Open, Closed, Filtered, Error };

int toInt(const string& s) { // no copy no changes ig
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

PortState scanPort(const sockaddr_in& base, uint16_t port, int timeoutMS) { // sockaddr_in& base host addres
    sockaddr_in addr = base;
    addr.sin_port = htons(port);
    int fd = socket(AF_INET, SOCK_STREAM, 0);// AF_INET = IPv4 SOCK_STREAM = TCP 0 = default protocol
    // create a socket
    if (fd == -1) return PortState::Error;

    int flags = fcntl(fd, F_GETFL, 0); // we take the setting (flags) for this socket and in nex we do new ones
    fcntl(fd, F_SETFL, flags | O_NONBLOCK); // normally connect() and recv() stop running program till end of op after O_NONBLOCK they stop so if no conn there is ther will be an error nad prograam can do smth else
    // it returns immidiently and dont wait for handshake
    PortState result = PortState::Error;

    int rc = connect(fd, (sockaddr*)&addr, sizeof(addr)); //

    if (rc == 0) {
        result = PortState::Open;
    } else if (errno == ECONNREFUSED) {
        result = PortState::Closed;
    } else if ( errno == EINPROGRESS) { // the things before this  are checking mostly on local network where it can con or err instantly
        pollfd pfd{};
        pfd.fd = fd;
        pfd.events = POLLOUT;

        int r = poll(&pfd, 1 , timeoutMS);// check the response
        if (r == 0) {
            result = PortState::Filtered;
        } else if (r > 0){
            int err = 0;
            socklen_t len = sizeof(err);
            if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0) {
                if (err == 0) result = PortState::Open;
                else if (err == ECONNREFUSED) result = PortState::Closed;
            }
        }
    }
    close(fd);
    return result;
}


int main(int argc, char* argv[]) {
    string portSpec = "1-1024";
    int timeoutMs = 1000;
    string host;
    vector<uint16_t> ports;

    try {
        for (int i = 1 ; i < argc; ++i) {
            string argument = argv[i];
            if (argument == "-p" || argument == "-t") {
                if (i + 1 >= argc) {
                    throw invalid_argument(argument + " needs a value");
                }
                string value = argv[++i];
                if (argument == "-p") {
                    portSpec = value;
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
        if (timeoutMs < 1) throw invalid_argument("timeout must be at least 1 ms");
        ports = parsePorts(portSpec);
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n" << "Usage: " << argv[0] << " -p ports -t timeout_ms <host> \n";
        return 1;
    }
    // -t = timeout -p = ports
    // if (argc != 2)
    // {
    //     cerr << "Usage: " << argv[0] << " <host>\n";
    //     return 1;
    // }
    addrinfo hints{}; // addrinfo = to typ który przchowuje teczke informacji o danym hoscie a hnints mówi co my tylko chcemy {} resetuje całą pamięć bez tego będzie miała losowe śmieci z pamięci bo nie bedzie zainicjonowana
    hints.ai_family = AF_INET; // only ipv4
    hints.ai_socktype = SOCK_STREAM; // tcp udp is sock_dgram

    addrinfo* res = nullptr; // pointer that shoud fill in addrinfo
    int rc = getaddrinfo(host.c_str(), nullptr, &hints, &res); // we check what ip is behind eg google.com also it only can use c string that ends in a 0
    if (rc != 0) { // check if no internet or typo in hostname
        cerr << "Resolve failed: " << gai_strerror(rc) << "\n";
        return 1;
    }
    sockaddr_in base = *(sockaddr_in*)res->ai_addr; // wskaźnik typu sockaddr* ponieważ wcześniej określiliśmy AF_INET (IPv4) rzutujemy na sockaddr_in* i pobieramy warość do zmiennej base
    freeaddrinfo(res); // zwalnia pamięć przydzieloną dla struktury res
    for (uint16_t port: ports) {
        if (scanPort(base, port, timeoutMs) == PortState::Open) {
            cout << "port " << port << " is OPEN\n";
        }
    }
    cout << "END OF PORTS\n";
    // for (int port = 1; port <= 1024; ++port) {
    //     if (scanPort(base, port, 1000) == PortState::Open) {
    //         cout << "port " << port << " is OPEN\n";
    //     }
    // }
    //

    return 0;
}




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