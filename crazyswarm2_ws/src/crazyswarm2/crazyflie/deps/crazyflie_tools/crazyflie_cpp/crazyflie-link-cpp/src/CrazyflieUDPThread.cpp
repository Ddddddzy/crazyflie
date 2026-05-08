#include <iostream>
#include <cstring>
#include <stdexcept>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "CrazyflieUDPThread.h"
#include "ConnectionImpl.h"

namespace bitcraze {
namespace crazyflieLinkCpp {

CrazyflieUDPThread::CrazyflieUDPThread(const std::string& hostname, uint16_t port)
    : hostname_(hostname)
    , port_(port)
    , thread_ending_(false)
{
}

CrazyflieUDPThread::CrazyflieUDPThread(CrazyflieUDPThread &&other)
{
    const std::lock_guard<std::mutex> lk(other.thread_mutex_);
    hostname_ = std::move(other.hostname_);
    port_ = other.port_;
    thread_ = std::move(other.thread_);
    thread_ending_ = other.thread_ending_;
    connection_ = std::move(other.connection_);
    runtime_error_ = std::move(other.runtime_error_);
}

CrazyflieUDPThread::~CrazyflieUDPThread()
{
    const std::lock_guard<std::mutex> lock(thread_mutex_);
    if (thread_.joinable()) {
        thread_.join();
    }
}

void CrazyflieUDPThread::addConnection(std::shared_ptr<ConnectionImpl> con)
{
    const std::lock_guard<std::mutex> lock(thread_mutex_);
    if (!thread_.joinable() && !connection_) {
        connection_ = con;
        thread_ = std::thread(&CrazyflieUDPThread::runWithErrorHandler, this);
    } else {
        throw std::runtime_error("Cannot operate more than one connection over UDP!");
    }
}

void CrazyflieUDPThread::removeConnection(std::shared_ptr<ConnectionImpl> con)
{
    if (connection_ != con) {
        throw std::runtime_error("Connection does not belong to this thread!");
    }

    const std::lock_guard<std::mutex> lock(thread_mutex_);
    thread_ending_ = true;
    thread_.join();
    thread_ = std::thread();
    thread_ending_ = false;
    connection_.reset();
}

void CrazyflieUDPThread::runWithErrorHandler()
{
    try {
        run();
    }
    catch (const std::runtime_error &error) {
        connection_->runtime_error_ = error.what();
        runtime_error_ = error.what();
    }
    catch (...) {
    }
}

void CrazyflieUDPThread::run()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        throw std::runtime_error("Failed to create UDP socket");
    }

    // Set receive timeout (1 ms)
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Resolve destination address
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, hostname_.c_str(), &dest_addr.sin_addr) <= 0) {
        close(sockfd);
        throw std::runtime_error("Invalid UDP address: " + hostname_);
    }

    while (!thread_ending_)
    {
        std::this_thread::yield();

        // Send
        {
            const std::lock_guard<std::mutex> lock(connection_->queue_send_mutex_);
            if (!connection_->queue_send_.empty())
            {
                Packet p_send = connection_->queue_send_.top();
                ssize_t sent = sendto(sockfd, p_send.raw(), p_send.size(), 0,
                    (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                if (sent > 0) {
                    ++connection_->statistics_.sent_count;
                    ++connection_->statistics_.ack_count;
                    connection_->queue_send_.pop();
                    --connection_->statistics_.enqueued_count;
                }
            }
        }

        // Receive
        Packet p_recv;
        ssize_t n = recvfrom(sockfd, p_recv.raw(), CRTP_MAXSIZE, 0, nullptr, nullptr);
        if (n > 0) {
            p_recv.setSize(static_cast<size_t>(n));
            {
                const std::lock_guard<std::mutex> lock(connection_->queue_recv_mutex_);
                p_recv.seq_ = connection_->statistics_.receive_count;
                connection_->queue_recv_.push(p_recv);
                ++connection_->statistics_.receive_count;
            }
            connection_->queue_recv_cv_.notify_one();
        }
    }

    ::close(sockfd);
}

} // namespace crazyflieLinkCpp
} // namespace bitcraze
