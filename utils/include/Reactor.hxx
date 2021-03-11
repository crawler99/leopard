#ifndef LEOPARD_UTILS_REACTOR_HXX_
#define LEOPARD_UTILS_REACTOR_HXX_

#include <sys/epoll.h>
#include <atomic>

namespace leopard { namespace utils {

class EventHandlerInterface
{
public:
    using HandlerFunc = std::function<void(void*)>;
    using ErrorFunc = std::function<void(void*)>;

    virtual HandlerFunc GetHandlerFunc() = 0;
    virtual ErrorFunc GetErrorFunc() = 0;
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

    bool AddFd(int fd, uint32_t events, EventHandlerInterface *handler)
    {
        struct epoll_event ep_event;
        ep_event.data.fd = fd;
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
        else if (number > 0)
        {
            HandleEvents(_events, number);
        }
    }

private:
    void HandleEvents(struct epoll_event *events, int number)
    {
        for (int i = 0; i < number; i++)
        {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLRDHUP))
            {
                std::cout << "fd Error: fd=" << events[i].data.fd << std::endl;
                static_cast<EventHandlerInterface*>(events[i].data.ptr)->GetErrorFunc()(&events[i].data.fd);
                if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr) == -1)
                {
                    std::cout << "Failed to remove fd from aggregator: fd=" << events[i].data.fd << ", errno=" << errno << ", err_text=" << strerror(errno) << std::endl;
                }
                // Don't close fd here, leaving it to upper level.
            }
            else if (events[i].events & EPOLLIN)
            {
                static_cast<EventHandlerInterface*>(events[i].data.ptr)->GetHandlerFunc()(&events[i].data.fd);
            }
        }
    }

    int _epoll_fd{0};
    struct epoll_event _events[MAX_EPOLL_EVENTS];
};

template <typename... Aggregators>
class Reactor : private NonCopyable, public Aggregators...
{
public:
    void Run()
    {
        while (!_stop.load(std::memory_order::memory_order_acquire))
        {
            (static_cast<void>(Aggregators::CollectEvents()), ...);
        }
    }

    void Stop()
    {
        _stop.store(true, std::memory_order_release);
    }

private:
    std::atomic<bool> _stop{false};
};

}} // namespace utils::leopard

#endif // LEOPARD_UTILS_REACTOR_HXX_