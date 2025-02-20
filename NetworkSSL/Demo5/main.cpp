#include<opencv2/opencv.hpp>
#include<iostream>

int main(int argc, char** argv)
{
    while (true) {
        std::string input;
        std::cout << "������ָ��: ";
        std::getline(std::cin, input); // ��ȡ��������

        cv::Mat src = cv::imread("../TestRec/cat.jpg");
        if (!src.data)
        {
            printf("could not find...\n");
            return -1;
        }

        if (input == "half") {        // �����ж��Ƿ�Ϊָ���ַ���
            
            cv::Mat src1, src2, dst;
            src1 = cv::imread("../TestRec/6x1.jpeg");
            src2 = cv::imread("../TestRec/6x2.jpeg");
            cv::namedWindow("src1", cv::WINDOW_AUTOSIZE);
            cv::namedWindow("src2", cv::WINDOW_AUTOSIZE);
            imshow("src1", src1);
            imshow("src2", src2);

            if (src1.rows == src2.rows && src1.cols == src2.cols && src1.type() == src2.type()) {//���ж�����ͼ��Ĵ�С�������Ƿ���ͬ
                addWeighted(src1, 0.5, src2, 0.5, 0, dst, -1);
                cv::namedWindow("dst", cv::WINDOW_AUTOSIZE);
                imshow("dst", dst);
            }
            else {
                printf("size is not same...\n");
                return -1;
            }

            //ֱ������ͼ�������ֵ��ӣ�add(src1,src2,dst,Mat());//Mat()Ϊmask,����Ϊ��mask
            //ֱ������ͼ�������ֵ��ˣ�multiply(src1, src2, dst);

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

            if (src1.rows == src2.rows && src1.cols == src2.cols && src1.type() == src2.type()) {//���ж�����ͼ��Ĵ�С�������Ƿ���ͬ
                cv::add(src1, src2, dst);
                cv::namedWindow("dst", cv::WINDOW_AUTOSIZE);
                imshow("dst", dst);
            }
            else {
                printf("size is not same...\n");
                return -1;
            }

            //ֱ������ͼ�������ֵ��ӣ�add(src1,src2,dst,Mat());//Mat()Ϊmask,����Ϊ��mask
            //ֱ������ͼ�������ֵ��ˣ�multiply(src1, src2, dst);

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

            if (src1.rows == src2.rows && src1.cols == src2.cols && src1.type() == src2.type()) {//���ж�����ͼ��Ĵ�С�������Ƿ���ͬ
                cv::multiply(src1, src2, dst);
                cv::namedWindow("dst", cv::WINDOW_AUTOSIZE);
                imshow("dst", dst);
            }
            else {
                printf("size is not same...\n");
                return -1;
            }

            //ֱ������ͼ�������ֵ��ӣ�add(src1,src2,dst,Mat());//Mat()Ϊmask,����Ϊ��mask
            //ֱ������ͼ�������ֵ��ˣ�multiply(src1, src2, dst);

            cv::waitKey(0);
        }
        else if (input == "exit") {
            break;
        }
        else {
            std::cerr << "��Чָ�֧��ָ��: half/full/mult/exit" << std::endl;
        }
    }

    
    return 0;
}