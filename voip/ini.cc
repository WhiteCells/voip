#include "ini.h"
#include "global.h"
#include "logger.h"
#include <string>
#include <fstream>

/**
 * @brief 去除多余空格
 *
 * @param str 输入字符串
 * @return std::string
 */
std::string trimSpace(const std::string &str)
{
    std::size_t start = 0, end = str.size() - 1;
    while (start < str.size() && std::isspace((unsigned char)str[start])) {
        ++start;
    }
    if (start == str.size()) {
        return "";
    }
    while (end > start && std::isspace((unsigned char)str[end])) {
        --end;
    }
    return str.substr(start, end - start + 1);
}

/**
 * @brief 加载 .ini 配置文件
 *
 * @param filename .ini 配置文件路径
 */
void loadINICfg(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_CRITICAL("load ini file failed");
        throw std::runtime_error {"load ini file failed"};
    }
    std::string line;

    while (std::getline(file, line)) {
        line = trimSpace(line);
        if (line.empty()) {
            continue;
        }
        if (line[0] == '#') {
            continue;
        }
        std::size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = line.substr(0, eq_pos);
            std::string val = line.substr(eq_pos + 1);
            key = trimSpace(key);
            val = trimSpace(val);
            cfg[key] = val;
            // std::cout << "key: " << key << ", "
            //           << "val: " << val << "\n";
        }
    }

    try {
        client_id = cfg.at("CLIENT_ID");
        backend_host = cfg.at("BACKEND_HOST");
        backend_port = cfg.at("BACKEND_PORT");
        backend_verify_file = cfg.at("BACKEND_VERIFY_FILE");
        asr_server_remote_host = cfg.at("ASR_SERVER_REMOTE_HOST");
        asr_server_remote_port = cfg.at("ASR_SERVER_REMOTE_PORT");
        asr_server_verify_file = cfg.at("ASR_SERVER_VERIFY_FILE");
        agent_session_remote_host = cfg.at("AGENT_SESSION_REMOTE_HOST");
        agent_session_remote_port = cfg.at("AGENT_SESSION_REMOTE_PORT");
        agent_session_remote_target = cfg.at("AGENT_SESSION_REMOTE_TARGET");
        agent_session_verify_file = cfg.at("AGENT_SESSION_VERIFY_FILE");
        tts_server_remote_host = cfg.at("TTS_SERVER_REMOTE_HOST");
        tts_server_remote_port = cfg.at("TTS_SERVER_REMOTE_PORT");
        tts_server_remote_target = cfg.at("TTS_SERVER_REMOTE_TARGET");
        g_timeout_ms = std::stoi(cfg.at("TIMEOUT_MS"));
        g_tts_speed = std::stof(cfg.at("TTS_SPEED"));
        g_tts_voice = cfg.at("TTS_VOICE");
    }
    catch (const std::exception &e) {
        LOG_CRITICAL("load ini file failed, lack key");
        throw std::runtime_error {"load ini file failed, lack key"};
    }
}