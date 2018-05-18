#ifndef QREADANT_H
#define QREADANT_H



#include "QCmpGPST.h"


/*程序说明
初始化的时候如果无法将"接收机"天线类型和观测"起始"时刻儒略日obvJD传入，必须调用setObsJD函数设置
天线类型和儒略日 否则无法匹配卫星起始有效时间天线的数据
1、类的初始化
QString AntFileName = "D:\\antmod.atx",AntType = "ASH700936B_M    SNOW";//AntFileName:天线文件路径（传入为空会去当前目录打开"antmod.atx"） AntType:O文件（观测文件）获取的天线类型
double ObsJD = 2455296.50;//观测起始时刻（大约时刻）儒略日
QReadAnt readAnt(AntFileName,AntType,ObsJD);
或者
QReadAnt readAnt(AntFileName);
readAnt.setObsJD(AntType,ObsJD);
2、数据文件的读取
在初始化地方只读取一次数据（耗时）
readAnt.getAllData();//读取卫星和接收机所有数据
3、函数调用方法
double E = 0.7,A = 0.3;//高度角和方位角
double L1Offset = 0, L2Offset = 0;//保存接收机L1和L2视线方向改正（单位m）
readAnt.getRecvL12(E,A,&L1Offset,&L2Offset);//计算接收机L1和L2视线方向改正（单位m）

int Year = 2010,Month =  4,Day = 10,Hours = 0,Minuts = 0,PRN = 32;
double Seconds = 0.0;
double StaPos[3] = {9999,999,9999},RecPos[3] = {9999,9999,9999};
double L12OffSet = 0;
gL12OffSet = readAnt.getSatOffSet(Year,Month,Day,Hours,Minuts,Seconds,PRN,StaPos,RecPos);//时间是卫星发射时刻UTC年月日时分秒，StaPos:卫星XYZ坐标，RecPos:接收机XYZ坐标，SatAntH:卫星天线改正 (由于各个卫星天线频率的PCO、PCV相同所以只需返回一个改正距离即可)返回m

*/

//定义存储天线数据结构 此处只保存常用到数据,获取其他数据还需更改代码
typedef struct _FrqunceData
{//定义一个频段的PCO和PCV数据格式
	QString FrqFlag;//START OF FREQUENCY
	double PCO[3];
	//PCV 两种数据
	short int Hang;//PCVAZI 行个数
	short int Lie;//PCVAZI和PCVNoAZI 列个数
	QVector< double > PCVNoAZI;
	QVector< double > PCVAZI;//矩阵转换为1维向量
}FrqData;

typedef struct _AntDataType
{
	QString StrAntType;//天线类型
	QString SatliCNN;//CNN 卫星编号 G,R,E...
	double DAZI;//存储方位角间隔 360/DAZI
	double ZEN_12N[3];//存储高度角区间及间隔  (ZEN_12N[1] - ZEN_12N[0])/ZEN_12N[2]
	short NumFrqu;//卫星的频段个数
	double ValidJD;//有效起始时间
	double EndJD;//有效结束时间
	QVector <FrqData> PCOPCV;//存储多个频段的PCO和PCV数据
	bool IsSat;//true 代表卫星 false 代表接收机数据
	bool isNan;//若搜索所有天线文件未找到，标记false;//初始化假设可以找到天线为true
}AntDataType;


class QReadAnt:public QBaseObject
{
//函数部分
public:
	QReadAnt(QString AntFileName = "",QString AnTypeName = "",double ObvJD = 0);
	~QReadAnt(void);
	bool getRecvL12(double E,double A,char SatType,double &L1Offset,double &L2Offset);//E:卫星高度角;A:卫星方位角（单位弧度），L1Offset和L2Offset两个波长改正（单位m）
	double getSatOffSet(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,int PRN,char SatType,double *StaPos,double *RecPos);//时间是卫星发射时刻UTC年月日时分秒，StaPos:卫星XYZ坐标，RecPos:接收机XYZ坐标，SatAntH:卫星天线改正 (由于各个卫星天线频率的PCO、PCV相同所以只需返回一个改正距离即可)返回m
	void setObsJD(QString AnTypeName,double ObsJD);//必须设置观测文件儒略日
	bool getAllData();//读取有效的卫星天线数据和接收机天线数据
private:
	void initVar();//初始化变量
	bool openFiles(QString AntTypeName);//打开文件
	bool readFileAntHead();//读取头文件
	bool readAntFile(QString AntTypeName);//读取天线文件
	bool readSatliData();//读取卫星需要用的数据
	bool readRecvData();//读取卫星需要的文件
	double computeJD(int Year,int Month,int Day,int HoursInt=0,int Minutes=0,int Seconds=0);//计算儒略日
	bool getPCV(const AntDataType &tempAntData,char SatType,double *PCV12,double Ztop,double AZI = 0);//计算PCV;Z:卫星天定角 = pi/2 - 高度角;A:卫星方位角（单位弧度）
//变量部分
public:
	double m_sunpos[3],m_moonpos[3],m_gmst;//每个历元更新一次为其他程序提供坐标，减少重复计算
private:
	QString m_AntFileName;//保存天线数据文件名字
	QString m_AntType;//保存接收机天线类型名字
	QFile m_ReadFileClass;//读取文件类
	bool isReadAllData;//是否读取所有数据标志
	bool isReadHead;//是否读取头文件
	bool isReadSatData;//是否读取卫星天线数据
	bool isReadRecvData;//是否读取接收机天线数据
	QString m_tempLine;//缓存读取的一行数据
	double m_ObeservTime;//存储观测O文件的儒略日 匹配卫星天线有效时间
	AntDataType m_RecvData;//保存接收机天线PCO和PCV数据
	QVector< AntDataType > m_SatData;//保存多个卫星天线PCO和PCV数据
	double m_pi;//圆周率 弧度
	QCmpGPST m_CmpClass;//计算函数库
	double m_sunSecFlag;//如果参考历元变化就需要重新计算太阳坐标存储参考历元的秒
};

#endif