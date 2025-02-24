#include<iostream>
#include<opencv2/opencv.hpp>

int main() {
    // 普通打开图
    // cv::Mat src = cv::imread("./cat.jpg", cv::IMREAD_UNCHANGED);

    // 以灰度图形式打开
    // cv::Mat src = cv::imread("./cat.jpg", cv::IMREAD_GRAYSCALE);

    // 以RPG格式打开
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