#if 0
#pragma once

#include <memory>
#include <vector>

struct PolyPoolSegment
{
};

template <typename T>
struct PolyPoolVectorSegment : public PolyPoolSegment
{
    std::vector<T> buffer;
};
#endif
