#include "QReadSP3.h"

const int QReadSP3::lagrangeFact = 8;//常量初始化


void QReadSP3::initVar()
{
	tempLine = "";
	isReadHead = false;
	isReadAllData = false;
	IsSigalFile = false;
	//只包含GPS系统最大共32颗卫星
	m_EndHeadNum = 8;
	m_lastGPSTimeFlag = 0;//作为起始标记
	m_lastCmpGPSTime = 999999999;//记录起始计算的GPS时间
	m_SP3FileName = "";
	m_WeekOrder = 0;
	InitStruct();

}

//初始化需要用的结构体内部的矩阵(有节约内存作用)
void QReadSP3::InitStruct()
{
	if (isInSystem('G'))
	{
		epochData.MatrixDataGPS.resize(32,4);
		epochData.MatrixDataGPS.setZero();
	}
	if (isInSystem('R'))
	{
		epochData.MatrixDataGlonass.resize(32,4);
		epochData.MatrixDataGlonass.setZero();
	}
	if (isInSystem('C'))
	{
		epochData.MatrixDataBDS.resize(32,4);
		epochData.MatrixDataBDS.setZero();
	}
	if (isInSystem('E'))
	{
		epochData.MatrixDataGalieo.resize(32,4);
		epochData.MatrixDataGalieo.setZero();
	}
}

QReadSP3::QReadSP3()
{
	initVar();
}

QReadSP3::QReadSP3(QString SP3FileName)
{
	initVar();
	openFiles(SP3FileName);
	IsSigalFile = true;
}

QReadSP3::QReadSP3(QStringList SP3FileNames)
{
	initVar();
	//debug:2017.07.08
	if (SP3FileNames.length() == 1)
	{
		QString SP3FileName = SP3FileNames.at(0);
		openFiles(SP3FileName);
		IsSigalFile = true;
	}
	if (SP3FileNames.length() > 0)
	{
		m_SP3FileNames =SP3FileNames;
	}
	else
	{
		isReadAllData = true;
		return ; 
	}
}

void QReadSP3::openFiles(QString SP3FileName)
{
	
	if (!SP3FileName.isEmpty())
	{
		m_readSP3FileClass.setFileName(SP3FileName);
		m_readSP3FileClass.open(QFile::ReadOnly);
		m_SP3FileName = SP3FileName;
	}
	else
	{
		isReadAllData = true;
		return;	
	}
		
}

QReadSP3::~QReadSP3(void)
{
	releaseAllData();
}

//转到GPS时间
int QReadSP3::YMD2GPSTime(int Year,int Month,int Day,int HoursInt,int Minutes,int Seconds,int *GPSWeek)//,int *GPSTimeArray
{
	double Hours = HoursInt + ((Minutes * 60) + Seconds)/3600.0;
	//Get JD
	double JD = 0.0;
	if(Month<=2)
		JD = (int)(365.25*(Year-1)) + (int)(30.6001*(Month+12+1)) + Day + Hours/24.0 + 1720981.5;
	else
		JD = (int)(365.25*(Year)) + (int)(30.6001*(Month+1)) + Day + Hours/24.0 + 1720981.5;
	
	int Week = (int)((JD - 2444244.5) / 7);
	int N =(int)(JD + 1.5)%7;
	if (GPSWeek) *GPSWeek = Week;
	return (N*24*3600 + HoursInt*3600 + Minutes*60 + Seconds);
}

//读取头文件
void QReadSP3::readHeadData()
{
	if (isReadHead)
		return ;
	tempLine = m_readSP3FileClass.readLine();//读取第一行
	QString flagHeadEnd = "*";
	QString pre3Char = tempLine.mid(0,1);
	while (pre3Char != flagHeadEnd)
	{
		tempLine = m_readSP3FileClass.readLine();//读取下一行
		pre3Char = tempLine.mid(0,1);
	}
	//结束时候tempLine 到达第一个历元
	isReadHead = true;//debug:2017.07.08
}

//读取单个文件数据
void QReadSP3::readAllData2Vec()
{
	if (isReadAllData)
		return;
	if (!isReadHead)
		readHeadData();
	m_allEpochData.clear();//清空原来数据
	//首次进入时候tempLine 到达第一个历元 * YYYY-MM.......
	int Year =0,Month = 0,Day = 0,Hours = 0,Minutes = 0,Week = 0;
	double Seconds = 0;
	while (!m_readSP3FileClass.atEnd())
	{
		//判断是否结束
		if (tempLine.mid(0,3) == "EOF")
			break;
		//读取头行历元数据
		epochData.MatrixDataGPS.setZero();//用之前最好清零虽然加大计算量但是安全
		epochData.MatrixDataGlonass.setZero();
		epochData.MatrixDataBDS.setZero();
		epochData.MatrixDataGalieo.setZero();
		Year = tempLine.mid(3,4).toInt();
		Month = tempLine.mid(8,2).toInt();
		Day = tempLine.mid(11,2).toInt();
		Hours = tempLine.mid(14,2).toInt();
		Minutes = tempLine.mid(17,2).toInt();
		Seconds = tempLine.mid(20,11).toDouble();
		epochData.GPSTime = YMD2GPSTime(Year,Month,Day,Hours,Minutes,Seconds,&Week) ;
		epochData.GPSWeek = Week;
		//读取卫星坐标//读取卫星坐标
		tempLine = m_readSP3FileClass.readLine();//读取一行坐标数据
		while (tempLine.mid(0,3) != "EOF"&&tempLine.mid(0,1) == "P")
		{
			int PRN = 0;
			char tempSatType = '0';
			//GPS系统
			tempSatType = *(tempLine.mid(1,1).toLatin1().data());
			PRN = tempLine.mid(2,2).toInt();
			if (tempSatType == 'G'&&isInSystem(tempSatType))
			{
				epochData.MatrixDataGPS(PRN - 1,0) = tempLine.mid(2,2).toDouble();//PRN
				epochData.MatrixDataGPS(PRN - 1,1) = 1000*tempLine.mid(5,13).toDouble();//X
				epochData.MatrixDataGPS(PRN - 1,2) = 1000*tempLine.mid(19,13).toDouble();//Y
				epochData.MatrixDataGPS(PRN - 1,3) = 1000*tempLine.mid(33,13).toDouble();//Z
			}
			//Glonass系统
			else if (tempSatType == 'R'&&isInSystem(tempSatType))
			{
				epochData.MatrixDataGlonass(PRN - 1,0) = tempLine.mid(2,2).toDouble();//PRN
				epochData.MatrixDataGlonass(PRN - 1,1) = 1000*tempLine.mid(5,13).toDouble();//X
				epochData.MatrixDataGlonass(PRN - 1,2) = 1000*tempLine.mid(19,13).toDouble();//Y
				epochData.MatrixDataGlonass(PRN - 1,3) = 1000*tempLine.mid(33,13).toDouble();//Z
			}
			//BDS系统
			else if (tempSatType == 'C'&&isInSystem(tempSatType))
			{
				epochData.MatrixDataBDS(PRN - 1,0) = tempLine.mid(2,2).toDouble();//PRN
				epochData.MatrixDataBDS(PRN - 1,1) = 1000*tempLine.mid(5,13).toDouble();//X
				epochData.MatrixDataBDS(PRN - 1,2) = 1000*tempLine.mid(19,13).toDouble();//Y
				epochData.MatrixDataBDS(PRN - 1,3) = 1000*tempLine.mid(33,13).toDouble();//Z
			}
			//Galieo系统
			else if (tempSatType == 'E'&&isInSystem(tempSatType))
			{
				epochData.MatrixDataGalieo(PRN - 1,0) = tempLine.mid(2,2).toDouble();//PRN
				epochData.MatrixDataGalieo(PRN - 1,1) = 1000*tempLine.mid(5,13).toDouble();//X
				epochData.MatrixDataGalieo(PRN - 1,2) = 1000*tempLine.mid(19,13).toDouble();//Y
				epochData.MatrixDataGalieo(PRN - 1,3) = 1000*tempLine.mid(33,13).toDouble();//Z
			}
			tempLine = m_readSP3FileClass.readLine();//读取一行坐标数据
		}
		m_allEpochData.append(epochData);//保存一个文件数据
	}
	isReadAllData = true;
	m_readSP3FileClass.close();
}

//读取多个文件数据(此处不必那么做，也可以直接调用类内部函数一个个文件读取，不想改代码啦。。。)
void QReadSP3::readFileData2Vec(QStringList SP3FileNames)
{
	if (SP3FileNames.length() == 0) isReadAllData = true;
	if (isReadAllData)
		return;
	m_allEpochData.clear();
	//首先读取头文件按照时间由小到大读取文件
	int minGPSWeek = 999999999;//保存最小周 减少int越界（周转换到秒时候）
	QVector< int > tempGPSWeek,tempGPSSeconds,fileFlagSeconds;//保存文件观测其实时间
	for (int i = 0;i < SP3FileNames.length();i++)
	{
		int Year =0,Month = 0,Day = 0,Hours = 0,Minutes = 0,GPSWeek = 0,GPSSeconds = 0;
		double Seconds = 0;
		QString Sp3FileName = SP3FileNames.at(i);
		QFile sp3file(Sp3FileName);
		if (!sp3file.open(QFile::ReadOnly))
		{
			QString erroInfo = "Open " + Sp3FileName + "faild!";
			ErroTrace(erroInfo);
		}
		//读取头文件
		tempLine = sp3file.readLine();//读取第一行
		Year = tempLine.mid(3,4).toInt();
		Month = tempLine.mid(8,2).toInt();
		Day = tempLine.mid(11,2).toInt();
		Hours = tempLine.mid(14,2).toInt();
		Minutes = tempLine.mid(17,2).toInt();
		Seconds = tempLine.mid(20,11).toDouble();
		GPSSeconds = YMD2GPSTime(Year,Month,Day,Hours,Minutes,Seconds,&GPSWeek);
		if (GPSWeek <= minGPSWeek)
			minGPSWeek = GPSWeek;
		tempGPSWeek.append(GPSWeek);
		tempGPSSeconds.append(GPSSeconds);
		sp3file.close();
	}
	//转化为秒
	QVector< int > WeekOrder;//保存数据 标记是否跨周，跨：1 不跨：0//读取多个文件需要用到
	for (int i = 0;i < tempGPSWeek.length();i++)
	{
		int Seconds = (tempGPSWeek.at(i) - minGPSWeek)*604800 + tempGPSSeconds.at(i);
		WeekOrder.append(tempGPSWeek.at(i) - minGPSWeek);
		fileFlagSeconds.append(Seconds);
	}
	//按照时间进行文件名字排序
	for (int i = 0; i < fileFlagSeconds.length();i++)
	{
		for (int j = i+1;j < fileFlagSeconds.length();j++)
		{
			if (fileFlagSeconds.at(j) < fileFlagSeconds.at(i))//交换
			{
				//交换文件名字
				QString tempFileName = SP3FileNames.at(i);
				SP3FileNames[i] = SP3FileNames.at(j);
				SP3FileNames[j] = tempFileName;
				//交换周标志
				int tempWeek = WeekOrder.at(i);
				WeekOrder[i] = WeekOrder.at(j);
				WeekOrder[j] = tempWeek;
			}
		}
	}
	m_WeekOrder = 0;
	for (int i = 0;i < WeekOrder.length();i++)
	{
		m_WeekOrder += WeekOrder[i];//保存跨周标记
	}
	//按照时间大小读取所有文件
	for (int i = 0;i < SP3FileNames.length();i++)
	{
		QString Sp3FileName = SP3FileNames.at(i);
		//打开文件
		QFile sp3file(Sp3FileName);
		if (!sp3file.open(QFile::ReadOnly))
		{
			QString erroInfo = "Open " + Sp3FileName + "faild!";
			ErroTrace(erroInfo);
		}
//读取头文件
		tempLine = sp3file.readLine();//读取第一行
		QString flagHeadEnd = "*";
		QString pre3Char = tempLine.mid(0,1);
		while (pre3Char != flagHeadEnd)
		{
			tempLine = sp3file.readLine();//读取下一行
			pre3Char = tempLine.mid(0,1);
		}

//读取数据部分
		//首次进入时候tempLine 到达第一个历元 * YYYY-MM.......
		int Year =0,Month = 0,Day = 0,Hours = 0,Minutes = 0,Week = 0;
		double Seconds = 0;
		while (!sp3file.atEnd())
		{
			//判断是否结束
			if (tempLine.mid(0,3) == "EOF")
				break;
			//读取头行历元数据
			epochData.MatrixDataGPS.setZero();//用之前最好清零虽然加大计算量但是安全
			epochData.MatrixDataGlonass.setZero();
			epochData.MatrixDataBDS.setZero();
			epochData.MatrixDataGalieo.setZero();
			Year = tempLine.mid(3,4).toInt();
			Month = tempLine.mid(8,2).toInt();
			Day = tempLine.mid(11,2).toInt();
			Hours = tempLine.mid(14,2).toInt();
			Minutes = tempLine.mid(17,2).toInt();
			Seconds = tempLine.mid(20,11).toDouble();
			epochData.GPSTime = YMD2GPSTime(Year,Month,Day,Hours,Minutes,Seconds,&Week) + WeekOrder.at(i)*604800;//跨周时间加上604800s
			epochData.GPSWeek = Week;
			//读取卫星坐标//读取卫星坐标
			tempLine = sp3file.readLine();//读取一行坐标数据
			while (tempLine.mid(0,3) != "EOF"&&tempLine.mid(0,1) == "P")
			{
				int PRN = 0;
				char tempSatType = '0';
				//GPS系统
				tempSatType = *(tempLine.mid(1,1).toLatin1().data());
				PRN = tempLine.mid(2,2).toInt();
				if (tempSatType == 'G'&&isInSystem(tempSatType))
				{
					epochData.MatrixDataGPS(PRN - 1,0) = tempLine.mid(2,2).toDouble();//PRN
					epochData.MatrixDataGPS(PRN - 1,1) = 1000*tempLine.mid(5,13).toDouble();//X
					epochData.MatrixDataGPS(PRN - 1,2) = 1000*tempLine.mid(19,13).toDouble();//Y
					epochData.MatrixDataGPS(PRN - 1,3) = 1000*tempLine.mid(33,13).toDouble();//Z
				}
				//Glonass系统
				else if (tempSatType == 'R'&&isInSystem(tempSatType))
				{
					epochData.MatrixDataGlonass(PRN - 1,0) = tempLine.mid(2,2).toDouble();//PRN
					epochData.MatrixDataGlonass(PRN - 1,1) = 1000*tempLine.mid(5,13).toDouble();//X
					epochData.MatrixDataGlonass(PRN - 1,2) = 1000*tempLine.mid(19,13).toDouble();//Y
					epochData.MatrixDataGlonass(PRN - 1,3) = 1000*tempLine.mid(33,13).toDouble();//Z
				}
				//BDS系统
				else if (tempSatType == 'C'&&isInSystem(tempSatType))
				{
					epochData.MatrixDataBDS(PRN - 1,0) = tempLine.mid(2,2).toDouble();//PRN
					epochData.MatrixDataBDS(PRN - 1,1) = 1000*tempLine.mid(5,13).toDouble();//X
					epochData.MatrixDataBDS(PRN - 1,2) = 1000*tempLine.mid(19,13).toDouble();//Y
					epochData.MatrixDataBDS(PRN - 1,3) = 1000*tempLine.mid(33,13).toDouble();//Z
				}
				//Galieo系统
				else if (tempSatType == 'E'&&isInSystem(tempSatType))
				{
					epochData.MatrixDataGalieo(PRN - 1,0) = tempLine.mid(2,2).toDouble();//PRN
					epochData.MatrixDataGalieo(PRN - 1,1) = 1000*tempLine.mid(5,13).toDouble();//X
					epochData.MatrixDataGalieo(PRN - 1,2) = 1000*tempLine.mid(19,13).toDouble();//Y
					epochData.MatrixDataGalieo(PRN - 1,3) = 1000*tempLine.mid(33,13).toDouble();//Z
				}
				tempLine = sp3file.readLine();//读取一行坐标数据
			}
			m_allEpochData.append(epochData);//保存一个文件数据
		}//读取文件结束
		sp3file.close();
	}//for (int i = 0;i < SP3FileNames.length();i++)//读取多个文件结束
	isReadAllData = true;
}

////pX,pY,pZ：lagrangeFact个点坐标；pGPST:lagrangeFact个点GPS周内秒;GPST卫星发射时刻周内秒
void QReadSP3::get8Point(int PRN,char SatType,double *pX,double *pY,double *pZ,int *pGPST,double GPST)
{//获取最邻近的lagrangeFact个点
	int lengthEpoch = m_allEpochData.length();
	//发现GPST在整个历元中的位置
	int GPSTflag = m_lastGPSTimeFlag;
	int numPoint = lagrangeFact / 2;//前后取numPoint 个点
	if (qAbs(m_lastCmpGPSTime - GPST) > 0.3)//大于0.3s说明不在一个历元假设观测历元间隔肯定在1s之上(减少相同历元多次查询位置)
	{
		if (ACCELERATE)	m_lastCmpGPSTime = GPST;
		for (int i = m_lastGPSTimeFlag;i < lengthEpoch;i++)
		{
			SP3Data epochData = m_allEpochData.at(i);
			if (epochData.GPSTime >= GPST)
				break; 
			else
				GPSTflag++;
		}
	}
	if (ACCELERATE) m_lastGPSTimeFlag = GPSTflag;//保存最新的位置

//前后取 numPoint 点 考虑边界问题
	if ((GPSTflag >= numPoint - 1) && (GPSTflag <= lengthEpoch - numPoint - 1))
	{//在中间位置前面四个包含本身 后面四个不包含本身
		for (int i = 0;i < lagrangeFact;i++)
		{
			SP3Data epochData = m_allEpochData.at(GPSTflag - numPoint + 1 + i);
			//判断属于那个系统数据
			switch(SatType)
			{
			case 'G':
				pX[i] = epochData.MatrixDataGPS(PRN - 1,1);
				pY[i] = epochData.MatrixDataGPS(PRN - 1,2);
				pZ[i] = epochData.MatrixDataGPS(PRN - 1,3);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'R':
				pX[i] = epochData.MatrixDataGlonass(PRN - 1,1);
				pY[i] = epochData.MatrixDataGlonass(PRN - 1,2);
				pZ[i] = epochData.MatrixDataGlonass(PRN - 1,3);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'C':
				pX[i] = epochData.MatrixDataBDS(PRN - 1,1);
				pY[i] = epochData.MatrixDataBDS(PRN - 1,2);
				pZ[i] = epochData.MatrixDataBDS(PRN - 1,3);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'E':
				pX[i] = epochData.MatrixDataGalieo(PRN - 1,1);
				pY[i] = epochData.MatrixDataGalieo(PRN - 1,2);
				pZ[i] = epochData.MatrixDataGalieo(PRN - 1,3);
				pGPST[i] = epochData.GPSTime;
				break;
			default:
				pX[i] = 0;
				pY[i] = 0;
				pZ[i] = 0;
				pGPST[i] = 0;
			}
		}
	}
	else if(GPSTflag < numPoint - 1)
	{//在开始位置边界
		for (int i = 0;i < lagrangeFact;i++)
		{
			SP3Data epochData = m_allEpochData.at(i);
			//判断属于那个系统数据
			switch(SatType)
			{
			case 'G':
				pX[i] = epochData.MatrixDataGPS(PRN - 1,1);
				pY[i] = epochData.MatrixDataGPS(PRN - 1,2);
				pZ[i] = epochData.MatrixDataGPS(PRN - 1,3);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'R':
				pX[i] = epochData.MatrixDataGlonass(PRN - 1,1);
				pY[i] = epochData.MatrixDataGlonass(PRN - 1,2);
				pZ[i] = epochData.MatrixDataGlonass(PRN - 1,3);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'C':
				pX[i] = epochData.MatrixDataBDS(PRN - 1,1);
				pY[i] = epochData.MatrixDataBDS(PRN - 1,2);
				pZ[i] = epochData.MatrixDataBDS(PRN - 1,3);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'E':
				pX[i] = epochData.MatrixDataGalieo(PRN - 1,1);
				pY[i] = epochData.MatrixDataGalieo(PRN - 1,2);
				pZ[i] = epochData.MatrixDataGalieo(PRN - 1,3);
				pGPST[i] = epochData.GPSTime;
				break;
			default:
				pX[i] = 0;
				pY[i] = 0;
				pZ[i] = 0;
				pGPST[i] = 0;
			}
		}
	}
	else if(GPSTflag > lengthEpoch - numPoint - 1)
	{//在结束位置边界
		for (int i = 0;i < lagrangeFact;i++)
		{
			//debug:2017.07.08
			SP3Data epochData = m_allEpochData.at(lengthEpoch - (lagrangeFact-i));
			//判断属于那个系统数据
			switch(SatType)
			{
			case 'G':
				pX[i] = epochData.MatrixDataGPS(PRN - 1,1);
				pY[i] = epochData.MatrixDataGPS(PRN - 1,2);
				pZ[i] = epochData.MatrixDataGPS(PRN - 1,3);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'R':
				pX[i] = epochData.MatrixDataGlonass(PRN - 1,1);
				pY[i] = epochData.MatrixDataGlonass(PRN - 1,2);
				pZ[i] = epochData.MatrixDataGlonass(PRN - 1,3);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'C':
				pX[i] = epochData.MatrixDataBDS(PRN - 1,1);
				pY[i] = epochData.MatrixDataBDS(PRN - 1,2);
				pZ[i] = epochData.MatrixDataBDS(PRN - 1,3);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'E':
				pX[i] = epochData.MatrixDataGalieo(PRN - 1,1);
				pY[i] = epochData.MatrixDataGalieo(PRN - 1,2);
				pZ[i] = epochData.MatrixDataGalieo(PRN - 1,3);
				pGPST[i] = epochData.GPSTime;
				break;
			default:
				pX[i] = 0;
				pY[i] = 0;
				pZ[i] = 0;
				pGPST[i] = 0;
			}
		}
	}
}

//拉格朗日方法
void QReadSP3::lagrangeMethod(int PRN,char SatType,double GPST,double *pXYZ,double *pdXYZ)
{//拉格朗日插值 此处选取前后---共8个点做插值 GPST卫星发射时刻周内秒
	for (int i = 0;i < 3;i++)
	{//初始化，安全
		pXYZ[i] = 0;
		pdXYZ[i] = 0;
	}
	//判断是否设置合法系统
	if (!isInSystem(SatType)) 
		return ;
	//判断是否读取数据
	if (!isReadAllData)
	{
		if (IsSigalFile)
			readAllData2Vec();
		else
			readFileData2Vec(m_SP3FileNames);
	}
	double pX[lagrangeFact]={0},pY[lagrangeFact] = {0},pZ[lagrangeFact] = {0};
	int pGPST[lagrangeFact] = {0};
	get8Point(PRN,SatType,pX,pY,pZ,pGPST,GPST);//获得PRN卫星发射时刻最近8点坐标
	//检验数据是否缺失（精密星历的轨道插值中数据不能缺失）
	for (int i = 0;i <lagrangeFact;i++)
		if (!(pX[i]&&pY[i]&&pZ[i])) return ;

	double sumX = 0,sumY = 0,sumZ = 0;//插值后的坐标XYZ
	double sumDX[2] = {0},sumDY[2] = {0},sumDZ[2] = {0};//插值前后+-0.5s的卫星坐标用来同时求取速度
	double lk = 1,ldk[2] = {1,1};
	for (int k = 0; k < lagrangeFact;k++)
	{
		for (int n = 0;n < lagrangeFact;n++)
		{
			if (k == n) continue;
			lk = lk*(GPST - pGPST[n])/(pGPST[k] - pGPST[n]);
			ldk[0] = ldk[0]*(GPST - 0.5 - pGPST[n])/(pGPST[k] - pGPST[n]);
			ldk[1] = ldk[1]*(GPST + 0.5 - pGPST[n])/(pGPST[k] - pGPST[n]);
		}
		sumX = sumX + pX[k]*lk;sumDX[0] = sumDX[0] + pX[k]*ldk[0];sumDX[1] = sumDX[1] + pX[k]*ldk[1];
		sumY = sumY + pY[k]*lk;sumDY[0] = sumDY[0] + pY[k]*ldk[0];sumDY[1] = sumDY[1] + pY[k]*ldk[1];
		sumZ = sumZ + pZ[k]*lk;sumDZ[0] = sumDZ[0] + pZ[k]*ldk[0];sumDZ[1] = sumDZ[1] + pZ[k]*ldk[1];

		lk = 1;ldk[0] = 1;ldk[1] = 1;
	}
	pXYZ[0] = sumX;pXYZ[1] = sumY;pXYZ[2] = sumZ;
	pdXYZ[0] = sumDX[1] - sumDX[0];pdXYZ[1] = sumDY[1] - sumDY[0];pdXYZ[2] = sumDZ[1] - sumDZ[0];
}

//获得精密星历坐标和速度
void QReadSP3::getPrcisePoint(int PRN,char SatType,double GPST,double *pXYZ,double *pdXYZ)
{
	if (IsSigalFile)
		readAllData2Vec();
	else
		readFileData2Vec(m_SP3FileNames);
	//如果位于GPS周第一天GPST时间需要加上604800，读取数据原因
	if (!IsSigalFile&&m_WeekOrder > 0)
	{
		if (GPST < 24*3600)  GPST += 604800;
	}
	//if (GPST < 24*3600)  GPST += 604800;//考虑不全面
	lagrangeMethod(PRN,SatType,GPST,pXYZ,pdXYZ);
}

//获得读取后的所有数据
QVector< SP3Data > QReadSP3::getAllData()
{
	if (m_SP3FileNames.isEmpty()&&m_SP3FileName.isEmpty())
		return m_allEpochData;
	if (IsSigalFile)
		readAllData2Vec();
	else
		readFileData2Vec(m_SP3FileNames);
	return m_allEpochData;
}

//释放所有数据
void QReadSP3::releaseAllData()
{
	m_allEpochData.clear();
}

bool QReadSP3::setSatlitSys(QString SystemStr)
{
	bool IsGood = QBaseObject::setSatlitSys(SystemStr);
	InitStruct();
	return IsGood;
}