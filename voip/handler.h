#ifndef _LOGIC_H_
#define _LOGIC_H_

#include "singleton.hpp"

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>

class Http;

class Handler :
    public std::enable_shared_from_this<Handler>
{
    friend class Singleton<Handler>;
    using callback = std::function<void(std::shared_ptr<Http>)>;

public:
    ~Handler();

    void registerGet(std::string path, callback cb);
    bool handleGet(std::string path, std::shared_ptr<Http> conn);

    void registerPost(std::string path, callback cb);
    bool handlePost(std::string path, std::shared_ptr<Http> conn);

private:
    Handler();

    std::map<std::string, callback> m_post;
    std::map<std::string, callback> m_get;
};

#endif // _LOGIC_H_