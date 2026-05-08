# UDP Transport Support for CRTP

## Overview

UDP communication support has been added to the crazyflie-link-cpp library, enabling CRTP packet exchange over UDP sockets. This is intended for connecting to simulated Crazyflies (e.g., CrazySim SITL instances) where each drone listens on a unique UDP port.

## URI Format

```
udp://hostname:port
```

Port range for CrazySim: 19850-19949

### Examples

```cpp
Crazyflie cf0("udp://127.0.0.1:19850");  // first simulated Crazyflie
Crazyflie cf1("udp://127.0.0.1:19851");  // second simulated Crazyflie
```

## Architecture

The UDP transport bypasses the `USBManager` singleton (which handles USB device enumeration) and is managed directly by the `Connection` class. A dedicated `CrazyflieUDPThread` runs a background send/receive loop for each connection, following the same pattern as `CrazyflieUSBThread`.

```
Crazyflie API
    |
Connection (parses udp:// URI)
    |
CrazyflieUDPThread (background thread)
    |
POSIX UDP socket (sendto / recvfrom)
```

## Protocol Details

- CRTP packets (up to 32 bytes) are sent as raw UDP datagrams
- Responses are received as UDP datagrams and enqueued for the application
- No safelink protocol (unnecessary for simulation)
- No radio scanning (user specifies address directly)
- Receive timeout: 1ms (ensures responsive thread shutdown)

## Files Changed

### New files (in `crazyflie_cpp/crazyflie-link-cpp/src/`)

| File | Description |
|------|-------------|
| `CrazyflieUDPThread.h` | Thread class header |
| `CrazyflieUDPThread.cpp` | UDP socket send/receive loop implementation |

### Modified files

| File | Change |
|------|--------|
| `crazyflie-link-cpp/src/ConnectionImpl.h` | Added `isUDP_`, `hostname_`, `port_` fields |
| `crazyflie-link-cpp/include/crazyflieLinkCpp/Connection.h` | Forward decl + `udpThread_` member |
| `crazyflie-link-cpp/src/Connection.cpp` | URI parsing for `udp://`, lifecycle management |
| `crazyflie-link-cpp/include/crazyflieLinkCpp/Packet.hpp` | `CrazyflieUDPThread` friend access |
| `crazyflie-link-cpp/CMakeLists.txt` | Added `CrazyflieUDPThread.cpp` to sources |

## Building

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

No additional dependencies are required — POSIX sockets are part of the standard C library on Linux/macOS.

## Limitations

- POSIX-only (Linux/macOS). Windows would require Winsock2 adaptations.
- No hostname resolution — the hostname field must be a valid IPv4 address (e.g., `127.0.0.1`).
- `Connection::scan()` does not discover UDP endpoints; the user must specify the URI directly.
- `broadcastUri()` and `address()` return empty/invalid for UDP connections (no radio address concept).
