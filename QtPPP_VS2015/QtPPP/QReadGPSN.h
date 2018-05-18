#pragma once
#include "QGlobalDef.h"




class QReadGPSN
{
//函数部分
public:
	QReadGPSN(void);
	QReadGPSN(QString NFileName);
	~QReadGPSN(void);
	QVector< BrdData > getAllData();//读取所有广播星历数据到allBrdData
	void getSatPos(int PRN,char SatType,double CA,int Year,int Month,int Day,int Hours,int Minutes,double Seconds,double *pXYZ,double *pdXYZ);//PRN:卫星号，SatType:卫星类型(G,C,R,E),年月日时分秒为观测时刻UTC时间(BDS和GLONASS函数内部自动转换)
private:
	void initVar();//初始化变量
	void getHeadInf();//读取去头部信息
	void readNFileVer2(QVector< BrdData > &allBrdData);//读取Rinex 2.X广播星历数据
	int SearchNFile(int PRN,char SatType,double GPSOTime);//搜索最近的导航数据
	double YMD2GPSTime(int Year,int Month,int Day,int Hours,int Minutes,int Seconds,int *WeekN = NULL);//YMD Change to GPST //GPSTimeArray[4]=GPSWeek  GPS_N（周内第几天） GPST_second JD
	double getLeapSecond(int Year,int Month,int Day,int Hours=0,int Minutes=0,int Seconds=0);//获取跳秒
	double computeJD(int Year,int Month,int Day,int HoursInt,int Minutes = 0,double Seconds = 0.0);
	Vector3d GlonassFun(Vector3d Xt,Vector3d dXt,Vector3d ddX0);
	Vector3d RungeKuttaforGlonass(const BrdData &epochBrdData,double tk,double t0,Vector3d &dX);
//数据部分
public:

private:
	QFile m_readGPSNFile;//读取广播星历文件
	QString m_NfileName;//保存广播星历文件名字

	int m_BaseYear;//基本年 定义为2000
	double m_leapSec;//保存跳秒
	QVector< BrdData > m_allBrdData;
	bool isReadHead;//判断是否读取头文件
	bool isReadAllData;//判断是否读取所有数据
	QString tempLine;//缓存一行字符串
	int m_epochDataNum;//存储一个数据段(GPS和BDS是28个7行GLONASS是12个3行)
//以下是头文件数据部分
	//RINEX VERSION / TYPE
	double RinexVersion;
	char FileIdType;//G,C,R 分别代表GPS,BDS,GLONASS系统
	//PGM / RUN BY / DATE
	QString PGM;
	QString RUNBY;
	QString CreatFileDate;
	//COMMENT
	QString CommentInfo;
	//ION ALPHA
	double IonAlpha[4];
	double IonBeta[4];
	//DELTA-UTC: A0,A1,T,W
	double DeltaA01[2];
	int DeltaTW[2];
	//	IsReadHeadInfo
	bool IsReadHeadInfo;
};

