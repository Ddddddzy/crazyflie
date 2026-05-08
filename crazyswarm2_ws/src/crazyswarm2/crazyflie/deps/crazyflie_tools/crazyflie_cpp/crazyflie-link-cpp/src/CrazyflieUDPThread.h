#pragma once

#include <thread>
#include <mutex>
#include <string>
#include <memory>

namespace bitcraze {
namespace crazyflieLinkCpp {

// forward declaration
class ConnectionImpl;

class CrazyflieUDPThread
{
public:
    CrazyflieUDPThread(const std::string& hostname, uint16_t port);
    CrazyflieUDPThread(CrazyflieUDPThread &&other);
    ~CrazyflieUDPThread();

    // Non-copyable
    CrazyflieUDPThread(const CrazyflieUDPThread&) = delete;
    CrazyflieUDPThread& operator=(const CrazyflieUDPThread&) = delete;

    bool hasError() const {
        return !runtime_error_.empty();
    }

private:
    friend class Connection;

    void runWithErrorHandler();
    void run();

    void addConnection(std::shared_ptr<ConnectionImpl> con);
    void removeConnection(std::shared_ptr<ConnectionImpl> con);

private:
    std::string hostname_;
    uint16_t port_;

    std::mutex thread_mutex_;
    std::thread thread_;
    bool thread_ending_;

    std::shared_ptr<ConnectionImpl> connection_;
    std::string runtime_error_;
};

} // namespace crazyflieLinkCpp
} // namespace bitcraze
