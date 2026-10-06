#include <iostream>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
using namespace std;
int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <host> <port>\n";
        return 1;
    }
    const char* host = argv[1];
    const char* port = argv[2];
    // argc  = number of command-line arguments // argc   0 = program name 1 = ip/website 2 = port num
    addrinfo hints{}; // addrinfo = to typ który przchowuje teczke informacji o danym hoscie a hnints mówi co my tylko chcemy {} resetuje całą pamięć bez tego będzie miała losowe śmieci z pamięci bo nie bedzie zainicjonowana
    hints.ai_family = AF_UNSPEC; // either ipv4 ipv6
    hints.ai_socktype = SOCK_STREAM; // tcp udp is sock_dgram
    addrinfo* result = nullptr; // pointer that shoud fill in addrinfo
    int rc = getaddrinfo(host, port, &hints, &result); // we check what ip is behind eg google.com
    if (rc != 0) {// we check if no internet and typo in hostname
        cerr << "Resolve failed: " << gai_strerror(rc) << "\n";
        return 1;
    }
    bool open = false;
    int lastError = 0;
    for (addrinfo* p = result ; p != nullptr ; p = p->ai_next) { // a hostname can be resolved to severall adresses result is head of linked list
        int fd = socket(p->ai_family,p->ai_socktype,p->ai_protocol);// creates socket for example 192.168.1.1:80
        if (fd == -1) continue;
        if (connect(fd, p->ai_addr, p->ai_addrlen) == 0) {// jeżeli połączenie działa
            open = true;
        } else {
            lastError = errno; // errno = numer ostatniego błędu przechowywanego przez biblioteke standardową albo wywyołanie systemowe
        }
        close(fd);
        if (open) break;
    }
    freeaddrinfo(result);
    if (open) {
        cout << host << ":" << port << "is OPEN\n";
    } else {
        cout << host << ":" << port << " is CLOSED (" << strerror(lastError) << ")\n";
    }
    return 0;
}
