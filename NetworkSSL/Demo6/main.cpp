#include<opencv2/opencv.hpp>
#include<iostream>

int main(int argc, char** argv)
{
    cv::Mat src, dst;
    src = cv::imread("../TestRec/cat.jpg");
    //cvtColor(src, src, COLOR_BGR2GRAY);
    int width = src.cols;
    int height = src.rows;
    float alpha = 1.2, beta = 30;
    dst = cv::Mat::zeros(src.size(), src.type());//前面不再需要Mat定义dst的数据类型

    cv::Mat m1;
    src.convertTo(m1, CV_32F);//图像类型转化为CV_32F之后，float b = m1.at<Vec3f>(row, col)[0];不再报错，float和Vec3f可以正常使用

    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            if (src.channels() == 1) {
                int v = src.at<uchar>(row, col);
                dst.at<uchar>(row, col) = cv::saturate_cast<uchar>(v * alpha + beta);
            }
            else if (src.channels() == 3) {
                float b = m1.at<cv::Vec3f>(row, col)[0];
                float g = m1.at<cv::Vec3f>(row, col)[1];
                float r = m1.at<cv::Vec3f>(row, col)[2];
                dst.at<cv::Vec3b>(row, col)[0] = cv::saturate_cast<uchar>(b * alpha + beta);//dst不能是Vec3f，因为dst的图像类型还没有转化为CV_32F，dst的数据类型应该是CV_8UC3
                dst.at<cv::Vec3b>(row, col)[1] = cv::saturate_cast<uchar>(g * alpha + beta);
                dst.at<cv::Vec3b>(row, col)[2] = cv::saturate_cast<uchar>(r * alpha + beta);
            }
        }
    }
    char name_win[] = "src";//数组，元素为字符
    cv::namedWindow(name_win, cv::WINDOW_AUTOSIZE);
    imshow("src", dst);
    cv::waitKey(0);
    return 0;
}