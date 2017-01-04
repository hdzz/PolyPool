#pragma once

#include <iostream>
#include <iterator>

#include <unordered_map>
#include <unordered_set>

#include "boost/poly_collection/base_collection.hpp"

/** A whole-collection iterator.

    See PolyPoolLocalIterator to iterate a single sub-type.

    OPTIMIZE: May be faster to keep list of used items?

    TODO: Change ValueType -> Root.
 */
template <typename Root>
class PolyPoolIterator : public std::iterator<std::forward_iterator_tag, Root>
{
    template <typename>
    friend class PolyPoolIterator;
    template<typename>
    friend class PolyPool;

    using iterator=PolyPoolIterator<Root>;

    using base_collection_iterator=typename boost::base_collection<Root>::iterator;
    using block_list=std::vector<boost::base_collection<Root> >;
    using block_list_iterator=typename std::vector<boost::base_collection<Root> >::iterator;
    using free_items_map=std::unordered_map<std::type_index, std::unordered_set<Root*> >;

    base_collection_iterator mIter;
    block_list_iterator mCurrentBlock;

    block_list& mBlocks;
    free_items_map& mFreeItems;

public:
    // PolyPoolIterator(const PolyPoolIterator& iter)
    //     : mIter(iter.mIter)
    // {
    // }

    iterator& operator++()
    {
        // Seek next non-free element, jumping from block to block as
        // needed.
        do
        {
            ++mIter;

            // Advance to next block.
            if (mIter == mCurrentBlock->end()
                and mCurrentBlock != mBlocks.end())
            {
                ++mCurrentBlock;
                mIter = mCurrentBlock->begin();
            }
        } while (mIter != mBlocks.back().end()
                 and mFreeItems[typeid(*mIter)].count(&(*mIter)));

        return *this;
    }

#if 0
    iterator& operator--()
    {
        // Seek previous non-free element.
        do
        {
            //FIXME: poly_collection::iterator does not seem to support reverse.
            --mIter;

            // Move to previous block.
            if (mIter == mCurrentBlock->begin()
                and mCurrentBlock != mBlocks.begin())
            {
                --mCurrentBlock;
                mIter = mCurrentBlock->end();
            }
        } while (mIter != mBlocks.begin().begin()
                 and mFreeItems[typeid(*mIter)].count(&(*mIter)));
        return *this;
    }
#endif

    //TODO: prefix operators
    // PolyPoolIterator<Root> operator++(Root)
    // {
    //     PolyPoolIterator<Root> tmp(*this);
    //     operator++();
    //     return tmp;
    // }
    // PolyPoolIterator<Root> operator--(???)
    // {
    // }

    bool operator==(const iterator& rhs)
    {
        return mIter == rhs.mIter;
    }

    bool operator!=(const iterator& rhs)
    {
        return mIter != rhs.mIter;
    }

    Root& operator*()
    {
        return *mIter;
    }

    Root* operator->()
    {
        return &(*mIter);
    }

protected:
    PolyPoolIterator(base_collection_iterator& iter,
                     block_list_iterator currentBlock,
                     block_list& blocks,
                     free_items_map& freeItems)
        : mIter(iter)
        , mCurrentBlock(currentBlock)
        , mBlocks(blocks)
        , mFreeItems(freeItems)
    {

    }
private:
};


/** A type-specific iterator.
 */
template <typename Child, typename Root>
class PolyPoolLocalIterator : public std::iterator<std::forward_iterator_tag, Root>
{
    template <typename,typename>
    friend class PolyPoolLocalIterator;
    template<typename>
    friend class PolyPool;

    using local_iterator=PolyPoolLocalIterator<Child, Root>;

    using base_collection_local_iterator=typename boost::base_collection<Root>::template local_iterator<Child>;
    using block_list=std::vector<boost::base_collection<Root> >;
    using block_list_iterator=typename std::vector<boost::base_collection<Root> >::iterator;
    using free_items=std::unordered_set<Root*>;

    base_collection_local_iterator mIter;
    block_list_iterator mCurrentBlock;
    block_list_iterator& mLastBlock;
    block_list& mBlocks; //TODO: May not need this for local iters.
    free_items& mFreeItems;

public:
    local_iterator& operator++()
    {
        // Seek next non-free element, jumping from block to block as
        // needed.
        do
        {
            ++mIter;

            // Advance to next block.
            if (mIter == mCurrentBlock->template end<Child>()
                and mCurrentBlock != mLastBlock)
            {
                ++mCurrentBlock;
                mIter = mCurrentBlock->template begin<Child>();
            }
        } while (mIter != mLastBlock->template end<Child>()
                 and mFreeItems.count(&(*mIter)));

        return *this;
    }

    //TODO: reverse operator
    //TODO: prefix operators

    bool operator==(const local_iterator& rhs)
    {
        return mIter == rhs.mIter;
    }

    bool operator!=(const local_iterator& rhs)
    {
        return mIter != rhs.mIter;
    }

    Child& operator*()
    {
        return *mIter;
    }

    Child* operator->()
    {
        return &(*mIter);
    }

protected:
    PolyPoolLocalIterator(base_collection_local_iterator& iter,
                          block_list_iterator currentBlock,
                          block_list_iterator& lastBlock,
                          block_list& blocks,
                          free_items& freeItems)
        : mIter(iter)
        , mCurrentBlock(currentBlock)
        , mLastBlock(lastBlock)
        , mBlocks(blocks)
        , mFreeItems(freeItems)
    {

    }

private:
};
