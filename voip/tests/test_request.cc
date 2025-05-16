#define BOOST_TEST_MODULE REQUEST_TEST

#include "../request.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/beast/http.hpp>
#include <json/json.h>
#include <map>
#include <string>

BOOST_AUTO_TEST_CASE(request_heartbeat)
{
    std::map<std::string, std::string> params;
    params["param1"] = "param1";
    params["param2"] = "param2";

    // Json::Value root;
    // root["key1"] = "val1";
    // root["key2"] = "val2";
    // Json::StreamWriterBuilder writer;
    // std::string body = Json::writeString(writer, root);

    std::string body = R"({"key1":"val1"})";

    voip::httpRequest("localhost", "5000", "/", boost::beast::http::verb::post, params, body);
}