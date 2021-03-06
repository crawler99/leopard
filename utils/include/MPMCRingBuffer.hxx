#ifndef LEOPARD_UTILS_MPMCRINGBUFFER_HXX_
#define LEOPARD_UTILS_MPMCRINGBUFFER_HXX_

#include <Common.hxx>
#include <Math.hxx>

#include <atomic>
#include <mutex>
#include <vector>

namespace leopard { namespace utils {

template <typename T>
class MPMCRingBuffer
{
public:
    /**
    * Initialize the ring-buffer to a certain capacity (num of slots) based on a suggested value.
    *
    * @param capacity Suggested ring-buffer capacity. It will be rounded to the power of 2 upwards to apply.
    * @return True if the ring-buffer is initialized successfully, false if it has already been initialized.
    */
    bool Init(std::size_t capacity)
    {
        std::size_t expected = 0;
        auto actual_capacity = Math::NextPowerOf2(capacity);

        if (!_capacity.compare_exchange_strong(expected, actual_capacity))
        {
            std::cout << "Buffer already initialized: capacity=" << _capacity.load(std::memory_order_acquire);
            return false;
        }

        _buffer.resize(actual_capacity);
        std::cout << "Buffer initialized: capacity=" << _capacity.load(std::memory_order_acquire);
        return true;
    }

    /**
    * Get the ring-buffer capacity.
    *
    * @return Current capcacity of the ring-buffer.
    */
    std::size_t GetCapacity()
    {
        return _capacity.load(std::memory_order_acquire);
    }

    /**
    * Get current message count in the ring-buffer.
    *
    * @return Current message count in the ring-buffer.
    */
    std::size_t GetMessageNumber()
    {
        return _write_commit_count.load(std::memory_order_acquire) - _read_commit_count.load(std::memory_order_acquire);
    }

    /**
    * Try to get a ring-buffer slot for equeuing a message.
    *
    * @return Pointer to the available ring-buffer slot if the queue is not full, nullptr otherwise.
    */
    T* GetMessageForWrite()
    {
        auto write_ctr_snapshot = _write_reserve_count.load(std::memory_order_acquire);
        auto read_ctr_snapshot = _read_commit_count.load(std::memory_order_acquire);

        if (UNLIKELY(IsFull(write_ctr_snapshot, read_ctr_snapshot)))
        {
            return nullptr;
        }

        while (!_write_reserve_count.compare_exchange_weak(write_ctr_snapshot, write_ctr_snapshot + 1, std::memory_order_release))
        {
            // Don't check against the latest _read_commit_count whether the buff is full as we want to
            // fail fast here and let the caller to decide the retry policy.
            if (UNLIKELY(IsFull(write_ctr_snapshot, read_ctr_snapshot)))
            {
                return nullptr;
            }
        }

        T *rst = &_buffer[GetPos(write_ctr_snapshot)];
        rst->_seq = write_ctr_snapshot;
        rst->_data_pointers.resize(0); // No memory cost for most of std containers if std::is_trivially_destructible<T>::value is true.
        return rst;
    }

    /**
    * Commit enqueuing a message to the ring-buffer.
    *
    * @param msg Pointer of the committed message.
    */
    void CommitMessageWrite(const T *msg)
    {
        while (_write_commit_count.load(std::memory_order_acquire) < msg->_seq) {}
        _write_commit_count.fetch_add(1, std::memory_order_release);
    }

    /**
    * Try to read a message from the ring-buffer before dequeuing.
    *
    * @return Pointer to the ring-buffer tail slot if the queue is not empty, nullptr otherwise.
    */
    const T* GetMessageForRead()
    {
        auto write_ctr_snapshot = _write_commit_count.load(std::memory_order_acquire);
        auto read_ctr_snapshot = _read_reserve_count.load(std::memory_order_acquire);

        if (IsEmpty(write_ctr_snapshot, read_ctr_snapshot))
        {
            return nullptr;
        }

        while (!_read_reserve_count.compare_exchange_weak(read_ctr_snapshot, read_ctr_snapshot + 1, std::memory_order_release))
        {
            // Don't check against the latest _write_commit_count whether the buff is empty as we want to
            // fail fast here and let the caller to decide the retry policy.
            if (IsEmpty(write_ctr_snapshot, read_ctr_snapshot))
            {
                return nullptr;
            }
        }

        return &_buffer[GetPos(read_ctr_snapshot)];
    }

    /**
    * Commit dequeuing a message from the ring-buffer.
    *
    * @param msg Pointer of the committed message.
    */
    void CommitMessageRead(const T *msg)
    {
        while (_read_commit_count.load(std::memory_order_acquire) < msg->_seq) {}
        _read_commit_count.fetch_add(1, std::memory_order_release);
    }

private:
    ALWAYS_INLINE std::size_t GetPos(std::size_t count)
    {
        return count & (_capacity.load(std::memory_order_acquire) - 1);
    }

    ALWAYS_INLINE bool IsFull(std::size_t writeCtr, std::size_t readCtr)
    {
        // Note: The caller of this function may pass in writeCtr and readCtr that
        // are not fetched at the same moment. So we need to use ">=" rather than "=="
        // to do the check.
        return writeCtr - readCtr >= _capacity.load(std::memory_order_acquire);
    }

    ALWAYS_INLINE bool IsEmpty(std::size_t writeCtr, std::size_t readCtr)
    {
        // Note: The caller of this function may pass in writeCtr and readCtr that
        // are not fetched at the same moment. So we need to use ">=" rather than "=="
        // to do the check.
        return readCtr >= writeCtr;
    }

    std::vector<T> _buffer;
    std::atomic<std::size_t> _capacity{0};
    std::atomic<std::size_t> _write_reserve_count{0};
    std::atomic<std::size_t> _write_commit_count{0};
    std::atomic<std::size_t> _read_reserve_count{0};
    std::atomic<std::size_t> _read_commit_count{0};
};

}} // namespace utils::leopard

#endif // LEOPARD_UTILS_MPMCRINGBUFFER_HXX_