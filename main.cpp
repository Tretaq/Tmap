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
    int rc = getaddrinfo(host, port, &hints, &result);
    if (rc != 0) {// swe check if no internet and typo in hostname
        cerr << "Resolve failed: " << gai_strerror(rc) << "\n";
        return 1;
    }



    return 0;
}
