#pragma once

#include <string>

// 在接收到 ENDEND 之后通知
class LLMHttpClient
{
public:
    LLMHttpClient();
    ~LLMHttpClient();

    void request(const std::string &msg)
    {
    }
};
