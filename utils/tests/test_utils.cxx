#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Singleton.hxx"
#include "Event.hxx"
#include "ThreadRAII.hxx"
#include "MPMCRingBuffer.hxx"

#include <functional>
#include <map>
#include <vector>
#include <chrono>
#include <thread>

TEST(Singleton, correctness)
{
    class A
    {
      public:
        A(const std::string &str)
        {
            std::cout << "Constructing A with L-Value Ref." << std::endl;
        }

        A(std::string &&str)
        {
            std::cout << "Constructing A with R-Value Ref." << std::endl;
        }
    };

    class B
    {
      public:
        B(const std::string &str)
        {
            std::cout << "Constructing B with L-Value Ref." << std::endl;
        }

        B(std::string &&str)
        {
            std::cout << "Constructing B with R-Value Ref." << std::endl;
        }
    };

    class C
    {
      public:
        C(int x, double y)
        {
            std::cout << "Constructing C with L-Value." << std::endl;
        }

        void Print()
        {
            std::cout << "Hello, here is a test function in C." << std::endl;
        }
    };

    std::string str{"test"};
    leopard::utils::Singleton<A>::Instance(str);
    leopard::utils::Singleton<B>::Instance(std::move(str));

    try
    {
        leopard::utils::Singleton<C>::GetInstance()->Print();
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }

    leopard::utils::Singleton<C>::Instance(1, 3.14);
    leopard::utils::Singleton<C>::GetInstance()->Print();

    try
    {
        leopard::utils::Singleton<C>::Instance(2, 3.14);
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }
}

void Print(int a, int b)
{
    std::cout << "GlobalFunc => a: " << a << ", b: " << b << std::endl;
}

TEST(Event, correctness)
{
    class A
    {
      public:
        void SetA(int a)
        {
            _a = a;
        }

        void SetB(int b)
        {
            _b = b;
        }

        void PrintSelf()
        {
            std::cout << "MemberFunc => _a: " << _a << ", _b: " << _b << std::endl;
        }

        void Print(int a, int b)
        {
            std::cout << "MemberFunc => a: " << a << ", b: " << b << std::endl;
        }

      private:
        int _a = {0};
        int _b = {0};
    };

    leopard::utils::Event<std::function<void(int, int)>> event;
    auto key = event.Connect(Print);

    A inst_a;
    auto lambda_key = event.Connect([&inst_a](int a, int b) { inst_a.SetA(a); inst_a.SetB(b); });

    auto f = std::bind(&A::Print, &inst_a, std::placeholders::_1, std::placeholders::_2);
    auto mem_func_key = event.Connect(f);

    // Notify observers
    int a = 1, b = 2;
    event.Notify(a, b);
    inst_a.PrintSelf();

    event.Disconnect(key);
    event.Disconnect(lambda_key);
    a = 3;
    b = 4;
    event.Notify(a, b);
}

bool Filter(int n)
{
    bool good = (n > 10);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Filter: " << n << " is " << (good ? " good" : " bad") << std::endl;
    return good;
}

bool ConditionsAreSatisfied()
{
    for (int i = 0; i < 10; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Cond: passed 1 sec" << std::endl;
    }
    return false;
}

TEST(ThreadRAII, correctness)
{
    int maxVal = 20;
    std::vector<int> goodVals;

    leopard::utils::ThreadRAII t(
        std::thread([maxVal, &goodVals] {
            for (auto i = 0; i <= maxVal; ++i)
            {
                if (Filter(i))
                {
                    goodVals.push_back(i);
                }
            }
        }),
        // The detach option will lead to undefined behaviour as the main stack will be destroyed.
        // leopard::utils::ThreadRAII::DtorAction::detach

        // We should join the filtering thread
        leopard::utils::ThreadRAII::DtorAction::join
    );

    if (ConditionsAreSatisfied())
    {
        std::cout << "Cond => satisfied" << std::endl;
        t.get().join();
    }
    std::cout << "Cond => unsatisfied" << std::endl;
}

TEST(MPMCRingBuffer, basic_equeue_dequeue)
{
    struct Message
    {
        std::size_t mSeq{0};
        std::vector<const char*> mDataPointers;
    };

    leopard::utils::MPMCRingBuffer<Message> buff;
    std::size_t capacity = 13;
    std::size_t realCapacity = 16;
    ASSERT_TRUE(buff.Init(capacity));
    ASSERT_EQ(buff.GetCapacity(), realCapacity);
    ASSERT_EQ(buff.GetMessageNumber(), 0U);

    // Enqueue
    std::vector<Message*> writeMsgVect;
    for (std::size_t i = 0; i < realCapacity; ++i)
    {
        auto writeMsg = buff.GetMessageForWrite();
        ASSERT_TRUE(writeMsg);
        ASSERT_EQ(writeMsg->mSeq, i);
        writeMsgVect.push_back(writeMsg);
    }
    auto writeMsg = buff.GetMessageForWrite();
    ASSERT_FALSE(writeMsg);
    ASSERT_EQ(buff.GetMessageNumber(), 0U);

    for (std::size_t i = 0; i < realCapacity; ++i)
    {
        buff.CommitMessageWrite(writeMsgVect[i]);
        ASSERT_EQ(buff.GetMessageNumber(), i + 1);
    }

    // Dequeue
    std::vector<const Message*> readMsgVect;
    for (std::size_t i = 0; i < realCapacity; ++i)
    {
        auto readMsg = buff.GetMessageForRead();
        ASSERT_TRUE(readMsg);
        ASSERT_EQ(readMsg->mSeq, i);
        readMsgVect.push_back(readMsg);
    }
    auto readMsg = buff.GetMessageForRead();
    ASSERT_FALSE(readMsg);
    ASSERT_EQ(buff.GetMessageNumber(), realCapacity);

    for (std::size_t i = 0; i < realCapacity; ++i)
    {
        buff.CommitMessageRead(readMsgVect[i]);
        ASSERT_EQ(buff.GetMessageNumber(), realCapacity - i - 1);
    }
}

TEST(MPMCRingBuffer, multiple_producer_multiple_consumer)
{
    struct Message
    {
        std::size_t mSeq{0};
        std::vector<const char*> mDataPointers;
    };

    leopard::utils::MPMCRingBuffer<Message> buff;
    std::size_t capacity = 13;
    std::size_t realCapacity = 16;
    ASSERT_TRUE(buff.Init(capacity));
    ASSERT_EQ(buff.GetCapacity(), realCapacity);
    ASSERT_EQ(buff.GetMessageNumber(), 0U);

    // Generate values to enqueue/dequeue
    uint32_t numValues{10000};
    std::vector<std::string> values;
    for (uint32_t i = 0; i < numValues; ++i)
    {
        values.push_back(std::to_string(i));
    }

    // Create producer threads
    uint32_t producerNum{2};
    std::vector<std::thread> producerThreads;
    std::atomic<uint32_t> writeCtr{0};
    std::atomic<uint32_t> writeFailureCtr{0};
    for (uint32_t i = 0; i < producerNum; ++i)
    {
        producerThreads.emplace_back([&](){
            while (true)
            {
                auto msg = buff.GetMessageForWrite();
                if (msg)
                {
                    if (msg->mSeq < numValues) // msg->mSeq can increase beyond numValues in this test.
                    {
                        msg->mDataPointers.push_back(values[msg->mSeq].c_str());
                        if (msg->mDataPointers.size() != 1)
                        {
                            ++writeFailureCtr;
                        }
                    }

                    buff.CommitMessageWrite(msg);
                    if (writeCtr.fetch_add(1) >= numValues)
                    {
                        return;
                    }
                }

                // Valgrind run all threads on a single core, this sleep is to avoid program hang.
                std::this_thread::sleep_for(std::chrono::nanoseconds(1000));
            }
        });
    }

    // Create consumer threads
    uint32_t consumerNum{2};
    std::vector<std::thread> consumerThreads;
    std::atomic<uint32_t> readCtr{0};
    std::atomic<uint32_t> readFailureCtr{0};
    for (uint32_t i = 0; i < consumerNum; ++i)
    {
        consumerThreads.emplace_back([&](){
            while (true)
            {
                auto msg = buff.GetMessageForRead();
                if (msg)
                {
                    if ((msg->mSeq < numValues) && // msg->mSeq can increase beyond numValues in this test.
                        ((msg->mDataPointers.size() != 1) || (msg->mDataPointers[0] != values[msg->mSeq].c_str())))
                    {
                        ++readFailureCtr;
                    }

                    buff.CommitMessageRead(msg);
                    if (readCtr.fetch_add(1) >= numValues)
                    {
                        return;
                    }
                }

                // Valgrind run all threads on a single core, this sleep is to avoid program hang.
                std::this_thread::sleep_for(std::chrono::nanoseconds(1000));
            }
        });
    }

    for (auto &pt : producerThreads)
    {
        pt.join();
    }

    for (auto &ct : consumerThreads)
    {
        ct.join();
    }

    ASSERT_EQ(writeCtr.load(), numValues + producerNum);
    ASSERT_EQ(writeFailureCtr.load(), 0U);
    ASSERT_EQ(readCtr.load(), numValues + consumerNum);
    ASSERT_EQ(readFailureCtr.load(), 0U);
}
