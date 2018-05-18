#pragma once
#include <QDebug>
#include<Eigen/Dense>
using namespace Eigen;

class QNewFunLib
{
//函数部分
public:
	QNewFunLib(void);
	~QNewFunLib(void);
	void computeCrossPoint(Vector3d Recv1Pos,Vector3d Recv2Pos,Vector3d SatPos,Vector3d *crossPoint,Vector3d *talpha = NULL);//计算SatPos在1,2号接收机直线上的投影点
private:

//数据部分
public:

private:
};

