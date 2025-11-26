#ifndef _SINGLETON_H_
#define _SINGLETON_H_

#include <memory>
#include <mutex>

/**
 * @brief 单例模板
 * 
 * @tparam T 
 */
template <typename T>
class Singleton
{
private:
    static std::shared_ptr<T> instance_;

public:
    ~Singleton();
    static std::shared_ptr<T> getInstance();

protected:
    Singleton() = default;
    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;
};

template <typename T>
Singleton<T>::~Singleton()
{
}

template <typename T>
std::shared_ptr<T> Singleton<T>::getInstance()
{
    static std::once_flag flag;
    std::call_once(flag, [&]() {
        instance_ = std::shared_ptr<T>(new T());
    });
    return instance_;
}

template <typename T>
std::shared_ptr<T> Singleton<T>::instance_ = nullptr;

#endif // _SINGLETON_H_