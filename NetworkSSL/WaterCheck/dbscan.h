#ifndef DBSCAN_H
#define DBSCAN_H


#include <vector>
#include <cmath>

#include <opencv2/opencv.hpp>

#define UNCLASSIFIED -1
#define CORE_POINT 1
#define BORDER_POINT 2
#define NOISE -2
#define SUCCESS 0
#define FAILURE -3

typedef struct Point_
{
    // float x, y, z;  // X, Y, Z position
    cv::Point _point;
    int clusterID;  // clustered ID
}Point;

class DBSCAN {
public:    
    DBSCAN(unsigned int minPts, std::vector<Point> points){
        m_minPoints = minPts;
        m_epsilon = calculateAdaptiveEpsilonSquared(points);
        m_points = points;
        m_pointSize = points.size();
    }
    ~DBSCAN(){}

    int run();
    std::vector<int> calculateCluster(Point point);
    int expandCluster(Point point, int clusterID);
    double calculateAdaptiveEpsilonSquared(const std::vector<Point>& points);
    inline double calculateDistance(const Point& pointCore, const Point& pointTarget);

    std::map<int, std::vector<Point>> groupClusters();

    int getTotalPointSize() {return m_pointSize;}
    int getMinimumClusterSize() {return m_minPoints;}
    int getEpsilonSize() {return m_epsilon;}
    
public:
    std::vector<Point> m_points;
    
private:    
    unsigned int m_pointSize;
    unsigned int m_minPoints;
    double m_epsilon;
};

#endif // DBSCAN_H
