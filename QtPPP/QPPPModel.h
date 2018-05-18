#ifndef QPPPMODEL_H
#define QPPPMODEL_H

#include "QCmpGPST.h"
#include "QReadOFile.h"
#include "QReadSP3.h"
#include "QReadClk.h"
#include "QTropDelay.h"
#include "QReadAnt.h"
#include "QWindUp.h"
#include "QTideEffect.h"
#include "QKalmanFilter.h"
#include "QWrite2File.h"

//使用Engin保存和计算
using namespace Eigen;

//使用无电离层做PPP模型



class QPPPModel:public QBaseObject
{
//函数部分
public:
	QPPPModel(void);
	QPPPModel(QString OFileName,QStringList Sp3FileNames,QStringList ClkFileNames,QString ErpFileName = "",QString BlqFileName = "OCEAN-GOT48.blq",QString AtxFileName = "antmod.atx",QString GrdFileName = "gpt2_5.grd");
	~QPPPModel();
	void Run();//读取卫星数据以及计算各项改正便于后面计算 存入SatlitData
	void Run(QString pppFileName);;//读取.ppp文件运行kalman滤波
	//设置文件系统 SystemStr:"G"(开启GPS系统);"GR":(开启GPS+GLONASS系统);"GRCE"(全部开启)等
	bool setSatlitSys(QString SystemStr);//其中GPS,GLONASS,BDS,Galieo分别使用：字母G,R,C,E
private:
	void initVar();
	void getSP3Pos(double GPST,int PRN,char SatType,double *p_XYZ,double *pdXYZ = NULL);//GPST卫星的周内秒 p_XYZ 返回WGS84坐标 和速度
	void getCLKData(int PRN,char SatType,double GPST,double *pCLKT);//获取卫星钟误差 GPST 卫星发射时刻周内秒
	void getSatEA(double X,double Y,double Z,double *approxRecvXYZ,double *EA);//计算高度角EA
	double getSagnac(double X,double Y,double Z,double *approxRecvXYZ);//地球自传
	double getRelativty(double *pSatXYZ,double *pRecXYZ,double *pSatdXYZ);//计算相对论效应
	double getTropDelay(double MJD,int TDay,double E,double *pBLH,double *mf = NULL);//计算对流层延迟.MJD:简化儒略日，TDay:年积日，E：高度角（rad） pBLH：大地坐标系，*mf：投影函数
	bool getRecvOffset(double *EA,char SatType,double &L1Offset,double &L2Offset);//计算接收机L1和L2相位中心改正PCO+PCV,EA：高度角，方位角（单位rad），L1Offset和L2Offset代表视线方向的距离改正(单位m)
	double getSatlitOffset(int Year,int Month,int Day,int Hours,int Minutes,double Seconds,int PRN,char SatType,double *StaPos,double *RecPos);//计算卫星PCO+PCV改正，因为卫星 G1和G2频率一样所以两个波段改正是一样的；StaPos和RecPos，卫星和接收机WGS84坐标（单位m）
	double getTideEffect(int Year,int Month,int Day,int Hours,int Minutes,double Seconds,double *pXYZ,double *EA,double *psunpos=NULL,
					   double *pmoonpos = NULL,double gmst = 0,QString StationName = "");//计算潮汐在视线方向的改正（单位m）出入太阳、月亮，gmst数据可以减少这些数据重复计算，StationName特可以传入
	double getWindup(int Year,int Month,int Day,int Hours,int Minutes,double Seconds,double *StaPos,double *RecPos,double &phw,double *psunpos = NULL);//SatPos和RecPos代表卫星和接收机WGS84坐标 返回周（单位周）范围[-0.5 +0.5]
	bool CycleSlip(const SatlitData &oneSatlitData,double *pLP);//周跳探测//返回pLP是三维数组，第一个是W-M组合（N2-N1 < 5） 第二个数电离层残差（<0.3） 第三个是（lamt2*N2-lamt1*N1 < 5）
	double getPreEpochWindUp(QVector < SatlitData > &prevEpochSatlitData,int PRN,char SatType);//获取前一个历元的WindUp 没有返回0
	void getGoodSatlite(QVector < SatlitData > &prevEpochSatlitData,QVector < SatlitData > &epochSatlitData,double eleAngle = 5);//得到质量高的卫星包括：周跳 高度角 数据是否缺失 C1-P2<50 相邻历元WindUp < 0.3 ;preEpochSatlitData:前一个历元卫星数据 epochSatlitData：当前历元卫星数据 (自动删除低质量卫星);eleAngle:高度角
	void readPPPFile(QString pppFileName,QVector < QVector < SatlitData > > &allEpochData);//读取各项改正之后的.ppp文件（自定义输出文件的格式）
//数据部分
public:
	//QVector< QVector < SatlitData > > g_AllSatlitData; //存储所有历元坐标以及各项改正
private:
	QString m_OFileName;//O文件路径+文件名
	QStringList m_Sp3FileNames;//SP3文件路径+文件名
	QStringList m_ClkFileNames;//CLK文件路径+文件名
	double m_ApproxRecPos[3];//测站近似坐标
	int multReadOFile;//每次缓冲O文件的历元数据（数目越大占用内存越高，速度相对快些。。。默认1000）
	int m_leapSeconds;
	//各项类库用于计算误差改正、kalman滤波、文件操作
	QCmpGPST qCmpGpsT;//用于计算GPS时间 以及坐标转化等的函数库
	QReadSP3 m_ReadSP3Class;//用于读取和计算卫星轨道
	QReadClk m_ReadClkClass;//读取卫星钟差文件
	QReadOFile m_ReadOFileClass;//读取O文件类
	QTropDelay m_ReadTropClass;//读取对流层需要文件
	QReadAnt m_ReadAntClass;//读取天线数据类
	QWindUp m_WinUpClass;//相位解缠
	QTideEffect m_TideEffectClass;//潮汐影响
	QKalmanFilter m_KalmanClass;//kalman滤波
	QWrite2File m_writeFileClass;//写入文件类
};

#endif // QPPPMODEL_H
