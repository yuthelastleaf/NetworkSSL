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

    cv::Mat dst;//�ȶ���һ���µ�ͼ��
    dst = cv::Mat(src.size(), src.type());//��srcͼ����ͬ�Ĵ�С������
    dst = cv::Scalar(127, 0, 255);//ÿ�����ص��RGB����(127��0��255)

    cv::Mat dst2, dst3;
    dst2 = src.clone();//��¡һ��һģһ����ͼƬ��������ȫ����
    src.copyTo(dst3);//Ҳ�ǿ�¡����ͬ��src.clone()��������ȫ����

    const uchar* p = dst3.ptr<uchar>(0);//��һ���׸����ص������ֵ
    const auto i = dst3.ptr<uchar>(0)[0];
    const int* q = dst3.ptr<int>(0);
    printf("the first row pixel is %d \n", *p);
    printf("the first pixel is %d \n", i);//*p=i=94
    std::cout << *q << std::endl;//*q=1515541342

    int col = dst3.cols;
    int row = dst3.rows;
    printf("the col is %d , the row is %d \n", col, row);

    cv::Mat M(100, 100, src.type(), cv::Scalar(21, 13, 43));//��һ��ͼ���巽ʽ
    //cout << "M= " << endl << M << endl;
    std::cout << src.type() << std::endl;

    cv::Mat dst4(src);//���ڲ��ָ���
    cv::Mat dst5 = dst4;

    cv::Mat L;
    L.create(src.size(), src.type());//L.create���ܸ�ֵ����Ҫ�����������и�ֵ����
    L = cv::Scalar(0, 0, 0);//����ʹ�ö�ά������и�ֵ

    cv::Mat dst6 = cv::Mat::eye(2, 3, src.type());//�Խ�����ÿ�����ص�ĵ�һ��ͨ����ֵΪ1�������Ϊ0
    /*[1,0,0,0,0,0,0,0,0
       0,0,0,1,0,0,0,0,0]*/
    std::cout << "dst6= " << std::endl << dst6 << std::endl;

    cv::namedWindow("output", cv::WINDOW_AUTOSIZE);
    imshow("output", dst3);
    cv::waitKey(0);
    return 0;
}