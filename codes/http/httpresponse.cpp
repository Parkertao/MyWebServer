/*
 * @auther      : Parkertao
 * @Date        : 2022-3-22
 * @copyright Apache 2.0
*/

#include "httpresponse.h"

const std::unordered_map<std::string, std::string> HttpResponse::kSuffixType = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "}
};

const std::unordered_map<int, std::string> HttpResponse::kCodeStatus = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" }
};


const std::unordered_map<int, std::string> HttpResponse::kCodePath = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" }
};

HttpResponse::HttpResponse() {
    code_ = -1;
    path_ = "";
    source_directory_ = "";
    keep_alive_ = false;
    mmFile_ = nullptr;
    mmFileStatus_ = { 0 };
}

HttpResponse::~HttpResponse() {
    UnmapFile();
}

char* HttpResponse::File() const {
    return mmFile_;
}

size_t HttpResponse::FileLength() const {
    return mmFileStatus_.st_size;
}

void HttpResponse::Init(const std::string& src_directory, std::string& path, 
                                bool keep_alive, int code){
    assert(src_directory != "");
    if (mmFile_) UnmapFile();
    code_ = code;
    keep_alive_ = keep_alive;
    path_ = path;
    source_directory_ = src_directory;
    mmFile_ = nullptr;
    mmFileStatus_ = { 0 };
}

void HttpResponse::MakeResponse(Buffer& buffer) {
    if (stat((source_directory_ + path_).data(), &mmFileStatus_) < 0 || S_ISDIR(mmFileStatus_.st_mode)) 
    {
        code_ = 404;
    }
    else if (!(mmFileStatus_.st_mode & S_IROTH)) 
    {
        code_ = 403;
    }
    else if (code_ = -1)
    {
        code_ = 200;
    }
    ErrorHtml();
    AddStateLine(buffer);
    AddHeader(buffer);
    AddContent(buffer);
}

void HttpResponse::ErrorHtml() {
    if (kCodePath.count(code_))
    {
        path_ = kCodePath.find(code_) -> second;
        // path_ = kCodePath[code_];
        stat((source_directory_ + path_).data(), &mmFileStatus_);
    }
}

void HttpResponse::AddStateLine(Buffer& buffer) {
    std::string status;
    if (kCodePath.count(code_))
    {
        status = kCodePath.find(code_) -> second;
        // status = kCodePath[code_];
    }
    else
    {
        code_ = 400;
        status = kCodePath.find(code_) -> second;
        // status = kCodePath[code_];
    }
    buffer.Append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader(Buffer& buffer) {
    buffer.Append("Connection: ");
    if (keep_alive_)
    {
        buffer.Append("keep-alive\r\n");
        buffer.Append("keep-alive: max = 6, timeout = 120\r\n");
    }
    else
    {
        buffer.Append("close\r\n");
    }
    buffer.Append("Content-type: " + GetFileType() + "\r\n");
}

void HttpResponse::AddContent(Buffer& buffer) {
    int src_fd = open((source_directory_ + path_).data(), O_RDONLY);
    if (src_fd < 0)
    {
        ErrorContent(buffer, "File Not Found!");
        return;
    }

    LOG_DEBUG("file path %s", (source_directory_ + path_).data());
    int* mm_fd = (int*)mmap(0, mmFileStatus_.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    if (*mm_fd == -1)
    {
        ErrorContent(buffer, "File Not Found!");
        return;
    }
    mmFile_ = (char*)mm_fd;
    close(src_fd);
    buffer.Append("Content-type: " + std::to_string(mmFileStatus_.st_size) + "\r\n\r\n");
}

void HttpResponse::UnmapFile() {
    if (mmFile_)
    {
        munmap(mmFile_, mmFileStatus_.st_size);
        mmFile_ = nullptr;
    }
}

std::string HttpResponse::GetFileType() {
    std::string::size_type idx = path_.find_last_of('.');
    if (idx == std::string::npos) return "text/plain";

    std::string suffix = path_.substr(idx);
    if (kSuffixType.count(suffix)) return kSuffixType.find(suffix) -> second;
    // if (kSuffixType.count(suffix)) return kSuffixType[suffix];

    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer& buffer, std::string&& message) 
{
    std::string body, status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(kCodeStatus.count(code_) == 1) 
    {
        status = kCodeStatus.find(code_) -> second;
        // status = kCodeStatus[code_];
    } 
    else 
    {
        status = "Bad Request";
    }
    body += std::to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buffer.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buffer.Append(body);
}