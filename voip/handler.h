#ifndef _LOGIC_H_
#define _LOGIC_H_

#include "singleton.hpp"

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

class Http;

class Handler :
    public Singleton<Handler>
{
    friend class Singleton<Handler>;
    using callback = std::function<void(std::shared_ptr<Http>)>;

public:
    ~Handler();

    void registerGet(std::string path, callback cb);
    bool handleGet(std::string path, std::shared_ptr<Http> conn);

    void registerPost(std::string path, callback cb);
    bool handlePost(std::string path, std::shared_ptr<Http> conn);

    void registerDelete(std::string path, callback cb);
    bool handleDelete(std::string path, std::shared_ptr<Http> conn);

    void registerPut(std::string path, callback cb);
    bool handlePut(std::string path, std::shared_ptr<Http> conn);

private:
    Handler();

    std::unordered_map<std::string, callback> m_post;
    std::unordered_map<std::string, callback> m_get;
    std::unordered_map<std::string, callback> m_delete;
    std::unordered_map<std::string, callback> m_put;
};

#endif // _LOGIC_H_