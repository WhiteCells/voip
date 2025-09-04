#include "io_context_pool.h"

IOContextPool::~IOContextPool()
{
    stopAllIOContext();
}

IOContextPool::IOContext &IOContextPool::getIOContext()
{
    IOContext &context = iocontexts_[iocontext_next_++];
    if (iocontexts_.size() == iocontext_next_) {
        iocontext_next_ = 0;
    }
    return context;
}

void IOContextPool::stopAllIOContext()
{
    for (auto &work : workers_) {
        work->get_executor().context().stop();
        work.reset();
    }
    for (auto &thread : threads_) {
        thread.join();
    }
}

IOContextPool::IOContextPool(std::size_t size) :
    iocontexts_(size),
    workers_(size),
    iocontext_next_(0)
{
    for (std::size_t i = 0; i < size; ++i) {
        workers_[i] = std::make_unique<Worker>(
            asio::make_work_guard(iocontexts_[i]));
    }

    for (std::size_t i = 0; i < iocontexts_.size(); ++i) {
        threads_.emplace_back([this, i]() {
            iocontexts_[i].run();
        });
    }
}