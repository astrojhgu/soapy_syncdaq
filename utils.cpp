#include "utils.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <string_view>
#include <charconv> // std::from_chars (高性能转换)
#include <cctype>

/**
 * 解析 IPv4 字符串并转换为 32 位大端序整数 (Network Byte Order)
 * 如果输入格式非法，返回 0
 */
uint32_t parse_ipv4(const char* input) {
    if (!input) return 0;

    uint32_t result = 0;
    int parts = 0;
    int current_part_val = 0;
    bool has_digit = false;

    while (true) {
        char c = *input++;

        if (c >= '0' && c <= '9') {
            current_part_val = current_part_val * 10 + (c - '0');
            has_digit = true;
            
            // 每一个分段不能超过 255
            if (current_part_val > 255) return 0;
        } 
        else if (c == '.' || c == '\0') {
            // 点号前必须有数字，且总段数不能超过 4
            if (!has_digit || parts >= 4) return 0;

            // 将当前段位移到正确的位置 (左移 24, 16, 8, 0 位)
            result = (result << 8) | current_part_val;
            parts++;
            
            // 重置段状态
            current_part_val = 0;
            has_digit = false;

            if (c == '\0') break;
        } 
        else {
            // 发现非法字符
            return 0;
        }
    }

    // 一个标准的 IPv4 必须恰好有 4 段
    return (parts == 4) ? result : 0;
}

std::string format_ipv4(uint32_t ip) {
    std::string result;
    result.reserve(15); // 预留最大长度（xxx.xxx.xxx.xxx），提升性能

    for (int i = 0; i < 4; ++i) {
        // 从高位到低位依次取出字节
        // i=0: 24位, i=1: 16位, i=2: 8位, i=3: 0位
        uint8_t octet = (ip >> (24 - i * 8)) & 0xFF;
        
        result += std::to_string(octet);
        
        // 前三个段后面需要加点
        if (i < 3) {
            result += '.';
        }
    }

    return result;
}

/**
 * 通用的 uint32_t 列表解析函数
 * @param s 输入字符串
 * @param delimiters 分隔符集合，默认为逗号和分号
 */
std::vector<uint32_t> parse_int_list(std::string_view s, std::string_view delimiters) {
    std::vector<uint32_t> result;
    
    size_t start = 0;
    while (start < s.size()) {
        // 1. 跳过前导空格和分隔符
        start = s.find_first_not_of(" \t\n\r" + std::string(delimiters), start);
        if (start == std::string_view::npos) break;

        // 2. 找到下一个分隔符的位置
        size_t end = s.find_first_of(std::string(delimiters), start);
        
        // 3. 提取子串并修剪尾部空格
        std::string_view part = s.substr(start, end - start);
        size_t last_num = part.find_last_not_of(" \t\n\r");
        if (last_num != std::string_view::npos) {
            part = part.substr(0, last_num + 1);
            
            // 4. 高性能转换 (std::from_chars 不分配内存，不抛异常)
            uint32_t value;
            auto [ptr, ec] = std::from_chars(part.data(), part.data() + part.size(), value);
            if (ec == std::errc()) {
                result.push_back(value);
            }
        }

        if (end == std::string_view::npos) break;
        start = end + 1;
    }

    return result;
}