#pragma once

/** Require PolyPool to register types by setting their block sizes
    prior to adding any items. An exception will be raised if an
    attempt is made to add an item whose type has not been registered.
 */
#define POLYPOOL_REQUIRE_REGISTRATION

/** If exceptions are not enabled then most errors will go unchecked.
    It is suggested not to disable exceptions in debug builds.
 */
#define POLYPOOL_ENABLE_EXCEPTIONS

#include "PolyPool.h"

#include <cstddef>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

    Types are registered the first time a block size is set or object
    is created.

    OPTIMIZE: Consider using boost::unordered_set & boost::unordered_map.
    See: https://tinodidriksen.com/2012/02/cpp-set-performance-2/
 */
template <typename Root>
class PolyPool
{
public:
    using size_type=std::size_t;

    PolyPool()
    {
        mContainers.emplace();
    }

    PolyPool(size_type defaultBlockSize)
#ifdef POLYPOOL_REQUIRE_REGISTRATION
    {
        defaultBlockSize; // avoid usage warning
    }
#else
    : mDefaultBlockSize(defaultBlockSize)
    {
    }
#endif

    template <typename Child, typename... Args, enable_if_acceptable<Child> = nullptr>
    Child* emplace(Args&&... args)
    {
        auto& freeItems = mFreeItems[typeid(Child)];
        if (not freeItems.empty())
        {
            auto iter = freeItems.begin();
            Child* item = *iter;
            freeItems.erase(iter);
            item->Child(args);
            return item;
        }
        else
        {
            auto& lastBlock = mLastBlock[typeid(Child)];
            if (lastBlock->size<Child>() == lastBlock->capacity<Child>())
            {
                if (lastBlock == mContainers.end() - 1)
                {
                    // Create new block.
                    mContainers.push_back();
                    lastBlock = mContainers.end() - 1;
#ifndef POLYPOOL_REQUIRE_REGISTRATION
                    if (mBlockSize.count(typeid(Child)) == 0)
                    {
                        mBlockSize[typeid(Child)] = mDefaultBlockSize;
                    }
#elif defined POLYPOOL_ENABLE_EXCEPTIONS
                    if (mBlockSize.count(typeid(Child)) == 0)
                    {
                        throw Exception("Cannot add unregistered type to PolyPool while POLYPOOL_REQUIRE_REGISTRATION is enabled.");
                    }
#endif
                }
                else
                {
                    // Move to next block.
                    lastBlock++;
                }
                lastBlock->reserve<Child>(mBlockSize[typeid(Child)]);
            }
            auto iter = lastBlock->emplace<Child>(args);
            return &(*iter);
        }
    }

    /** Call object destructor and add it to free object list for
        later reuse by the pool.
    */
    template <typename Child>
    void destroy(Child*& item)
    {
        item->~Child();
        mFreeItems[typeid(Child)].insert(item)
        item = nullptr;
    }

    /** Set the block size for a type and apply to existing blocks.
        Does not cause rellocation of existing blocks.
     */
    template <typename Child>
    void setBlockSize(size_type size)
    {
        setDefaultBlockSize<Child>(size);
        for (auto& container : mContainers)
        {
            container.reserve<Child>(size);
        }
    }
    /** Set the default block size for newly created blocks.
        No-op if POLYPOOL_REQUIRE_REGISTRATION is enabled.
     */
    template <typename Child>
    void setDefaultBlockSize(size_type size)
    {
        mBlockSize[typeid(Child)] = numItems;
    }
    void setDefaultBlockSize(size_type size)
    {
#ifdef POLYPOOL_REQUIRE_REGISTRATION
        size; // avoid usage warning
#else
        mDefaultBlockSize = size;
#endif
    }

    void clear()
    {
        mContainers.clear();
        mFreeItems.clear();
    }
    template <typename Child>
    void clear()
    {
        for (auto& container : mContainers)
        {
            container.clear<Child>();
        }
        mFreeItems[typeid(Child)];
    }

    PolyPoolIterator<Root> begin()
    {
        auto iter = mContainers[0].begin();
        return PolyPoolIterator<Root>(iter);
    }
    PolyPoolIterator<Root> end()
    {

    }

#if 0
    template <typename Child>
    PolyPoolLocalIterator<Child> begin()
    {
        auto iter = mContainers[0].begin<Child>();
        return PolyPoolLocalIterator<Root, Child>(iter);
    }
    template <typename Child>
    PolyPoolLocalIterator<Child> end()
    {
        auto iter = mLastBlock[typeid(Child)]->end<Child>();
        return PolyPoolLocalIterator<Root, Child>(iter);
    }
#endif

    /** TODO: Deallocate empty blocks from the last non-empty block to
        the end of the block list.

        Note that empty blocks prior to the last non-empty block will
        not be preserved.

        Empty blocks = blocks with only free objects. Empty blocks are
        created when all the objects in the block have been freed.
     */
    void shrink_to_fit()
    {
    }
    template <typename Child>
    void shrink_to_fit()
    {
    }

    /**TODO: Move active objects until they are contiguous in memory.

       Reduces memory fragmentation by moving active objects into
       'holes' left by freed objects.

       If calling shrink_to_fit(), call it after defragment() since
       defragmentation may create separate nodes.

       WARNING: This can be an expensive operation if there are many
       'holes'.

       WARNING: Since some objects are moved, this will invalidate
       some pointers.
    */
    void defragment()
    {
    }
    template <typename Child>
    void defragment()
    {
    }

    /** Make active objects contiguous and deallocate empty blocks.

        Calls defragment() followed by shrink_to_fit().
     */
    void compactify()
    {
        defragment();
        shrink_to_fit();
    }
    template <typename Child>
    void compactify()
    {
        defragment<Child>();
        shrink_to_fit<Child>();
    }

protected:
    /// The underlying polymorphic containers.
    std::vector<boost::base_collection<Root> > mContainers;
    // std::unordered_map<std::type_index, std::unique_ptr<PolyPoolSegment> > mContainers;

    /// The size of each block per type.
    std::unordered_map<std::type_index, size_type> mBlockSize;
    /// The current block being filled for a specific type.
    std::unordered_map<std::type_index, std::vector<boost::base_collection<Root> >::iterator > mLastBlock;
    /// Tracks free items of every type.
    std::unordered_map<std::type_index, std::unordered_set<Root*> > mFreeItems;

#ifndef POLYPOOL_REQUIRE_REGISTRATION
    /// Default block size used for unregistered types.
    size_type mDefaultBlockSize = 0;
#endif

private:
};
