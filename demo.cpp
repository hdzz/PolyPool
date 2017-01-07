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

template <typename Child, typename Root>
void printPoolStats(PolyPool<Root>& pool)
{
    std::cout << pool.template size<Child>() << " " << pool.template holes<Child>() << std::endl;
}

int main()
{
    PolyPool<A> pool;
    pool.setDefaultBlockSize(3);
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
#if 1
    for (auto& item : pool)
    {
        item.say();
        // pool.destroy(&item);
    }
#else
    for (auto item = pool.begin(); item != pool.end(); ++item)
    {
        item->say();
    }
#endif
    C* hoo = pool.insert(C("HOOOOO"));
    printPoolStats<B>(pool);
    printPoolStats<C>(pool);
    hoo->say();
    pool.nullify(hoo);
    if (not hoo) std::cout << "No hoo... =D" << std::endl;
#if 1
    for (auto& c : pool.local<C>())
    {
        c.say();
        pool.destroy(&c);
    }
#else
    for (auto c = pool.begin<C>(); c != pool.end<C>(); ++c)
    {
        c->say();
    }
#endif
    // pool.clear<C>();
    printPoolStats<B>(pool);
    printPoolStats<C>(pool);
    pool.clear();
    printPoolStats<B>(pool);
    printPoolStats<C>(pool);
    return 0;
}
