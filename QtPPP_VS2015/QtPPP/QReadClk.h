#ifndef QREADCLK_H
#define QREADCLK_H


#include "QGlobalDef.h"

/*

1、说明：采用拉格朗日插值获取卫星钟误差，30s钟差自动采用2两点插值，其他时间间隔用8点插值
如果无法获取完整数据，钟误差为0

2、示例如下:
(1)类的初始化
QReadClk readClk("D:\\igs15786.clk");//必须指定路径

(2)在调用类之前需要读取数据一次，在想要读取数据调用getAllData();耗时较长
readClk.getAllData();

(3)然后调用getStaliteClk(int PRN,double GPST,double *pCLKT);
double GPST = 518400,SatClock = 0;//定义GPS周内秒GPST,以及要保存的钟差
readClk.getStaliteClk(32,GPST,&SatClock);//获得卫星钟差延迟保存到SatClock

3、注意：
(1)本类可以读取 RINEX VERSION / TYPE：3.00 / C (当前只读取GPS卫星 如需修改可以自己简单修改readFileData2Vec和readAllData函数)                                      
(2)调用getStaliteClk建议按照GPS时间（GPST）由小到大 调用才能“提升运行速度”，否则不明显。如果不想按照时间大小调用请 将ACCELERATE 改为0


*/

#define  ACCELERATE  1 // 1：加速程序（时间由小到大） 0：不加速程序（时间乱序）

typedef struct _CLKData
{//Clk文件的数据格式
	int GPSWeek;
	double GPSTime;//GPS周内秒
	MatrixXd MatrixDataGPS;//GPS系统第i列分别存放PRN X,Y,Z
	MatrixXd MatrixDataGlonass;//Glonass
	MatrixXd MatrixDataBDS;//BDS
	MatrixXd MatrixDataGalieo;//Galieo
}CLKData;

class QReadClk:public QBaseObject
{
//函数部分
public:
	QReadClk();
	QReadClk(QString ClkFileName);
	QReadClk(QStringList ClkFileNames);
	~QReadClk(void);
	QVector< CLKData > getAllData();//获得所有数据
	bool setSatlitSys(QString SystemStr);//其中GPS,GLONASS,BDS,Galieo分别使用：字母G,R,C,E（覆盖父类）
	void releaseAllData();//用完及时释放内存
	void getStaliteClk(int PRN,char SatType,double GPST,double *pCLKT);//获取卫星钟误差 GPST 卫星发射时刻周内秒
private:
	void initVar();//初始化变量
	void InitStruct();//初始化需要用的结构体内部的矩阵(有节约内存作用)
	void openFiles(QString ClkFileName);//只读方式打开文件
	void readAllHead();//读取头文件信息
	void readAllData();//读取单个文件卫星钟误差
	void readFileData2Vec(QStringList ClkFileNames);// 读取多个文件数据 到m_allEpochData
	void get8Point(int PRN,char SatType,double *pCLKT,double *pGPST,double GPST);//pCLKT：8个点坐标；pGPST:8个点GPS周内秒;GPST卫星发射时刻周内秒
	void lagrangeMethod(int PRN,char SatType,double GPST,double *pCLKT);//七阶拉格朗日插值 此处选取前后---共8个点做插值 GPST卫星发射时刻周内秒
	double YMD2GPSTime(int Year,int Month,int Day,int Hours,int Minutes,double Seconds,int *GPSWeek = NULL);//YMD Change to GPST //GPSTimeArray[4]=GPSWeek  GPS_N（周内第几天） GPST_second JD
//变量部分
public:

private:
	QFile m_readCLKFileClass;//读取.clk文件类
	QVector< CLKData > m_allEpochData;
	CLKData epochData;
	QString m_ClkFileName;
	QStringList m_ClkFileNames;
	QString tempLine;
	int m_WeekOrder;//保存跨周文件数目
	bool IsSigalFile;//是否多个文件或单个文件
	bool isReadHead;//是否读取头文件
	bool isReadAllData;//是否读取整个文件数据
	int lagrangeFact;//判断clk文件是30s还是5min 30s就可以简单差值lagrangeFact = 2;5min lagrangeFact = 8
//用于优化查找钟差的变量 防止一直从0开始查找
	int m_lastGPSTimeFlag;//保存上次get8Point获取的第一个GPS时间数据
	double m_lastCmpGPSTime;//保存上次计算的GPST防止一个历元
	int m_EndHeadNum;//第一个和最后一个钟差文件读取块数
	MatrixXd *pSysChMat;//选择系统指针
	
};

#endif

