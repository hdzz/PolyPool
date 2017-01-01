#pragma once

#include <iostream>
#include <iterator>

/** A whole-collection iterator.

    See PolyPoolLocalIterator to iterate a single sub-type.
 */
template <typename ValueType>
class PolyPoolIterator : public std::iterator<std::bidirectional_iterator_tag, ValueType>
{
    boost::base_collection::iterator<ValueType> mIter;
    std::unordered_map<std::type_index, std::unordered_set<ValueType*> >& mFreeItems;

public:
    PolyPoolIterator(boost::base_collection<ValueType>::iterator iter,
                     std::unordered_map<std::type_index, std::unordered_set<ValueType*> >& freeItems)
        : mIter(iter)
        , mFreeItems(freeItems)
    {

    }

    // PolyPoolIterator(const PolyPoolIterator& iter)
    //     : mIter(iter.mIter)
    // {
    // }

    PolyPoolIterator& operator++()
    {
        // Seek next non-free element.
        do
        {
            ++mIter;
        } while (mFreeItems[typeid(mIter.segment())].count(&(*mIter)));
        return *this;
    }

    // PolyPoolIterator operator++(ValueType)
    // {
    //     PolyPoolIterator tmp(*this);
    //     operator++();
    //     return tmp;
    // }

    bool operator==(const PolyPoolIterator& rhs)
    {
        return mIter == rhs.mIter;
    }

    bool operator!=(const PolyPoolIterator& rhs)
    {
        return mIter != rhs.mIter;
    }

    ValueType& operator*()
    {
        return *mIter;
    }
};


#if 0
/** A segment iterator.
 */
template <typename BaseType, typename ValueType>
class PolyPoolLocalIterator : public std::iterator<std::bidirectional_iterator_tag, ValueType>
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
