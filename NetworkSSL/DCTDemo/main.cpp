#include <vector>
#include <iostream>
#include <bitset>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// 水印参数配置
const float ALPHA = 0.2f;       // 水印强度系数
const int BLOCK_SIZE = 8;       // DCT块大小
const Point EMBED_POS(5, 5);    // 嵌入位置

// 生成二值水印图像
Mat generate_watermark_image(const string& text, Size host_size) {
    // 计算最大可嵌入位数
    const int max_bits = (host_size.width / BLOCK_SIZE) * (host_size.height / BLOCK_SIZE);

    // 将字符串转换为二进制位
    vector<bool> bits;
    for (char c : text) {
        bitset<8> bs(c);
        for (int i = 7; i >= 0; --i) {
            bits.push_back(bs[i]);
        }
    }

    // 检查容量
    if (bits.size() > max_bits) {
        cerr << "水印容量不足! 最大支持" << max_bits / 8 << "字符" << endl;
        exit(EXIT_FAILURE);
    }

    // 创建二值图像
    const int wm_width = host_size.width / BLOCK_SIZE;
    const int wm_height = host_size.height / BLOCK_SIZE;
    Mat watermark(wm_height, wm_width, CV_8UC1, Scalar(0));

    // 填充数据
    for (int i = 0; i < bits.size(); ++i) {
        int row = i / wm_width;
        int col = i % wm_width;
        watermark.at<uchar>(row, col) = bits[i] ? 255 : 0;
    }

    return watermark;
}

// 从二值图像提取字符串
string extract_watermark_text(Mat watermark_img) {
    vector<bool> bits;
    for (int i = 0; i < watermark_img.rows; ++i) {
        for (int j = 0; j < watermark_img.cols; ++j) {
            uchar pixel = watermark_img.at<uchar>(i, j);
            bits.push_back(pixel > 128);
        }
    }

    // 转换为字节
    string text;
    for (size_t i = 0; i < bits.size(); i += 8) {
        if (i + 8 > bits.size()) break;

        bitset<8> byte;
        for (int j = 0; j < 8; ++j) {
            byte[7 - j] = bits[i + j];
        }
        text += static_cast<char>(byte.to_ulong());
    }

    return text;
}

// 修改后的嵌入函数
void embed_watermark(const string& input_path,
    const string& output_path,
    const string& watermark_text) {
    // 读取彩色图像
    Mat color_host = imread(input_path);
    if (color_host.empty()) {
        cerr << "无法读取输入图像: " << input_path << endl;
        exit(EXIT_FAILURE);
    }

    // 转换为YUV颜色空间
    Mat yuv_host;
    cvtColor(color_host, yuv_host, COLOR_BGR2YUV);

    // 分离通道
    vector<Mat> channels(3);
    split(yuv_host, channels);
    Mat y_channel = channels[0].clone();

    // 预处理尺寸
    const int new_height = (y_channel.rows / BLOCK_SIZE) * BLOCK_SIZE;
    const int new_width = (y_channel.cols / BLOCK_SIZE) * BLOCK_SIZE;
    resize(y_channel, y_channel, Size(new_width, new_height));

    // 生成水印图像
    Mat watermark = generate_watermark_image(watermark_text, y_channel.size());

    // 转换为浮点型处理
    Mat y_float;
    y_channel.convertTo(y_float, CV_32F, 1.0 / 255);

    // DCT分块处理（仅处理Y通道）
    for (int i = 0; i < y_float.rows; i += BLOCK_SIZE) {
        for (int j = 0; j < y_float.cols; j += BLOCK_SIZE) {
            Rect roi(j, i, BLOCK_SIZE, BLOCK_SIZE);
            Mat block = y_float(roi).clone();

            // std::cout << "Matrix: \n" << block << std::endl;

            dct(block, block);

            // std::cout << "Matrix: \n" << block << std::endl;

            // 嵌入水印
            int wm_row = i / BLOCK_SIZE;
            int wm_col = j / BLOCK_SIZE;
            if (wm_row < watermark.rows && wm_col < watermark.cols) {
                float wm_value = watermark.at<uchar>(wm_row, wm_col) / 255.0f;
                block.at<float>(EMBED_POS) += ALPHA * wm_value;
            }

            idct(block, block);

            // std::cout << "Matrix: \n" << block << std::endl;

            block.copyTo(y_float(roi));
        }
    }

    // 合并通道
    y_float.convertTo(channels[0], CV_8U, 255);
    resize(channels[0], channels[0], color_host.size()); // 恢复原始尺寸

    // 保持UV通道原始尺寸
    for (int i = 1; i < 3; ++i) {
        resize(channels[i], channels[i], color_host.size());
    }

    Mat merged;
    merge(channels, merged);
    cvtColor(merged, merged, COLOR_YUV2BGR);

    imwrite(output_path, merged);
}

// 修改后的提取函数
string extract_watermark(const string& input_path) {
    Mat color_img = imread(input_path);
    if (color_img.empty()) {
        cerr << "无法读取输入图像: " << input_path << endl;
        exit(EXIT_FAILURE);
    }

    // 转换为YUV并提取Y通道
    Mat yuv_img;
    cvtColor(color_img, yuv_img, COLOR_BGR2YUV);
    vector<Mat> channels;
    split(yuv_img, channels);
    Mat y_channel = channels[0];

    // 预处理尺寸
    const int valid_height = (y_channel.rows / BLOCK_SIZE) * BLOCK_SIZE;
    const int valid_width = (y_channel.cols / BLOCK_SIZE) * BLOCK_SIZE;
    y_channel = y_channel(Rect(0, 0, valid_width, valid_height));

    Mat y_float;
    y_channel.convertTo(y_float, CV_32F, 1.0 / 255);

    // 创建水印图像
    Mat extracted_wm(y_channel.rows / BLOCK_SIZE,
        y_channel.cols / BLOCK_SIZE,
        CV_8UC1);

    // 提取过程
    for (int i = 0; i < y_float.rows; i += BLOCK_SIZE) {
        for (int j = 0; j < y_float.cols; j += BLOCK_SIZE) {
            Rect roi(j, i, BLOCK_SIZE, BLOCK_SIZE);
            Mat block = y_float(roi).clone();

            dct(block, block);
            float wm_value = block.at<float>(EMBED_POS) / ALPHA;

            int wm_row = i / BLOCK_SIZE;
            int wm_col = j / BLOCK_SIZE;
            extracted_wm.at<uchar>(wm_row, wm_col) =
                static_cast<uchar>(round(wm_value * 255));
        }
    }

    return extract_watermark_text(extracted_wm);
}

int main(int argc, char** argv) {
    // 解析命令行参数
    if (argc < 3) {
        cout << "使用方法:\n"
            << "编码模式: " << argv[0]
            << " -encode <原始图像> <输出图像> <水印字符串>\n"
            << "解码模式: " << argv[0]
            << " -decode <含水印图像>\n";
        return EXIT_FAILURE;
    }

    string mode(argv[1]);
    if (mode == "-encode" && argc == 5) {
        embed_watermark(argv[2], argv[3], argv[4]);
        cout << "水印嵌入完成!\n";
    }
    else if (mode == "-decode" && argc == 3) {
        string extracted = extract_watermark(argv[2]);
        cout << "提取的水印信息: " << extracted << endl;
    }
    else {
        cerr << "参数错误!\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
