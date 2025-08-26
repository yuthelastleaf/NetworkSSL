#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <bitset>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <map>

#ifdef _WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "dbscan.h"

#include <functional>

// 定义函数类型别名
using DotDetectorFunc = std::function<std::vector<cv::Point>(const cv::Mat&, int)>;


struct ParsedInfo {
    std::string featureCode;     // 特征码 "1010"
    std::string checkCode;       // 校验码（后4位）
    std::vector<int> ipBytes;    // IPv4的4个字节
    std::string ipAddress;       // 完整IP地址
    bool isValid;                // 是否是有效的点阵
};

class DotMatrixParser {
private:
    const int MATRIX_ROWS = 5;
    const int MATRIX_COLS = 8;
    const std::string TARGET_FEATURE = "1010";  // 目标特征码

    // 将聚类点转换为5x8的二进制矩阵
    std::vector<std::vector<int>> clusterToMatrix(const std::vector<Point>& clusterPoints) {
        if (clusterPoints.size() == 0) {
            return std::vector<std::vector<int>>(MATRIX_ROWS, std::vector<int>(MATRIX_COLS, 0));
        }

        // 找到点的边界
        int minX = clusterPoints[0]._point.x, maxX = minX;
        int minY = clusterPoints[0]._point.y, maxY = minY;

        for (const auto& point : clusterPoints) {
            minX = std::min(minX, point._point.x);
            maxX = std::max(maxX, point._point.x);
            minY = std::min(minY, point._point.y);
            maxY = std::max(maxY, point._point.y);
        }

        // 计算网格间距
        double stepX = (maxX - minX) / (double)(MATRIX_COLS - 1);
        double stepY = (maxY - minY) / (double)(MATRIX_ROWS - 1);

        // 初始化矩阵
        std::vector<std::vector<int>> matrix(MATRIX_ROWS, std::vector<int>(MATRIX_COLS, 0));

        // 将点映射到网格
        for (const auto& point : clusterPoints) {
            // 计算在网格中的位置
            int gridX = std::round((point._point.x - minX) / stepX);
            int gridY = std::round((point._point.y - minY) / stepY);

            // 确保在有效范围内
            if (gridX >= 0 && gridX < MATRIX_COLS && gridY >= 0 && gridY < MATRIX_ROWS) {
                matrix[gridY][gridX] = 1;
            }
        }

        return matrix;
    }

    // 将二进制字符串转换为十进制数
    int binaryToDecimal(const std::string& binary) {
        int decimal = 0;
        for (size_t i = 0; i < binary.length(); i++) {
            if (binary[i] == '1') {
                decimal += (1 << (binary.length() - 1 - i));
            }
        }
        return decimal;
    }

    // 解析单个矩阵
    ParsedInfo parseMatrix(const std::vector<std::vector<int>>& matrix) {
        ParsedInfo info;
        info.isValid = false;

        // 检查矩阵尺寸
        if (matrix.size() != MATRIX_ROWS || matrix[0].size() != MATRIX_COLS) {
            // std::cout << "矩阵尺寸不匹配" << std::endl;
            return info;
        }

        // 解析第一行（同步位+校验码）
        std::string firstRow = "";
        for (int col = 0; col < MATRIX_COLS; col++) {
            firstRow += std::to_string(matrix[0][col]);
        }

        // 转换为字节值
        int headerByte = binaryToDecimal(firstRow);

        // 提取同步位（高4位）
        uint8_t syncBits = (headerByte >> 4) & 0x0F;
        std::string syncPattern = "";
        for (int i = 3; i >= 0; i--) {
            syncPattern += std::to_string((syncBits >> i) & 1);
        }
        info.featureCode = syncPattern;

        // 检查同步位是否匹配（假设TARGET_FEATURE是"1010"）
        if (info.featureCode != TARGET_FEATURE) {
            /*std::cout << "同步位不匹配，期望: " << TARGET_FEATURE
                << "，实际: " << info.featureCode << std::endl;*/
            return info;  // 不是目标点阵
        }

        // 提取校验码（低4位）
        uint8_t receivedChecksum = headerByte & 0x0F;
        info.checkCode = std::to_string(receivedChecksum);

        /*std::cout << "\n=== 解析矩阵 ===" << std::endl;
        std::cout << "头部字节: 0x" << std::hex << headerByte << std::dec << std::endl;
        std::cout << "同步位: " << info.featureCode << std::endl;
        std::cout << "接收到的校验码: " << (int)receivedChecksum << std::endl;*/

        // 解析后四行为IPv4地址
        info.ipBytes.clear();
        for (int row = 1; row < MATRIX_ROWS; row++) {
            std::string rowBinary = "";
            for (int col = 0; col < MATRIX_COLS; col++) {
                rowBinary += std::to_string(matrix[row][col]);
            }
            int byteValue = binaryToDecimal(rowBinary);
            info.ipBytes.push_back(byteValue);
            /*std::cout << "数据行" << row << ": " << byteValue
                << " 二进制: " << rowBinary << std::endl;*/
        }

        // 校验IP字节数量
        if (info.ipBytes.size() != 4) {
            // std::cout << "IP字节数量不正确: " << info.ipBytes.size() << std::endl;
            return info;
        }

        // 计算校验和进行验证
        uint32_t calculatedSum = 0;
        for (uint8_t byte : info.ipBytes) {
            calculatedSum += byte;
        }
        uint8_t calculatedChecksum = calculatedSum & 0x0F; // 只取低4位

        /*std::cout << "IP字节: ";
        for (int i = 0; i < 4; i++) {
            std::cout << info.ipBytes[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "计算的校验和: " << (int)calculatedChecksum
            << " (0x" << std::hex << (int)calculatedChecksum << std::dec << ")" << std::endl;*/

        // 验证校验和
        if (receivedChecksum != calculatedChecksum) {
            /*std::cout << "校验和不匹配！接收: " << (int)receivedChecksum
                << "，计算: " << (int)calculatedChecksum << std::endl;*/
            return info;  // 校验失败
        }

        // std::cout << "校验和验证通过！" << std::endl;

        // 构造IP地址字符串
        info.ipAddress = std::to_string(info.ipBytes[0]) + "." +
            std::to_string(info.ipBytes[1]) + "." +
            std::to_string(info.ipBytes[2]) + "." +
            std::to_string(info.ipBytes[3]);

        // std::cout << "解析出的IP地址: " << info.ipAddress << std::endl;

        info.isValid = true;
        return info;
    }

public:
    // 主要解析方法
    std::vector<ParsedInfo> parseClusters(const std::map<int, std::vector<Point>>& clusters) {
        std::vector<ParsedInfo> results;

        for (const auto& cluster : clusters) {
            int clusterID = cluster.first;
            const std::vector<Point>& points = cluster.second;

            // 转换为矩阵
            auto matrix = clusterToMatrix(points);

            //// 打印矩阵（调试用）
            //std::cout << "点阵矩阵:" << std::endl;
            //for (int row = 0; row < MATRIX_ROWS; row++) {
            //    for (int col = 0; col < MATRIX_COLS; col++) {
            //        std::cout << matrix[row][col] << " ";
            //    }
            //    std::cout << std::endl;
            //}

            // 解析信息
            ParsedInfo info = parseMatrix(matrix);

            if (info.isValid) {
                /*std::cout << "✓ 发现有效的点阵信息!" << std::endl;
                std::cout << "  特征码: " << info.featureCode << std::endl;
                std::cout << "  校验码: " << info.checkCode << std::endl;
                std::cout << "  IPv4地址: " << info.ipAddress << std::endl;
                std::cout << "  字节详情: ";
                for (size_t i = 0; i < info.ipBytes.size(); i++) {
                    std::cout << info.ipBytes[i];
                    if (i < info.ipBytes.size() - 1) std::cout << ".";
                }
                std::cout << std::endl;*/

                results.push_back(info);
            }
            /*else {
                std::cout << "✗ 聚类 " << clusterID << " 不匹配目标点阵模式" << std::endl;
                if (!info.featureCode.empty()) {
                    std::cout << "  检测到的特征码: " << info.featureCode << " (期望: " << TARGET_FEATURE << ")" << std::endl;
                }
            }*/
        }

        // std::cout << "\n=== 解析完成，找到 " << results.size() << " 个有效信息 ===" << std::endl;
        return results;
    }

    // 打印所有解析结果
    void printResults(const std::vector<ParsedInfo>& results) {
        if (results.empty()) {
            std::cout << "没有找到有效的点阵信息。" << std::endl;
            return;
        }

        std::cout << "\n========== 解析结果汇总 ==========" << std::endl;
        for (size_t i = 0; i < results.size(); i++) {
            const auto& info = results[i];
            std::cout << "结果 " << (i + 1) << ":" << std::endl;
            std::cout << "  IP地址: " << info.ipAddress << std::endl;
            std::cout << "  特征码: " << info.featureCode << std::endl;
            std::cout << "  校验码: " << info.checkCode << std::endl;
            std::cout << std::endl;
        }
    }

    // 返回所有IP地址，用分号分隔
    std::string getResultsString(const std::vector<ParsedInfo>& results) {
        if (results.empty()) {
            return "";  // 或者返回空字符串
        }

        std::string resultString = "";
        for (size_t i = 0; i < results.size(); i++) {
            const auto& info = results[i];
            if (resultString.find(info.ipAddress) != std::string::npos) {
                continue;
            }
            resultString += info.ipAddress;

            // 如果不是最后一个，添加分号分隔符
            if (i < results.size() - 1) {
                resultString += ";";
            }
        }

        return resultString;
    }
};

class IntelligentDotMatrixReader {
private:
    static const int BITS_PER_ROW = 8;
    static const int TOTAL_ROWS = 5;
    static const int HEADER_ROW = 0;
    static const int DATA_START_ROW = 1;
    static const uint8_t SYNC_PATTERN = 0xA0;
    static const uint8_t SYNC_MASK = 0xF0;
    static const uint8_t CHECKSUM_MASK = 0x0F;

private:
    std::string m_parseip;

public:
    struct DecodedResult {
        std::string ipAddress;
        bool isValid;
        float confidence;

        // 验证信息
        bool syncFound;
        bool checksumValid;
        bool regionValid;      // 区域验证
        bool patternValid;     // 模式验证
        bool contextValid;     // 上下文验证

        uint8_t receivedChecksum;
        uint8_t calculatedChecksum;
        std::vector<uint8_t> rawBytes;

        // 几何信息
        cv::Rect boundingRect;           // 包围矩形
        std::vector<cv::Point> dotPoints; // 实际检测到的点
        float avgDotSize;                // 平均点大小
        float uniformityScore;           // 均匀性评分
        int backgroundNoise;             // 背景噪声水平

        DecodedResult() : isValid(false), confidence(0.0f), syncFound(false),
            checksumValid(false), regionValid(false), patternValid(false),
            contextValid(false), receivedChecksum(0), calculatedChecksum(0),
            avgDotSize(0.0f), uniformityScore(0.0f), backgroundNoise(0) {}
    };

    struct IntelligentDetectionParams {
        // 前景分离参数
        bool enableBackgroundSubtraction = true;
        bool enableMoireFiltering = true;        // 莫尔条纹滤波
        bool enableTextureFiltering = true;      // 纹理滤波

        // 区域检测参数
        float minRegionArea = 0.001f;           // 最小区域面积比例
        float maxRegionArea = 0.1f;             // 最大区域面积比例
        float expectedAspectRatio = 1.6f;       // 期望长宽比 (8:5)
        float aspectRatioTolerance = 0.4f;      // 长宽比容忍度

        // 点检测参数
        int minDotsInRegion = 15;               // 区域内最少点数
        int maxDotsInRegion = 40;               // 区域内最多点数
        float dotSizeVariationThreshold = 0.3f; // 点大小变化阈值
        float spacingVariationThreshold = 0.25f; // 间距变化阈值

        // 背景分析参数
        int backgroundSampleRadius = 50;        // 背景采样半径
        float backgroundNoiseThreshold = 30.0f; // 背景噪声阈值

        // 莫尔条纹检测参数
        float moireFrequencyMin = 0.1f;         // 最小莫尔频率
        float moireFrequencyMax = 0.5f;         // 最大莫尔频率
        float moireStrengthThreshold = 20.0f;   // 莫尔强度阈值

        // 调试参数
        bool saveDebugImages = true;
        bool verboseOutput = true;
        std::string debugPrefix = "debug";
    };

private:
    IntelligentDetectionParams params;

public:
    IntelligentDotMatrixReader() = default;

    void setDetectionParams(const IntelligentDetectionParams& newParams) {
        params = newParams;
    }

    std::vector<DecodedResult> readFromImage(const std::string& imagePath) {
        cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);
        if (image.empty()) {
            std::cerr << "[ERROR] 无法读取图片: " << imagePath << std::endl;
            return {};
        }
        return processImageIntelligently(image, imagePath);
    }

    std::vector<DecodedResult> processImageIntelligently(const cv::Mat& image, const std::string& debugPrefix = "debug") {
        std::vector<DecodedResult> results;

        // 1. 图像预处理和背景分析
        cv::Mat cleanImage = intelligentPreprocessing(image, debugPrefix);

        // 2. 检测候选区域（关键步骤！）
        detectDotMatrixRegions(cleanImage, debugPrefix);


        // 5. 调试可视化
        /*if (params.saveDebugImages) {
            saveIntelligentDebugVisualization(image, candidateRegions, results, debugPrefix);
        }*/

        return results;
    }

    std::string GetParseIp() {
        return m_parseip;
    }

private:
    // 检测二值图中的点
    std::vector<cv::Point> detectDotsFromMultipleBinaries(const std::vector<cv::Mat>& binaryImages,
        DotDetectorFunc det_func,
        const std::string& debugPrefix = "") {

        

        std::vector<cv::Point> allCandidateDots;
        if (!det_func) {
            return allCandidateDots;
        }

        // 1. 从每个二值图中检测候选点
        for (size_t i = 0; i < binaryImages.size(); i++) {
            auto dotsFromBinary = det_func(binaryImages[i], i);
            allCandidateDots.insert(allCandidateDots.end(), dotsFromBinary.begin(), dotsFromBinary.end());

            if (params.saveDebugImages && !debugPrefix.empty()) {
                saveDotsVisualization(binaryImages[i], dotsFromBinary,
                    debugPrefix + "_binary_" + std::to_string(i) + "_dots.png");
            }
        }

        // 2. 合并和去重相近的点
        std::vector<cv::Point> mergedDots = mergeNearbyDots(allCandidateDots);

        // 3. 基于上下文进一步筛选
        std::vector<cv::Point> filteredDots = filterDotsByContext(mergedDots);

        return filteredDots;
    }

    static std::vector<cv::Point> detectDotsInSingleBinary_old(const cv::Mat& binary, int binaryIndex) {
        std::vector<cv::Point> dots;
        std::vector<std::vector<cv::Point>> contours;

        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // 动态计算点的合理大小范围
        int imageArea = binary.rows * binary.cols;
        double minDotArea = imageArea * 0.00005;  // 0.005%的图像面积
        double maxDotArea = imageArea * 0.003;    // 0.3%的图像面积

        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);

            // 面积筛选
            if (area < minDotArea || area > maxDotArea) continue;

            // 圆度检测
            double perimeter = cv::arcLength(contour, true);
            double circularity = (perimeter > 0) ? 4 * CV_PI * area / (perimeter * perimeter) : 0;

            if (circularity < 0.3) continue; // 圆度阈值

            // 长宽比检测（圆点应该接近正方形）
            cv::Rect boundingRect = cv::boundingRect(contour);
            double aspectRatio = static_cast<double>(boundingRect.width) / boundingRect.height;
            if (aspectRatio < 0.5 || aspectRatio > 2.0) continue; // 长宽比应该接近1

            // 实心度检测（排除空心圆环）
            std::vector<cv::Point> hull;
            cv::convexHull(contour, hull);
            double convexArea = cv::contourArea(hull);
            double solidity = area / convexArea;
            if (solidity < 0.8) continue; // 实心度阈值

            // 计算质心作为点的位置
            cv::Moments moments = cv::moments(contour);
            if (moments.m00 > 0) {
                cv::Point center(static_cast<int>(moments.m10 / moments.m00),
                    static_cast<int>(moments.m01 / moments.m00));
                dots.push_back(center);
            }
        }

        return dots;
    }

    static std::vector<cv::Point> detectDotsInSingleBinary(const cv::Mat& binary, int binaryIndex) {
        std::vector<cv::Point> dots;
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // 大幅放宽面积范围
        int imageArea = binary.rows * binary.cols;
        double minDotArea = imageArea * 0.000001;  // 0.0001% - 更小的最小面积
        double maxDotArea = imageArea * 0.01;      // 1% - 更大的最大面积

        std::cout << "二值图 " << binaryIndex << " 检测参数:" << std::endl;
        std::cout << "图像尺寸: " << binary.cols << "x" << binary.rows << std::endl;
        std::cout << "面积范围: " << minDotArea << " - " << maxDotArea << std::endl;
        std::cout << "总轮廓数: " << contours.size() << std::endl;

        for (size_t i = 0; i < contours.size(); i++) {
            const auto& contour = contours[i];
            double area = cv::contourArea(contour);

            // 调试信息：显示每个轮廓的参数
            if (i < 10) { // 只显示前10个轮廓的详细信息
                std::cout << "轮廓 " << i << " - 面积: " << area << std::endl;
            }

            // 1. 面积筛选 - 放宽范围
            if (area < minDotArea) {
                if (i < 10) std::cout << "  -> 面积太小，跳过" << std::endl;
                continue;
            }
            if (area > maxDotArea) {
                if (i < 10) std::cout << "  -> 面积太大，跳过" << std::endl;
                continue;
            }

            // 2. 大幅放宽圆度检测
            double perimeter = cv::arcLength(contour, true);
            double circularity = (perimeter > 0) ? 4 * CV_PI * area / (perimeter * perimeter) : 0;
            if (circularity < 0.1) { // 从0.3降到0.1
                if (i < 10) std::cout << "  -> 圆度过低: " << circularity << std::endl;
                continue;
            }

            // 3. 放宽长宽比检测
            cv::Rect boundingRect = cv::boundingRect(contour);
            double aspectRatio = static_cast<double>(boundingRect.width) / boundingRect.height;
            if (aspectRatio < 0.3 || aspectRatio > 3.0) { // 从0.5-2.0放宽到0.3-3.0
                if (i < 10) std::cout << "  -> 长宽比不符: " << aspectRatio << std::endl;
                continue;
            }

            // 4. 放宽实心度检测
            std::vector<cv::Point> hull;
            cv::convexHull(contour, hull);
            double convexArea = cv::contourArea(hull);
            double solidity = (convexArea > 0) ? area / convexArea : 0;
            if (solidity < 0.5) { // 从0.8降到0.5
                if (i < 10) std::cout << "  -> 实心度过低: " << solidity << std::endl;
                continue;
            }

            // 5. 计算质心作为点的位置
            cv::Moments moments = cv::moments(contour);
            if (moments.m00 > 0) {
                cv::Point center(static_cast<int>(moments.m10 / moments.m00),
                    static_cast<int>(moments.m01 / moments.m00));
                dots.push_back(center);

                if (i < 10) {
                    std::cout << "  -> 检测到点: (" << center.x << ", " << center.y
                        << ") 面积:" << area << " 圆度:" << circularity
                        << " 长宽比:" << aspectRatio << " 实心度:" << solidity << std::endl;
                }
            }
        }

        std::cout << "二值图 " << binaryIndex << " 检测到 " << dots.size() << " 个点" << std::endl;
        return dots;
    }

    // 或者更简化的版本，只保留最基本的筛选条件
    std::vector<cv::Point> detectDotsSimple(const cv::Mat& binary, int binaryIndex) {
        std::vector<cv::Point> dots;
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // 只用面积筛选，其他条件都去掉
        int imageArea = binary.rows * binary.cols;
        double minDotArea = 3;  // 至少3个像素
        double maxDotArea = imageArea * 0.01;  // 最大1%

        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);

            if (area >= minDotArea && area <= maxDotArea) {
                // 直接计算质心
                cv::Moments moments = cv::moments(contour);
                if (moments.m00 > 0) {
                    cv::Point center(static_cast<int>(moments.m10 / moments.m00),
                        static_cast<int>(moments.m01 / moments.m00));
                    dots.push_back(center);
                }
            }
        }

        return dots;
    }


    std::vector<cv::Point> mergeNearbyDots(const std::vector<cv::Point>& candidateDots) {
        if (candidateDots.empty()) return {};

        std::vector<cv::Point> mergedDots;
        std::vector<bool> used(candidateDots.size(), false);

        // 合并距离阈值（像素）
        const double mergeDistance = 8.0;

        for (size_t i = 0; i < candidateDots.size(); i++) {
            if (used[i]) continue;

            // 收集所有距离当前点很近的点
            std::vector<cv::Point> cluster;
            cluster.push_back(candidateDots[i]);
            used[i] = true;

            for (size_t j = i + 1; j < candidateDots.size(); j++) {
                if (used[j]) continue;

                double distance = cv::norm(candidateDots[i] - candidateDots[j]);
                if (distance <= mergeDistance) {
                    cluster.push_back(candidateDots[j]);
                    used[j] = true;
                }
            }

            // 计算聚类中心作为合并后的点位置
            if (!cluster.empty()) {
                int sumX = 0, sumY = 0;
                for (const auto& point : cluster) {
                    sumX += point.x;
                    sumY += point.y;
                }
                cv::Point mergedCenter(sumX / static_cast<int>(cluster.size()),
                    sumY / static_cast<int>(cluster.size()));
                mergedDots.push_back(mergedCenter);
            }
        }

        return mergedDots;
    }

    std::vector<cv::Point> filterDotsByContext(const std::vector<cv::Point>& dots) {
        if (dots.size() < 5) return dots; // 点太少，不进行上下文筛选

        std::vector<cv::Point> filteredDots;

        // 计算点之间的平均距离，用于判断合理的邻居距离
        std::vector<double> distances;
        for (size_t i = 0; i < dots.size(); i++) {
            for (size_t j = i + 1; j < dots.size(); j++) {
                distances.push_back(cv::norm(dots[i] - dots[j]));
            }
        }

        if (distances.empty()) return dots;

        std::sort(distances.begin(), distances.end());
        double medianDistance = distances[distances.size() / 2];
        double maxNeighborDistance = medianDistance * 2.0; // 合理邻居的最大距离

        // 为每个点计算"邻居支持度"
        for (const auto& dot : dots) {
            int neighborCount = 0;

            for (const auto& otherDot : dots) {
                if (dot == otherDot) continue;

                double distance = cv::norm(dot - otherDot);
                if (distance <= maxNeighborDistance) {
                    neighborCount++;
                }
            }

            // 如果一个点有足够的邻居，认为它是有效的
            // 对于5×8=40个点的网格，每个点平均应该有2-6个近邻
            if (neighborCount >= 1) { // 至少有1个邻居（对于稀疏如1.1.1.1的情况）
                filteredDots.push_back(dot);
            }
        }

        return filteredDots;
    }

    void saveDotsVisualization(const cv::Mat& binary, const std::vector<cv::Point>& dots,
        const std::string& filename) {
        cv::Mat vis;
        cv::cvtColor(binary, vis, cv::COLOR_GRAY2BGR);

        // 绘制检测到的点
        for (size_t i = 0; i < dots.size(); i++) {
            cv::circle(vis, dots[i], 3, cv::Scalar(0, 255, 0), 2);
            // 可选：添加点的编号
            cv::putText(vis, std::to_string(i),
                cv::Point(dots[i].x + 5, dots[i].y - 5),
                cv::FONT_HERSHEY_SIMPLEX, 0.3, cv::Scalar(255, 255, 0), 1);
        }

        // 添加统计信息
        std::string info = "Dots: " + std::to_string(dots.size());
        cv::putText(vis, info, cv::Point(10, 30),
            cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);

        cv::imwrite(filename, vis);
    }


    void visualizeDotsOnOriginal(const cv::Mat& originalImage,
        const std::vector<cv::Point>& detectedDots,
        const std::string& outputFilename = "detected_dots_on_original.png") {

        cv::Mat visualization = originalImage.clone();

        // 确保是彩色图像
        if (visualization.channels() == 1) {
            cv::cvtColor(visualization, visualization, cv::COLOR_GRAY2BGR);
        }

        std::cout << "[INFO] 在原图上绘制 " << detectedDots.size() << " 个检测点" << std::endl;

        // 绘制每个检测到的点
        for (size_t i = 0; i < detectedDots.size(); i++) {
            const cv::Point& dot = detectedDots[i];

            // 绘制大圆圈突出显示点位置
            cv::circle(visualization, dot, 8, cv::Scalar(0, 0, 255), 2);        // 红色外圈
            cv::circle(visualization, dot, 4, cv::Scalar(0, 255, 0), 2);        // 绿色中圈
            cv::circle(visualization, dot, 1, cv::Scalar(255, 255, 255), -1);   // 白色中心点

            // 添加点的编号（可选）
            cv::putText(visualization, std::to_string(i),
                cv::Point(dot.x + 12, dot.y + 5),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 0), 1);
        }

        // 在图像顶部添加统计信息
        std::string info = "Detected Dots: " + std::to_string(detectedDots.size());
        cv::putText(visualization, info, cv::Point(10, 30),
            cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 255), 2);

        // 如果点数在合理范围内，尝试绘制可能的网格连接线
        if (detectedDots.size() >= 10 && detectedDots.size() <= 50) {
            drawPossibleGrid(visualization, detectedDots);
        }

        // 保存结果
        cv::imwrite(outputFilename, visualization);
        std::cout << "[SUCCESS] 可视化结果已保存到: " << outputFilename << std::endl;
    }

    void visualizeClusteredDots(const cv::Mat& originalImage,
        const std::vector<Point>& clusteredDots,
        const std::string& outputFilename = "clustered_dots_visualization.png") {

        cv::Mat visualization = originalImage.clone();

        // 确保是彩色图像
        if (visualization.channels() == 1) {
            cv::cvtColor(visualization, visualization, cv::COLOR_GRAY2BGR);
        }

        // 统计聚类信息
        std::map<int, int> clusterCounts;
        int maxClusterID = -2;  // -1是噪声点，所以从-2开始
        int noisePoints = 0;

        for (const auto& point : clusteredDots) {
            if (point.clusterID == -1) {
                noisePoints++;
            }
            else {
                clusterCounts[point.clusterID]++;
                maxClusterID = std::max(maxClusterID, point.clusterID);
            }
        }

        /*std::cout << "[INFO] 聚类结果: " << clusterCounts.size() << " 个聚类, "
            << noisePoints << " 个噪声点" << std::endl;*/

        // 预定义颜色列表（BGR格式）
        std::vector<cv::Scalar> clusterColors = {
            cv::Scalar(255, 0, 0),     // 蓝色
            cv::Scalar(0, 255, 0),     // 绿色
            cv::Scalar(0, 0, 255),     // 红色
            cv::Scalar(255, 255, 0),   // 青色
            cv::Scalar(255, 0, 255),   // 洋红色
            cv::Scalar(0, 255, 255),   // 黄色
            cv::Scalar(128, 0, 128),   // 紫色
            cv::Scalar(255, 165, 0),   // 橙色
            cv::Scalar(0, 128, 128),   // 深青色
            cv::Scalar(128, 128, 0),   // 橄榄色
            cv::Scalar(255, 192, 203), // 粉色
            cv::Scalar(0, 128, 0),     // 深绿色
        };

        // 如果聚类数超过预定义颜色数，生成随机颜色
        cv::RNG rng(12345);
        while (clusterColors.size() <= maxClusterID) {
            clusterColors.push_back(cv::Scalar(rng.uniform(0, 256),
                rng.uniform(0, 256),
                rng.uniform(0, 256)));
        }

        // 绘制每个点
        for (size_t i = 0; i < clusteredDots.size(); i++) {
            const Point& point = clusteredDots[i];
            const cv::Point& pos = point._point;

            cv::Scalar color;
            int circleSize = 8;

            if (point.clusterID == -1) {
                // 噪声点用灰色表示
                color = cv::Scalar(128, 128, 128);
                circleSize = 4;  // 噪声点画小一些
            }
            else {
                // 使用聚类对应的颜色
                color = clusterColors[point.clusterID % clusterColors.size()];
            }

            // 绘制点
            cv::circle(visualization, pos, circleSize, color, 2);              // 外圈
            cv::circle(visualization, pos, circleSize / 2, color, -1);           // 填充内圈
            cv::circle(visualization, pos, 1, cv::Scalar(255, 255, 255), -1);  // 白色中心点

            // 添加聚类ID标签
            if (point.clusterID != -1) {
                cv::putText(visualization, std::to_string(point.clusterID),
                    cv::Point(pos.x + 12, pos.y + 5),
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, color, 1);
            }
        }

        // 在图像上添加聚类统计信息
        int yOffset = 30;
        std::string totalInfo = "Total Points: " + std::to_string(clusteredDots.size()) +
            ", Clusters: " + std::to_string(clusterCounts.size()) +
            ", Noise: " + std::to_string(noisePoints);
        cv::putText(visualization, totalInfo, cv::Point(10, yOffset),
            cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 255, 255), 2);

        // 显示每个聚类的点数
        yOffset += 30;
        for (const auto& cluster : clusterCounts) {
            std::string clusterInfo = "Cluster " + std::to_string(cluster.first) +
                ": " + std::to_string(cluster.second) + " points";
            cv::Scalar textColor = clusterColors[cluster.first % clusterColors.size()];
            cv::putText(visualization, clusterInfo, cv::Point(10, yOffset),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, textColor, 2);
            yOffset += 25;

            // 避免文字太多超出图像范围
            if (yOffset > visualization.rows - 50) break;
        }

        // 绘制聚类的凸包（可选，帮助可视化聚类边界）
        drawClusterHulls(visualization, clusteredDots, clusterColors);

        // 保存结果
        cv::imwrite(outputFilename, visualization);
        // std::cout << "[SUCCESS] 聚类可视化结果已保存到: " << outputFilename << std::endl;
    }

    // 辅助函数：绘制每个聚类的凸包
    void drawClusterHulls(cv::Mat& image, const std::vector<Point>& clusteredDots,
        const std::vector<cv::Scalar>& clusterColors) {

        // 按聚类ID分组点
        std::map<int, std::vector<cv::Point>> clusterPoints;
        for (const auto& point : clusteredDots) {
            if (point.clusterID != -1) {  // 忽略噪声点
                clusterPoints[point.clusterID].push_back(point._point);
            }
        }

        // 为每个聚类绘制凸包
        for (const auto& cluster : clusterPoints) {
            if (cluster.second.size() >= 3) {  // 至少需要3个点才能形成凸包
                std::vector<cv::Point> hull;
                cv::convexHull(cluster.second, hull);

                // 绘制半透明的凸包区域
                cv::Scalar color = clusterColors[cluster.first % clusterColors.size()];

                // 创建临时mask来绘制半透明区域
                cv::Mat overlay = image.clone();
                cv::fillPoly(overlay, hull, color);
                cv::addWeighted(image, 0.7, overlay, 0.3, 0, image);

                // 绘制凸包边界线
                cv::polylines(image, hull, true, color, 2);
            }
        }
    }

    void drawPossibleGrid(cv::Mat& image, const std::vector<cv::Point>& dots) {
        if (dots.size() < 4) return;

        // 计算点之间的平均距离来估算网格间距
        std::vector<double> distances;
        for (size_t i = 0; i < dots.size(); i++) {
            for (size_t j = i + 1; j < dots.size(); j++) {
                double dist = cv::norm(dots[i] - dots[j]);
                if (dist > 10 && dist < 200) { // 合理的距离范围
                    distances.push_back(dist);
                }
            }
        }

        if (distances.empty()) return;

        std::sort(distances.begin(), distances.end());
        double estimatedSpacing = distances[distances.size() / 4]; // 使用25%分位数作为间距估计

        // 绘制可能的连接线（连接距离接近估计间距的点对）
        double tolerance = estimatedSpacing * 0.4;
        for (size_t i = 0; i < dots.size(); i++) {
            for (size_t j = i + 1; j < dots.size(); j++) {
                double dist = cv::norm(dots[i] - dots[j]);

                // 如果距离接近估计的网格间距，绘制连接线
                if (std::abs(dist - estimatedSpacing) < tolerance) {
                    cv::line(image, dots[i], dots[j], cv::Scalar(128, 128, 128), 1);
                }

                // 对于对角线距离（大约是间距的√2倍）
                double diagonalSpacing = estimatedSpacing * 1.414;
                if (std::abs(dist - diagonalSpacing) < tolerance) {
                    cv::line(image, dots[i], dots[j], cv::Scalar(200, 200, 200), 1);
                }
            }
        }
    }


private:
    cv::Mat intelligentPreprocessing(const cv::Mat& input, const std::string& debugPrefix) {
        cv::Mat processed = input.clone();

        // 转换为灰度
        if (processed.channels() == 3) {
            cv::cvtColor(processed, processed, cv::COLOR_BGR2GRAY);
        }

        // 1. 莫尔条纹检测和消除
        if (params.enableMoireFiltering) {
            processed = removeMoirePattern(processed, debugPrefix + "_01_moire_removed");
        }

        // 2. 纹理噪声滤波
        if (params.enableTextureFiltering) {
            processed = removeTextureNoise(processed, debugPrefix + "_02_texture_filtered");
        }

        // 3. 背景均匀化
        if (params.enableBackgroundSubtraction) {
            processed = normalizeBackground(processed, debugPrefix + "_03_background_normalized");
        }

        // 4. 对比度增强（保留细节）
        cv::Mat enhanced;
        cv::createCLAHE(2.0, cv::Size(16, 16))->apply(processed, enhanced);
        processed = enhanced;

        if (params.saveDebugImages) {
            cv::imwrite(debugPrefix + "_04_final_preprocessed.png", processed);
        }
        return processed;
    }

    cv::Mat removeMoirePattern(const cv::Mat& input, const std::string& debugSuffix) {

        cv::Mat result = input.clone();

        // 1. FFT分析检测莫尔条纹
        cv::Mat float_img;
        input.convertTo(float_img, CV_32F);

        // 2. 应用低通滤波器消除高频莫尔模式
        cv::Mat blurred;
        cv::GaussianBlur(result, blurred, cv::Size(5, 5), 1.5);

        // 3. 保留边缘信息的同时去除莫尔条纹
        cv::Mat edges;
        cv::Canny(result, edges, 50, 150);
        cv::Mat dilated_edges;
        cv::dilate(edges, dilated_edges, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));

        // 4. 结合原图和滤波结果
        cv::Mat mask;
        cv::bitwise_not(dilated_edges, mask);

        cv::Mat output = result.clone();
        blurred.copyTo(output, mask);

        if (params.saveDebugImages) {
            cv::imwrite(debugSuffix + ".png", output);
        }

        return output;
    }

    cv::Mat removeTextureNoise(const cv::Mat& input, const std::string& debugSuffix) {

        cv::Mat result;

        // 1. 双边滤波保边去噪
        cv::bilateralFilter(input, result, 9, 75, 75);

        // 2. 形态学操作去除小噪声
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::Mat opened;
        cv::morphologyEx(result, opened, cv::MORPH_OPEN, kernel);

        // 3. 中值滤波去除椒盐噪声
        cv::Mat median_filtered;
        cv::medianBlur(opened, median_filtered, 3);

        if (params.saveDebugImages) {
            cv::imwrite(debugSuffix + ".png", median_filtered);
        }

        return median_filtered;
    }

    cv::Mat normalizeBackground(const cv::Mat& input, const std::string& debugSuffix) {

        // 1. 估算背景（使用形态学顶帽操作）
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(50, 50));
        cv::Mat background;
        cv::morphologyEx(input, background, cv::MORPH_CLOSE, kernel);

        // 2. 背景减除
        cv::Mat foreground;
        cv::subtract(background, input, foreground);

        // 3. 归一化
        cv::Mat normalized;
        cv::normalize(foreground, normalized, 0, 255, cv::NORM_MINMAX);

        if (params.saveDebugImages) {
            cv::imwrite(debugSuffix + "_background.png", background);
            cv::imwrite(debugSuffix + "_foreground.png", foreground);
            cv::imwrite(debugSuffix + ".png", normalized);
        }

        return normalized;
    }

    std::vector<cv::Rect> detectDotMatrixRegions(const cv::Mat& image, const std::string& debugPrefix) {

        std::vector<cv::Rect> regions;

        DotDetectorFunc det_func[2] = { 
            detectDotsInSingleBinary,
            detectDotsInSingleBinary_old
        };
        for (int i = 0; i < 2; i++) {
            // 1. 多种阈值二值化
            std::vector<cv::Mat> binaryImages = createSmartBinaryImages(image);

            std::vector<cv::Point> detectedDots = detectDotsFromMultipleBinaries(binaryImages, det_func[i], "dottest");
            // 在原图上可视化检测结果
            char buffer[256];
            std::snprintf(buffer, sizeof(buffer), "dots_on_original%d.png", i);
            std::string output(buffer);

            visualizeDotsOnOriginal(image, detectedDots, output);

            std::vector<Point> cluster_point;
            for (int i = 0; i < detectedDots.size(); i++) {
                Point pon = { detectedDots[i], UNCLASSIFIED };
                cluster_point.push_back(pon);
            }
            DBSCAN dbscan(3, cluster_point);
            dbscan.run();

            std::snprintf(buffer, sizeof(buffer), "dots_cluster_on_original%d.png", i);
            std::string clus_output(buffer);
            visualizeClusteredDots(image, dbscan.m_points, clus_output);

            auto clusters = dbscan.groupClusters();

            DotMatrixParser parser;
            // 解析所有聚类
            std::vector<ParsedInfo> results = parser.parseClusters(clusters);
            // 打印结果汇总
            // parser.printResults(results);
            m_parseip = parser.getResultsString(results);
            if (!m_parseip.empty()) {
                break;
            }
        }

        return regions;
    }

    std::vector<cv::Mat> createSmartBinaryImages(const cv::Mat& input) {
        std::vector<cv::Mat> binaries;

        // 1. 自适应阈值（针对不均匀光照）
        cv::Mat adaptive1, adaptive2;
        cv::adaptiveThreshold(input, adaptive1, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 21, 10);
        cv::adaptiveThreshold(input, adaptive2, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 15, 8);
        binaries.push_back(adaptive1);
        binaries.push_back(adaptive2);

        // 2. OTSU + 后处理
        cv::Mat otsu;
        cv::threshold(input, otsu, 0, 255, cv::THRESH_BINARY + cv::THRESH_OTSU);

        // 形态学清理
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::Mat cleaned;
        cv::morphologyEx(otsu, cleaned, cv::MORPH_CLOSE, kernel);
        binaries.push_back(cleaned);

        // 3. 边缘增强二值化
        cv::Mat edges;
        cv::Canny(input, edges, 30, 90);
        cv::Mat dilated;
        cv::dilate(edges, dilated, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2)));
        binaries.push_back(dilated);

        return binaries;
    }

    std::vector<cv::Rect> findRectangularRegions(const cv::Mat& binary) {
        std::vector<cv::Rect> rectangles;
        std::vector<std::vector<cv::Point>> contours;

        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        int imageArea = binary.rows * binary.cols;
        int minArea = static_cast<int>(imageArea * params.minRegionArea);
        int maxArea = static_cast<int>(imageArea * params.maxRegionArea);

        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area < minArea || area > maxArea) continue;

            // 获取边界矩形
            cv::Rect boundingRect = cv::boundingRect(contour);

            // 检查长宽比
            float aspectRatio = static_cast<float>(boundingRect.width) / boundingRect.height;
            float expectedRatio = params.expectedAspectRatio;

            if (std::abs(aspectRatio - expectedRatio) / expectedRatio <= params.aspectRatioTolerance ||
                std::abs(1.0f / aspectRatio - expectedRatio) / expectedRatio <= params.aspectRatioTolerance) {

                rectangles.push_back(boundingRect);
            }
        }

        return rectangles;
    }

    std::vector<cv::Rect> filterAndMergeRegions(const std::vector<cv::Rect>& regions, const cv::Size& imageSize) {
        std::vector<cv::Rect> filtered;

        // 1. 去重：合并重叠度高的区域
        std::vector<bool> merged(regions.size(), false);

        for (size_t i = 0; i < regions.size(); i++) {
            if (merged[i]) continue;

            cv::Rect currentRegion = regions[i];

            // 查找与当前区域重叠的其他区域
            for (size_t j = i + 1; j < regions.size(); j++) {
                if (merged[j]) continue;

                cv::Rect intersection = currentRegion & regions[j];
                float overlap1 = static_cast<float>(intersection.area()) / currentRegion.area();
                float overlap2 = static_cast<float>(intersection.area()) / regions[j].area();

                if (overlap1 > 0.3f || overlap2 > 0.3f) {
                    // 合并区域
                    currentRegion = currentRegion | regions[j];
                    merged[j] = true;
                }
            }

            filtered.push_back(currentRegion);
            merged[i] = true;
        }

        // 2. 边界检查
        std::vector<cv::Rect> finalRegions;
        for (const auto& region : filtered) {
            cv::Rect clampedRegion = region & cv::Rect(0, 0, imageSize.width, imageSize.height);
            if (clampedRegion.area() > 0) {
                finalRegions.push_back(clampedRegion);
            }
        }

        return finalRegions;
    }

    std::vector<cv::Rect> validateRegionsByContent(const cv::Mat& image, const std::vector<cv::Rect>& regions,
        const std::string& debugPrefix) {
        std::cout << "[INFO] 基于内容验证区域..." << std::endl;

        std::vector<cv::Rect> validRegions;

        for (size_t i = 0; i < regions.size(); i++) {
            const cv::Rect& region = regions[i];
            cv::Mat roi = image(region);

            // 1. 点数量检查
            int dotCount = countDotsInRegion(roi);

            // 2. 背景噪声评估
            float noiseLevel = assessBackgroundNoise(roi, region);

            // 3. 结构规整性检查
            float structuralScore = assessStructuralRegularity(roi);

            std::cout << "[INFO] 区域" << i << ": 点数=" << dotCount
                << ", 噪声=" << noiseLevel
                << ", 结构=" << structuralScore << std::endl;

            // 综合评判
            bool isValid = (dotCount >= params.minDotsInRegion && dotCount <= params.maxDotsInRegion) &&
                (noiseLevel < params.backgroundNoiseThreshold) &&
                (structuralScore > 0.3f);

            if (isValid) {
                validRegions.push_back(region);

                if (params.saveDebugImages) {
                    cv::imwrite(debugPrefix + "_valid_region_" + std::to_string(i) + ".png", roi);
                }
            }
        }

        std::cout << "[SUCCESS] 验证通过 " << validRegions.size() << " 个区域" << std::endl;
        return validRegions;
    }

    int countDotsInRegion(const cv::Mat& regionImage) {
        cv::Mat binary;
        cv::adaptiveThreshold(regionImage, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 5);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        int dotCount = 0;
        int regionArea = regionImage.rows * regionImage.cols;

        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);

            // 动态点大小范围
            double minDotArea = regionArea * 0.001;  // 0.1%
            double maxDotArea = regionArea * 0.05;   // 5%

            if (area >= minDotArea && area <= maxDotArea) {
                // 检查圆度
                double perimeter = cv::arcLength(contour, true);
                double circularity = (perimeter > 0) ? 4 * CV_PI * area / (perimeter * perimeter) : 0;

                if (circularity > 0.2) {
                    dotCount++;
                }
            }
        }

        return dotCount;
    }

    float assessBackgroundNoise(const cv::Mat& regionImage, const cv::Rect& globalRegion) {
        // 计算区域内的梯度变化来评估噪声水平
        cv::Mat grad_x, grad_y;
        cv::Sobel(regionImage, grad_x, CV_32F, 1, 0, 3);
        cv::Sobel(regionImage, grad_y, CV_32F, 0, 1, 3);

        cv::Mat grad_magnitude;
        cv::magnitude(grad_x, grad_y, grad_magnitude);

        cv::Scalar meanGrad = cv::mean(grad_magnitude);
        return static_cast<float>(meanGrad[0]);
    }

    float assessStructuralRegularity(const cv::Mat& regionImage) {
        // 检查是否有规律的点阵结构
        cv::Mat binary;
        cv::adaptiveThreshold(regionImage, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 5);

        // 查找点的质心
        std::vector<cv::Point2f> points;
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (const auto& contour : contours) {
            cv::Moments m = cv::moments(contour);
            if (m.m00 > 0) {
                points.push_back(cv::Point2f(m.m10 / m.m00, m.m01 / m.m00));
            }
        }

        if (points.size() < 10) return 0.0f;

        // 计算点间距离的标准差（越小说明越规律）
        std::vector<float> distances;
        for (size_t i = 0; i < points.size(); i++) {
            for (size_t j = i + 1; j < points.size(); j++) {
                float dist = cv::norm(points[i] - points[j]);
                if (dist > 5 && dist < 50) { // 合理的点间距范围
                    distances.push_back(dist);
                }
            }
        }

        if (distances.empty()) return 0.0f;

        cv::Scalar mean, stddev;
        cv::meanStdDev(distances, mean, stddev);

        // 标准差越小，规律性越强
        float regularity = 1.0f - std::min(1.0f, static_cast<float>(stddev[0] / mean[0]));
        return regularity;
    }

    std::vector<cv::Point> detectPreciseDots(const cv::Mat& roi, const std::string& debugPrefix) {
        std::vector<cv::Point> preciseDots;

        // 1. 创建优化的二值图
        cv::Mat binary;
        cv::adaptiveThreshold(roi, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 8);

        // 2. 形态学优化
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2, 2));
        cv::Mat cleaned;
        cv::morphologyEx(binary, cleaned, cv::MORPH_CLOSE, kernel);

        // 3. 检测轮廓
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(cleaned, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // 4. 严格的点筛选
        int roiArea = roi.rows * roi.cols;
        double minArea = roiArea * 0.0005;  // 更严格的最小面积
        double maxArea = roiArea * 0.02;    // 更严格的最大面积

        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area < minArea || area > maxArea) continue;

            // 严格的圆度检查
            double perimeter = cv::arcLength(contour, true);
            double circularity = (perimeter > 0) ? 4 * CV_PI * area / (perimeter * perimeter) : 0;

            if (circularity > 0.4) { // 更高的圆度要求
                cv::Moments moments = cv::moments(contour);
                if (moments.m00 > 0) {
                    cv::Point center(static_cast<int>(moments.m10 / moments.m00),
                        static_cast<int>(moments.m01 / moments.m00));
                    preciseDots.push_back(center);
                }
            }
        }

        if (params.saveDebugImages) {
            cv::Mat dotVis = roi.clone();
            if (dotVis.channels() == 1) {
                cv::cvtColor(dotVis, dotVis, cv::COLOR_GRAY2BGR);
            }
            for (const auto& dot : preciseDots) {
                cv::circle(dotVis, dot, 2, cv::Scalar(0, 255, 0), 2);
            }
            cv::imwrite(debugPrefix + "_precise_dots.png", dotVis);
        }

        return preciseDots;
    }

    DecodedResult buildDotMatrix(const std::vector<cv::Point>& dots, const cv::Size& roiSize,
        const std::string& debugPrefix) {
        DecodedResult result;

        if (dots.size() < 15) return result; // 至少需要3/4的点

        // 1. 估算网格参数
        auto gridParams = estimateGridParameters(dots, roiSize);
        if (gridParams.first <= 0 || gridParams.second <= 0) return result;

        float colSpacing = gridParams.first;
        float rowSpacing = gridParams.second;

        std::cout << "[INFO] 估算网格间距: 列=" << colSpacing << ", 行=" << rowSpacing << std::endl;

        // 2. 寻找最佳网格起点
        cv::Point2f bestOrigin = findBestGridOrigin(dots, colSpacing, rowSpacing);

        // 3. 构建网格矩阵
        std::vector<std::vector<bool>> matrix(TOTAL_ROWS, std::vector<bool>(BITS_PER_ROW, false));

        float tolerance = std::max(colSpacing, rowSpacing) * 0.3f;

        for (int row = 0; row < TOTAL_ROWS; row++) {
            for (int col = 0; col < BITS_PER_ROW; col++) {
                cv::Point2f expectedPos(bestOrigin.x + col * colSpacing,
                    bestOrigin.y + row * rowSpacing);

                // 寻找最近的点
                float minDistance = tolerance;
                for (const auto& dot : dots) {
                    float distance = cv::norm(cv::Point2f(dot) - expectedPos);
                    if (distance < minDistance) {
                        matrix[row][col] = true;
                        minDistance = distance;
                    }
                }
            }
        }

        // 4. 解码矩阵
        result = decodeMatrixWithValidation(matrix);
        result.avgDotSize = colSpacing; // 使用间距作为平均点大小的近似

        if (params.saveDebugImages) {
            saveGridDebugVisualization(dots, bestOrigin, colSpacing, rowSpacing, matrix,
                roiSize, debugPrefix);
        }

        return result;
    }

    std::pair<float, float> estimateGridParameters(const std::vector<cv::Point>& dots, const cv::Size& roiSize) {
        if (dots.size() < 4) return { 0, 0 };

        // 1. 计算所有点对之间的距离
        std::vector<float> distances;
        for (size_t i = 0; i < dots.size(); i++) {
            for (size_t j = i + 1; j < dots.size(); j++) {
                float dist = cv::norm(dots[i] - dots[j]);
                if (dist > 5 && dist < roiSize.width / 2) { // 合理范围内的距离
                    distances.push_back(dist);
                }
            }
        }

        if (distances.empty()) return { 0, 0 };

        // 2. 距离聚类找到主要间距
        std::sort(distances.begin(), distances.end());

        // 寻找距离的峰值（最常见的距离）
        std::map<int, int> distanceHistogram;
        for (float dist : distances) {
            int binned = static_cast<int>(dist / 2) * 2; // 2像素精度的分箱
            distanceHistogram[binned]++;
        }

        // 找到最频繁的距离
        int maxCount = 0;
        float mostCommonDistance = 0;
        for (const auto& pair : distanceHistogram) {
            if (pair.second > maxCount) {
                maxCount = pair.second;
                mostCommonDistance = pair.first;
            }
        }

        // 3. 基于最常见距离估算行列间距
        // 假设点阵是规则的，行列间距应该相近
        float estimatedSpacing = mostCommonDistance;

        // 4. 根据期望的5x8网格调整
        float expectedColSpacing = roiSize.width / 8.0f;
        float expectedRowSpacing = roiSize.height / 5.0f;

        // 选择更接近实际检测的间距
        float finalColSpacing = (std::abs(estimatedSpacing - expectedColSpacing) < expectedColSpacing * 0.5f)
            ? expectedColSpacing : estimatedSpacing;
        float finalRowSpacing = (std::abs(estimatedSpacing - expectedRowSpacing) < expectedRowSpacing * 0.5f)
            ? expectedRowSpacing : estimatedSpacing;

        return { finalColSpacing, finalRowSpacing };
    }

    cv::Point2f findBestGridOrigin(const std::vector<cv::Point>& dots, float colSpacing, float rowSpacing) {
        cv::Point2f bestOrigin(0, 0);
        int bestScore = 0;

        // 尝试每个点作为潜在的网格起点
        for (const auto& dot : dots) {
            // 尝试该点为网格中不同位置的情况
            for (int testRow = 0; testRow < TOTAL_ROWS; testRow++) {
                for (int testCol = 0; testCol < BITS_PER_ROW; testCol++) {
                    cv::Point2f testOrigin(dot.x - testCol * colSpacing,
                        dot.y - testRow * rowSpacing);

                    int score = calculateGridScore(dots, testOrigin, colSpacing, rowSpacing);
                    if (score > bestScore) {
                        bestScore = score;
                        bestOrigin = testOrigin;
                    }
                }
            }
        }

        return bestOrigin;
    }

    int calculateGridScore(const std::vector<cv::Point>& dots, const cv::Point2f& origin,
        float colSpacing, float rowSpacing) {
        int score = 0;
        float tolerance = std::max(colSpacing, rowSpacing) * 0.3f;

        for (int row = 0; row < TOTAL_ROWS; row++) {
            for (int col = 0; col < BITS_PER_ROW; col++) {
                cv::Point2f expectedPos(origin.x + col * colSpacing,
                    origin.y + row * rowSpacing);

                for (const auto& dot : dots) {
                    if (cv::norm(cv::Point2f(dot) - expectedPos) < tolerance) {
                        score++;
                        break;
                    }
                }
            }
        }

        return score;
    }

    DecodedResult decodeMatrixWithValidation(const std::vector<std::vector<bool>>& matrix) {
        DecodedResult result;

        if (matrix.size() != TOTAL_ROWS || matrix[0].size() != BITS_PER_ROW) {
            return result;
        }

        // 1. 同步位验证
        uint8_t headerByte = extractByteFromRow(matrix[HEADER_ROW]);
        uint8_t syncBits = headerByte & SYNC_MASK;
        uint8_t receivedChecksum = headerByte & CHECKSUM_MASK;

        if (syncBits == SYNC_PATTERN) {
            result.syncFound = true;
        }
        else {
            return result;
        }

        // 2. 提取IP数据
        std::vector<uint8_t> ipBytes;
        for (int i = 0; i < 4; i++) {
            uint8_t dataByte = extractByteFromRow(matrix[DATA_START_ROW + i]);
            ipBytes.push_back(dataByte);
            result.rawBytes.push_back(dataByte);
        }

        // 3. 校验和验证
        uint32_t sum = 0;
        for (uint8_t byte : ipBytes) {
            sum += byte;
        }
        uint8_t calculatedChecksum = sum & 0x0F;

        result.receivedChecksum = receivedChecksum;
        result.calculatedChecksum = calculatedChecksum;
        result.checksumValid = (receivedChecksum == calculatedChecksum);

        // 4. 附加验证
        result.regionValid = validateMatrixRegion(matrix);
        result.patternValid = validateMatrixPattern(matrix);
        result.contextValid = validateMatrixContext(ipBytes);

        // 5. 构建IP字符串
        if (result.checksumValid && result.contextValid) {
            std::ostringstream oss;
            oss << static_cast<int>(ipBytes[0]) << "." << static_cast<int>(ipBytes[1]) << "."
                << static_cast<int>(ipBytes[2]) << "." << static_cast<int>(ipBytes[3]);
            result.ipAddress = oss.str();
            result.isValid = true;
        }

        return result;
    }

    bool validateMatrixRegion(const std::vector<std::vector<bool>>& matrix) {
        int totalDots = 0;
        int headerDots = 0;

        for (int row = 0; row < TOTAL_ROWS; row++) {
            for (int col = 0; col < BITS_PER_ROW; col++) {
                if (matrix[row][col]) {
                    totalDots++;
                    if (row == HEADER_ROW) headerDots++;
                }
            }
        }

        return (headerDots >= 2 && headerDots <= 6) && (totalDots >= 15 && totalDots <= 35);
    }

    bool validateMatrixPattern(const std::vector<std::vector<bool>>& matrix) {
        // 检查是否有完全空白或完全填满的行（数据行）
        for (int row = DATA_START_ROW; row < TOTAL_ROWS; row++) {
            int count = 0;
            for (int col = 0; col < BITS_PER_ROW; col++) {
                if (matrix[row][col]) count++;
            }
            // 数据行不应该全0或全1
            if (count == 0 || count == BITS_PER_ROW) return false;
        }

        return true;
    }

    bool validateMatrixContext(const std::vector<uint8_t>& ipBytes) {
        if (ipBytes.size() != 4) return false;

        // IP语义验证
        bool allZero = true, allMax = true;
        for (uint8_t byte : ipBytes) {
            if (byte != 0) allZero = false;
            if (byte != 255) allMax = false;
        }

        if (allZero || allMax) return false;
        if (ipBytes[0] == 0 || ipBytes[0] == 255) return false;
        if (ipBytes[0] >= 224 && ipBytes[0] <= 239) return false; // 多播
        if (ipBytes[0] == 127) return false; // 回环

        return true;
    }

    uint8_t extractByteFromRow(const std::vector<bool>& row) {
        uint8_t value = 0;
        for (int i = 0; i < BITS_PER_ROW && i < static_cast<int>(row.size()); i++) {
            if (row[i]) {
                value |= (1 << (7 - i));
            }
        }
        return value;
    }

    float calculateIntelligentConfidence(const DecodedResult& result, const cv::Mat& roi) {
        float confidence = 0.0f;

        // 基础验证权重
        if (result.syncFound) confidence += 0.25f;
        if (result.checksumValid) confidence += 0.25f;
        if (result.regionValid) confidence += 0.2f;
        if (result.patternValid) confidence += 0.15f;
        if (result.contextValid) confidence += 0.15f;

        return std::min(1.0f, confidence);
    }

    void saveGridDebugVisualization(const std::vector<cv::Point>& dots, const cv::Point2f& origin,
        float colSpacing, float rowSpacing,
        const std::vector<std::vector<bool>>& matrix,
        const cv::Size& roiSize, const std::string& debugPrefix) {
        cv::Mat vis(roiSize, CV_8UC3, cv::Scalar(255, 255, 255));

        // 绘制检测到的点
        for (const auto& dot : dots) {
            cv::circle(vis, dot, 3, cv::Scalar(0, 255, 0), 2);
        }

        // 绘制网格
        for (int row = 0; row < TOTAL_ROWS; row++) {
            for (int col = 0; col < BITS_PER_ROW; col++) {
                cv::Point gridPos(static_cast<int>(origin.x + col * colSpacing),
                    static_cast<int>(origin.y + row * rowSpacing));

                if (matrix[row][col]) {
                    cv::circle(vis, gridPos, 2, cv::Scalar(0, 0, 255), 2); // 红色：匹配的格点
                }
                else {
                    cv::circle(vis, gridPos, 1, cv::Scalar(128, 128, 128), 1); // 灰色：空格点
                }
            }
        }

        // 绘制网格框架
        cv::Point topLeft(static_cast<int>(origin.x), static_cast<int>(origin.y));
        cv::Point bottomRight(static_cast<int>(origin.x + (BITS_PER_ROW - 1) * colSpacing),
            static_cast<int>(origin.y + (TOTAL_ROWS - 1) * rowSpacing));
        cv::rectangle(vis, topLeft, bottomRight, cv::Scalar(255, 0, 0), 1);

        cv::imwrite(debugPrefix + "_grid_analysis.png", vis);
    }

    void saveIntelligentDebugVisualization(const cv::Mat& originalImage,
        const std::vector<cv::Rect>& regions,
        const std::vector<DecodedResult>& results,
        const std::string& debugPrefix) {
        cv::Mat vis;
        if (originalImage.channels() == 1) {
            cv::cvtColor(originalImage, vis, cv::COLOR_GRAY2BGR);
        }
        else {
            vis = originalImage.clone();
        }

        // 绘制所有候选区域
        for (size_t i = 0; i < regions.size(); i++) {
            cv::rectangle(vis, regions[i], cv::Scalar(128, 128, 128), 1);
            cv::putText(vis, "R" + std::to_string(i),
                cv::Point(regions[i].x, regions[i].y - 5),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(128, 128, 128), 1);
        }

        // 绘制识别结果
        for (size_t i = 0; i < results.size(); i++) {
            const auto& result = results[i];

            cv::Scalar color = result.isValid ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);

            // 绘制边界框
            cv::rectangle(vis, result.boundingRect, color, 2);

            // 绘制检测到的点
            for (const auto& point : result.dotPoints) {
                cv::circle(vis, point, 2, color, 1);
            }

            // 添加标签
            std::string label = result.isValid ? result.ipAddress : "INVALID";
            label += " (" + std::to_string(static_cast<int>(result.confidence * 100)) + "%)";

            cv::putText(vis, label,
                cv::Point(result.boundingRect.x, result.boundingRect.y - 10),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        }

        cv::imwrite(debugPrefix + "_intelligent_final_result.png", vis);
    }



};

// 主程序
int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cout << "\n使用方法:" << std::endl;
        std::cout << "  单张图片: " << argv[0] << " -i <图片路径> [选项]" << std::endl;
        std::cout << "  批处理:   " << argv[0] << " -batch <目录路径>" << std::endl;
        std::cout << "\n选项:" << std::endl;
        std::cout << "  -i <path>          输入图片路径" << std::endl;
        std::cout << "  -debug             保存详细调试图片" << std::endl;
        std::cout << "  -quiet             静默模式" << std::endl;
        std::cout << "  -nobackground      禁用背景分离" << std::endl;
        std::cout << "  -nomoire           禁用莫尔滤波" << std::endl;
        std::cout << "  -notexture         禁用纹理滤波" << std::endl;
        std::cout << "\n示例:" << std::endl;
        std::cout << "  " << argv[0] << " -i photo_with_noise.jpg -debug" << std::endl;
        std::cout << "  " << argv[0] << " -i moire_pattern.png -debug" << std::endl;
        std::cout << "  " << argv[0] << " -i complex_background.jpg -debug" << std::endl;
        return 0;
    }

    // 解析参数
    std::string imagePath;
    bool enableDebug = false;
    bool verboseOutput = true;
    bool enableBackground = true;
    bool enableMoire = true;
    bool enableTexture = true;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            imagePath = argv[++i];
        }
        else if (arg == "-debug") {
            enableDebug = true;
        }
        else if (arg == "-quiet") {
            verboseOutput = false;
        }
        else if (arg == "-nobackground") {
            enableBackground = false;
        }
        else if (arg == "-nomoire") {
            enableMoire = false;
        }
        else if (arg == "-notexture") {
            enableTexture = false;
        }
        else if (arg == "-help" || arg == "-h") {
            return 0;
        }
    }

    if (imagePath.empty()) {
        std::cerr << "[ERROR] 请指定输入图片路径，使用 -help 查看帮助" << std::endl;
        return 1;
    }

    // 配置识别器
    IntelligentDotMatrixReader reader;
    IntelligentDotMatrixReader::IntelligentDetectionParams params;

    params.saveDebugImages = enableDebug;
    params.verboseOutput = verboseOutput;
    params.enableBackgroundSubtraction = enableBackground;
    params.enableMoireFiltering = enableMoire;
    params.enableTextureFiltering = enableTexture;
    params.debugPrefix = "debug";

    reader.setDetectionParams(params);

    /*std::cout << "\n[CONFIG] 智能识别配置:" << std::endl;
    std::cout << "背景分离: " << (enableBackground ? "启用" : "禁用") << std::endl;
    std::cout << "莫尔滤波: " << (enableMoire ? "启用" : "禁用") << std::endl;
    std::cout << "纹理滤波: " << (enableTexture ? "启用" : "禁用") << std::endl;
    std::cout << "调试模式: " << (enableDebug ? "启用" : "禁用") << std::endl;*/

    // 开始识别
    auto startTime = std::chrono::high_resolution_clock::now();
    auto results = reader.readFromImage(imagePath);
    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << reader.GetParseIp() << std::endl;

    /*auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    std::cout << "\n[性能] 识别总耗时: " << duration.count() << " 毫秒" << std::endl;*/

    return 0;
}