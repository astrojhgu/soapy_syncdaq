#include "utils.hpp"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

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

std::vector<int> parse_int_list(const std::string& s) {
    std::vector<int> result;

    int current = 0;
    bool in_number = false;
    bool negative = false;
    bool has_digit = false; // 用于判断是否真的读到了数字

    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            continue; // 忽略空白
        } else if (c == ',') {
            if (in_number) {
                result.push_back(negative ? -current : current);
                current = 0;
                in_number = false;
                negative = false;
                has_digit = false;
            }
            // 连续逗号直接跳过
        } else if (c == '-') {
            if (in_number) {
                // 非法情况：数字中间出现 '-'
                return {};
            }
            negative = true;
            in_number = true;
        } else if (c >= '0' && c <= '9') {
            current = current * 10 + (c - '0');
            in_number = true;
            has_digit = true;
        } else {
            // 非法字符 → 按你的需求可以直接返回空
            return {};
        }
    }

    if (in_number) {
        result.push_back(negative ? -current : current);
    }

    return result;
}
