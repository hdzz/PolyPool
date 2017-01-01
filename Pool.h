#if 0
#pragma once

#include <array>
#include <queue>

#include "boost/poly_collection/base_collection.hpp"

/** Polymorphic object pool.

    The goal of a object pool is to:
    * allow fast object creation and deletion
    * avoid memory fragmentation and improve cache coherency by using
    contiguous memory chunks
    * allow object reuse for expensive objects (e.g., network objects)

    The main downsides of an object pool is:
    * no index-based element access (use refs, pointers or iterators)
    * possible wasted space due to fixed-size chunks
    * extra memory needed to track free elements
    * size must be optimized for application

    The goal of a polymorphic container is to:
    * allow types sharing an interface to be stored in a single
      container rather than one container per sub-type
    * allow new user-made child classes to be stored in same container
      as library classes without modifying the library
    * improve cache coherency by bundling like-types together (as
      opposed to allowing random distribution as in a pointer list)

    The main downside of polymorphic containers in C++ is that it
    forces the library to at least be partly template-based and thus
    cannot be pre-compiled with the rest of the library. Also, most
    implementations use RTTI which has a small fixed performance
    overhead.

    FIXME: Pooling is easy, but polymorphism is more involved. Best to
    implement pool as segment backend.

    For now, use boost::base_collection, reserve each subtype ahead of
    time, and accept the performance hit from slow deletions. Do not
    allow reallocation or else all those sweet, sweet
    refs/pointers/iterators will be invalidated.
 */
template <typename Base>
class Pool
{
    std::vector<boost::base_collection<Base>> mChunks;
    std::queue<uint> mFree;
    uint mMaxNum;

public:
    void reserve(uint num)
    {
        mMaxNum = num;
    }

    template <typename T, typename... Args, enable_if_acceptable<T> = nullptr>
    std::pair<uint, T*> emplace(Args&&... args)
    {
        if (not mFree.empty())
        {
            auto iter = mFree.front();
            *iter = T(args);
            mFree.pop();
        }
        else
        {
            mChunks.emplace(args);
        }
    }

    void erase(iterator iter)
    {

    }
};
#endif
