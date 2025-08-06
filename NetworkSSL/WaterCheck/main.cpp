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

// 针对Windows MSVC编译器的修复
#ifdef _WIN32
#include <io.h>
#include <direct.h>
#define ACCESS _access
#define GETCWD _getcwd
#else
#include <unistd.h>
#define ACCESS access
#define GETCWD getcwd
#endif

/*
改进的点阵协议设计：
第0行（Header行）：同步头 + 校验信息
第1-4行（Data行）：IP地址的4个字节

详细格式：
第0行：[同步位4bit][校验和4bit] = 8bit
第1行：IP第1字节 (8bit)
第2行：IP第2字节 (8bit)
第3行：IP第3字节 (8bit)
第4行：IP第4字节 (8bit)

同步位固定为：1010 (0xA) - 用于定位和确认
校验和：(IP字节1 + IP字节2 + IP字节3 + IP字节4) & 0x0F
*/

class ImageDotMatrixReader {
private:
    static const int BITS_PER_ROW = 8;
    static const int TOTAL_ROWS = 5;
    static const int HEADER_ROW = 0;
    static const int DATA_START_ROW = 1;

    // 协议常量
    static const uint8_t SYNC_PATTERN = 0xA0;  // 1010 0000
    static const uint8_t SYNC_MASK = 0xF0;
    static const uint8_t CHECKSUM_MASK = 0x0F;

public:
    // 解码结果结构
    struct DecodedResult {
        std::string ipAddress;
        bool isValid;
        bool syncFound;
        bool checksumValid;
        uint8_t receivedChecksum;
        uint8_t calculatedChecksum;
        std::vector<uint8_t> rawBytes;
        cv::Point headerPosition;
        int spacing;

        DecodedResult() : isValid(false), syncFound(false), checksumValid(false),
            receivedChecksum(0), calculatedChecksum(0), spacing(0) {}
    };

    // 检测参数
    struct DetectionParams {
        // 点检测参数
        double minDotArea = 3.0;
        double maxDotArea = 500.0;
        double circularityThreshold = 0.2;

        // 网格检测参数
        std::vector<int> spacingCandidates = { 8, 10, 12, 15, 18, 20, 25, 30, 35, 40 };
        double spacingTolerance = 0.4;
        int minDotsInHeader = 2;
        int maxDotsInHeader = 6;

        // 图像预处理参数
        bool enablePreprocessing = true;
        bool enableDenoising = true;
        bool enableContrastEnhancement = true;
        double rotationAngle = 0.0;

        // 调试参数
        bool saveDebugImages = true;
        bool verboseOutput = true;
    };

private:
    DetectionParams params;

public:
    ImageDotMatrixReader() = default;

    void setDetectionParams(const DetectionParams& newParams) {
        params = newParams;
    }

    std::vector<DecodedResult> readFromImage(const std::string& imagePath) {
        std::cout << "\n[INFO] 开始读取图片: " << imagePath << std::endl;

        cv::Mat image = cv::imread(imagePath, cv::IMREAD_COLOR);
        if (image.empty()) {
            std::cerr << "[ERROR] 无法读取图片: " << imagePath << std::endl;
            return {};
        }

        std::cout << "[INFO] 图片尺寸: " << image.cols << "x" << image.rows << std::endl;
        return processImage(image, imagePath);
    }

    std::vector<DecodedResult> processImage(const cv::Mat& image, const std::string& debugPrefix = "debug") {
        std::vector<DecodedResult> results;

        // 1. 图像预处理
        cv::Mat processedImage = preprocessImage(image, debugPrefix);

        // 2. 检测所有圆点
        std::vector<cv::Point> allDots = detectDots(processedImage, debugPrefix);
        if (allDots.empty()) {
            std::cout << "[ERROR] 未检测到任何圆点" << std::endl;
            return results;
        }

        std::cout << "[INFO] 检测到 " << allDots.size() << " 个候选圆点" << std::endl;

        // 3. 寻找可能的头部位置
        std::vector<cv::Point> headerCandidates = findHeaderCandidates(allDots);
        std::cout << "[INFO] 找到 " << headerCandidates.size() << " 个头部候选位置" << std::endl;

        // 4. 对每个头部候选进行解码尝试
        for (const auto& headerPos : headerCandidates) {
            auto headerResults = attemptDecodeFromHeader(allDots, headerPos);
            results.insert(results.end(), headerResults.begin(), headerResults.end());
        }

        // 5. 生成调试可视化
        if (params.saveDebugImages) {
            saveDebugVisualization(image, allDots, results, debugPrefix);
        }

        // 6. 输出结果总结
        printResultsSummary(results);

        return results;
    }

private:
    cv::Mat preprocessImage(const cv::Mat& input, const std::string& debugPrefix) {
        cv::Mat processed = input.clone();

        std::cout << "[INFO] 开始图像预处理..." << std::endl;

        // 1. 旋转图片（如果需要）
        if (params.rotationAngle != 0.0) {
            processed = rotateImage(processed, params.rotationAngle);
            std::cout << "[INFO] 图片已旋转 " << params.rotationAngle << " 度" << std::endl;
            if (params.saveDebugImages) {
                cv::imwrite(debugPrefix + "_00_rotated.png", processed);
            }
        }

        if (!params.enablePreprocessing) {
            return processed;
        }

        // 2. 转换为灰度
        if (processed.channels() == 3) {
            cv::cvtColor(processed, processed, cv::COLOR_BGR2GRAY);
        }

        // 3. 降噪
        if (params.enableDenoising) {
            cv::Mat denoised;
            cv::bilateralFilter(processed, denoised, 9, 75, 75);
            processed = denoised;
            if (params.saveDebugImages) {
                cv::imwrite(debugPrefix + "_01_denoised.png", processed);
            }
        }

        // 4. 对比度增强
        if (params.enableContrastEnhancement) {
            cv::Mat enhanced;
            cv::createCLAHE(2.0, cv::Size(8, 8))->apply(processed, enhanced);
            processed = enhanced;
            if (params.saveDebugImages) {
                cv::imwrite(debugPrefix + "_02_enhanced.png", processed);
            }
        }

        // 5. 锐化
        cv::Mat kernel = (cv::Mat_<float>(3, 3) << 0, -1, 0, -1, 5, -1, 0, -1, 0);
        cv::Mat sharpened;
        cv::filter2D(processed, sharpened, -1, kernel);
        processed = sharpened;

        if (params.saveDebugImages) {
            cv::imwrite(debugPrefix + "_03_preprocessed.png", processed);
        }

        std::cout << "[SUCCESS] 图像预处理完成" << std::endl;
        return processed;
    }

    cv::Mat rotateImage(const cv::Mat& input, double angle) {
        if (angle == 0.0) {
            return input.clone();
        }

        cv::Point2f center(static_cast<float>(input.cols) / 2.0f, static_cast<float>(input.rows) / 2.0f);
        cv::Mat rotMatrix = cv::getRotationMatrix2D(center, angle, 1.0);

        cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), input.size(), static_cast<float>(angle)).boundingRect2f();

        rotMatrix.at<double>(0, 2) += bbox.width / 2.0 - input.cols / 2.0;
        rotMatrix.at<double>(1, 2) += bbox.height / 2.0 - input.rows / 2.0;

        cv::Mat rotated;
        cv::warpAffine(input, rotated, rotMatrix, bbox.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

        return rotated;
    }

    std::vector<cv::Point> detectDots(const cv::Mat& image, const std::string& debugPrefix) {
        std::vector<cv::Point> dots;
        std::vector<cv::Mat> binaryImages = createMultipleBinaryImages(image);
        std::vector<cv::Point> allCandidates;

        for (size_t i = 0; i < binaryImages.size(); i++) {
            auto candidates = detectDotsInBinary(binaryImages[i]);
            allCandidates.insert(allCandidates.end(), candidates.begin(), candidates.end());

            if (params.saveDebugImages) {
                cv::imwrite(debugPrefix + "_binary_" + std::to_string(i) + ".png", binaryImages[i]);
            }
        }

        dots = filterAndMergeDots(allCandidates);
        std::cout << "[INFO] 从 " << allCandidates.size() << " 个候选点中筛选出 " << dots.size() << " 个有效点" << std::endl;

        if (params.saveDebugImages) {
            cv::Mat dotVis;
            cv::cvtColor(image, dotVis, cv::COLOR_GRAY2BGR);
            for (const auto& dot : dots) {
                cv::circle(dotVis, dot, 3, cv::Scalar(0, 255, 0), 2);
            }
            cv::imwrite(debugPrefix + "_04_detected_dots.png", dotVis);
        }

        return dots;
    }

    std::vector<cv::Mat> createMultipleBinaryImages(const cv::Mat& input) {
        std::vector<cv::Mat> binaryImages;

        cv::Mat adaptive1, adaptive2;
        cv::adaptiveThreshold(input, adaptive1, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
            cv::THRESH_BINARY_INV, 15, 5);
        cv::adaptiveThreshold(input, adaptive2, 255, cv::ADAPTIVE_THRESH_MEAN_C,
            cv::THRESH_BINARY_INV, 21, 8);
        binaryImages.push_back(adaptive1);
        binaryImages.push_back(adaptive2);

        cv::Mat otsu;
        cv::threshold(input, otsu, 0, 255, cv::THRESH_BINARY_INV + cv::THRESH_OTSU);
        binaryImages.push_back(otsu);

        std::vector<int> thresholds = { 80, 120, 160, 200 };
        for (int thresh : thresholds) {
            cv::Mat fixed;
            cv::threshold(input, fixed, thresh, 255, cv::THRESH_BINARY_INV);
            binaryImages.push_back(fixed);
        }

        return binaryImages;
    }

    std::vector<cv::Point> detectDotsInBinary(const cv::Mat& binary) {
        std::vector<cv::Point> dots;
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area < params.minDotArea || area > params.maxDotArea) {
                continue;
            }

            double perimeter = cv::arcLength(contour, true);
            double circularity = (perimeter > 0) ? 4 * CV_PI * area / (perimeter * perimeter) : 0;

            if (circularity >= params.circularityThreshold) {
                cv::Moments moments = cv::moments(contour);
                if (moments.m00 != 0) {
                    cv::Point center(static_cast<int>(moments.m10 / moments.m00),
                        static_cast<int>(moments.m01 / moments.m00));
                    dots.push_back(center);
                }
            }
        }

        return dots;
    }

    std::vector<cv::Point> filterAndMergeDots(const std::vector<cv::Point>& candidates) {
        if (candidates.empty()) return {};

        std::vector<cv::Point> merged;
        std::vector<bool> used(candidates.size(), false);

        for (size_t i = 0; i < candidates.size(); i++) {
            if (used[i]) continue;

            std::vector<cv::Point> cluster;
            cluster.push_back(candidates[i]);
            used[i] = true;

            for (size_t j = i + 1; j < candidates.size(); j++) {
                if (used[j]) continue;

                double distance = cv::norm(candidates[i] - candidates[j]);
                if (distance < 8.0) {
                    cluster.push_back(candidates[j]);
                    used[j] = true;
                }
            }

            if (!cluster.empty()) {
                int avgX = 0, avgY = 0;
                for (const auto& pt : cluster) {
                    avgX += pt.x;
                    avgY += pt.y;
                }
                avgX /= static_cast<int>(cluster.size());
                avgY /= static_cast<int>(cluster.size());
                merged.push_back(cv::Point(avgX, avgY));
            }
        }

        return merged;
    }

    std::vector<cv::Point> findHeaderCandidates(const std::vector<cv::Point>& allDots) {
        std::vector<cv::Point> candidates;

        for (int spacing : params.spacingCandidates) {
            auto spacingCandidates = findHeadersWithSpacing(allDots, spacing);
            candidates.insert(candidates.end(), spacingCandidates.begin(), spacingCandidates.end());
        }

        return removeDuplicatePositions(candidates, 15);
    }

    std::vector<cv::Point> findHeadersWithSpacing(const std::vector<cv::Point>& allDots, int spacing) {
        std::vector<cv::Point> headers;

        for (const auto& startDot : allDots) {
            if (couldBeHeaderStart(allDots, startDot, spacing)) {
                headers.push_back(startDot);
            }
        }

        return headers;
    }

    bool couldBeHeaderStart(const std::vector<cv::Point>& allDots, const cv::Point& start, int spacing) {
        int foundInFirstRow = 0;
        double tolerance = spacing * params.spacingTolerance;

        for (int col = 0; col < BITS_PER_ROW; col++) {
            cv::Point expected(start.x + col * spacing, start.y);

            for (const auto& dot : allDots) {
                if (cv::norm(dot - expected) < tolerance) {
                    foundInFirstRow++;
                    break;
                }
            }
        }

        return foundInFirstRow >= params.minDotsInHeader && foundInFirstRow <= params.maxDotsInHeader;
    }

    std::vector<DecodedResult> attemptDecodeFromHeader(const std::vector<cv::Point>& allDots, const cv::Point& headerPos) {
        std::vector<DecodedResult> results;

        for (int spacing : params.spacingCandidates) {
            DecodedResult result = tryDecodeWithSpacing(allDots, headerPos, spacing);
            if (result.syncFound) {
                result.headerPosition = headerPos;
                result.spacing = spacing;
                results.push_back(result);

                if (result.isValid) {
                    if (params.verboseOutput) {
                        std::cout << "[SUCCESS] 成功解码 - 位置:(" << headerPos.x << "," << headerPos.y
                            << ") 间距:" << spacing << " IP:" << result.ipAddress << std::endl;
                    }
                    break;
                }
            }
        }

        return results;
    }

    DecodedResult tryDecodeWithSpacing(const std::vector<cv::Point>& allDots, const cv::Point& headerStart, int spacing) {
        DecodedResult result;
        auto matrix = buildMatrixFromHeader(allDots, headerStart, spacing);
        if (matrix.size() != TOTAL_ROWS || matrix[0].size() != BITS_PER_ROW) {
            return result;
        }
        return decodeMatrix(matrix);
    }

    std::vector<std::vector<bool>> buildMatrixFromHeader(const std::vector<cv::Point>& allDots,
        const cv::Point& headerStart, int spacing) {
        std::vector<std::vector<bool>> matrix(TOTAL_ROWS, std::vector<bool>(BITS_PER_ROW, false));
        double tolerance = spacing * params.spacingTolerance;

        for (int row = 0; row < TOTAL_ROWS; row++) {
            for (int col = 0; col < BITS_PER_ROW; col++) {
                cv::Point expected(headerStart.x + col * spacing, headerStart.y + row * spacing);

                for (const auto& dot : allDots) {
                    if (cv::norm(dot - expected) < tolerance) {
                        matrix[row][col] = true;
                        break;
                    }
                }
            }
        }

        return matrix;
    }

    DecodedResult decodeMatrix(const std::vector<std::vector<bool>>& matrix) {
        DecodedResult result;

        if (matrix.size() != TOTAL_ROWS || matrix[0].size() != BITS_PER_ROW) {
            return result;
        }

        uint8_t headerByte = extractByteFromRow(matrix[HEADER_ROW]);
        uint8_t syncBits = headerByte & SYNC_MASK;
        uint8_t receivedChecksum = headerByte & CHECKSUM_MASK;

        if (syncBits != SYNC_PATTERN) {
            return result;
        }
        result.syncFound = true;

        std::vector<uint8_t> ipBytes;
        for (int i = 0; i < 4; i++) {
            uint8_t dataByte = extractByteFromRow(matrix[DATA_START_ROW + i]);
            ipBytes.push_back(dataByte);
            result.rawBytes.push_back(dataByte);
        }

        uint32_t sum = 0;
        for (uint8_t byte : ipBytes) {
            sum += byte;
        }
        uint8_t calculatedChecksum = sum & 0x0F;

        result.receivedChecksum = receivedChecksum;
        result.calculatedChecksum = calculatedChecksum;

        if (receivedChecksum == calculatedChecksum) {
            result.checksumValid = true;
            result.isValid = true;

            std::ostringstream oss;
            oss << static_cast<int>(ipBytes[0]) << "." << static_cast<int>(ipBytes[1]) << "."
                << static_cast<int>(ipBytes[2]) << "." << static_cast<int>(ipBytes[3]);
            result.ipAddress = oss.str();
        }

        return result;
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

    std::vector<cv::Point> removeDuplicatePositions(const std::vector<cv::Point>& positions, int minDistance) {
        std::vector<cv::Point> unique;

        for (const auto& pos : positions) {
            bool isDuplicate = false;
            for (const auto& existing : unique) {
                if (cv::norm(pos - existing) < minDistance) {
                    isDuplicate = true;
                    break;
                }
            }
            if (!isDuplicate) {
                unique.push_back(pos);
            }
        }

        return unique;
    }

    void saveDebugVisualization(const cv::Mat& originalImage, const std::vector<cv::Point>& allDots,
        const std::vector<DecodedResult>& results, const std::string& debugPrefix) {
        cv::Mat vis;
        if (originalImage.channels() == 1) {
            cv::cvtColor(originalImage, vis, cv::COLOR_GRAY2BGR);
        }
        else {
            vis = originalImage.clone();
        }

        for (const auto& dot : allDots) {
            cv::circle(vis, dot, 2, cv::Scalar(128, 128, 128), 2);
        }

        for (size_t i = 0; i < results.size(); i++) {
            const auto& result = results[i];
            cv::Scalar color;

            if (result.isValid) {
                color = cv::Scalar(0, 255, 0);
            }
            else if (result.syncFound) {
                color = cv::Scalar(0, 255, 255);
            }
            else {
                color = cv::Scalar(0, 0, 255);
            }

            cv::Point headerPos = result.headerPosition;
            int spacing = result.spacing;

            for (int row = 0; row < TOTAL_ROWS; row++) {
                for (int col = 0; col < BITS_PER_ROW; col++) {
                    cv::Point gridPos(headerPos.x + col * spacing, headerPos.y + row * spacing);

                    if (row == HEADER_ROW) {
                        cv::circle(vis, gridPos, 4, cv::Scalar(0, 0, 255), 2);
                    }
                    else {
                        cv::circle(vis, gridPos, 3, cv::Scalar(255, 0, 0), 1);
                    }
                }
            }

            std::string label = result.isValid ? result.ipAddress : "INVALID";
            cv::putText(vis, label, cv::Point(headerPos.x, headerPos.y - 10),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, color, 2);
        }

        cv::imwrite(debugPrefix + "_05_final_result.png", vis);
    }

    void printResultsSummary(const std::vector<DecodedResult>& results) {
        std::cout << "\n[SUMMARY] 识别结果总结:" << std::endl;
        std::cout << "总候选数量: " << results.size() << std::endl;

        int validCount = 0;
        int syncFoundCount = 0;

        for (const auto& result : results) {
            if (result.isValid) {
                validCount++;
                std::cout << "[SUCCESS] 有效IP: " << result.ipAddress
                    << " (位置:" << result.headerPosition.x << "," << result.headerPosition.y
                    << ", 间距:" << result.spacing << ")" << std::endl;
            }
            else if (result.syncFound) {
                syncFoundCount++;
                std::cout << "[WARNING] 同步成功但校验失败 (位置:" << result.headerPosition.x << "," << result.headerPosition.y
                    << ", 间距:" << result.spacing << ", 校验和:" << static_cast<int>(result.receivedChecksum)
                    << "!=" << static_cast<int>(result.calculatedChecksum) << ")" << std::endl;
            }
        }

        std::cout << "有效结果: " << validCount << std::endl;
        std::cout << "同步成功: " << syncFoundCount << std::endl;

        if (validCount == 0 && syncFoundCount == 0) {
            std::cout << "[ERROR] 未找到任何有效的点阵数据" << std::endl;
        }
    }
};

// 简化的批处理器类
class SimpleBatchProcessor {
public:
    static void processBatch(const std::string& inputDir, const std::string& outputFile = "", double rotationAngle = 0.0) {
        ImageDotMatrixReader reader;
        ImageDotMatrixReader::DetectionParams params;
        params.saveDebugImages = false;
        params.verboseOutput = false;
        params.rotationAngle = rotationAngle;
        reader.setDetectionParams(params);

        std::vector<std::pair<std::string, std::vector<std::string>>> allResults;

        std::cout << "[INFO] 批处理模式 - 扫描目录: " << inputDir << std::endl;
        if (rotationAngle != 0.0) {
            std::cout << "[INFO] 应用旋转角度: " << rotationAngle << " 度" << std::endl;
        }

        // 简化的文件枚举（不使用filesystem）
        std::vector<std::string> imageFiles;

        // 这里可以手动添加文件，或使用特定平台的文件枚举API
        // 为了编译兼容性，这里提供一个简化版本
        std::cout << "[WARNING] 批处理功能需要手动指定文件列表" << std::endl;
        std::cout << "[INFO] 请使用单文件模式: -i filename.png" << std::endl;
    }
};

// 主程序
int main(int argc, char* argv[]) {
    std::cout << "[INFO] 点阵IP识别器 v2.0" << std::endl;
    std::cout << "[INFO] 支持改进协议：头部行(同步+校验) + 数据行(IP)" << std::endl;

    if (argc == 1) {
        std::cout << "\n使用方法:" << std::endl;
        std::cout << "  单张图片识别: " << argv[0] << " -i <图片路径> [-rotate <角度>] [-debug] [-quiet]" << std::endl;
        std::cout << "  批处理模式:   " << argv[0] << " -batch <目录路径> [-rotate <角度>] [-output <结果文件>]" << std::endl;
        std::cout << "  帮助信息:     " << argv[0] << " -help" << std::endl;
        std::cout << "\n参数说明:" << std::endl;
        std::cout << "  -i <path>      指定输入图片路径" << std::endl;
        std::cout << "  -batch <dir>   批处理指定目录下的所有图片" << std::endl;
        std::cout << "  -output <file> 批处理结果输出文件 (CSV格式)" << std::endl;
        std::cout << "  -rotate <angle> 图片旋转角度（度），正数顺时针，负数逆时针" << std::endl;
        std::cout << "  -debug         保存调试图片" << std::endl;
        std::cout << "  -quiet         静默模式" << std::endl;
        std::cout << "\n示例:" << std::endl;
        std::cout << "  " << argv[0] << " -i screenshot.png -debug" << std::endl;
        std::cout << "  " << argv[0] << " -i photo.jpg -rotate 15 -debug" << std::endl;
        std::cout << "  " << argv[0] << " -i tilted.png -rotate -30" << std::endl;
        return 0;
    }

    // 解析命令行参数
    std::string mode = "";
    std::string imagePath = "";
    std::string batchDir = "";
    std::string outputFile = "";
    bool enableDebug = false;
    bool verboseOutput = true;
    double rotationAngle = 0.0;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            mode = "single";
            imagePath = argv[++i];
        }
        else if (arg == "-batch" && i + 1 < argc) {
            mode = "batch";
            batchDir = argv[++i];
        }
        else if (arg == "-output" && i + 1 < argc) {
            outputFile = argv[++i];
        }
        else if (arg == "-debug") {
            enableDebug = true;
        }
        else if (arg == "-quiet") {
            verboseOutput = false;
        }
        else if (arg == "-rotate" && i + 1 < argc) {
            rotationAngle = std::stod(argv[++i]);
        }
        else if (arg == "-help" || arg == "-h") {
            return 0;
        }
    }

    // 根据模式执行相应功能
    if (mode == "batch") {
        SimpleBatchProcessor::processBatch(batchDir, outputFile, rotationAngle);
        return 0;
    }
    else if (mode == "single" && !imagePath.empty()) {
        // 单张图片识别模式
        ImageDotMatrixReader reader;
        ImageDotMatrixReader::DetectionParams params;
        params.saveDebugImages = enableDebug;
        params.verboseOutput = verboseOutput;
        params.rotationAngle = rotationAngle;

        // 根据图片类型调整参数
        if (imagePath.find("screenshot") != std::string::npos) {
            params.minDotArea = 4.0;
            params.maxDotArea = 200.0;
            params.spacingCandidates = { 12, 15, 18, 20, 25, 30 };
        }
        else if (imagePath.find("photo") != std::string::npos) {
            params.minDotArea = 3.0;
            params.maxDotArea = 500.0;
            params.spacingTolerance = 0.5;
            params.spacingCandidates = { 8, 10, 12, 15, 18, 20, 25, 30, 35, 40 };
        }

        reader.setDetectionParams(params);

        // 开始识别
        auto results = reader.readFromImage(imagePath);

        // 输出结果
        std::vector<std::string> validIPs;
        for (const auto& result : results) {
            if (result.isValid) {
                validIPs.push_back(result.ipAddress);
            }
        }

        std::cout << "\n[COMPLETE] 识别完成！" << std::endl;

        if (rotationAngle != 0.0) {
            std::cout << "[INFO] 应用旋转角度: " << rotationAngle << " 度" << std::endl;
        }

        if (validIPs.empty()) {
            std::cout << "[ERROR] 未识别到任何有效的IP地址" << std::endl;
            if (rotationAngle == 0.0) {
                std::cout << "\n[SUGGESTION] 建议:" << std::endl;
                std::cout << "1. 如果水印有旋转，尝试使用 -rotate 参数" << std::endl;
                std::cout << "2. 常见角度: -rotate 15, -rotate 30, -rotate 45" << std::endl;
                std::cout << "3. 或者使用 -debug 参数查看检测过程" << std::endl;
            }
            return 1;
        }

        std::cout << "[SUCCESS] 识别到的IP地址:" << std::endl;
        for (const auto& ip : validIPs) {
            std::cout << "  [IP] " << ip << std::endl;
        }

        if (enableDebug) {
            std::cout << "\n[DEBUG] 调试图片已保存" << std::endl;
            if (rotationAngle != 0.0) {
                std::cout << "  00_rotated.png - 旋转后的图片" << std::endl;
            }
            std::cout << "  01_denoised.png - 降噪处理结果" << std::endl;
            std::cout << "  02_enhanced.png - 对比度增强结果" << std::endl;
            std::cout << "  03_preprocessed.png - 预处理完成结果" << std::endl;
            std::cout << "  04_detected_dots.png - 圆点检测结果" << std::endl;
            std::cout << "  05_final_result.png - 最终识别结果" << std::endl;
            std::cout << "  binary_*.png - 各种二值化结果" << std::endl;
        }

        return 0;
    }
    else {
        std::cerr << "[ERROR] 无效的参数，使用 -help 查看帮助" << std::endl;
        return 1;
    }
}

// 编译说明：
// Windows (MSVC): 
//   cl /EHsc /std:c++17 main.cpp /I"path_to_opencv\include" /link /LIBPATH:"path_to_opencv\lib" opencv_world4XX.lib
// Linux (GCC):
//   g++ -std=c++17 main.cpp `pkg-config --cflags --libs opencv4` -o image_reader
//
// 使用示例：
//   ./image_reader -i test.png -debug
//   ./image_reader -i rotated.jpg -rotate 15 -debug
//   ./image_reader -i screenshot.png -rotate -30