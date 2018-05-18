#ifndef QREADOFILE_H
#define QREADOFILE_H

#include "QGlobalDef.h"


/*
1、读取卫星观测文件，继承了QReadGPSO 类读取2.0版本
*/

typedef struct  _obsVarNames
{//版本3使用
	int obsNum3ver;//版本3使用
	QString SatType;//G R C S E：系统号码
	QVector< QString > obsNames3ver;//版本3使用
	int CPflags[20];//0-3存储Li,Lj,Ci,Cj(版本3.X用,波段谁靠前储存谁)4-7存储对应的频率号码i,j,i,j
}obsVarNamesVer3;


class QReadOFile:public QBaseObject
{
//函数部分
public:
	QReadOFile(void);
	~QReadOFile(void);
	QReadOFile(QString OfileName);
//读取数据的几个重要函数
	void getEpochData(QVector< SatlitData > &epochData);//读取一个历元数据（可根据版本进行扩展）
	void getMultEpochData(QVector< QVector< SatlitData > >&multEpochData,int epochNum);//读取epochNum个历元数据(提前到文件底部可能不足epochNum个)
	void closeFile();//不想读取数据或读取结束请调用此函数关闭文件
	bool isEnd();//判断是否到达文件尾部（结束）
//获取头文件信息的函数
	QString getComment();//获取头文件注释信息
	void getApproXYZ(double* AppXYZ);//获取近似坐标
	void getAntHEN(double* m_AntHEM);//获取天线的HEN改正
	void getFistObsTime(int* m_YMDHM,double &Seconds);//获得起始观测历元时间
	QString getMakerName();//获得天线标志点名字
	double getInterval();//获取观测间隔（单位s）
	QString getAntType();//获取接收机天线类型
	QString getReciveType();//获取接收机类型
private:
	void initVar();//初始化变量
	void getHeadInf();//读取去头部信息
	void getLPFlag2();//获取载波和伪距所在位置（版本小于3使用）
	void getLPFlag3();//获取载波和伪距所在位置（版本大于3使用）
	void readEpochVer3(QVector< SatlitData > &epochData);//读取3.x版本文件
	void readEpochVer2(QVector< SatlitData > &epochData);//读取2.x版本文件
	double getMJD(int Year,int Month,int Day,int HoursInt,int Minutes,int Seconds);//计算约化儒略日（本类未用到）
	bool getOneSatlitData(QString &dataString,SatlitData &oneSatlite);//将字符转数据行，解析到卫星数据，获取卫星测距码和载波数据
	//debug:2017.07.08
	void getFrequencyVer3(SatlitData &oneSatlite);//根据卫星PRN，类型获取L1和L2的频率
	void getFrequencyVer2(SatlitData &oneSatlite);//根据卫星PRN，类型获取L1和L2的频率
	void getFrequency(SatlitData &oneSatlite);//根据卫星PRN，类型获取L1和L2的频率
//数据部分
public:
	
private:
//内部变量
	QFile m_readOFileClass;//读取O文件类
	QString m_OfileName;//保存O文件名字

	bool isReadHead;
	QString tempLine;//一行字符串缓冲
	int CPflags[20];//存储L1,L2,C1,P2,P1(版本2.X用)
	int CPflagVer3[5][20];//5行分别存储了GPS，GLONASS，BDS，SBAS，Galieo系统测距码和载波位置,以及频段
	QVector< obsVarNamesVer3 > m_obsVarNamesVer3;//版本3存储SYS / # / OBS TYPES
	QRegExp matchHead;//版本2.X用匹配头文件（初始化一次即可）
	int baseYear;//版本2.X 用于获取基本年 （例如：1989 则baseYear = 1900;2010 则baseYear = 2000）
//头文件信息
	//RINEX VERSION / TYPE
	double RinexVersion;
	QChar FileIdType;
	QChar SatelliteSys;
	//PGM / RUN BY / DATE
	QString PGM;
	QString RUNBY;
	QString CreatFileDate;
	//COMMENT
	QString CommentInfo;
	//MARKER NAME
	QString MarkerName;
	//MARKER NUMBER
	QString MarkerNumber;
	//OBSERVER / AGENCY
	QString ObserverNames;
	QString Agency;
	//REC # / TYPE / VERS
	QString ReciverREC;
	QString ReciverType;
	QString ReciverVers;
	//ANT # / TYPE
	QString AntNumber;
	QString AntType;
	//APPROX POSITION XYZ
	double ApproxXYZ[3];
	//ANTENNA: DELTA H/E/N
	double AntHEN[3];
	//WAVELENGTH FACT L1/2
	int FactL12[2];
	//# / TYPES OF OBSERV 
	int TypeObservNum;// 版本小于3使用
	QVector<QString> ObservVarsNames;
	//INTERVAL
	double IntervalSeconds;
	//TIME OF FIRST OBS
	int YMDHM[5];
	double ObsSeconds;
	QString SateSystemOTime;//GPS UTC
};

#endif

