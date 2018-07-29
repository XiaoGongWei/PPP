#ifndef QCMPGPST_H
#define QCMPGPST_H

/*
1、本类是PPP常用的函数计算库类，提供了大量的的计算方法
卫星计算常用的坐标和时间转换，以及太阳月亮WGS84下坐标计算需要读取.erp文件参数
*/

#include "QGlobalDef.h"

const  double gpst0[]={1980,1, 6,0,0,0}; /* gps time reference */
const  double gst0 []={1999,8,22,0,0,0}; /* galileo system time reference */
const  double bdt0 []={2006,1, 1,0,0,0}; /* beidou time reference */

//{2015,7,1,0,0,0,-17},
const static double leaps[][7]={ /* leap seconds {y,m,d,h,m,s,utc-gpst,...} */
	{2015,7,1,0,0,0,-17},
	{2012,7,1,0,0,0,-16},
	{2009,1,1,0,0,0,-15},
	{2006,1,1,0,0,0,-14},
	{1999,1,1,0,0,0,-13},
	{1997,7,1,0,0,0,-12},
	{1996,1,1,0,0,0,-11},
	{1994,7,1,0,0,0,-10},
	{1993,7,1,0,0,0, -9},
	{1992,7,1,0,0,0, -8},
	{1991,1,1,0,0,0, -7},
	{1990,1,1,0,0,0, -6},
	{1988,1,1,0,0,0, -5},
	{1985,7,1,0,0,0, -4},
	{1983,7,1,0,0,0, -3},
	{1982,7,1,0,0,0, -2},
	{1981,7,1,0,0,0, -1},
	{1980,1,6,0,0,0, 0}
};

/* coordinate rotation matrix ------------------------------------------------*/
#define Rx(t,X) do { \
	(X)[0]=1.0; (X)[1]=(X)[2]=(X)[3]=(X)[6]=0.0; \
	(X)[4]=(X)[8]=cos(t); (X)[7]=sin(t); (X)[5]=-(X)[7]; \
} while (0)

#define Ry(t,X) do { \
	(X)[4]=1.0; (X)[1]=(X)[3]=(X)[5]=(X)[7]=0.0; \
	(X)[0]=(X)[8]=cos(t); (X)[2]=sin(t); (X)[6]=-(X)[2]; \
} while (0)

#define Rz(t,X) do { \
	(X)[8]=1.0; (X)[2]=(X)[5]=(X)[6]=(X)[7]=0.0; \
	(X)[0]=(X)[4]=cos(t); (X)[3]=sin(t); (X)[1]=-(X)[3]; \
} while (0)

class QCmpGPST
{
//函数部分
public:
	QCmpGPST();
	~QCmpGPST();
	int getSatPRN(QString StaliteName);//获取卫星PRN
	int YearAccDay(int Year, int Month, int Day);//计算年积日
	double YMD2GPSTime(int Year,int Month,int Day,int Hours,int Minutes,int Seconds,int *WeekN = NULL);//YMD Change to GPST //GPSTimeArray[4]=GPSWeek  GPS_N（周内第几天） GPST_second JD
	void XYZ2SAE(double X,double Y,double Z,double *m_pSAZ,double *PX);//XYZ 接收机近似坐标 m_SAZ（弧度）返回的计算结果 PX测站坐标
	void XYZ2BLH(double X,double Y,double Z,double *m_pBLH,double *ellipseCoeff = NULL);
	void XYZ2ENU(double X,double Y,double Z,double *m_pENU,double *PX);
	void XYZ2SAE(double *pXYZ,double *m_pSAZ,double *PX);//XYZ 接收机近似坐标 m_SAZ（弧度）返回的计算结果 PX测站坐标
	void XYZ2BLH(double *pXYZ,double *m_pBLH);
	void XYZ2ENU(double *pXYZ,double *m_pENU,double *PX);
	double computeJD(int Year,int Month,int Day,int HoursInt=0,int Minutes=0,double Seconds=0);//计算儒略日
	double computeMJD(int Year,int Month,int Day,int HoursInt=0,int Minutes=0,double Seconds=0);//计算简化儒略日
	bool getSunMoonPos(int Year,int Month,int Day,int HoursInt,int Minutes,double Seconds,double *sunpos,double *moonpos,double *gmst = NULL);//计算太阳月亮WGS84坐标 和GMST平格里尼治时间
	void sunmoonpos(gtime_t tutc, const double *erpv, double *rsun,
		double *rmoon, double *gmst);//计算太阳坐标 参考RTKlab
	double getLeapSecond(int Year,int Month,int Day,int Hours=0,int Minutes=0,int Seconds=0);//获取跳秒
	//常用的向量数学函数
	double InnerVector(double *a,double *b,int Vectorlen = 3);//计算a,b向量内积
	bool OutVector(double *a,double *b,double *c);//计算三维a,b向量外积使用c返回结果
	double norm(const double *a, int n);
	void cross3(const double *a, const double *b, double *c);
	int normv3(const double *a, double *b);
	double dot(const double *a, const double *b, int n);
	//RTKLIB时间转化
	gtime_t gpst2utc(gtime_t t);
	gtime_t epoch2time(const double *ep);
	gtime_t gpst2time(int week, double sec);
	void time2epoch(gtime_t t, double *ep);
	double timediff(gtime_t t1, gtime_t t2);
	//RTKLIB坐标转换
	void ecef2pos(const double *r, double *pos);
	void xyz2enu(const double *pos, double *E);
	//读取erp文件
	int readerp(const char *file, erp_t *erp);
	int geterp(const erp_t *erp, gtime_t time, double *erpv);
	bool readRepFile(QString m_erpFileName);//初始化读取erp文件一次
private:
	double MyAtanA(double x,double y);//tan函数
	double MyAtanL(double x,double y);//tan函数
	//RTKLab 求取太阳坐标
	gtime_t timeadd(gtime_t t, double sec);
	void ast_args(double t, double *f);
	gtime_t utc2gpst(gtime_t t);
	double time2gpst(gtime_t t, int *week);
	double time2sec(gtime_t time, gtime_t *day);
	double utc2gmst(gtime_t t, double ut1_utc);
	void sunmoonpos_eci(gtime_t tut, double *rsun, double *rmoon);
	void nut_iau1980(double t, const double *f, double *dpsi, double *deps);
	void eci2ecef(gtime_t tutc, const double *erpv, double *U, double *gmst);
	void matmul(const char *tr, int n, int k, int m, double alpha,
		const double *A, const double *B, double beta, double *C);
	

//数据部分
public:
	double elipsePara[6];//椭球参数，默认WGS84参数
private:
	erp_t m_erpData;//保存erp文件数据
	bool isuseErp;//是否读取了erp文件数据
};

#endif // QCMPGPST_H
