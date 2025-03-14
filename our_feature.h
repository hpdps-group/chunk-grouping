#pragma once
#include <vector>

enum Mode
{
    AVERAGE = 0,
    FANGCHA = 1,
    MAX = 2,
    MIN = 3,
    TOP_K = 4,
    ENTROPY = 5,
    SKEWNESS = 6,
    KURTOSIS = 7,
    DOWN_SAMPLE = 8,
    ZERO_CAL = 9
};

float cal(enum Mode mode,const std::vector<unsigned char>& content);
float Average(const std::vector<unsigned char>& content);
float Fangcha(const std::vector<unsigned char>& content);
int Max(const std::vector<unsigned char>& content);
int Min(const std::vector<unsigned char>& content);
float Shannon_entropy(const std::vector<unsigned char>& content);
std::vector<unsigned char> downsample(const std::vector<unsigned char>& content, int factor);
std::vector<unsigned char> region_sample_max(const std::vector<unsigned char>& content, size_t region_size);