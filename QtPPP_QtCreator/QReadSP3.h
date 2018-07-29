#ifndef QREADSP3_H
#define QREADSP3_H


#include "QGlobalDef.h"

/*
1、此处需要优化计算：8点插值 不需要频繁对每个卫星插值
2、舍弃后期阶段不精确坐标
3、同时返回插值时刻卫星的速度
4、示例:

(1)类的初始化
QReadSP3 readSP3("D:\\igs15786.sp3");//必须指定路径

(2)在调用类之前需要读取数据一次，在想要读取数据调用getAllData();耗时较长
readSP3.getAllData();

(3)然后调用getPrcisePoint(int PRN,double GPST,double *pXYZ,double *pdXYZ = NULL);
double pXYZ[3] ={0}，pdXYZ[3] = {0};//定义坐标和速度
getPrcisePoint(32,514800,pXYZ,pdXYZ);//得到坐标和速度
或者getPrcisePoint(32,514800,pXYZ);//得到坐标

5、注意：
（1）本类可以读取 RINEX VERSION / TYPE：3.00 / C (当前只读取GPS卫星 如需修改可以自己简单修改readFileData2Vec和readAllData函数) 
（2）调用getPrcisePoint建议按照GPS时间（GPST）由小到大 调用才能“提升运行速度”，否则不明显。如果不想按照时间大小调用请 将ACCELERATE 改为0
*/

#define  ACCELERATE  1 // 1：加速程序（getPrcisePoint时间由小到大） 0：不加速程序（时间乱序）
//坐标系统采用SP3自带坐标系统
typedef struct _SP3Data
{//SP3文件的数据格式
	int GPSWeek;//GPS周
	int GPSTime;//GPS周内秒
	MatrixXd MatrixDataGPS;//GPS系统第i列分别存放PRN X,Y,Z
	MatrixXd MatrixDataGlonass;//Glonass
	MatrixXd MatrixDataBDS;//BDS
	MatrixXd MatrixDataGalieo;//Galieo
}SP3Data;

class QReadSP3: public QBaseObject
{
//函数部分
public:
	QReadSP3();
	QReadSP3(QString SP3FileName);//读取一个文件
	QReadSP3(QStringList SP3FileNames);//读取多个文件包括一个
	~QReadSP3(void);
	QVector< SP3Data > getAllData();//读取所有数据（耗时）
	bool setSatlitSys(QString SystemStr);//其中GPS,GLONASS,BDS,Galieo分别使用：字母G,R,C,E（覆盖父类）
	void getPrcisePoint(int PRN,char SatType,double GPST,double *pXYZ,double *pdXYZ = NULL);//获得精密星历坐标pXYZ 以及速度 pDXYZ(三维)
	void releaseAllData();
private:
	void initVar();//初始化变量
	void InitStruct();//初始化需要用的结构体内部的矩阵(有节约内存作用)
	void openFiles(QString SP3FileName);//只读方式打开文件
	void readAllData2Vec();//读取初始化单个文件所有数据 到m_allEpochData
	void readFileData2Vec(QStringList SP3FileNames);// 读取多个文件数据 到m_allEpochData
	void get8Point(int PRN,char SatType,double *pX,double *pY,double *pZ,int *pGPST,double GPST);//pX,pY,pZ：8个点坐标；pGPST:8个点GPS周内秒;GPST卫星发射时刻周内秒
	void lagrangeMethod(int PRN,char SatType,double GPST,double *pXYZ,double *pdXYZ = NULL);//七阶拉格朗日插值 此处选取前后---共8个点做插值 GPST卫星发射时刻周内秒 返回pXYZ：sp3格式（WGS84）坐标; pdXYZsp3格式（WGS84）速度
	void readHeadData();//读取头文件信息
	int YMD2GPSTime(int Year,int Month,int Day,int Hours,int Minutes,int Seconds,int *GPSWeek = NULL);//计算GPS时间。返回 GPS周内秒，GPSWeek:GPS周
//数据部分
public:
	
private:
	QFile m_readSP3FileClass;//读取文件类
	QString m_SP3FileName;//保存单个文件名字
	QStringList m_SP3FileNames;
	QVector< SP3Data > m_allEpochData;//每个历元坐标数据。年月日时分秒 第i列分别存放PRN X,Y,Z
	SP3Data epochData;//存储每个历元数据
	QString tempLine;//临时存放读取的一行数据
	bool isReadHead;//是否读取头文件
	bool isReadAllData;//是否读取整个文件数据
	bool IsSigalFile;//是否多个文件或单个文件
	int m_WeekOrder;//保存跨周文件数目
	static const int lagrangeFact;
//用于优化查找钟差的变量 防止一直从0开始查找
	int m_lastGPSTimeFlag;//保存上次get8Point获取的第一个GPS时间数据
	double m_lastCmpGPSTime;//保存上次计算的GPST防止一个历元
	int m_EndHeadNum;//第一个和最后一个钟差文件读取块数
};

#endif
