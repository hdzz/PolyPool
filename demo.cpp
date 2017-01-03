#include "PolyPool.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

struct A
{
    virtual void say() = 0;
};

struct B : public A
{
    virtual void say() { std::cout << "Beee the bee" << std::endl; }
};

struct C : public A
{
    C(std::string nameIn) : name(nameIn) {}
    std::string name;
    virtual void say() { std::cout << "Ceee Senor " << name << std::endl; }
};

int main()
{
    PolyPool<A> pool;
    pool.setDefaultBlockSize(10);
    // pool.setDefaultBlockSize<B>(10);
    // pool.setDefaultBlockSize<C>(10);
    for (int i = 0; i < 20; i++) pool.emplace<B>();
    pool.emplace<C>("Coo");
    C* boo = pool.emplace<C>("Boo");
    pool.emplace<C>("Woo");
    C* moo = pool.emplace<C>("Moo");
    pool.emplace<C>("Foo");
    pool.destroy(moo);
    pool.emplace<C>("Doo");
    pool.destroy(boo);
    for (auto& item : pool)
    {
        item.say();
        pool.destroy(&item);
    }
    C* hoo = pool.insert(C("HOOOOO"));
    std::cout << pool.size<B>() << " " << pool.holes<B>() << std::endl;
    std::cout << pool.size<C>() << " " << pool.holes<C>() << std::endl;
    hoo->say();
    return 0;
}
