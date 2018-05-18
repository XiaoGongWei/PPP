#include "QNewFunLib.h"


QNewFunLib::QNewFunLib(void)
{
}


QNewFunLib::~QNewFunLib(void)
{
}

//计算SatPos在1,2号接收机直线上的投影点
void QNewFunLib::computeCrossPoint(Vector3d Recv1Pos,Vector3d Recv2Pos,Vector3d SatPos,Vector3d *crossPoint,Vector3d *talpha)
{
	Vector3d crossPos,alpha,Recv1_Sat;
	double t = 0,alphaNorm = 0;
	crossPos.setZero();
	alpha.setZero();
	Recv1_Sat.setZero();
	alpha = Recv2Pos - Recv1Pos;
	Recv1_Sat = SatPos - Recv1Pos;
	alphaNorm = alpha.norm();
	t = alpha.dot(Recv1_Sat)/(alphaNorm*alphaNorm);
	crossPos = t*alpha + Recv1Pos;
	alpha = alpha/alpha.norm();
	*crossPoint = crossPos;
	if (talpha) *talpha = alpha;
}
