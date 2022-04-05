/*
 * @auther      : Parkertao
 * @Date        : 2022-3-22
 * @copyright Apache 2.0
*/

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cassert>

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string& , std::string& , bool keep_alive = false, int code = -1);
    void MakeResponse(Buffer& );
    void UnmapFile();
    char* File() const;
    size_t FileLength() const;
    void ErrorContent(Buffer& , std::string&& );
    int Code() const { return code_; }

private:
    void AddStateLine(Buffer& );
    void AddHeader(Buffer& );
    void AddContent(Buffer& );

    void ErrorHtml();
    std::string GetFileType();

    int code_;
    bool keep_alive_;
    std::string path_;
    std::string source_directory_;
    char* mmFile_;
    struct stat mmFileStatus_;

    static const std::unordered_map<std::string, std::string> kSuffixType;
    static const std::unordered_map<int, std::string> kCodeStatus;
    static const std::unordered_map<int, std::string> kCodePath;
};


#endif  // HTTPRESPONSE_H