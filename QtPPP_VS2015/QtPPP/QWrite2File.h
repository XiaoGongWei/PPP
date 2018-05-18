#ifndef QWRITE2FILE_H
#define QWRITE2FILE_H



#include "QCmpGPST.h"


class QWrite2File
{
//函数部分
public:
	QWrite2File(void);
	~QWrite2File(void);
	bool writeRecivePos2Txt(QString fileName);//计算ENU方向的结果写到txt
	bool writePPP2Txt(QString fileName);//各项误差改正写到.ppp文件
	bool writeClockZTDW2Txt(QString fileName);//将天顶湿延迟和钟差写到txt 第一列湿延迟第二列钟差
	bool writeAmbiguity2Txt();//将变量allAmbiguity存储的卫星写入txt文件名字命名为"G32.txt","C02.txt","R08.txt"等形式，第一列模糊度
	bool WriteEpochPRN(QString fileName);//将变量allAmbiguity存储的卫星写入到文件，第一列历元数，第二列卫星编号
private:
	bool WriteAmbPRN(int PRN,char SatType);//写入PRN号卫星的模糊度
//变量部分
public:
	QVector< RecivePos > allReciverPos;//保存ENU方向的结果写到txt
	QVector< QVector < SatlitData > > allPPPSatlitData;//保存ppp计算的数据写到.ppp文件
	QVector< Ambiguity> allAmbiguity;//存储卫星对应的模糊度
	QVector< ClockData > allClock;//存储一个
private:
	QCmpGPST m_qcmpClass;//函数计算库类

};

#endif

