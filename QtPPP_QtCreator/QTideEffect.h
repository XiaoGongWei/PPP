#ifndef QTIDEEFFECT_H
#define QTIDEEFFECT_H

#include "QCmpGPST.h"
/*
目前拥有的潮汐改正为：
1、极潮：需要下载erp文件，使用RTKLIB C语言标准库读取数据erp文件
2、固体潮：考虑2、3阶潮，频域修正，以及默认不考虑永久变形
3、海洋潮汐：要设置“测站”的名字4个字符，或者使用getAllTideEffectENU和getAllTideEffect函数传入测站名字

4、示例：
（1）类初始化：
QString OCEANFileName = "",QString erpFileName = "";//没有可以为空，程序会在当前执行目录下面自动搜索
QString StationName = "BJFS";//定义测站名字 搜索海洋数据（4个字节）

QTideEffect tideEffect(OCEANFileName,erpFileName);
tideEffect.setStationName(StationName);//想用海洋潮汐 必须设置测站名字
tideEffect.getAllData();//选择读取数据

（2）函数调用方法：
//计算所有潮汐改正
//pXYZ：测站坐标 EA:高度角和方位角，psunpos太阳坐标 pmoonpos：月亮坐标（传入加速计算，没有可以不传）gmst：平格里尼治时间
double pXYZ[3]={99999,999999,99999},EA[2] = {0.9,0.9},psunpos[3] = {99999,9999,9999},pmoonpos[3] = {99999,9999,9999},gmst = 0;
QString StationName = "BJFS";//定义测站名字 搜索海洋数据（4个字节）
doubel result = 0;//潮汐对视线方向的影响距离（单位m）
result = tideEffect.getAllTideEffect(Year,Month,Day,Hours,Minutes,Seconds,pXYZ,EA,psunpos,pmoonpos,gmst,StationName);

double pENU[3] = {0};//保存潮汐对ENU方向的影响
tideEffect.getAllTideEffectENU(Year,Month,Day,Hours,Minutes,Seconds,pXYZ,pENU,psunpos,pmoonpos,gmst,StationName);



备注：使用getAllTideEffect函数传入太阳、月亮、gmst数值可以不用重复计算太阳月亮坐标（也可以实时使用setSunMoonPos函数），可以减少计算量。
	  这些数值可以从QReadAnt类获取，先通过计算卫星天线改正就可以的到以上数据。
*/

#include <QFile>

typedef struct _OCEANData
{
	QString StationName;//大写站名
	double amp[3][11];
	double phasedats[3][11];
	bool isRead;//判断读取数据是否正确，可能读取失败，true正确，flase失败
}OCEANData;

//潮汐改正类
class QTideEffect
{
//函数部分
public:
	QTideEffect(QString OCEANFileName = "",QString erpFileName = "");//传入海洋数据,erp文件路径，否则就从当前目录搜索 否则就不适用海洋潮汐和极潮改正
	~QTideEffect(void);
	//获取视线方向 转换到距离改正
	void getAllTideEffectENU(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,double *pXYZ,double *pENU,double *psunpos=NULL,
		double *pmoonpos = NULL,double gmst = 0,QString StationName = "");//获得所有的海潮影响对ENU方向的影响
	//获取ENU方向的改正
	double getAllTideEffect(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,double *pXYZ,double *EA,double *psunpos=NULL,
		double *pmoonpos = NULL,double gmst = 0,QString StationName = "");//获得所有的海潮对视线方向的影响pXYZ:测站WGS84坐标系，EA卫星视线的高度角和方位角(弧度)，二维数组EA[2]
	//单独获取极潮、固体潮、海洋潮ENU改正
	void getPoleTide(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,double *pBLH,double *pTideENU);//输入观测时刻年月日时分秒，pXYZ：I为测站WGS84坐标,输出测站XYZ改正*pTideXYZ：O改正
	void getSoildTide(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,double *pXYZ,double *pTideENU,bool isElimate = false);//输入观测时刻年月日时分秒，pXYZ：I为测站WGS84坐标,输出测站XYZ改正*pTideXYZ：O改正 isElimate:是否考虑永久变形，永久变形模型貌似不精确，无大的影响，可选
	void getOCEANTide(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,double *pXYZ,double *pTideENU,QString StationName = "");//计算海潮影响 StationName:测站名字（例如：bjfs或BJFS）, StationName 为空就去当前目录搜索，否则不使用海潮改正
	//读取数据之前需要设置测站名字
	void setStationName(QString StationName = "");//初始化之前“必须”设置测站名字
	void getAllData();
	//读取海洋文件必须要测站名字，如果没有文件名字当前目录搜索变量.blq文件
	bool readOCEANFile(QString  StationName,OCEANData &oceaData,QString  OCEANFileName = "");// StationName:测站名字（例如：bjfs或BJFS），使用QFile读取（只能读取IGS站的数据 其他任意一点不可以读取，也可以把接收机名字替换为临近测站）OCEANFileName为空就使用类初始化文件，否则就搜索当前目录.dat文件，否则不使用海洋潮汐
	void setSunMoonPos(double *psun,double *pmoon,double gmst=0);//设置-更新坐标（和getAllTideEffectENU传入日月坐标一样加速计算）
private:
	void initVar();
	bool readRepFile();//存在erp文件就读取
	bool getErpV(gtime_t obsGPST,double *erpV);//读取erp文件数据到私有变量
	void tide_pole(const double *pos, const double *erpv, double *denu);
	void subSolidTide(double *sunmoonPos,double *pXYZ,double *pTideXYZ,int flag);//flag:0代表计算月亮，1代表太阳
//变量部分
public:

private:
	QString m_erpFileName;
	erp_t m_erpData;
	QCmpGPST m_cmpClass;//需要用QCmpGPST类计算计算函数
	bool isOCEANTide;
	bool isPoleEffect;//是否使用极潮 默认true
	bool isSolidTide;//是否使用固体潮 默认true
	//固体潮需要参数
	double m_GMi[3];//分别存储了地球、月亮、太阳引力常数
	double loveShida2[2];//二次固体潮love和Shadi参数初始化参数
	bool isgetLoveShida2;//是否得到二次固体潮love和Shadi参数
	double loveShida3[2];//三次固体潮love和Shadi参数
	double m_SationBLH[3];//测站BLH
	double m_SecondFlag;//保存历元秒，防止多次计算，减少计算量
	double m_AllTideENU[3];//保存本历元ENU，防止多次计算，减少计算量
	int m_epochFlag;//存储历元计算次数,假设50个历元后测站坐标相对精确，不在重复计算和测站坐标有关的极潮等
	int m_MaxCompution;//epochFlag 最大次数 默认50次在构造函数
	double m_pSolidENU[3];//存储计算的固体潮
	double m_pPoleENU[3];//存储计算的极潮
	double m_pOCEANENU[3];//存储计算的海潮
	double m_erpV[5];//存储第一个历元的极移参数
	bool isReadErp;//判断是否读取ERP文件(只读一次,即使失败也是true，不在读取，对应的潮汐会关闭，没有影响)
	bool isReadOCEAN;//判断是否读取了OCEAN数据(只读一次,即使失败也是true，不在读取，对应的潮汐会关闭，没有影响)
	//读取海洋数据类
	QString m_OCEANFileName;//海洋数据文件名字
	QFile m_readOCEANClass;
	OCEANData m_OCEANData;//保存测站数据
	QString m_StationName;
	static const double args[][5];
	double LeapSeconds;//当前年份的跳秒，用于年月日转到UTC
	//保存太阳、月亮、gmst数值可以不用重复计算太阳月亮坐标
	double m_sunpos[3];
	double m_moonpos[3];
	double m_gmst;
	bool isGetPos;//判断是否获取以上数据
};

#endif