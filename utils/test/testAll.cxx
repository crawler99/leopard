#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "Singleton.hxx"

TEST(Correctness, SimpleInput)
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
    utils::Singleton<A>::instance(str);
    utils::Singleton<B>::instance(std::move(str));
    
    try
    {
        utils::Singleton<C>::get_instance()->Print();
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }
    
    utils::Singleton<C>::instance(1, 3.14);
    utils::Singleton<C>::get_instance()->Print();
    
    try
    {
        utils::Singleton<C>::instance(2, 3.14);
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception caught: " << e.what() << std::endl;
    }
}
