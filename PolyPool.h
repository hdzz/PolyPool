#pragma once

/** Require PolyPool to register types by setting their default block
    sizes prior to adding any items. An exception will be raised if an
    attempt is made to add an item whose default block size has not
    been set. This is useful to ensure all types have manually tweaked
    optimal block sizes.
 */
//#define POLYPOOL_REQUIRE_REGISTRATION

// /** If exceptions are not enabled then most errors will go unchecked.
//     It is suggested not to disable exceptions in debug builds.
//     TODO: Currently unused, consider removing.
//  */
// #define POLYPOOL_ENABLE_EXCEPTIONS

#include "PolyPoolIterator.h"

#include <cstddef>
#include <stdexcept>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "boost/poly_collection/base_collection.hpp"

/** Polymorphic object pool written in C++11 using RTTI.

    Typical goals of an object pool are:
    * fast object creation and deletion
    * reduced memory fragmentation and improved cache coherency by using
      contiguous memory chunks
    * reuse of expensive objects (e.g., network objects)

    The main downsides of an object pool are:
    * no index-based element access (must use refs, pointers or iterators)
    * possible wasted space due to fixed-size chunks and unused free objects
    * extra memory needed to track free objects
    * for optimal performance, block size must be set for each type
      based on application usage

    Types are registered the first time a default block size is set or
    object is added to the pool.

    All types stored must be children of the root type or the root
    type itself.

    OPTIMIZE: Consider using boost::unordered_set & boost::unordered_map.
    See: https://tinodidriksen.com/2012/02/cpp-set-performance-2/

    OPTIMIZE: Consider creating simpler alternative to
    boost::poly_collection if duplicating its type registry and
    working around its quirks proves inefficient.
 */
template <typename Root>
class PolyPool
{
public:
    // using enable_if_subtype=
    //     typename std::enable_if<is_subtype<Root>::value>::type*;

    // template<typename T,typename Model,typename=void>
    // struct is_acceptable:std::integral_constant<
    //     bool,
    //     Model::template is_subtype<T>::value&&
    //     std::is_move_constructible<typename std::decay<T>::type>::value&&
    //     (std::is_move_assignable<typename std::decay<T>::type>::value||
    //      std::is_nothrow_move_constructible<typename std::decay<T>::type>::value)
    //     >{};
    // template<typename T>
    // using enable_if_acceptable=
    //     typename std::enable_if<is_acceptable<Root>::value>::type*;

    using base_collection_iterator=
        typename boost::base_collection<Root>::iterator;
    using block_list_iterator=
        typename std::vector<boost::base_collection<Root> >::iterator;
    using size_type=std::size_t;

    PolyPool()
    {
        mBlocks.emplace_back();
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

    // template <typename Child, enable_if_acceptable<Child> = nullptr>
    template <typename Child>
    Child* insert(Child&& child)
    {
        Child* freeItem = popFreeItem<Child>();
        if (freeItem)
        {
            *freeItem = child;
            return freeItem;
        }
        else
        {
            block_list_iterator block = getBlockForNewItem<Child>();
            auto iter = block->insert(std::forward<Child>(child));
            return (Child*)(&(*iter));
        }
    }
    
    // template <typename Child, typename... Args, enable_if_acceptable<Child> = nullptr>
    template <typename Child, typename... Args>
    Child* emplace(Args&&... args)
    {
        Child* freeItem = popFreeItem<Child>();
        if (freeItem)
        {
            *freeItem = Child(args...);
            return freeItem;
        }
        else
        {
            block_list_iterator block = getBlockForNewItem<Child>();
            auto iter = block->emplace<Child>(args...);
            return (Child*)(&(*iter));
        }
    }

    /** Add object to free object list.

        The object destructor is not called. To both destruct and free
        an object, see destroy().
     */
    template <typename Child>
    void free(Child* item)
    {
        mFreeItems[typeid(Child)].insert(item);
    }
    /** Call object destructor and add it to free object list.

        The object should no longer be accessible after it is
        destroyed.

        It is good practice to set lingering pointers to this object
        to nullptr. To have this done for you along with destruction,
        see nullify().
     */
    template <typename Child>
    void destroy(Child* item)
    {
        item->~Child();
        free(item);
    }
    /** Destroy object and set its pointer to nullptr.
        See destroy() for notes.
     */
    template <typename Child>
    void nullify(Child*& item)
    {
        destroy(item);
        item = nullptr;
    }

    bool empty()
    {
        for (auto& block : mBlocks)
        {
            if (not block.empty()) return false;
        }
        return true;
    }
    template <typename Child>
    bool empty()
    {
        for (auto block = mBlocks.begin(); block <= mLastBlock[typeid(Child)]; block++)
        {
            if (not block->empty<Child>()) return false;
        }
        return true;
    }

    /// Number of active items.
    size_type active()
    {
        return size() - holes();
    }
    template <typename Child>
    size_type active()
    {
        return size<Child>() - holes<Child>();
    }

    /// Number of free items.
    size_type holes()
    {
        size_type size = 0;
        for (auto& freeItems : mFreeItems)
        {
            size += freeItems.second.size();
        }
        return size;
    }
    template <typename Child>
    size_type holes()
    {
        return mFreeItems[typeid(Child)].size();
    }

    /// Number of active + free items.
    size_type size()
    {
        size_type size = 0;
        for (auto& block : mBlocks)
        {
            size += block.size();
        }
        return size;
    }
    template <typename Child>
    size_type size()
    {
        size_type size = 0;
        for (auto block = mBlocks.begin(); block <= mLastBlock[typeid(Child)]; block++)
        {
            size += block->template size<Child>();
        }
        return size;
    }

    /// Total number of items, active + free + spare.
    size_type capacity()
    {
        size_type size = 0;
        for (auto& block : mBlocks)
        {
            size += block.capacity();
        }
        return size;
    }
    template <typename Child>
    size_type capacity()
    {
        size_type size = 0;
        for (auto block = mBlocks.begin(); block <= mLastBlock[typeid(Child)]; block++)
        {
            size += block->template capacity<Child>();
        }
        return size;
    }

    ///TODO: Number of blocks.
    /// size_type blocks()

    //todo: size_type max_size()

    /** Set the block size for a type and apply to existing blocks.
        Does not cause rellocation of existing blocks.
     */
    // template <typename Child>
    // void setBlockSize(size_type size)
    // {
    //     setDefaultBlockSize<Child>(size);
    //     for (auto& container : mBlocks)
    //     {
    //         container.reserve(typeid(Child), size);
    //     }
    // }
    /** Set the default block size for newly created blocks.
        No-op if POLYPOOL_REQUIRE_REGISTRATION is enabled.
     */
    template <typename Child>
    void setDefaultBlockSize(size_type size)
    {
        mBlockSize[typeid(Child)] = size;
        registerType<Child>();
    }
    void setDefaultBlockSize(size_type size)
    {
#ifdef POLYPOOL_REQUIRE_REGISTRATION
        size; // avoid usage warning
#else
        mDefaultBlockSize = size;
#endif
    }

    /** Destruct all objects in container and unregister all types.
        May not deallocate memory depending on std::vector
        implementation.
     */
    void clear()
    {
        //FIXME: This may call destruction on already destroyed free objects.
        mBlocks.clear();
        mFreeItems.clear();
        mLastBlock.clear();
        mBlockSize.clear();
    }
    /** Destruct all objects of given type in container and unregister
        the type.
        May not deallocate memory depending on std::vector
        implementation.
     */
    template <typename Child>
    void clear()
    {
        const auto& childID = typeid(Child);
        for (auto block = mBlocks.begin();
             block <= mLastBlock[childID]; block++)
        {
            //FIXME: This may call destruction on already destroyed free objects.
            block->clear<Child>();
        }
        mFreeItems.erase(childID);
        mLastBlock.erase(childID);
        mBlockSize.erase(childID);
    }

    PolyPoolIterator<Root> begin()
    {
        base_collection_iterator begin = mBlocks[0].begin();
        return PolyPoolIterator<Root>(
            begin, mBlocks.begin(), mBlocks, mFreeItems);
    }
    PolyPoolIterator<Root> end()
    {
        base_collection_iterator sentinel = mBlocks.end()->end();
        return PolyPoolIterator<Root>(
            sentinel, mBlocks.end() - 1, mBlocks, mFreeItems);
    }

    template <typename Child>
    PolyPoolLocalIterator<Child, Root> begin()
    {
        auto& lastBlock = mLastBlock[typeid(Child)];
        auto begin = mBlocks[0].template begin<Child>();
        return PolyPoolLocalIterator<Child, Root>(
            begin, mBlocks.begin(), lastBlock, mBlocks, mFreeItems[typeid(Child)]);
    }
    template <typename Child>
    PolyPoolLocalIterator<Child, Root> end()
    {
        auto& lastBlock = mLastBlock[typeid(Child)];
        auto sentinel = lastBlock->template end<Child>();
        return PolyPoolLocalIterator<Child, Root>(
            sentinel, lastBlock, lastBlock, mBlocks, mFreeItems[typeid(Child)]);
    }


    // For range loops of local iterators.
    template <typename Child>
    struct Local
    {
        PolyPool<Root>* pool;
        Local(PolyPool<Root>* poolIn) : pool(poolIn) {}

        PolyPoolLocalIterator<Child, Root> begin()
        {
            return pool->template begin<Child>();
        }
        PolyPoolLocalIterator<Child, Root> end()
        {
            return pool->template end<Child>();
        }
    };

    template <typename Child>
    Local<Child> local()
    {
        return Local<Child>(this);
    }

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
    /// The underlying polymorphic block containers.
    std::vector<boost::base_collection<Root> > mBlocks;
    // std::unordered_map<std::type_index, std::unique_ptr<PolyPoolSegment> > mBlocks;

    /// The size of each block per type.
    std::unordered_map<std::type_index, size_type> mBlockSize;
    /// The current block being filled for a specific type.
    std::unordered_map<std::type_index, block_list_iterator> mLastBlock;
    /// Tracks free items of every type.
    std::unordered_map<std::type_index, std::unordered_set<Root*> > mFreeItems;

#ifndef POLYPOOL_REQUIRE_REGISTRATION
    /// Default block size used for unregistered types.
    size_type mDefaultBlockSize = 20;
#endif

    template <typename Child>
    Child* popFreeItem()
    {
        auto& freeItems = mFreeItems[typeid(Child)];
        if (not freeItems.empty())
        {
            auto iter = freeItems.begin();
            Child* item = (Child*)(*iter);
            freeItems.erase(iter);
            return item;
        }
        else
        {
            return nullptr;
        }
    }

    /** Get a block with room for a new item.
        May create a block if all current blocks are occupied.
     */
    template <typename Child>
    block_list_iterator getBlockForNewItem()
    {
        const std::type_info& childID = typeid(Child);
#ifdef POLYPOOL_REQUIRE_REGISTRATION
        if (mLastBlock.count(childID) == 0)
        {
            throw std::logic_error("Cannot add unregistered type to PolyPool while POLYPOOL_REQUIRE_REGISTRATION is enabled.");
        }
#else
        registerType<Child>(mDefaultBlockSize);
#endif
        auto& lastBlock = mLastBlock[childID];
        if (lastBlock->size(childID) == lastBlock->capacity(childID))
        {
            if (lastBlock == mBlocks.end() - 1)
            {
                // Create new block.
                mBlocks.emplace_back();
                lastBlock = mBlocks.end() - 1;
            }
            else
            {
                // Move to next block.
                lastBlock++;
            }
            lastBlock->template reserve<Child>(mBlockSize[childID]);
        }
        return lastBlock;
    }

    template <typename Type>
    void registerType(const size_type& blockSize)
    {
        const std::type_info& type = typeid(Type);
        if (mLastBlock.count(type) == 0)
        {
            mLastBlock[type] = mBlocks.begin();
            mBlockSize[type] = blockSize;
            mLastBlock[type]->template reserve<Type>(mBlockSize[type]);
        }
    }
    template <typename Type>
    void registerType()
    {
        const std::type_info& type = typeid(Type);
        if (mLastBlock.count(type) == 0)
        {
            // Register type.
            mLastBlock[type] = mBlocks.begin();
            mLastBlock[type]->template reserve<Type>(mBlockSize[type]);
        }
    }

private:
};
