# Tmap

Tmap is a multithreaded port scaner written in C++17 for Linux. 

## Features

- TCP connect scan with non-blocking sockets and a per-port timeout
- Port lists and ranges (`-p 22,80,8000-8100`)
- Worker threads for fast scans (`-T`)
- Optionall banner grabbing and service guessing (`-b`)
- Summary with scan time and open port count

## Build

You need C++17 compiler and Cmake 

```bash
git clone https://github.com/Tretaq/Tmap
cd Tmap
cmake -B build
cmake --build build
./build/Tmap -h
```
## Usage 

```bash
Tmap [-p ports] [-t timeout_ms] [-T threads] [-b] [-v] <host>
```

| Flag | Meaning | Default |
|------|---------|---------|
| `-p` | ports, e.g. `22,80,8000-8100` | `1-1024` |
| `-t` | timeout per port in ms | `1000` |
| `-T` | number of threads | `100` |
| `-b` | grab banners | off |
| `-v` | also show closed/filtered ports | off |
| `-help` | show functions | off |

### Examples

```
Tmap -p 22,80,443 scanme.nmap.org
Tmap -b -p 1-1024 -t 500 localhost
```

## Example output

```
./Tmap -p 22,80,443 scanme.nmap.org              
port 22 is Open 
port 80 is Open 
Scanned 3 ports in 0.344559 sec, 2 open
```

## Legal note 

Don't be a jerk and hammer shit.
Also only scan ports you have permission to.
![alt text](image.png)