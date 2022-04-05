/*
 * @auther      : Parkertao
 * @Date        : 2022-3-21
 * @copyright Apache 2.0
*/

#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>
#include <mysql/mysql.h>
#include <algorithm>
#include <cassert>

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH
    };

    enum HTTP_CODE {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURSE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    HttpRequest() {
        Init();
    }
    ~HttpRequest() = default;

    void Init();

    bool Parse(Buffer& buffer);

    std::string path() const;
    std::string& path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string& key) const;
    std::string GetPost(const char* key) const;

    bool alive() const;

private:
    bool ParseRequestLine(const std::string& line);
    void ParseHeader(const std::string& line);
    void ParseBody(const std::string& line);

    void ParsePath();
    void ParsePost();
    void ParseFromUrlEncoded();

    static bool UserVerify(const std::string& name, const std::string& password, bool logined);

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> kDefaultHtml;
    static const std::unordered_map<std::string, int> kDefaultHtmlTag;
    static int ConvertHex(char c);
};


#endif  //HTTPREQUEST_H