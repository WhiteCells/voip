#include "agent_ws_client.h"
#include "../io_context_pool.h"
// #include "tts_request.h"

AgentWsClient::AgentWsClient(const std::string &host, const std::string &port)
    : m_host(host)
    , m_port(port)
    , m_target("/")
{
    // auto &ioc = IOContextPool::getInstance()->getIOContext();

    // m_llm_client = std::make_shared<LLMRequestAsync>(
    //     ioc,
    //     agent_session_remote_host,
    //     agent_session_remote_port,
    //     agent_session_remote_target);
}

AgentWsClient::~AgentWsClient()
{
}

void AgentWsClient::set_web_ws_sender(WebWsMsgHandler handler)
{
    m_ws_msg_handler = std::move(handler);
}

// void AgentWsClient::set_server_sender(std::shared_ptr<IWSSender> sender)
// {
//     // m_gui_server_sender = sender;
// }

void AgentWsClient::send(const std::string &msg)
{
    if (!m_ws || !m_ws->is_open()) {
        LOG_WARN("WebSocket not open, cannot send text message");
        return;
    }

    net::dispatch(*m_strand, [self = shared_from_this(), msg]() {
        // 标记为文本模式
        self->m_send_queue.push_back("T:" + msg);
        if (!self->m_writing) {
            self->do_write();
        }
    });
}

void AgentWsClient::restart()
{
    stop();
    start();
}

void AgentWsClient::start()
{
    LOG_INFO("AgentWsClient start");
    auto &ioc = IOContextPool::getInstance()->getIOContext();
    m_strand.emplace(net::make_strand(ioc));
    m_resolver = std::make_unique<tcp::resolver>(*m_strand);

    ssl::context ssl_ctx(ssl::context::tls_client);
    // ssl_ctx.set_verify_mode(ssl::verify_peer);
    ssl_ctx.set_verify_mode(ssl::verify_none);
    ssl_ctx.load_verify_file("asr_server_verify_file");
    m_ws = std::make_unique<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(*m_strand, ssl_ctx);

    m_resolver->async_resolve(m_host, m_port,
                              beast::bind_front_handler(&AgentWsClient::on_resolver,
                                                        shared_from_this()));
}

void AgentWsClient::stop()
{
    net::dispatch(*m_strand, [self = shared_from_this()]() {
        if (self->m_ws && self->m_ws->is_open()) {
            beast::error_code ec;
            self->m_ws->close(websocket::close_code::normal, ec);
            if (ec) {
                LOG_ERROR("Close failed: {}", ec.message());
            }
            else {
                LOG_INFO("WebSocket closed cleanly");
            }
        }
        self->m_send_queue.clear();
        self->m_writing = false;
    });
}

void AgentWsClient::start_config_send()
{
    Json::Value config;
    config["mode"] = "2pass";
    config["wav_name"] = "record";
    config["wav_format"] = "pcm";
    config["audio_fs"] = 16000.0;
    config["is_speaking"] = true;
    config["itn"] = true;
    config["svs_itn"] = true;

    Json::Value chunk_size(Json::arrayValue);
    chunk_size.append(0);
    chunk_size.append(6);
    chunk_size.append(3);
    config["chunk_size"] = chunk_size;

    Json::StreamWriterBuilder builder;
    std::string config_str = Json::writeString(builder, config);
    send(config_str);
    LOG_INFO("success send asr start config");

    if (m_call_method == "agent") {
        m_llm_start = true;
        m_llm_start_time = std::chrono::steady_clock::now();
        end_timeout_check();
        start_timeout_check(m_llm_start_time, 20);
    }
}

void AgentWsClient::end_config_send()
{
    Json::Value config;
    config["is_speaking"] = "false";

    Json::StreamWriterBuilder builder;
    std::string config_str = Json::writeString(builder, config);
    send(config_str);
    LOG_INFO("success send asr end config");
}

void AgentWsClient::sendBinary(const std::string &data, const std::string &role)
{
    m_role = role;
    if (!m_ws || !m_ws->is_open()) {
        LOG_WARN("WebSocket not open, cannot send binary data");
        return;
    }

    net::dispatch(*m_strand, [self = shared_from_this(), data]() {
        // 标记为二进制模式
        self->m_send_queue.push_back("B:" + data);
        // LOG_INFO("Send queue size: {}", self->m_send_queue.size());
        if (!self->m_writing) {
            self->do_write();
        }
    });
}

void AgentWsClient::do_write()
{
    if (m_send_queue.empty() || !m_ws || !m_ws->is_open()) {
        m_writing = false;
        return;
    }

    m_writing = true;
    std::string msg = std::move(m_send_queue.front());
    m_send_queue.pop_front();

    bool is_binary = (msg.size() >= 2 && msg[0] == 'B' && msg[1] == ':');
    std::string real_data = msg.substr(2);

    if (is_binary)
        m_ws->binary(true);
    else
        m_ws->text(true);

    m_ws->async_write(
        net::buffer(real_data),
        net::bind_executor(
            *m_strand,
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (ec) {
                    LOG_ERROR("Send Failed: {}", ec.message());
                    self->m_send_queue.clear();
                    self->m_writing = false;
                    return;
                }
                self->do_write();
            }));
}

void AgentWsClient::start_llm_style()
{
    if (m_call_method == "agent" || m_call_method == "incoming_agent") {
        m_llm_start = false;
        m_llm_start_time = std::chrono::steady_clock::now();
        end_timeout_check();
        start_timeout_check(m_llm_start_time, 60);

        // m_llm_client->asyncSend("请用开场话术开始对话", "agent", "mediator", m_session_id, m_access_token, [self = shared_from_this()](const std::string &response) {
        //     self->tts_create(response);
        // });
    }
}

void AgentWsClient::clear_llm_msg_list()
{
    m_llm_msg_list.clear();
    //    m_llm_msg_text.clear();
    // TTSPlayer::getInstance()->stop();
    LOG_INFO("clear llm msg list");
}

void AgentWsClient::get_session_id(const std::string &call_method,
                                   const std::string &session_id,
                                   const std::string &access_token)
{
    m_call_method = call_method;
    m_session_id = session_id;
    m_access_token = access_token;

    LOG_INFO("get call_method {} session_id {} access_token {}", m_call_method, m_session_id, m_access_token);

    //    start_llm_style();
}

void AgentWsClient::on_resolver(beast::error_code ec, tcp::resolver::results_type results)
{
    if (ec) {
        LOG_ERROR("resolver error: {}", ec.message());
        return;
    }
    beast::get_lowest_layer(*m_ws)
        .async_connect(results,
                       beast::bind_front_handler(&AgentWsClient::on_connect,
                                                 shared_from_this()));
}

void AgentWsClient::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
{
    if (ec) {
        LOG_ERROR("connect error: {}", ec.message());
        return;
    }
    beast::get_lowest_layer(*m_ws).expires_never();
    m_ws->set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
        req.set(http::field::user_agent, "<ws>");
    }));
    m_host += ":" + std::to_string(ep.port());
    m_ws->next_layer().async_handshake(ssl::stream_base::client,
                                       beast::bind_front_handler(&AgentWsClient::on_tls_handshake,
                                                                 shared_from_this()));
}

void AgentWsClient::on_tls_handshake(beast::error_code ec)
{
    if (ec) {
        LOG_ERROR("tls handshake error: {}", ec.message());
        return;
    }
    m_ws->set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
    m_ws->set_option(websocket::stream_base::decorator([](websocket::request_type &req) {
        req.set(http::field::user_agent, "voip-client");
    }));
    m_ws->async_handshake(m_host, m_target,
                          beast::bind_front_handler(&AgentWsClient::on_ws_handshake,
                                                    shared_from_this()));
}

void AgentWsClient::on_ws_handshake(beast::error_code ec)
{
    Json::Value resp;
    Json::StreamWriterBuilder builder;
    if (ec) {
        LOG_ERROR("ws handshake error: {}", ec.message());
        return;
    }
    do_read();
}

void AgentWsClient::do_read()
{
    m_ws->async_read(m_buffer,
                     net::bind_executor(
                         *m_strand,
                         beast::bind_front_handler(&AgentWsClient::on_read, shared_from_this())));
}

void AgentWsClient::on_read(beast::error_code ec, std::size_t bytes_transferred)
{
    if (ec == websocket::error::closed) {
        LOG_WARN("WebSocket closed by server");
        return;
    }
    if (ec) {
        LOG_ERROR("Read error: {}", ec.message());
        return;
    }

    std::string msg = beast::buffers_to_string(m_buffer.data());
    m_buffer.consume(bytes_transferred);
    LOG_INFO("recv Agent Ws Client {}", msg);

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string errors;

    if (reader->parse(msg.c_str(), msg.c_str() + msg.size(), &root, &errors)) {
        std::string text = root.get("text", "").asString();
        std::string mode = root.get("mode", "").asString();

        if (mode == "2pass-offline") {
            if (m_vad_flag) {
                LOG_INFO("m_role: {}, mode:2pass-offline mode test: {}", m_role, text);
                //            process_asr_with_llm(text);
                if (m_call_method == "manual") {
                    manual_asr_with_llm(text);
                }
                else if (m_call_method == "agent" || m_call_method == "incoming_agent") {
                    if (m_llm_start) {
                        start_llm_style();
                    }
                    else {
                        // 在处理新的offline文本之前，先取消当前的LLM和TTS请求
                        cancel_current_requests();

                        if (!text.empty()) {
                            {
                                std::lock_guard<std::mutex> lock(text_mutex_);
                                // 将当前文本保存为待处理文本
                                pending_text_ = text;
                                last_text_time_ = std::chrono::steady_clock::now();

                                // 如果已有累积文本，则拼接；否则直接使用当前文本
                                if (!text_buffer_.empty()) {
                                    text_buffer_ += text;
                                }
                                else {
                                    text_buffer_ = text;
                                }
                            }

                            // 立即处理累积的文本
                            agent_asr_with_llm(text_buffer_);

                            // 清空文本缓冲区，因为我们已经处理过了
                            {
                                std::lock_guard<std::mutex> lock(text_mutex_);
                                text_buffer_.clear();
                            }
                        }
                    }
                }
            }
            m_vad_flag = false;
            m_first_flag = true;
        }
        else if (mode == "2pass-online") {
            if (m_first_flag) {
                m_first_flag = false;
            }
            else {
                m_vad_flag = true;
                // 收到online信号时，取消当前的LLM和TTS请求
                cancel_current_requests();

                // 清空TTS播放队列
                // TTSPlayer::getInstance()->clear();

                // 保存当前累积的文本，以便下次offline时使用
                {
                    std::lock_guard<std::mutex> lock(text_mutex_);
                    if (!pending_text_.empty()) {
                        text_buffer_ = pending_text_;
                        pending_text_.clear();
                    }
                }
            }
        }
    }
    else {
        LOG_ERROR("Failed to parse JSON: {}", errors);
    }

    do_read();
}

void AgentWsClient::process_buffer_if_timeout()
{
    std::lock_guard<std::mutex> lock(text_mutex_);
    auto now = std::chrono::steady_clock::now();
    // if (!text_buffer_.empty() && std::chrono::duration_cast<std::chrono::milliseconds>(now - last_text_time_).count() > g_timeout_ms) {
    //     LOG_INFO("process_buffer_if_timeout: {}", text_buffer_);
    //     agent_asr_with_llm(text_buffer_);
    //     text_buffer_.clear();
    // }
}

void AgentWsClient::manual_asr_with_llm(const std::string &text)
{
    m_llm_manual_text = text;
    LOG_INFO("m_call_method: {}, process_asr_with_llm: {}", m_call_method, text);
    //    LOG_INFO("LLM manual start");
    if (m_role == "customer") {
        LOG_INFO("LLM manual_customer start");
        // m_llm_client->asyncSend(m_llm_manual_text, "manual", "customer", m_session_id, m_access_token, [self = shared_from_this()](const std::string &response) {
        //     LOG_INFO("LLM manual_customer response: {}", response);
        // });
    }
    else if (m_role == "mediator") {
        LOG_INFO("LLM manual_mediator start");
        // m_llm_client->asyncSend(m_llm_manual_text, "manual", "mediator", m_session_id, m_access_token, [self = shared_from_this()](const std::string &response) {
        //     LOG_INFO("LLM manual_mediator response: {}", response);
        // });
    }
}

void AgentWsClient::agent_asr_with_llm(const std::string &text)
{
    m_llm_agent_text = text;
    m_llm_start_time = std::chrono::steady_clock::now();
    end_timeout_check();
    start_timeout_check(m_llm_start_time, 60);

    // 标记LLM请求为活跃状态
    m_llm_request_active = true;

    // m_llm_client->asyncSend(m_llm_agent_text, m_call_method, m_role, m_session_id, m_access_token, [self = shared_from_this()](const std::string &response) {
    //     // 请求完成后，标记为非活跃状态
    //     self->m_llm_request_active = false;
    //     self->on_agent_llm_response(response);
    // });
}

void AgentWsClient::on_agent_llm_response(const std::string &response)
{
    LOG_INFO("LLM agent response: {}", response);
    if (response.empty()) {
        net::post(*m_strand, [self = shared_from_this()]() {
            self->do_read();
        });
        return;
    }
    else {
        //        llm_ok_flag_ = true;
        tts_create(response);
    }
}

void AgentWsClient::tts_create(std::string response)
{
    // 标记TTS请求为活跃状态
    m_tts_request_active = true;

    //    LOG_INFO("prolog llm response: {}", response);
    Json::Value response_json;
    Json::CharReaderBuilder response_builder;
    std::unique_ptr<Json::CharReader> response_reader(response_builder.newCharReader());
    std::string response_errors;

    if (response_reader->parse(response.c_str(), response.c_str() + response.size(), &response_json, &response_errors)) {
        if (response_json.isMember("data") && response_json["data"].isObject()) {
            const auto &data = response_json["data"];
            if (data.isMember("text") && data["text"].isArray()) {
                m_llm_msg_list.clear();
                for (const auto &item : data["text"]) {
                    m_llm_msg_list.push_back(item.asString());
                }
                LOG_INFO("LLM response data pushed to m_llm_msg_list, size: {}", m_llm_msg_list.size());

                if (!m_llm_msg_list.empty()) {
                    // TTSPlayer::getInstance()->resume();                                      // 恢复播放
                    // TTSPlayer::getInstance()->produceTTSAsync(m_llm_msg_list, m_session_id); // 将LLM的文本转换为TTS的音频
                    m_llm_msg_list.clear();
                }
            }
        }
    }

    // 请求完成后，标记为非活跃状态
    m_tts_request_active = false;
}

void AgentWsClient::start_timeout_check(std::chrono::steady_clock::time_point timeout_time, int llm_timeout_seconds)
{
    m_llm_start_time = timeout_time;
    m_llm_timer_running = true;

    LOG_INFO("Starting new LLM timeout timer...");

    m_llm_timer_thread = std::thread([self = shared_from_this(), llm_timeout_seconds]() {
        LOG_INFO("LLM timeout timer started ({}s)", llm_timeout_seconds);

        // 等待超时时间或被手动中止
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(llm_timeout_seconds);
        while (self->m_llm_timer_running && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (!self->m_llm_timer_running) {
            LOG_INFO("LLM timeout timer manually stopped.");
            return;
        }

        // 超时触发
        LOG_WARN("LLM timeout ({}s) reached", llm_timeout_seconds);
        if (!self->m_is_hangup) {
            self->clear_llm_msg_list();
            self->hangup_call();
            self->m_is_hangup = false;
            LOG_INFO("Hangup call and llm_timeout_seconds {}s", llm_timeout_seconds);
        }
    });
}

void AgentWsClient::end_timeout_check()
{
    if (m_llm_timer_running) {
        LOG_INFO("Stopping LLM timeout timer...");
        m_llm_timer_running = false;
        try {
            if (m_llm_timer_thread.joinable()) {
                m_llm_timer_thread.join();
            }
        }
        catch (const std::system_error &e) {
            LOG_ERROR("Error joining LLM timer thread: {}", e.what());
        }
        LOG_INFO("LLM timeout timer stopped.");
    }
}

// 新增：取消当前的LLM和TTS请求
void AgentWsClient::cancel_current_requests()
{
    // 取消LLM请求（如果活跃）
    if (m_llm_request_active) {
        LOG_INFO("Canceling active LLM request");
        // 由于LLMRequestAsync是异步的，我们通过设置标志位来标识请求已被取消
        // 在实际应用中，可能需要更复杂的取消机制
        m_llm_request_active = false;
    }

    // 取消TTS请求（如果活跃）
    if (m_tts_request_active) {
        LOG_INFO("Canceling active TTS request");
        // TTSPlayer::getInstance()->stop();
        m_tts_request_active = false;
    }
}
