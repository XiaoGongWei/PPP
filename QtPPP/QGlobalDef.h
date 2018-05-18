#ifndef _QGLOBALDEF
#define _QGLOBALDEF

//必要的头文件
#include <QVector>
#include <QString>
#include <Qdir>
#include <QDebug>
#include <QFile>
#include <QRegExp>
#include <qmath.h>
//Eigen库（有些类不需要Eigen 可以独立）

#include <Eigen\Dense>
using namespace Eigen;
//C  标准库函数
#include <time.h>//RTKLAB 需求C语言标准库函数
#include <math.h>
//自定义 基类
#include "QBaseObject.h"

#define PI 3.1415926535897932385//圆周率 弧度
#define M_C 299792458.0 //光速m
#define M_GM 3.986004415E+14//地球引力常数(米m)
#define M_GMK 398600.5//地球引力常数(千米km)
#define M_We 0.000072921151467//地球自转角速度
#define M_Re 6378136//参考椭圆长半径(米m)
#define M_ReK 6378.136//参考椭圆长半径(千米km)
#define M_C20 -0.001082657//重力位第二带谐系数
//GPS 系统参数
#define M_F1   (1.57542e9)//F1
#define M_F2   (1.2276e9)//F2
#define M_F5   (1.17645e9)//F2
#define M_Lamta1   (0.190293672798365)//波长1
#define M_Lamta2   (0.244210213424568)//波长2
#define M_Lamta5	  (0.254828048790854)//波长5
static double g_GPSFrq[5] = {1.57542e9,1.2276e9,0,0,1.17645e9};//debug:2017.07.08
//GLONASS 系统参数
static int g_GlonassK[24] = {1,-4,5,6,1,-4,5,6,-2,-7,0,-1,-2,-7,0,-1,4,-3,3,2,4,-3,3,2};//Glonass PRN 对应的 频率号
#define M_GLONASSF1(k) (1602 + k*0.5625)*1e6
#define M_GLONASSF2(k) (1246 + k*0.4375)*1e6
#define M_GLOSSLamta1k(k) (M_C/((1602 + k*0.5625)*1e6))// GLONASS 频率号对应的L1波长
#define M_GLOSSLamta2k(k) (M_C/((1246 + k*0.4375)*1e6))// GLONASS 频率号对应的L1波长

//BDS 系统参数
static double g_BDSFrq[4] = {1.561098e9,1.561098e9,1.20714e9,1.26852e9};//存储北斗频率
#define M_BDSLamtak(k) (M_C/g_BDSFrq[k])//北斗L1波长

//Galieo参数
static double g_GalieoFrq[5] = {1.57542e9,1.17645e9,1.27875e9,1.20714e9,1.191795};//Galieo频段 1 5 6 7 8 频率
#define M_GalieoLamtak(k) (M_C/g_GalieoFrq[k])//北斗L1波长


//PPP消除电离层组合波长
//GPS波长
#define  M_alpha1 (M_F1*M_F1/(M_F1*M_F1 - M_F2*M_F2))
#define  M_alpha2 (M_F2*M_F2/(M_F1*M_F1 - M_F2*M_F2))
#define  M_GPSLamta3 (M_alpha1*M_Lamta1 - M_alpha2*M_Lamta2)//0.106953378142147

//计算LL3

#define M_GetLamta3(F1,F2) ( F1*M_C/(F1*F1 - F2*F2) - (F2*M_C)/(F1*F1 - F2*F2) )

//定义RTKlib 变量
#define D2R         (PI/180.0)          /* deg to rad */
#define AU          149597870691.0      /* 1 AU (m) */
#define AS2R        (D2R/3600.0)        /* arc sec to radian */
#define RE_WGS84    6378137.0           /* earth semimajor axis (WGS84) (m) */
#define FE_WGS84    (1.0/298.257223563) /* earth flattening (WGS84) */

//RTKLIB 结构体

//从1970-1-1到2038-1-18有效整数和小数时间
typedef struct {        /* time struct */
	time_t time;        /* time (s) expressed by standard time_t */
	double sec;         /* fraction of second under 1 s */
} gtime_t;

typedef struct {        /* earth rotation parameter data type */
	double mjd;         /* mjd (days) */
	double xp,yp;       /* pole offset (rad) */
	double xpr,ypr;     /* pole offset rate (rad/day) */
	double ut1_utc;     /* ut1-utc (s) */
	double lod;         /* length of day (s/day) */
} erpd_t;

typedef struct {        /* earth rotation parameter type */
	int n,nmax;         /* number and max number of data */
	erpd_t *data;       /* earth rotation parameter data */
} erp_t;




typedef struct _GPSOHeadData
{
	int Year;//年
	int Month;//月
	int Day;//日
	int Hours;//时
	int Minutes;//分
	double Seconds;//秒
	int ErrorFlag;//0 正常 1 出现电源故障
	int SatelliteNumbers;
	QVector< QString > SatelliteNames;
	double ClockDiv;
} GPSOHeadData;

typedef struct _GPSObsData
{
	int LL1;
	int SingnalIndex;
	QVector<double> ObsDataLine;
}GPSObsData;

typedef struct _GPSPosTime
{
	int Year;//年
	int Month;//月
	int Day;//日
	int Hours;//时
	int Minutes;//分
	double Seconds;//秒
}GPSPosTime;



typedef struct _RecivePos
{//单点定位得出接收机近似坐标改正
	double dX;//X的改正
	double dY;//Y的改正
	double dZ;//Z的改正
	int totolEpochStalitNum;//一个历元卫星数目
	int Year;//年
	int Month;//月
	int Day;//日
	int Hours;//时
	int Minutes;//分
	double Seconds;//秒
}RecivePos;

typedef struct _MWLLP
{//检测周跳时候用到的参数
	double MW;
	double dL;
	double LP;
}CyclySlipNum;


//每个历元广播星历数据格式
typedef struct _brdeph
{
	char SatType;//卫星类型
	int PRN;//卫星编号
	GPSPosTime UTCTime;//UTC时间
	double TimeDiv;//钟偏差
	double TimeMove;//钟漂移
	double TimeMoveSpeed;//钟漂移加速度
	QVector< double > epochNData;//存储一个数据段(GPS和BDS是28个7行GLONASS是12个3行)
}BrdData;


//PPP 保存计算的各项数据
//保存卫星的数据以及各项改正
typedef struct _SatlitData
{
	int PRN;//卫星PRN
	char SatType;//卫星类型
	GPSPosTime UTCTime;//约化儒略日
	int EpochFlag;//状态信号（一般不用）参考http://gage14.upc.es/gLAB/HTML/Observation_Rinex_v3.01.html
	double L1;
	double L2;
	double C1;
	double P1;
	double P2;//或者C2也储存到P2
	double LL3;//消除电离层载波
	double PP3;//消除电离层伪距
	double Frq[2];//记录L1，L2对应的频率
	double X;//WGS-84卫星坐标 需要从文件转换
	double Y;
	double Z;
	double EA[2];//卫星高度角以及方位角(度)
	//各项改正
	double Relativty;//相对论改正
	double Sagnac;//地球自传改正
	double StaClock;//卫星钟差
	double SatTrop;//对流层改正
	double StaTropMap;//对流层投影函数
	double AntHeight;//Antenna Height改正
	double L1Offset;//L1因为天线周数改正(周)
	double L2Offset;//L2因为天线周数改正（周）
	double SatOffset;//卫星天线相位中心改正(m)
	double TideEffect;//潮汐改正（m）
	double AntWindup;//相位缠绕改正（周）
	double SatWight;//卫星的权重 =  sin(E)
} SatlitData;


//定义模糊度
typedef struct _Ambiguity
{
	int PRN;//卫星PRN
	char SatType;//卫星类型
	double Amb;//模糊度(无论整数还是浮点数都用Amb存储)
	bool isIntAmb;//是否为整数模糊度
	GPSPosTime UTCTime;//UTC年月日时分秒
	int epochNum;//所属第几个历元数
}Ambiguity;

//定义存储钟差和天顶湿延迟
typedef struct _Clock
{
	double clockData;//接收机钟差
	double ZTD_W;//天顶湿延迟
	GPSPosTime UTCTime;//UTC年月日时分秒
}ClockData;

//打印错误信息
#define ErroTrace(erroInfo) {qDebug()<<erroInfo;}




#endif // _QGLOBALDEF

