#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "Singleton.hxx"
#include "Event.hxx"
#include <functional>
#include <map>

TEST(Singleton, Correctness)
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

    std::string str { "test" };
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

TEST(Event, Correctness)
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
        int _a = { 0 };
        int _b = { 0 };
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