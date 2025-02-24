#include <vector>
#include <iostream>
#include <bitset>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

// ˮӡ��������
const float ALPHA = 0.2f;       // ˮӡǿ��ϵ��
const int BLOCK_SIZE = 8;       // DCT���С
const Point EMBED_POS(5, 5);    // Ƕ��λ��

// ���ɶ�ֵˮӡͼ��
Mat generate_watermark_image(const string& text, Size host_size) {
    // ��������Ƕ��λ��
    const int max_bits = (host_size.width / BLOCK_SIZE) * (host_size.height / BLOCK_SIZE);

    // ���ַ���ת��Ϊ������λ
    vector<bool> bits;
    for (char c : text) {
        bitset<8> bs(c);
        for (int i = 7; i >= 0; --i) {
            bits.push_back(bs[i]);
        }
    }

    // �������
    if (bits.size() > max_bits) {
        cerr << "ˮӡ��������! ���֧��" << max_bits / 8 << "�ַ�" << endl;
        exit(EXIT_FAILURE);
    }

    // ������ֵͼ��
    const int wm_width = host_size.width / BLOCK_SIZE;
    const int wm_height = host_size.height / BLOCK_SIZE;
    Mat watermark(wm_height, wm_width, CV_8UC1, Scalar(0));

    // �������
    for (int i = 0; i < bits.size(); ++i) {
        int row = i / wm_width;
        int col = i % wm_width;
        watermark.at<uchar>(row, col) = bits[i] ? 255 : 0;
    }

    return watermark;
}

// �Ӷ�ֵͼ����ȡ�ַ���
string extract_watermark_text(Mat watermark_img) {
    vector<bool> bits;
    for (int i = 0; i < watermark_img.rows; ++i) {
        for (int j = 0; j < watermark_img.cols; ++j) {
            uchar pixel = watermark_img.at<uchar>(i, j);
            bits.push_back(pixel > 128);
        }
    }

    // ת��Ϊ�ֽ�
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

// �޸ĺ��Ƕ�뺯��
void embed_watermark(const string& input_path,
    const string& output_path,
    const string& watermark_text) {
    // ��ȡ��ɫͼ��
    Mat color_host = imread(input_path);
    if (color_host.empty()) {
        cerr << "�޷���ȡ����ͼ��: " << input_path << endl;
        exit(EXIT_FAILURE);
    }

    // ת��ΪYUV��ɫ�ռ�
    Mat yuv_host;
    cvtColor(color_host, yuv_host, COLOR_BGR2YUV);

    // ����ͨ��
    vector<Mat> channels(3);
    split(yuv_host, channels);
    Mat y_channel = channels[0].clone();

    // Ԥ����ߴ�
    const int new_height = (y_channel.rows / BLOCK_SIZE) * BLOCK_SIZE;
    const int new_width = (y_channel.cols / BLOCK_SIZE) * BLOCK_SIZE;
    resize(y_channel, y_channel, Size(new_width, new_height));

    // ����ˮӡͼ��
    Mat watermark = generate_watermark_image(watermark_text, y_channel.size());

    // ת��Ϊ�����ʹ���
    Mat y_float;
    y_channel.convertTo(y_float, CV_32F, 1.0 / 255);

    // DCT�ֿ鴦��������Yͨ����
    for (int i = 0; i < y_float.rows; i += BLOCK_SIZE) {
        for (int j = 0; j < y_float.cols; j += BLOCK_SIZE) {
            Rect roi(j, i, BLOCK_SIZE, BLOCK_SIZE);
            Mat block = y_float(roi).clone();

            // std::cout << "Matrix: \n" << block << std::endl;

            dct(block, block);

            // std::cout << "Matrix: \n" << block << std::endl;

            // Ƕ��ˮӡ
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

    // �ϲ�ͨ��
    y_float.convertTo(channels[0], CV_8U, 255);
    resize(channels[0], channels[0], color_host.size()); // �ָ�ԭʼ�ߴ�

    // ����UVͨ��ԭʼ�ߴ�
    for (int i = 1; i < 3; ++i) {
        resize(channels[i], channels[i], color_host.size());
    }

    Mat merged;
    merge(channels, merged);
    cvtColor(merged, merged, COLOR_YUV2BGR);

    imwrite(output_path, merged);
}

// �޸ĺ����ȡ����
string extract_watermark(const string& input_path) {
    Mat color_img = imread(input_path);
    if (color_img.empty()) {
        cerr << "�޷���ȡ����ͼ��: " << input_path << endl;
        exit(EXIT_FAILURE);
    }

    // ת��ΪYUV����ȡYͨ��
    Mat yuv_img;
    cvtColor(color_img, yuv_img, COLOR_BGR2YUV);
    vector<Mat> channels;
    split(yuv_img, channels);
    Mat y_channel = channels[0];

    // Ԥ����ߴ�
    const int valid_height = (y_channel.rows / BLOCK_SIZE) * BLOCK_SIZE;
    const int valid_width = (y_channel.cols / BLOCK_SIZE) * BLOCK_SIZE;
    y_channel = y_channel(Rect(0, 0, valid_width, valid_height));

    Mat y_float;
    y_channel.convertTo(y_float, CV_32F, 1.0 / 255);

    // ����ˮӡͼ��
    Mat extracted_wm(y_channel.rows / BLOCK_SIZE,
        y_channel.cols / BLOCK_SIZE,
        CV_8UC1);

    // ��ȡ����
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
    // ���������в���
    if (argc < 3) {
        cout << "ʹ�÷���:\n"
            << "����ģʽ: " << argv[0]
            << " -encode <ԭʼͼ��> <���ͼ��> <ˮӡ�ַ���>\n"
            << "����ģʽ: " << argv[0]
            << " -decode <��ˮӡͼ��>\n";
        return EXIT_FAILURE;
    }

    string mode(argv[1]);
    if (mode == "-encode" && argc == 5) {
        embed_watermark(argv[2], argv[3], argv[4]);
        cout << "ˮӡǶ�����!\n";
    }
    else if (mode == "-decode" && argc == 3) {
        string extracted = extract_watermark(argv[2]);
        cout << "��ȡ��ˮӡ��Ϣ: " << extracted << endl;
    }
    else {
        cerr << "��������!\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
