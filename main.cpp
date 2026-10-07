#include <iostream>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdint>
#include <fcntl.h>// flags F_GETFL, F_SETFL, O_NONBLOCK
#include <poll.h>// pollfd POLLOUT
using namespace std;

enum class PortState { Open, Closed, Filtered, Error };

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

        int r = poll(&pfd, 1 , timeoutMS);
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
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <host>\n";
    }
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
    return 0;
}
