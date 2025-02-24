#include<opencv2/opencv.hpp>
#include<iostream>

cv::Mat bgImage;//����һ��ȫ�ֱ���
void MyLines();
void MyRectangle();
void MyEllipse();
void MyCircle();
void MyPolygon();
void RandomLineDemo();
int main(int argc, char** argv) {
    bgImage = cv::imread("../TestRec/cat.jpg");
    //Mat dst = bgImage(Rect(0, 0, 100, 100));
    MyLines();
    MyRectangle();
    MyEllipse();
    MyCircle();
    MyPolygon();
    //�˸�������Ŀ��ͼ�����֣��ı������½�λ�ã����壬�ֺ�(���ű���)����ɫ���߿�������
    putText(bgImage, "hello opencv", cv::Point(300, 300), cv::FONT_HERSHEY_COMPLEX, 1.0, cv::Scalar(0, 255, 211), 1, 8);
    cv::namedWindow("output", cv::WINDOW_AUTOSIZE);
    imshow("output", bgImage);
    RandomLineDemo();
    cv::waitKey(0);
    return 0;
}
void MyLines() {
    cv::Point p1 = cv::Point(20, 30);
    cv::Point p2;
    p2.x = 300;
    p2.y = 300;
    cv::Scalar color = cv::Scalar(0, 0, 255);
    //���������ֱ��ǣ�ԭͼ����ʼ�㣬�����㣬��ɫ���߿�������
    line(bgImage, p1, p2, color, 1, 8);//��opencv�汾��֧��LINE_8��ֱ��д8����
}
void MyRectangle() {
    cv::Rect rect = cv::Rect(200, 100, 300, 300);
    cv::Scalar color = cv::Scalar(255, 0, 0);
    //���ľ���
    rectangle(bgImage, rect, color, 2, 8);//��������ֱ��ǣ�ԭͼ�����δ�С����ɫ���߿�������
}
void MyEllipse() {
    cv::Scalar color = cv::Scalar(0, 255, 0);
    //�Ÿ�������ԭͼ��Բ�ģ���С��width��height������Բ�İڷŽǶȣ���ʼ�Ƕȣ������Ƕȣ���ɫ���߿�������
    ellipse(bgImage, cv::Point(bgImage.cols / 2, bgImage.rows / 2), cv::Size(100, 200), 0, 0, 360, color, 2, 8);
}
void MyCircle() {
    cv::Scalar color = cv::Scalar(0, 255, 255);
    cv::Point center = cv::Point(bgImage.cols / 2, bgImage.rows / 2);
    circle(bgImage, center, 150, color, 2, 8);//����������ԭͼ��Բ�ģ��뾶����ɫ���߿�������
}
void MyPolygon() {
    cv::Point pts[1][5];
    pts[0][0] = cv::Point(100, 100);
    pts[0][1] = cv::Point(100, 200);
    pts[0][2] = cv::Point(200, 200);
    pts[0][3] = cv::Point(200, 100);
    pts[0][4] = cv::Point(100, 100);
    const cv::Point* ppts[] = { pts[0] };//ָ��ָ���ָ��
    int npt[] = { 5 };//����һ����������
    cv::Scalar color = cv::Scalar(255, 12, 255);
    //����������Ŀ��ͼ�񣬶���εĶ��㼯�ϣ�ָ�����飩������ζ������Ŀ���������飩��Ҫ���ƵĶ���ε���������ɫ���߿�
    fillPoly(bgImage, ppts, npt, 1, color, 8);

}
void RandomLineDemo() {
    cv::RNG rng(12345);
    cv::Point pt1, pt2;
    cv::Mat bg = cv::Mat::zeros(bgImage.size(), bgImage.type());
    for (int i = 0; i < 100000; i++) {
        pt1.x = rng.uniform(0, bgImage.cols);
        pt2.x = rng.uniform(0, bgImage.cols);
        pt1.y = rng.uniform(0, bgImage.rows);
        pt2.y = rng.uniform(0, bgImage.rows);
        cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
        if (cv::waitKey(50) > 0) {
            break;
        }
        line(bg, pt1, pt2, color, 1, 8);
        cv::namedWindow("random_line", cv::WINDOW_AUTOSIZE);
        imshow("random_line", bg);
    }
}