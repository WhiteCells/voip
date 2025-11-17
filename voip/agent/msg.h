#pragma once

#include <string>

struct LLMTextMsg
{
    std::string text;
};

struct LLMEndMsg
{
    std::string text;
};

struct LLMHangupMsg
{
};

struct PCMMsg
{
    std::string pcm;
    std::string role;
};

struct ASRTextMsg
{
    std::string text;
    std::string role;
};

struct PrologTextMsg
{
    std::string text;
};