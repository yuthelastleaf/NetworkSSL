#include <opencv2/opencv.hpp>
#include <iostream>

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

        if (input == "s") {        // 这里判断是否为指定字符串
            // 你的逻辑代码
            //单通道图像
            cv::Mat dst;
            cvtColor(src, dst, cv::COLOR_BGR2GRAY);//等同于CV_BGR2GRAY
            int height = dst.rows;
            int width = dst.cols;
            for (int row = 0; row < height; row++)
            {
                for (int col = 0; col < width; col++)
                {
                    int intensity = dst.at<uchar>(row, col);//得到每一点的像素值
                    dst.at<uchar>(row, col) = 255 - intensity;//255 - intensity;//改变每个点的像素值
                }
            }

            cv::Scalar p1 = dst.at<uchar>(0, 0);//输出为[211,0,0,0]
            int p2 = dst.at<uchar>(0, 0);//输出为211
            std::cout << p1 << '\n' << p2 << std::endl;

            cv::namedWindow("output", cv::WINDOW_AUTOSIZE);
            imshow("output", dst);
            cv::waitKey(0);
        }
        else if (input == "m") {
            cv::Mat dst;
            cvtColor(src, dst, cv::COLOR_BGR2GRAY);
            cv::Mat dst2 = cv::Mat::zeros(src.size(), src.type());
            int height_3 = src.rows;
            int width_3 = src.cols;
            int nc = src.channels();
            for (int row = 0; row < height_3; row++)
            {
                for (int col = 0; col < width_3; col++)
                {
                    if (nc == 1)//如果是单通道
                    {
                        int intensity = src.at<uchar>(row, col);
                        dst.at<uchar>(row, col) = 255 - intensity;
                    }
                    else if (nc == 3)//如果是三通道
                    {
                        int blue = src.at<cv::Vec3b>(row, col)[0];
                        int green = src.at<cv::Vec3b>(row, col)[1];
                        int red = src.at<cv::Vec3b>(row, col)[2];
                        dst2.at<cv::Vec3b>(row, col)[0] = 255 - blue;
                        dst2.at<cv::Vec3b>(row, col)[1] = 255 - green;
                        dst2.at<cv::Vec3b>(row, col)[2] = 255 - red;
                    }
                    else
                        return -1;
                }
            }
            cv::Vec3b intensity_1 = src.at<cv::Vec3b>(0, 0);//输出为[216,211,210]
            cv::Scalar intensity_2 = src.at<cv::Vec3b>(0, 0);//输出为[216,211,210,0]
            std::cout << intensity_1 << '\n' << intensity_2 << std::endl;

            int blue = intensity_2.val[0];
            int green = intensity_2.val[1];
            int red = intensity_2.val[2];
            std::cout << blue << '\n' << green << '\n' << red << std::endl;//和Vec3b一样

            bitwise_not(src, dst2);//位操作,非。一句即可代替上面的转化操作

            cv::namedWindow("output", cv::WINDOW_AUTOSIZE);
            imshow("output", dst2);
            cv::waitKey(0);
        }
        else if (input == "cut") {
            cv::Mat smalling = src(cv::Rect(0, 0, 500, 500));
            cv::namedWindow("output", cv::WINDOW_AUTOSIZE);
            imshow("output", smalling);
            cv::waitKey(0);
        }
        else if (input == "exit") {
            break;
        }
        else {
            std::cerr << "无效指令！支持指令: s/stop" << std::endl;
        }
    }

    return 0;
}