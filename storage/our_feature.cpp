#include <iostream>
#include <stdio.h>
#include <cmath>
#include <gsl/gsl_statistics.h>
#include <vector>
#include <map>
#include <variant>

#include "our_feature.h"

#define FILE_SIZE 8192

float cal(enum Mode mode,const std::vector<unsigned char>& content)
{
    switch (mode)
    {
        case AVERAGE:
        {
            float average = Average(content);
            return average;
        }
        case FANGCHA:
        {
            float fangcha = Fangcha(content);
            return fangcha;
        }
        case MAX:
        {
            int max = Max(content);
            return max;
        }
        case MIN:
        {
            int min = Min(content);
            return min;
        }
        case TOP_K:
        {
            /* TODO */
            char* temp = new char[FILE_SIZE];
            for(int i = 0;i < FILE_SIZE;i++)
            {
                temp[i] = content[i];
            }
            return 0;
        }
        case ENTROPY:
        {
            // Shannon entropy : H(X)=− (i=1)∑(n) p(xi)log2p(xi​)
            float sum = Shannon_entropy(content);
            return sum;
        }
        case SKEWNESS:
        {
            double* temp = new double[FILE_SIZE];
            for(int i = 0;i < FILE_SIZE;i++)
            {
                temp[i] = content[i];
            }
            double skew = gsl_stats_skew(temp, 1, FILE_SIZE);
            return skew;
        }
        case KURTOSIS:
        {
            double* temp = new double[FILE_SIZE];
            for(int i = 0;i < FILE_SIZE;i++)
            {
                temp[i] = content[i];
            }
            double kurtosis = gsl_stats_kurtosis(temp, 1, FILE_SIZE);
            return kurtosis;
        }
        case DOWN_SAMPLE:
        {
            downsample(content, 8);
            // #TODO
            return 0;
        }
        case ZERO_CAL:
        {
            size_t zero_cnt = 0;
            for (unsigned char c : content) 
            {
                if (c == 0) 
                {
                    zero_cnt++;
                }
            }
            return (float)zero_cnt / content.size();
        }
        default: return 0;
    };
}

float Average(const std::vector<unsigned char>& content)
{
    float sum = 0;
    for (unsigned char c : content) 
    {
        sum += static_cast<unsigned char>(c);
    }
    return sum / content.size();
}

float Fangcha(const std::vector<unsigned char>& content)
{
    float sum = 0;
    float average = Average(content); 
    for (unsigned char c : content) 
    {
        float diff = static_cast<unsigned char>(c) - average;
        sum += diff * diff;
    }
    return sum / content.size();
}

int Max(const std::vector<unsigned char>& content)
{
    float max = 0;
    for (unsigned char c : content) 
    {
        if (static_cast<unsigned char>(c) > max) 
        {
            max = static_cast<unsigned char>(c);
        }
    }
    return max;
}

int Min(const std::vector<unsigned char>& content)
{
    float min = content[0];
    for (unsigned char c : content) 
    {
        if ((c) < min) 
        {
            min = (c);
        }
    }
    return min;
}

float Shannon_entropy(const std::vector<unsigned char>& content)
{
    float entropy = 0.0f;
    std::map<unsigned char, int> frequency;
    for (unsigned char c : content)
    {
        frequency[c]++;
    }
    for (const auto& pair : frequency)
    {
        float p = static_cast<float>(pair.second) / content.size();
        entropy -= p * std::log2(p);
    }
    return entropy;
}

// 固定下采样
std::vector<unsigned char> downsample(const std::vector<unsigned char>& content, int factor) 
{
    std::vector<unsigned char> downsampled_content;
    for (size_t i = 0; i < content.size(); i += factor) {
        downsampled_content.push_back(content[i]);
    }
    return downsampled_content;
}

// 最大下采样
std::vector<unsigned char> region_sample_max(const std::vector<unsigned char>& content, size_t region_size = 8) 
{
    std::vector<unsigned char> sampled_max;
    for (size_t i = 0; i < content.size(); i += region_size) 
    {
        size_t end = std::min(i + region_size, content.size());  // 超出size处理
        std::vector<unsigned char> region(content.begin() + i, content.begin() + end); // 一个region
        float max_value = Max(region);
        sampled_max.push_back(static_cast<unsigned char>(max_value));
    }
    return sampled_max;
}