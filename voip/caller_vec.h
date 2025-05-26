#ifndef _CALLER_VEC_H_
#define _CALLER_VEC_H_

#include <vector>
#include <memory>

namespace voip {
class Caller;
}

class CallerVec
{
    using CallerSPtr = std::shared_ptr<voip::Caller>;

public:
    CallerVec();
    ~CallerVec();

    CallerSPtr getCaller(unsigned i);
    void push(CallerSPtr caller);

private:
    std::vector<CallerSPtr> m_caller_vec;
};

#endif // _CALLER_VEC_H_