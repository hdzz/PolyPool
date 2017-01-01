#include <iostream>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

/// Recreate the basis of poly_collection for use in PolyPool.
/// Uses the bizarre value-me

class A
{
    virtual void say() { std::cout << "Ayyy" << std::endl; }
};

class B : public A
{
    virtual void say() { std::cout << "Beee" << std::endl; }
};

struct SegmentBackend
{
    virtual void emplace() = 0;
};

template <typename T>
struct VectorSegmentBackend : public SegmentBackend
{
    std::vector<T> buffer;

    void emplace()
    {
        buffer.emplace();
    }
};

struct Segment
{
    std::unique_ptr<SegmentBackend> backend;

    virtual void emplace()
    {
        backend->emplace();
    }
};

template <typename BaseType>
struct Pool
{
    std::unordered_map<std::type_index, Segment> mContainers;

    template <typename ChildType>
    void reserve()
    {
        mContainers.emplace(typeid(ChildType), {new VectorSegmentBackend<ChildType>()});
    }

    template <typename ChildType>
    ChildType& emplace()
    {
        mContainers[typeid(ChildType)].emplace();
    }
};

int main()
{
    Pool<A> pool;
    pool.reserve<A>();
    pool.reserve<B>();
    pool.emplace<A>();
    pool.emplace<B>();
    pool.emplace<B>();
    for (auto& segment : pool.mContainers)
    {
        auto& buffer = ((std::unique_ptr<VectorSegmentBackend<A> >)(segment.second.backend))->buffer;
        for (A& a : buffer)
        {
            std::cout << a << std::endl;
        }
    }
    return 0;
}
