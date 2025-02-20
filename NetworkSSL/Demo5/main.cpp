#include<opencv2/opencv.hpp>
#include<iostream>

int main(int argc, char** argv)
{
    while (true) {
        std::string input;
        std::cout << "请输入指令: ";
        std::getline(std::cin, input); // 读取整行输入

        cv::Mat src = cv::imread("../TestRec/cat.jpg");
        if (!src.data)
        {
            printf("could not find...\n");
            return -1;
        }

        if (input == "half") {        // 这里判断是否为指定字符串
            
            cv::Mat src1, src2, dst;
            src1 = cv::imread("../TestRec/6x1.jpeg");
            src2 = cv::imread("../TestRec/6x2.jpeg");
            cv::namedWindow("src1", cv::WINDOW_AUTOSIZE);
            cv::namedWindow("src2", cv::WINDOW_AUTOSIZE);
            imshow("src1", src1);
            imshow("src2", src2);

            if (src1.rows == src2.rows && src1.cols == src2.cols && src1.type() == src2.type()) {//先判定两幅图像的大小和类型是否相同
                addWeighted(src1, 0.5, src2, 0.5, 0, dst, -1);
                cv::namedWindow("dst", cv::WINDOW_AUTOSIZE);
                imshow("dst", dst);
            }
            else {
                printf("size is not same...\n");
                return -1;
            }

            //直接两幅图像的像素值相加：add(src1,src2,dst,Mat());//Mat()为mask,这里为空mask
            //直接两幅图像的像素值相乘：multiply(src1, src2, dst);

            cv::waitKey(0);
        }
        else if (input == "full") {

            cv::Mat src1, src2, dst;
            src1 = cv::imread("../TestRec/6x1.jpeg");
            src2 = cv::imread("../TestRec/6x2.jpeg");
            cv::namedWindow("src1", cv::WINDOW_AUTOSIZE);
            cv::namedWindow("src2", cv::WINDOW_AUTOSIZE);
            imshow("src1", src1);
            imshow("src2", src2);

            if (src1.rows == src2.rows && src1.cols == src2.cols && src1.type() == src2.type()) {//先判定两幅图像的大小和类型是否相同
                cv::add(src1, src2, dst);
                cv::namedWindow("dst", cv::WINDOW_AUTOSIZE);
                imshow("dst", dst);
            }
            else {
                printf("size is not same...\n");
                return -1;
            }

            //直接两幅图像的像素值相加：add(src1,src2,dst,Mat());//Mat()为mask,这里为空mask
            //直接两幅图像的像素值相乘：multiply(src1, src2, dst);

            cv::waitKey(0);
        }
        else if (input == "mult") {
            
            cv::Mat src1, src2, dst;
            src1 = cv::imread("../TestRec/6x1.jpeg");
            src2 = cv::imread("../TestRec/6x2.jpeg");
            cv::namedWindow("src1", cv::WINDOW_AUTOSIZE);
            cv::namedWindow("src2", cv::WINDOW_AUTOSIZE);
            imshow("src1", src1);
            imshow("src2", src2);

            if (src1.rows == src2.rows && src1.cols == src2.cols && src1.type() == src2.type()) {//先判定两幅图像的大小和类型是否相同
                cv::multiply(src1, src2, dst);
                cv::namedWindow("dst", cv::WINDOW_AUTOSIZE);
                imshow("dst", dst);
            }
            else {
                printf("size is not same...\n");
                return -1;
            }

            //直接两幅图像的像素值相加：add(src1,src2,dst,Mat());//Mat()为mask,这里为空mask
            //直接两幅图像的像素值相乘：multiply(src1, src2, dst);

            cv::waitKey(0);
        }
        else if (input == "exit") {
            break;
        }
        else {
            std::cerr << "无效指令！支持指令: half/full/mult/exit" << std::endl;
        }
    }

    
    return 0;
}