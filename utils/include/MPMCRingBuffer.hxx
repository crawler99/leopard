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
        auto actualCapacity = Math::NextPowerOf2(capacity);

        if (!mCapacity.compare_exchange_strong(expected, actualCapacity))
        {
            std::cout << "Buffer already initialized: capacity=" << mCapacity.load(std::memory_order_acquire);
            return false;
        }

        mBuffer.resize(actualCapacity);
        std::cout << "Buffer initialized: capacity=" << mCapacity.load(std::memory_order_acquire);
        return true;
    }

    /**
    * Get the ring-buffer capacity.
    *
    * @return Current capcacity of the ring-buffer.
    */
    std::size_t GetCapacity()
    {
        return mCapacity.load(std::memory_order_acquire);
    }

    /**
    * Get current message count in the ring-buffer.
    *
    * @return Current message count in the ring-buffer.
    */
    std::size_t GetMessageNumber()
    {
        return mWriteCommitCount.load(std::memory_order_acquire) - mReadCommitCount.load(std::memory_order_acquire);
    }

    /**
    * Try to get a ring-buffer slot for equeuing a message.
    *
    * @return Pointer to the available ring-buffer slot if the queue is not full, nullptr otherwise.
    */
    T* GetMessageForWrite()
    {
        auto writeCtrSnapshot = mWriteReserveCount.load(std::memory_order_acquire);
        auto readCtrSnapshot = mReadCommitCount.load(std::memory_order_acquire);

        if (UNLIKELY(IsFull(writeCtrSnapshot, readCtrSnapshot)))
        {
            return nullptr;
        }

        while (!mWriteReserveCount.compare_exchange_weak(writeCtrSnapshot, writeCtrSnapshot + 1, std::memory_order_release))
        {
            // Don't check against the latest mReadCommitCount whether the buff is full as we want to
            // fail fast here and let the caller to decide the retry policy.
            if (UNLIKELY(IsFull(writeCtrSnapshot, readCtrSnapshot)))
            {
                return nullptr;
            }
        }

        T *rst = &mBuffer[GetPos(writeCtrSnapshot)];
        rst->mSeq = writeCtrSnapshot;
        rst->mDataPointers.resize(0); // No memory cost
        return rst;
    }

    /**
    * Commit enqueuing a message to the ring-buffer.
    *
    * @param msg Pointer of the committed message.
    */
    void CommitMessageWrite(const T *msg)
    {
        while (mWriteCommitCount.load(std::memory_order_acquire) < msg->mSeq) {}
        mWriteCommitCount.fetch_add(1, std::memory_order_release);
    }

    /**
    * Try to read a message from the ring-buffer before dequeuing.
    *
    * @return Pointer to the ring-buffer tail slot if the queue is not empty, nullptr otherwise.
    */
    const T* GetMessageForRead()
    {
        auto writeCtrSnapshot = mWriteCommitCount.load(std::memory_order_acquire);
        auto readCtrSnapshot = mReadReserveCount.load(std::memory_order_acquire);

        if (IsEmpty(writeCtrSnapshot, readCtrSnapshot))
        {
            return nullptr;
        }

        while (!mReadReserveCount.compare_exchange_weak(readCtrSnapshot, readCtrSnapshot + 1, std::memory_order_release))
        {
            // Don't check against the latest mWriteCommitCount whether the buff is empty as we want to
            // fail fast here and let the caller to decide the retry policy.
            if (IsEmpty(writeCtrSnapshot, readCtrSnapshot))
            {
                return nullptr;
            }
        }

        return &mBuffer[GetPos(readCtrSnapshot)];
    }

    /**
    * Commit dequeuing a message from the ring-buffer.
    *
    * @param msg Pointer of the committed message.
    */
    void CommitMessageRead(const T *msg)
    {
        while (mReadCommitCount.load(std::memory_order_acquire) < msg->mSeq) {}
        mReadCommitCount.fetch_add(1, std::memory_order_release);
    }

private:
    ALWAYS_INLINE std::size_t GetPos(std::size_t count)
    {
        return count & (mCapacity.load(std::memory_order_acquire) - 1);
    }

    ALWAYS_INLINE bool IsFull(std::size_t writeCtr, std::size_t readCtr)
    {
        // Note: The caller of this function may pass in writeCtr and readCtr that
        // are not fetched at the same moment. So we need to use ">=" rather than "=="
        // to do the check.
        return writeCtr - readCtr >= mCapacity.load(std::memory_order_acquire);
    }

    ALWAYS_INLINE bool IsEmpty(std::size_t writeCtr, std::size_t readCtr)
    {
        // Note: The caller of this function may pass in writeCtr and readCtr that
        // are not fetched at the same moment. So we need to use ">=" rather than "=="
        // to do the check.
        return readCtr >= writeCtr;
    }

    std::vector<T> mBuffer;
    std::atomic<std::size_t> mCapacity{0};
    std::atomic<std::size_t> mWriteReserveCount{0};
    std::atomic<std::size_t> mWriteCommitCount{0};
    std::atomic<std::size_t> mReadReserveCount{0};
    std::atomic<std::size_t> mReadCommitCount{0};
};

}} // namespace utils::leopard

#endif // LEOPARD_UTILS_MPMCRINGBUFFER_HXX_