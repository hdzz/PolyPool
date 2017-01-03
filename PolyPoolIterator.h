#pragma once

#include <iostream>
#include <iterator>

#include <unordered_map>
#include <unordered_set>

#include "boost/poly_collection/base_collection.hpp"

/** A whole-collection iterator.

    See PolyPoolLocalIterator to iterate a single sub-type.

    OPTIMIZE: May be faster to keep list of used items?
 */
template <typename ValueType>
class PolyPoolIterator : public std::iterator<std::forward_iterator_tag, ValueType>
{
    template <typename>
    friend class PolyPoolIterator;
    template<typename>
    friend class PolyPool;

    using base_collection_iterator=typename boost::base_collection<ValueType>::iterator;
    using block_list=std::vector<boost::base_collection<ValueType> >;
    using block_list_iterator=typename std::vector<boost::base_collection<ValueType> >::iterator;
    using free_items_map=std::unordered_map<std::type_index, std::unordered_set<ValueType*> >;

    base_collection_iterator mIter;
    block_list_iterator mCurrentBlock;

    block_list& mBlocks;
    free_items_map& mFreeItems;

public:
    // PolyPoolIterator(const PolyPoolIterator& iter)
    //     : mIter(iter.mIter)
    // {
    // }

    PolyPoolIterator<ValueType>& operator++()
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
    PolyPoolIterator<ValueType>& operator--()
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
    // PolyPoolIterator<ValueType> operator++(ValueType)
    // {
    //     PolyPoolIterator<ValueType> tmp(*this);
    //     operator++();
    //     return tmp;
    // }
    // PolyPoolIterator<ValueType> operator--(???)
    // {
    // }

    bool operator==(const PolyPoolIterator<ValueType>& rhs)
    {
        return mIter == rhs.mIter;
    }

    bool operator!=(const PolyPoolIterator<ValueType>& rhs)
    {
        return mIter != rhs.mIter;
    }

    ValueType& operator*()
    {
        return *mIter;
    }

    ValueType* operator->()
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


#if 0
/** A segment iterator.
 */
template <typename BaseType, typename ValueType>
class PolyPoolLocalIterator : public std::iterator<std::forward_iterator_tag, ValueType>
{
    boost::base_collection<BaseType>::local_iterator<ValueType> mIter;
    std::unordered_set<ValueType*>& mFreeItems;

public:
    PolyPoolLocalIterator(
        boost::base_collection<BaseType>::local_iterator<ValueType> iter,
        std::unordered_set<ValueType*>& freeItems)
        : mIter(iter)
        , mFreeItems(freeItems)
    {

    }
};
#endif
