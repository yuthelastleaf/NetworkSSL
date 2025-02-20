#include<opencv2/opencv.hpp>
#include<iostream>

int main()
{
    cv::Mat src;
    src = cv::imread("../TestRec/cat.jpg");
    if (!src.data)
    {
        std::cout << "can not find the image...\n" << std::endl;
        return -1;
    }

    cv::Mat dst;//先定义一张新的图像
    dst = cv::Mat(src.size(), src.type());//和src图像相同的大小和类型
    dst = cv::Scalar(127, 0, 255);//每个像素点的RGB都是(127，0，255)

    cv::Mat dst2, dst3;
    dst2 = src.clone();//克隆一张一模一样的图片，属于完全复制
    src.copyTo(dst3);//也是克隆，等同于src.clone()，属于完全复制

    const uchar* p = dst3.ptr<uchar>(0);//第一行首个像素点的像素值
    const auto i = dst3.ptr<uchar>(0)[0];
    const int* q = dst3.ptr<int>(0);
    printf("the first row pixel is %d \n", *p);
    printf("the first pixel is %d \n", i);//*p=i=94
    std::cout << *q << std::endl;//*q=1515541342

    int col = dst3.cols;
    int row = dst3.rows;
    printf("the col is %d , the row is %d \n", col, row);

    cv::Mat M(100, 100, src.type(), cv::Scalar(21, 13, 43));//另一种图像定义方式
    //cout << "M= " << endl << M << endl;
    std::cout << src.type() << std::endl;

    cv::Mat dst4(src);//属于部分复制
    cv::Mat dst5 = dst4;

    cv::Mat L;
    L.create(src.size(), src.type());//L.create不能赋值，需要用下条语句进行赋值操作
    L = cv::Scalar(0, 0, 0);//可以使用多维数组进行赋值

    cv::Mat dst6 = cv::Mat::eye(2, 3, src.type());//对角线上每个像素点的第一个通道赋值为1，其余均为0
    /*[1,0,0,0,0,0,0,0,0
       0,0,0,1,0,0,0,0,0]*/
    std::cout << "dst6= " << std::endl << dst6 << std::endl;

    cv::namedWindow("output", cv::WINDOW_AUTOSIZE);
    imshow("output", dst3);
    cv::waitKey(0);
    return 0;
}