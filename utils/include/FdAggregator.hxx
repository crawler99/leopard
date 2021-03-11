#ifndef LEOPARD_UTILS_FDAGGREGATOR_HXX_
#define LEOPARD_UTILS_FDAGGREGATOR_HXX_

#include <sys/epoll.h>
#include <atomic>
#include <functional>

namespace leopard { namespace utils {

class FdEventHandler
{
public:
    using HandlerFunc = std::function<void()>;
    using ErrorFunc = std::function<void()>;

    FdEventHandler(int fd) : _fd(fd)
    {
    }

    int GetFd()
    {
        return _fd;
    }

    virtual HandlerFunc GetHandlerFunc() = 0;
    virtual ErrorFunc GetErrorFunc() = 0;

private:
    int _fd{0};
};

class FdAggregator
{
public:
    static constexpr uint32_t MAX_EPOLL_EVENTS{1024};

    FdAggregator()
    {
        _epoll_fd = epoll_create1(0);
        if (_epoll_fd == -1)
        {
            throw std::system_error(errno, std::system_category(), std::string("Failed to initialize FdAggregator: ").append(strerror(errno)).c_str());
        }
    }

    bool AddFd(int fd, uint32_t events, FdEventHandler *handler)
    {
        struct epoll_event ep_event;
        ep_event.data.ptr = handler;
        ep_event.events = events;
        if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &ep_event) == -1)
        {
            std::cout << "Failed to add fd to aggregator: fd=" << fd << ", errno=" << errno << ", err_text=" << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }

protected:
    void CollectEvents()
    {
        int number = epoll_wait(_epoll_fd, _events, MAX_EPOLL_EVENTS, 0);
        if (number == -1)
        {
            throw std::system_error(errno, std::system_category(), std::string("epoll_wait() error: ").append(strerror(errno)).c_str());
        }
        else
        {
            HandleEvents(_events, number);
        }
    }

private:
    void HandleEvents(struct epoll_event *events, int number)
    {
        for (int i = 0; i < number; ++i)
        {
            auto *handler = static_cast<FdEventHandler*>(events[i].data.ptr);

            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLRDHUP))
            {
                if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, handler->GetFd(), nullptr) == -1)
                {
                    std::cout << "Failed to remove fd from aggregator: fd=" << handler->GetFd() << ", errno=" << errno << ", err_text=" << strerror(errno) << std::endl;
                }
                else
                {
                    std::cout << "Detected error when polling fd, removed it from aggregator: fd=" << handler->GetFd() << std::endl;
                }
                handler->GetErrorFunc()();
                // Don't close fd here, leaving it to upper level.
            }
            else if (events[i].events & EPOLLIN)
            {
                handler->GetHandlerFunc()();
            }
        }
    }

    int _epoll_fd{0};
    struct epoll_event _events[MAX_EPOLL_EVENTS];
};

}} // namespace utils::leopard

#endif // LEOPARD_UTILS_FDAGGREGATOR_HXX_