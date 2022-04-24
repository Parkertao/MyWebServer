/*
 * @auther      : Parkertao
 * @Date        : 2022-3-21
 * @copyright Apache 2.0
*/

#include "httprequest.h"


const std::unordered_set<std::string> HttpRequest::kDefaultHtml {
            "/index", "/register", "/login", "/welcome", "/video", "/picture"};

const std::unordered_map<std::string, int> HttpRequest::kDefaultHtmlTag {
            {"/register.html", 0}, {"/login.html", 1} };

void HttpRequest::Init() {
    method_ = "";
    path_ = "";
    version_ = "";
    body_ = "";
    state_ = REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::alive() const {
    if (header_.count("Connection") == 1)
    {
        return header_.find("Connection") -> second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::Parse(Buffer& buffer) {
    const char crlf[4] = "\r\n";
    if (buffer.ReadableBytes() <= 0)
        return false;

    while (buffer.ReadableBytes() && state_ != FINISH)
    {
        const char* line_end = std::search(buffer.read_position(), 
                                buffer.write_position(), crlf, crlf + 2);
        std::string line(buffer.read_position(), line_end);
        switch (state_)
        {
        case REQUEST_LINE:
            if (!ParseRequestLine(line))
            {
                return false;
            }
            ParsePath();
            break;
        case HEADERS:
            ParseHeader(line);
            if (buffer.ReadableBytes() <= 2)
            {
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody(line);
            break;
        default:
            break;
        }
        if (line_end == buffer.write_position()) break;
        buffer.RetrieveUntil(line_end + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

void HttpRequest::ParsePath() {
    if (path_ == "/")
    {
        path_ = "/index.html";
    }
    else
    {
        for (auto& item : kDefaultHtml)
        {
            if (item == path_)
            {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine(const std::string& line) {
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch submatch;
    if (std::regex_match(line, submatch, pattern))
    {
        method_ = submatch[1];
        path_ = submatch[2];
        version_ = submatch[3];
        state_ = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine error!");
    return false;
}

void HttpRequest::ParseHeader(const std::string& line) {
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch submatch;
    if (std::regex_match(line, submatch, pattern)) 
    {
        header_[submatch[1]] = submatch[2];
    }
    else 
    {
        state_ = BODY;
    }
}

void HttpRequest::ParseBody(const std::string& line) {
    body_ = line;
    ParsePost();
    state_ = FINISH;
    LOG_DEBUG("Body: %s, length: %d", line.c_str(), line.size());
}

int HttpRequest::ConvertHex(char c) {
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return c;
}

std::string HttpRequest::path() const{
    return path_;
}

std::string& HttpRequest::path(){
    return path_;
}
std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::GetPost(const std::string& key) const {
    assert(key != "");
    if (post_.count(key)) 
    {
        return post_.find(key) -> second;
    }
    return "";
}

std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if (post_.count(key))
    {
        return post_.find(key) -> second;
    }
    return "";
}

void HttpRequest::ParsePost() {
    if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded")
    {
        ParseFromUrlEncoded();
        if (kDefaultHtmlTag.count(path_))
        {
            int tag = kDefaultHtmlTag.find(path_) -> second;
            LOG_DEBUG("Tag: %d", tag);
            if (tag == 0 || tag == 1)
            {
                bool logined = (tag == 1);
                if (UserVerify(post_["username"], post_["password"], logined))
                {
                    path_ = "/welcome.html";
                }
                else
                {
                    path_ = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::ParseFromUrlEncoded() {
    if (body_.size() == 0) return ;

    std::string key, value;
    int num = 0;
    int n = body_.size();
    int i = 0, j = 0;

    for (; i < n; i++)
    {
        char ch = body_[i];
        switch (ch)
        {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = ConvertHex(body_[i + 1]) * 16 + ConvertHex(body_[i + 2]);
            body_[i + 1] = num %10 + '0';
            body_[i + 2] = num /10 + '0';
            i += 2;
            break;
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);

    if (!post_.count(key) && j < i)
    {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::UserVerify(const std::string& name, const std::string& password, bool logined) {
    if (name == "" || password == "") return false;

    LOG_INFO("Verify name: %s password: %s", name.c_str(), password.c_str());
    MYSQL* sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    unsigned int j = 0;
    char order[256] = {0};
    MYSQL_FIELD* fileds = nullptr;
    MYSQL_RES* res = nullptr;

    if (!logined) flag = true;

    snprintf(order, 256, "SELECT username, passwd FROM user WHERE username='%s' LIMIT 1", 
                name.c_str());
    LOG_DEBUG("%s", order);

    if (mysql_query(sql, order)) 
    {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fileds = mysql_fetch_field(res);

    while (MYSQL_ROW row = mysql_fetch_row(res))
    {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string passwd(row[1]);
        // 注册行为 用户名未使用
        if (logined)
        {
            if (passwd == password)
            {
                flag = true;
            }
            else
            {
                flag = false;
                LOG_DEBUG("Password Error!");
            }
        }
        else
        {
            flag = false;
            LOG_DEBUG("User used!");
        }
    }
    mysql_free_result(res);

    // 注册行为 且 用户名未被使用
    if (!logined && flag) 
    {
        LOG_DEBUG("Register!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, passwd) VALUES('%s', '%s')", 
                    name.c_str(), password.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order))
        {
            LOG_DEBUG("Insert error!");
            flag = false;
        }
        flag = true;
    }
    SqlConnPool::Instance() -> FreeSqlConn(sql);
    LOG_DEBUG("User Verify Successfully!");
    return flag;
}