#include<iostream>
#include<opencv2/opencv.hpp>

int main() {
    // ��ͨ��ͼ
    // cv::Mat src = cv::imread("./cat.jpg", cv::IMREAD_UNCHANGED);

    // �ԻҶ�ͼ��ʽ��
    // cv::Mat src = cv::imread("./cat.jpg", cv::IMREAD_GRAYSCALE);

    // ��RPG��ʽ��
    cv::Mat src = cv::imread("./cat.jpg", cv::IMREAD_COLOR);
    if (src.empty()) {
        printf("could not find the image...\n");
        return -1;
    }
    cv::namedWindow("src", cv::WINDOW_AUTOSIZE);
    imshow("src", src);
    cv::waitKey(0);
    return 0;
}