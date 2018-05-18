#include "QPPPModel.h"
#include <QDir.h>
#include <QDebug>
#include <QTime>

int main(int argc, char *argv[])
{
//ppp定位
	QString m_strdir = "F:\\TempCmpData\\CUT0_PPP\\";
	//QString m_strdir = "C:\\Users\\Administrator\\Desktop\\123\\";
	//QString m_strdir = "F:\\LastTermGnss\\XiaoGongwei\\TestData\\PPP\\";
	//QString m_strdir = "F:\\LastTermGnss\\XiaoGongwei\\TestData\\P001168\\";
	//QString m_strdir = "F:\\LastTermGnss\\XiaoGongwei\\TestData\\Rinex3\\";
	//QString m_strdir = "F:\\LastTermGnss\\XiaoGongwei\\TestData\\Rinex4\\";
	QDir m_dir(m_strdir);
	QStringList m_fliterList;

//发现路径下的文件
	//找到所有SP3文件
	m_fliterList.clear();
	m_fliterList.append("*.sp3");
	QStringList Sp3FileNameList = m_dir.entryList(m_fliterList);
	//找到所有clk文件
	m_fliterList.clear();
	m_fliterList.append("*.clk");
	QStringList ClkFileNameList = m_dir.entryList(m_fliterList); 
	QString ClkfileName = "";
	if (!ClkFileNameList.isEmpty())
		ClkfileName = m_strdir +  ClkFileNameList.at(0);
	//找到所有o文件
	m_fliterList.clear();
	m_fliterList.append("*.14o");//需要改年份！！！！！！！！！！！！！！！！！
	QStringList OfileNamesList = m_dir.entryList(m_fliterList);
	//找到所有grd（对流层GPT2需要的插值文件）
	m_fliterList.clear();
	m_fliterList.append("*.grd");
	QStringList GrdfileNamesList = m_dir.entryList(m_fliterList);
	//找到所有.atx（卫星天线和接收机天线文件）
	m_fliterList.clear();
	m_fliterList.append("*.atx");
	QStringList AtxfileNamesList = m_dir.entryList(m_fliterList);
	//找到所有.blq文件（海洋数据）
	m_fliterList.clear();
	m_fliterList.append("*.blq");
	QStringList BlqfileNamesList = m_dir.entryList(m_fliterList);
	//找到所有.erp文件（极潮数据）
	m_fliterList.clear();
	m_fliterList.append("*.erp");
	QStringList ErpfileNamesList = m_dir.entryList(m_fliterList);

//拼接路径和文件名字
	QString OfileName = m_strdir + OfileNamesList.at(0);
	for (int i = 0;i < Sp3FileNameList.length();i++)
	{//Sp3路径
		Sp3FileNameList[i] = m_strdir + Sp3FileNameList[i];
	}
	for (int i = 0;i < ClkFileNameList.length();i++)
	{//Clk路径
		ClkFileNameList[i] = m_strdir + ClkFileNameList[i];
	}
	
	//erp文件
	QString ErpfileName = "";
	if (!ErpfileNamesList.isEmpty())
		ErpfileName = m_strdir + ErpfileNamesList.at(0);
	//grd文件
	QString GrdfileName = "";
	if (!GrdfileNamesList.isEmpty())
		GrdfileName = m_strdir + GrdfileNamesList.at(0);
	//blq文件
	QString BlqfileName = "";
	if (!BlqfileNamesList.isEmpty())
		BlqfileName = m_strdir + BlqfileNamesList.at(0);
	//atx文件
	QString AtxfileName = "";
	if (!AtxfileNamesList.isEmpty())
		AtxfileName = m_strdir + AtxfileNamesList.at(0);


	//测试.ppp文件（葛玉龙和王老师的.ppp）
	//QPPPModel myppp;
	//myppp.Run("C:\\Users\\Administrator\\Desktop\\BJFS_Result\\bjfs1000_Ge.PPP");

	//根据精密星历计算卫星定位
	
	//QPPPModel m_QpppModel(OfileName,Sp3FileNameList,ClkFileNameList,ErpfileName);
	//m_QpppModel.Run("C:\\Users\\Administrator\\Desktop\\Cut01220\\cut01220.PPP");
	//m_QpppModel.Run();


	
	//测试内存是否泄漏
	for (int i = 0;i < 1;i++)
	{
		QTime myTime;
		myTime.start();//开始计时
		QPPPModel *p = new QPPPModel(OfileName, Sp3FileNameList, ClkFileNameList, ErpfileName);
		p->Run();
		float m_diffTime = myTime.elapsed() / 1000.0;
		qDebug() << "The Elapse Time: " << QString::number(m_diffTime) << "s";
		delete p;
	}
	

	
	return 0;
}
