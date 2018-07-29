#include "QBaseObject.h"


//初始化数据
void QBaseObject::inintVar()
{
	IsaddGPS = false;
	IsaddGLOSS = false;
	IsaddBDS = false;
	IsaddGalieo = false;
	m_SystemNum = 0;
}

//
QBaseObject::QBaseObject(void)
{
	inintVar();
}

//
QBaseObject::~QBaseObject(void)
{
}

//设置文件系统 SystemStr:"G"(开启GPS系统);"GR":(开启GPS+GLONASS系统);"GRCE"(全部开启)等
//其中GPS,GLONASS,BDS,Galieo分别使用：字母G,R,C,E
bool QBaseObject::setSatlitSys(QString SystemStr)
{
	if (!(SystemStr.contains("G")||SystemStr.contains("R")||SystemStr.contains("C")||SystemStr.contains("E")))
		return	false;
	//
	if (SystemStr.contains("G"))
	{
		IsaddGPS = true;
		m_SystemNum++;
	}
	else
		IsaddGPS = false;
	//
	if (SystemStr.contains("R"))
	{
		IsaddGLOSS = true;
		m_SystemNum++;
	}
	else
		IsaddGLOSS = false;
	//
	if (SystemStr.contains("C"))
	{
		IsaddBDS = true;
		m_SystemNum++;
	}
	else
		IsaddBDS = false;
	//
	if (SystemStr.contains("E"))
	{
		IsaddGalieo = true;
		m_SystemNum++;
	}
	else
		IsaddGalieo = false;
	return true;
}


//Sys = G R C E(分别代表GPS，GLONASS，BDS，Galieo系统)判断是否需要该系统数据
bool QBaseObject::isInSystem(char Sys)
{
	if (IsaddGPS&&Sys == 'G')
		return true;
	else if (IsaddGLOSS&&Sys == 'R')
		return true;
	else if (IsaddBDS&&Sys == 'C')
		return true;
	else if (IsaddGalieo&&Sys == 'E')
		return true;
	return false;
}

//获取当前面设定了几个系统
int QBaseObject::getSystemnum()
{
	return m_SystemNum;
}
