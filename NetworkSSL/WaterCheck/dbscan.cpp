#include "dbscan.h"

// 根据检测到的点分布动态计算eps
std::vector<double> DBSCAN::calculateNearestDistances(const std::vector<Point>& points) {
    std::vector<double> distances;
    for (const auto& point : points) {
        double minDist = std::numeric_limits<double>::max();
        for (const auto& other : points) {
            if (point._point == other._point) continue;
            double dist = cv::norm(point._point - other._point);
            if (dist < minDist) minDist = dist;
        }
        distances.push_back(minDist);
    }
    std::sort(distances.begin(), distances.end());
    return distances;
}

// 使用k-distance图选择最优eps
double DBSCAN::findOptimalEps(const std::vector<Point>& points, int k) {
    auto distances = calculateNearestDistances(points);
    // 返回第k个最近邻距离的90分位数作为eps
    return distances[static_cast<size_t>(distances.size() * 0.9)];
}

int DBSCAN::run()
{
    int clusterID = 1;
    std::vector<Point>::iterator iter;
    for(iter = m_points.begin(); iter != m_points.end(); ++iter)
    {
        if ( iter->clusterID == UNCLASSIFIED )
        {
            if ( expandCluster(*iter, clusterID) != FAILURE )
            {
                clusterID += 1;
            }
        }
    }

    return 0;
}

int DBSCAN::expandCluster(Point point, int clusterID)
{    
    std::vector<int> clusterSeeds = calculateCluster(point);

    if ( clusterSeeds.size() < m_minPoints )
    {
        point.clusterID = NOISE;
        return FAILURE;
    }
    else
    {
        int index = 0, indexCorePoint = 0;
        std::vector<int>::iterator iterSeeds;
        for( iterSeeds = clusterSeeds.begin(); iterSeeds != clusterSeeds.end(); ++iterSeeds)
        {
            m_points.at(*iterSeeds).clusterID = clusterID;
            if (m_points.at(*iterSeeds)._point.x == point._point.x && m_points.at(*iterSeeds)._point.y == point._point.y )
            {
                indexCorePoint = index;
            }
            ++index;
        }
        clusterSeeds.erase(clusterSeeds.begin()+indexCorePoint);

        for( std::vector<int>::size_type i = 0, n = clusterSeeds.size(); i < n; ++i )
        {
            std::vector<int> clusterNeighors = calculateCluster(m_points.at(clusterSeeds[i]));

            if ( clusterNeighors.size() >= m_minPoints )
            {
                std::vector<int>::iterator iterNeighors;
                for ( iterNeighors = clusterNeighors.begin(); iterNeighors != clusterNeighors.end(); ++iterNeighors )
                {
                    if ( m_points.at(*iterNeighors).clusterID == UNCLASSIFIED || m_points.at(*iterNeighors).clusterID == NOISE )
                    {
                        if ( m_points.at(*iterNeighors).clusterID == UNCLASSIFIED )
                        {
                            clusterSeeds.push_back(*iterNeighors);
                            n = clusterSeeds.size();
                        }
                        m_points.at(*iterNeighors).clusterID = clusterID;
                    }
                }
            }
        }

        return SUCCESS;
    }
}

std::vector<int> DBSCAN::calculateCluster(Point point)
{
    int index = 0;
    std::vector<Point>::iterator iter;
    std::vector<int> clusterIndex;
    for( iter = m_points.begin(); iter != m_points.end(); ++iter)
    {
        if ( calculateDistance(point, *iter) <= m_epsilon )
        {
            clusterIndex.push_back(index);
        }
        index++;
    }
    return clusterIndex;
}

inline double DBSCAN::calculateDistance(const Point& pointCore, const Point& pointTarget )
{
    return pow(pointCore._point.x - pointTarget._point.x,2)+pow(pointCore._point.y - pointTarget._point.y,2);
}

std::map<int, std::vector<Point>> DBSCAN::groupClusters() {
    std::map<int, std::vector<Point>> clusterMap;

    for (const auto& point : m_points) {
        if (point.clusterID != -1) {  // 排除噪声点
            clusterMap[point.clusterID].push_back(point);
        }
    }

    return clusterMap;
}

double DBSCAN::calculateAdaptiveEpsilonSquared(const std::vector<Point>& points) {
    std::vector<double> nearest_distances;

    // 计算每个点到最近邻点的距离
    for (int i = 0; i < points.size(); i++) {
        double min_dist_squared = DBL_MAX;
        for (int j = 0; j < points.size(); j++) {
            if (i != j) {
                double dist_squared = calculateDistance(points[i], points[j]); // 这里已经是平方
                min_dist_squared = std::min(min_dist_squared, dist_squared);
            }
        }
        nearest_distances.push_back(sqrt(min_dist_squared)); // 开方得到真实距离
    }

    sort(nearest_distances.begin(), nearest_distances.end());
    int index = (int)(nearest_distances.size() * 0.75);
    double epsilon = nearest_distances[index] * 1.5;

    return epsilon * epsilon;  // 返回平方值
}

double DBSCAN::calculateReasonableEps(const std::vector<Point>& points) {
    if (points.size() < 10) return 30.0; // 默认值

    std::vector<double> distances;

    // 计算每个点到其他所有点的距离
    for (size_t i = 0; i < points.size(); i++) {
        std::vector<double> pointDistances;
        for (size_t j = 0; j < points.size(); j++) {
            if (i != j) {
                double dist = cv::norm(points[i]._point - points[j]._point);
                if (dist > 5 && dist < 200) { // 过滤异常值
                    pointDistances.push_back(dist);
                }
            }
        }

        // 对每个点的距离排序，取第3-5近邻的距离
        if (pointDistances.size() >= 5) {
            std::sort(pointDistances.begin(), pointDistances.end());
            distances.push_back(pointDistances[3]); // 第4近邻距离
        }
    }

    if (distances.empty()) return 35.0;

    // 返回中位数作为eps
    std::sort(distances.begin(), distances.end());
    return distances[distances.size() / 2] * 1.3; // 稍微放宽一些
}




