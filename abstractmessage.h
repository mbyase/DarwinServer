#pragma once

#include <string>
#include <map>
#include <vector>

class AbstractMessage
{
public:
    virtual std::string first_line() const = 0;
    virtual std::string header() const = 0;
    virtual std::string to_string() const = 0;

    std::map<std::string, std::string> header_map;
    std::string version = "HTTP/1.1";
    std::string data;

protected:
    static std::vector<std::string> _split_string(
        const std::string & source, const std::string & c);
};