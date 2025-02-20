#include<opencv2/opencv.hpp>
#include<iostream>
#include<math.h>


int main(int argc, char** argv)
{
    cv::Mat src, dst;
    src = cv::imread("../TestRec/cat.jpg");
    if (!src.data)
    {
        printf("could not find image...\n");
        return -1;
    }

    //src.cols�������ͼ��x������ظ�������Width
    //src.rows�������ͼ��y������ظ�������Height
    //src.channels()�������ÿ�����ص��ͨ��������RGBͼ��ͨ������Ϊ3
    /*cout << src.cols << '\n' << src.rows << '\n' << src.channels() << endl;*/

    //��Ĥд��һ��
    int cols = (src.cols - 1) * src.channels();//ע�����src.cols-1
    int offsetx = src.channels();
    int rows = src.rows;

    /*Mat��ʼ��*/
    /*Mat M(2,2,CV_8UC3,cv::Scalar::all(1))
    ǰ����������ָ���������������(int rows,int cols)������(Size size)
    �����������Ǿ������������(CV_8UC3 (3ͨ�����У�8 bit �޷�������))
    ��4���Ƕ�ÿ������ֵ����ֵ�����������ǰ�ÿ��ͨ��������ֵ����ֵ1�������Scalar(255,0,0),���ǽ�255��0��0�ֱ���ÿ�����ص��3��ͨ��*/
    dst = cv::Mat::zeros(src.size(), src.type());//��ʼ��һ���µĿ�ͼ����src��ͼ���С��ͼ�����Ͷ�һ��(zeros��ȫ����ֵΪ0)(��zeros������Ҳ������������������)

    for (int row = 1; row < (rows - 1); row++)//���rowsΪʲôҪ-1
    {
        const uchar* current = src.ptr<uchar>(row);
        const uchar* previous = src.ptr<uchar>(row - 1);
        const uchar* next = src.ptr<uchar>(row + 1);
        uchar* output = dst.ptr<uchar>(row);
        for (int col = offsetx; col < cols; col++)
        {
            //������Ĥ���������ұ�֤���ط�Χ��0~255֮��
            output[col] = cv::saturate_cast<uchar>(5 * current[col] - (current[col + offsetx] + current[col - offsetx] + previous[col] + next[col]));
        }
    }

    //��Ĥ��������
    double t = cv::getTickCount();//��ʼ��ʱ

    cv::Mat kernel = (cv::Mat_<char>(3, 3) << 0, -1, 0, -1, 5, -1, 0, -1, 0);
    //filter2D(src, dst, -1, kernel);
    filter2D(src, dst, src.depth(), kernel);//src.depth()��ʾ��ԭͼsrc��λͼ���һ������ͬ��-1

    //getTickCount()Ϊ��ʼ���������ļ�ʱ�ܴ���
    //tΪ��ʼ���������ļ�ʱ�ܴ���
    double timeconsume = (cv::getTickCount() - t) / cv::getTickFrequency();
    printf("time consume %.2f", timeconsume);//�����������ĵ�ʱ��

    namedWindow("contrast image", cv::WINDOW_AUTOSIZE);
    imshow("contrast image", dst);
    namedWindow("input image", cv::WINDOW_AUTOSIZE);
    imshow("input image", src);
    cv::waitKey(0);
    return 0;
}