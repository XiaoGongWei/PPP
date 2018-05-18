#ifndef QBASEOBJECT_H
#define QBASEOBJECT_H

#include <QString>
#include <QDebug>
/*
1、说明：
QString tempLine = "G";
char tempSatType = '0';
tempSatType = *(tempLine.mid(0,1).toLatin1().data());//类型QString 转换到 char

*/


class QBaseObject
{
//函数部分
public:
	QBaseObject(void);
	~QBaseObject(void);
	//设置文件系统 SystemStr:"G"(开启GPS系统);"GR":(开启GPS+GLONASS系统);"GRCE"(全部开启)等
	bool setSatlitSys(QString SystemStr);//其中GPS,GLONASS,BDS,Galieo分别使用：字母G,R,C,E
	bool isInSystem(char Sys);//Sys:G R C(代表GPS，GLONASS，BDS系统)判断是否需要该系统数据
	int getSystemnum();//获取当前面设定了几个系统
private:
	void inintVar();//初始化数据(默认开启GPS)
	int m_SystemNum;
//数据部分
protected:
	//加入系统判断
	bool IsaddGPS;//是否加入GPS(默认开启)
	bool IsaddGLOSS;//是否加入GLONASS
	bool IsaddBDS;//是否加入北斗
	bool IsaddGalieo;//是否加入Galieo
private:
};

#endif
