#ifndef QWINDUP_H
#define QWINDUP_H

#include "QCmpGPST.h"
/*天线相位中心解缠，使用RTKLIB代码封装
*/


class QWindUp
{
//函数部分
public:
	QWindUp(void);
	~QWindUp(void);
	double getWindUp(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,double *StaPos,double *RecPos,double &phw,double *psunpos = NULL);//SatPos和RecPos代表卫星和接收机WGS84坐标 返回周（单位周）范围[-0.5 +0.5],phw是上个历元的相位旋转
private:
	void windupcorr(gtime_t time, const double *rs, const double *rr,	double *phw,double *psunpos = NULL);//传入太阳坐标，可以减少重复计算
//变量部分
public:

private:
	QCmpGPST m_qCmpClass;
};

#endif

