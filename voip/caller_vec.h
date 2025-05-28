#ifndef _CALLER_VEC_H_
#define _CALLER_VEC_H_

#include <vector>
#include <memory>

namespace voip {
class Caller;
}

/**
 * @brief 呼叫者容器
 * 该类用于测试每个线程单独的呼叫者
 */
class CallerVec
{
    using CallerSPtr = std::shared_ptr<voip::Caller>;

public:
    CallerVec();
    CallerVec(const CallerVec &) = delete;
    CallerVec &operator=(const CallerVec &) = delete;
    ~CallerVec();

    CallerSPtr getCaller(unsigned i);
    void push(CallerSPtr caller);

private:
    std::vector<CallerSPtr> m_caller_vec;
};

#endif // _CALLER_VEC_H_