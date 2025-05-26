#include "caller_vec.h"

CallerVec::CallerVec()
{
}

CallerVec::~CallerVec()
{
}

CallerVec::CallerSPtr CallerVec::getCaller(unsigned i)
{
    return m_caller_vec[i];
}

void CallerVec::push(CallerSPtr caller)
{
    m_caller_vec.push_back(caller);
}
