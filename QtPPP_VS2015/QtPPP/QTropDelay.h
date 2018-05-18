#ifndef QTROPDELAY_H
#define QTROPDELAY_H

#include "QGlobalDef.h"

/*
1、说明：
本类包含了GPT2 UNB3M模型 以及获取GPT2和GPT参数的方法；
对流层模型：Sass模型、Hofield模型、
对流层投影函数模型：Neill，VMF1,GMF
#ifdef EIGEN_CORE_H (Eigen库的标志，没有Eigen自动屏蔽UNB3M模型)
.........
#endif

2、示例：
（1）类的初始化：
QTropDelay tropDelay;//不使用GPT2相关的模型（只是用UNB3M模型）
QTropDelay tropDelay("D:\\gpt2_5.grd","Neil","GPT2"),使用GPT2相关的模型必须有gpt2_5.grd文件路径，后面两个可以不选默认Neil和UNB3M

使用GPT2以及与之相关的Sass、Hofild模型之前需要读取所有数据
tropDelay.getAllData();

（2）GPT2+Sass模型（默认投影函数Neil初始化时候可以设置VMF1投影）：
double MJD = 852221.0,E = 0.72,pBLH[3] ={0.3,1.5,88.3};//约化儒略日,高度角（rad）,BLH的B经度（rad），BLH的L纬度（rad），BLH的H（m）
int TDay = 132;//年积日
double TropDelay = 0,mf = 0;//对流层干延迟（m），投影函数（m）
TropDelay = tropDelay.getGPT2SasstaMDelay(MJD,TDay,E,pBLH,&mf);//GPT2+Sass计算结果到TropDelay和mf保存

（3）UNB3M模型，投影函数Neil：
int TDay = 132;//年积日
double pBLH[3] ={0.3,1.5,88.3},E = 0.72;//BLH（rad,rad,m）和 卫星高度角E（rad）
double TropDelay = 0,mf = 0;//对流层干延迟（m），投影函数（m）
TropDelay = tropDelay.getUNB3mDelay(pBLH, TDay, E,&mf);

（4）GPT参数获取：
double MJD = 852221.0,Lat = 0.3,Lon = 1.5,Hell = 88.3;//MJD约化儒略日,Lat是BLH的B经度（rad），Lon是BLH的L纬度（rad），Hell是BLH的H（m）
GPT2Result tempGPT2;//计算结果
tempGPT2 = tropDelay.getGPT2Model(MJD,Lat,Lon,Hell);//计算GPT2 到GPT2Result结构体保存

double GPTresult[3] = {0};//存储了 气压（hPa） 温度（C） 椭球高差值（Geoid undulation）（m）(前两个有用)
tropDelay.getGPTModel(MJD,Lat,Lon,Hell，GPTresult);//计算GPT到 GPTresult变量保存
（5）对流层投影函数
double MJD = 852221.0,pBLH[3] ={0.3,1.5,88.3},E = 0.72;
double md = 0,mw = 0;//干量(md)和湿量(mw)投影函数
tropDelay.getGMFParm(MJD,pBLH,E,&md,&mw);//计算GMF投影函数到md和mw

GPT2Result tempGPT2;//计算结果
int TDay = 132;//年积日
tempGPT2 = tropDelay.getGPT2Model(MJD,pBLH[0],pBLH[1],pBLH[2]);
getVMF1Parm(tempGPT2.ah,tempGPT2.aw,E,pBLH[0],pBLH[2],TDay,&md,&mw);//计算VMF1投影函数到md和mw

tropDelay.getNeilParm(E,pBLH[2],pBLH[0],TDay,&md,&mw);//计算Neil投影函数到md和mw

3、注意：本类返回的对流层延迟只是干延迟（如需更该参考对应函数的“尾部”带有感叹号注释地方进行简单更改）
4、参考网站：http://ggosatm.hg.tuwien.ac.at/DELAY/SOURCE/ （包含matlab以及数据）
*/

#define d2r  (Pi/180.0)
#define r2d  (180.0/Pi)

#define MIN(x,y)    ((x)<(y)?(x):(y))
#define MAX(x,y)    ((x)>(y)?(x):(y))
#define SQRT(x)     ((x)<=0.0?0.0:sqrt(x))

typedef struct _GPT2Result
{//保存GPT2的结果
	double p;//pressure in hPa
	double T;//temperature in degrees
	double dT;//temperature lapse rate in degrees per km
	double e;// water vapour pressure in hPa
	double ah;//hydrostatic mapping function coefficient at zero height (VMF1)
	double aw;//wet mapping function coefficient (VMF1)
	double undu;//geoid undulation in m
}GPT2Result;

typedef struct _GrdFileVar
{//存储GRD文件数据用于计算
	double lat;//latitude
	double lon;//lontitude
	double pgrid[5];//pressure in Pascal
	double Tgrid[5];// temperature in Kelvin
	double Qgrid[5];//specific humidity in kg/kg
	double dTgrid[5];//temperature lapse rate in Kelvin/m
	double u;//geoid undulation in m
	double Hs;//orthometric grid height in m
	double ahgrid[5];//hydrostatic mapping function coefficient, dimensionless
	double awgrid[5];// wet mapping function coefficient, dimensionless
}GrdFileVar;

class QTropDelay
{
//函数部分
public:
	QTropDelay(QString GrdFileName = "",QString ProjectionFun = "Neil",QString Model = "UNB3M");//读取GPT2模型的数据文件 GrdFileName，对流层函数ProjectionFun = "VMF1" 或 "Neil";Model可以选择GPT 或者 GPT2模型（需要读取.grd文件数据）读取失败自动选择GPT估计
	~QTropDelay(void);
	//下面两个函数是GPT2估计的对流层模型
	double getGPT2HopfieldDelay(double MJD,int TDay,double E,double *pBLH,double *mf = NULL);//获取GPT2+Hopfield+VMF1的天顶ZHD+ZWD总延迟 MJD:简化儒略日 T年积日 Lat,lon,Hell：经纬度，测站高 T:年积日 E高度角（rad）,*mf:投影函数
	double getGPT2SasstaMDelay(double MJD,int TDay,double E,double *pBLH,double *mf = NULL);//获取GPT2+Hopfield+VMF1的天顶ZHD+ZWD总延迟 MJD:简化儒略日 T年积日 Lat,lon,Hell：经纬度，测站高 T:年积日 E高度角（rad）,*mf:投影函数
	//下面函数是UNB3估计的对流层模型
#ifdef EIGEN_CORE_H
	double getUNB3mDelay(double *pBLH, double TDay, double E,double *mf = NULL);
#endif
	//下面是获得GPT2、GPT参数方法 
	GPT2Result getGPT2Model(double dmjd,double dlat,double dlon,double hell,double it = 0);//使用GPT2计算出 详细输入见函数体 it：1表示无时间变化 0：有时间变化
	void getGPTModel(double dmjd,double dlat,double dlon,double hell,double *GPTdata);//使用GPT计算出 GPTdata[3]存储了 气压（hPa） 温度（C） 椭球高差值（Geoid undulation）（m）
	//下面是获取投影函数方法
	void getVMF1Parm(double ah,double aw,double E,double Lat,double H,int TDay,double &md,double &mw);//计算VMF1干量(md)和湿量(mw)投影函数 E为高度角（rad）H为正高（m）T为年积日 Lat为大地纬度(rad 北纬大于零 南纬小于零) aw bw来自GPT2 model估计
	void getNeilParm(double E,double H,double Lat,int TDay,double &md,double &mw);//计算Neil干量(md)和湿量(mw)投影函数 E为高度角（rad）H为正高（m）ph为大地纬度(rad) T年积日 
	void getGMFParm(double MJD,double *pBLH,double E,double &md,double &mw);//MJD：约化儒略日，pBLH大地坐标系(rad,rad,m), E为高度角（rad） 干量(md)和湿量(mw)投影函数 
	//经验的Sasssta模型(天顶方向)
	double getSassDelay(double &ZHD,double &ZWD,double B, double H,double E);
	//经验的Hopfield模型(视线方向)
	double getHopfieldDelay(double &SD,double &SW, double H,double E);//采用经验模型计算视线方向（H：测站的BLH，单位m；E：卫星高度角，单位（弧度））
	void getAllData();//读取所有数据 只有读取了数据才可以计算（可以在程序初始化的时候读取）
private:
	void initVar();
	bool openGrdFile(QString GrdFileName);
	void readGrdFile(QString grdFileName);//读取GRD文件数据用于计算
	GPT2Result HopfieldDelay(double &ZHD,double &ZWD,double dmjd,double dlat,double dlon,double hell,double it = 0);//采用GPT2估计+Hopfield（天顶方向）  it：1表示无时间变化 0：有时间变化
	GPT2Result SassstaMDelay(double &ZHD,double &ZWD,double dmjd,double dlat,double dlon,double hell,double it = 0);//采用GPT2估计+简化的Saastamoinen模型（天顶方向）  it：1表示无时间变化 0：有时间变化
	void trop_map_gmf(double dmjd,double dlat,double dlon,double dhgt,double zd,double *gmfh,double *gmfw);
	void trop_gpt(double dmjd,double dlat,double dlon,double dhgt,double *pres,double *temp,double *undu);
	int ipow(int base,int exp);//被trop_gpt调用
#ifdef EIGEN_CORE_H
	VectorXd UNB3M(Vector3d &BLH, double DAYOYEAR, double ELEVRAD);//UNB3 模型（依赖Eigen库）
#endif
//变量部分 
public:

private:
	//下面定义Neil模型参数
	//ad,bd,cd 15-75度的干分量平均值和波动幅度表
	double lat[5];
	double Avgad[5],Avgbd[5],Avgcd[5];
	double Ampad[5],Ampbd[5],Ampcd[5];
	//下面定义湿分量系数表
	double Avgaw[5],Avgbw[5],Avgcw[5];
	double m_PI;//圆周率
	QString m_GrdFileName;
	QFile m_ReadFileClass;
	QVector< GrdFileVar > m_allGrdFile;
	QString tempLine;
	bool isReadAllData;
	bool isGPT2;//判断是否是GPT2模型：1 否则选择GPT：0
	QString m_ProjectionFun;//选择使用的投影函数
	int m_ProjectFunFlag;//1:Neil 2:VMF1 3:GMF
};

#endif

