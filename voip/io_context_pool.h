#ifndef _IO_CONTEXT_POOL_H_
#define _IO_CONTEXT_POOL_H_

#include "singleton.hpp"
#include <boost/asio.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <thread>
#include <vector>
#include <memory>

namespace asio = boost::asio;

class IOContextPool : public Singleton<IOContextPool>
{
private:
    friend class Singleton<IOContextPool>;

    using IOContext = asio::io_context;
    using Worker = asio::executor_work_guard<IOContext::executor_type>;
    using WorkerUPtr = std::unique_ptr<Worker>;

private:
    std::vector<IOContext> iocontexts_;
    std::vector<WorkerUPtr> workers_;
    std::vector<std::thread> threads_;
    std::size_t iocontext_next_;

public:
    ~IOContextPool();
    IOContext &getIOContext();
    void stopAllIOContext();

private:
    IOContextPool(std::size_t size = 2);
};

#endif // _IO_CONTEXT_POOL_H_