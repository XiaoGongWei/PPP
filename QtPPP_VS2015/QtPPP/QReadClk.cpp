#include "QReadClk.h"


QReadClk::QReadClk()
{

}

QReadClk::QReadClk(QString ClkFileName)
{
	initVar();
	openFiles(ClkFileName);
	IsSigalFile =true;
}
QReadClk::QReadClk(QStringList ClkFileNames)
{
	initVar();
	//debug:2017.07.08
	if (ClkFileNames.length() == 1)
	{
		QString ClkFileName = ClkFileNames.at(0);
		openFiles(ClkFileName);
		IsSigalFile = true;
	}
	if (ClkFileNames.length() > 0)
	{
		m_ClkFileNames =ClkFileNames;
		IsSigalFile = false;
	}
	else
	{
		isReadAllData = true;
		return ; 
	}
}

//析构函数
QReadClk::~QReadClk(void)
{
	releaseAllData();
}

void QReadClk::initVar()
{
	isReadHead = false;
	IsSigalFile = false;
	isReadAllData = false;
	m_ClkFileName = "";
	lagrangeFact = 10;
	//只包含GPS系统最大共32颗卫星
	m_lastGPSTimeFlag = 0;//作为起始标记
	m_lastCmpGPSTime = 999999999;//记录起始计算的GPS时间
	m_EndHeadNum = 8;
	m_WeekOrder = 0;
	InitStruct();
}

//初始化需要用的结构体内部的矩阵(有节约内存作用)
void QReadClk::InitStruct()
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


void QReadClk::openFiles(QString ClkFileName)
{
	initVar();
	if (!ClkFileName.isEmpty())
	{
		m_readCLKFileClass.setFileName(ClkFileName);
		m_readCLKFileClass.open(QFile::ReadOnly);
		m_ClkFileName = ClkFileName;
	}
	else
	{
		isReadAllData = true;
		return;
	}
		
}

//转到GPS时间
double QReadClk::YMD2GPSTime(int Year,int Month,int Day,int HoursInt,int Minutes,double Seconds,int *GPSWeek)//,int *GPSTimeArray
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
	if (GPSWeek)	*GPSWeek = Week;
	return (N*24*3600 + HoursInt*3600 + Minutes*60 + Seconds);
}

//读取头文件
void QReadClk::readAllHead()
{
	if (isReadHead)
		return ;
	tempLine = m_readCLKFileClass.readLine();//读取第一行
	QString flagHeadEnd = "END";
	QString endHeadStr = tempLine.mid(60,20).trimmed();

	while (!endHeadStr.contains(flagHeadEnd,Qt::CaseInsensitive))
	{
		//进行头文件数据读取......
		//此处跳过头文件

		//读取下一行
		tempLine = m_readCLKFileClass.readLine();//读取下一行
		endHeadStr = tempLine.mid(60,20).trimmed();
	}
	tempLine = m_readCLKFileClass.readLine();//读取下一行进入数据区域
	isReadHead = true;
}

//读取单个文件所有数据
void QReadClk::readAllData()
{
	if (isReadAllData) return ;
	if (!isReadHead)	readAllHead();
	//首次进入时候tempLine AR......
	int Year =0,Month = 0,Day = 0,Hours = 0,Minutes = 0,Week = 0;
	double Seconds = 0;
	while (!m_readCLKFileClass.atEnd())
	{
		//跳过不是钟误差开头的区域
		while(tempLine.mid(0,3) != "AS ")
			tempLine = m_readCLKFileClass.readLine();
		//读取一个历元钟误差
		epochData.MatrixDataGPS.setZero();//用之前最好清零虽然加大计算量但是安全
		epochData.MatrixDataGlonass.setZero();
		epochData.MatrixDataBDS.setZero();
		epochData.MatrixDataGalieo.setZero();
		while (tempLine.mid(0,3) == "AS ")
		{//进入第一个历元
			int PRN = 0;
			char tempSatType = '0';
			tempSatType = *(tempLine.mid(3,1).toLatin1().data());
			//GPS系统
			if (isInSystem(tempSatType))
			{
				//读取年月日时分秒
				Year = tempLine.mid(8,4).toInt();
				Month = tempLine.mid(13,2).toInt();
				Day = tempLine.mid(16,2).toInt();
				Hours = tempLine.mid(19,2).toInt();
				Minutes = tempLine.mid(22,2).toInt();
				Seconds = tempLine.mid(25,9).toDouble();
				epochData.GPSTime = YMD2GPSTime(Year,Month,Day,Hours,Minutes,Seconds,&Week);//
				epochData.GPSWeek = Week;
				//读取卫星PRN 钟误差 中误差
				PRN = tempLine.mid(4,2).toInt();//PRN
				epochData.MatrixDataGPS(PRN - 1,0) = PRN;
				epochData.MatrixDataGPS(PRN - 1,1) = tempLine.mid(40,20).toDouble();//钟误差
				epochData.MatrixDataGPS(PRN - 1,2) = tempLine.mid(60,20).toDouble();//中误差
			}
			//Glonass系统
			else if (isInSystem(tempSatType))
			{
				//读取年月日时分秒
				Year = tempLine.mid(8,4).toInt();
				Month = tempLine.mid(13,2).toInt();
				Day = tempLine.mid(16,2).toInt();
				Hours = tempLine.mid(19,2).toInt();
				Minutes = tempLine.mid(22,2).toInt();
				Seconds = tempLine.mid(25,9).toDouble();
				epochData.GPSTime = YMD2GPSTime(Year,Month,Day,Hours,Minutes,Seconds,&Week);//跨周时间加上604800s
				epochData.GPSWeek = Week;
				//读取卫星PRN 钟误差 中误差
				PRN = tempLine.mid(4,2).toInt();//PRN
				epochData.MatrixDataGlonass(PRN - 1,0) = PRN;
				epochData.MatrixDataGlonass(PRN - 1,1) = tempLine.mid(40,20).toDouble();//钟误差
				epochData.MatrixDataGlonass(PRN - 1,2) = tempLine.mid(60,20).toDouble();//中误差
			}
			//BDS系统
			else if (isInSystem(tempSatType))
			{
				//读取年月日时分秒
				Year = tempLine.mid(8,4).toInt();
				Month = tempLine.mid(13,2).toInt();
				Day = tempLine.mid(16,2).toInt();
				Hours = tempLine.mid(19,2).toInt();
				Minutes = tempLine.mid(22,2).toInt();
				Seconds = tempLine.mid(25,9).toDouble();
				epochData.GPSTime = YMD2GPSTime(Year,Month,Day,Hours,Minutes,Seconds,&Week);//跨周时间加上604800s
				epochData.GPSWeek = Week;
				//读取卫星PRN 钟误差 中误差
				PRN = tempLine.mid(4,2).toInt();//PRN
				epochData.MatrixDataBDS(PRN - 1,0) = PRN;
				epochData.MatrixDataBDS(PRN - 1,1) = tempLine.mid(40,20).toDouble();//钟误差
				epochData.MatrixDataBDS(PRN - 1,2) = tempLine.mid(60,20).toDouble();//中误差
			}
			//Galieo系统
			else if (isInSystem(tempSatType))
			{
				//读取年月日时分秒
				Year = tempLine.mid(8,4).toInt();
				Month = tempLine.mid(13,2).toInt();
				Day = tempLine.mid(16,2).toInt();
				Hours = tempLine.mid(19,2).toInt();
				Minutes = tempLine.mid(22,2).toInt();
				Seconds = tempLine.mid(25,9).toDouble();
				epochData.GPSTime = YMD2GPSTime(Year,Month,Day,Hours,Minutes,Seconds,&Week);//跨周时间加上604800s
				epochData.GPSWeek = Week;
				//读取卫星PRN 钟误差 中误差
				PRN = tempLine.mid(4,2).toInt();//PRN
				epochData.MatrixDataGalieo(PRN - 1,0) = PRN;
				epochData.MatrixDataGalieo(PRN - 1,1) = tempLine.mid(40,20).toDouble();//钟误差
				epochData.MatrixDataGalieo(PRN - 1,2) = tempLine.mid(60,20).toDouble();//中误差
			}
			tempLine = m_readCLKFileClass.readLine();//读取一行坐标数据
		}
		m_allEpochData.append(epochData);
	}
	isReadAllData = true;
	m_readCLKFileClass.close();
	//判断钟误差历元间隔确定插值点个数
	CLKData epoch1 = m_allEpochData.at(0);
	CLKData epoch2 = m_allEpochData.at(1);
	if (qAbs(epoch1.GPSTime - epoch2.GPSTime) < 60)
		lagrangeFact = 2;
}

//读取多个文件所有数据
void QReadClk::readFileData2Vec(QStringList ClkFileNames)
{
	if (ClkFileNames.length() == 0)	isReadAllData = true;
	if (isReadAllData)
		return;
	m_allEpochData.clear();
	//首先读取头文件按照时间由小到大读取文件
	int minGPSWeek = 999999999;//保存最小周 减少int越界（周转换到秒时候）
	QVector< int > tempGPSWeek,fileFlagSeconds;//保存文件观测其实时间
	QVector< double > tempGPSSeconds;
	for (int i = 0;i < ClkFileNames.length();i++)
	{
		int Year =0,Month = 0,Day = 0,Hours = 0,Minutes = 0,GPSWeek = 0;
		double Seconds = 0,GPSSeconds = 0;
		QString CLKFileName = ClkFileNames.at(i);
		QFile clkfile(CLKFileName);
		if (!clkfile.open(QFile::ReadOnly))
		{
			QString erroInfo = "Open " + CLKFileName + "faild!(QReadClk::readFileData2Vec)";
			ErroTrace(erroInfo);
		}
		//跳过头文件
		do 
		{
			tempLine = clkfile.readLine();//读取第一行
			if (clkfile.atEnd())
			{
				ErroTrace("Can not read clk file!(QReadClk::readFileData2Vec)");
				break;
			}
		} while (!tempLine.contains("END OF HEADER",Qt::CaseInsensitive));
		//查找AS行对应的时间
		do 
		{
			tempLine = clkfile.readLine();//读取第一行
			if (clkfile.atEnd())
			{
				ErroTrace("Can not read clk file!(QReadClk::readFileData2Vec)");
				break;
			}
		} while (!tempLine.mid(0,2).contains("AS",Qt::CaseInsensitive));
		//获取时间
		Year = tempLine.mid(8,4).toInt();
		Month = tempLine.mid(13,2).toInt();
		Day = tempLine.mid(16,2).toInt();
		Hours = tempLine.mid(19,2).toInt();
		Minutes = tempLine.mid(22,2).toInt();
		Seconds = tempLine.mid(25,11).toDouble();
		GPSSeconds = YMD2GPSTime(Year,Month,Day,Hours,Minutes,Seconds,&GPSWeek);
		if (GPSWeek <= minGPSWeek)
			minGPSWeek = GPSWeek;
		tempGPSWeek.append(GPSWeek);
		tempGPSSeconds.append(GPSSeconds);
		clkfile.close();
	}
	//转化为秒
	QVector< int > WeekOrder;//保存数据 标记是否跨周，跨：1 不跨：0//读取多个文件需要用到
	for (int i = 0;i < tempGPSWeek.length();i++)
	{
		double Seconds = (tempGPSWeek.at(i) - minGPSWeek)*604800 + tempGPSSeconds.at(i);
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
				QString tempFileName = ClkFileNames.at(i);
				ClkFileNames[i] = ClkFileNames.at(j);
				ClkFileNames[j] = tempFileName;
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
	for (int i = 0;i < ClkFileNames.length();i++)
	{
		QString CLKFileName = ClkFileNames.at(i);
//打开文件
		QFile clkfile(CLKFileName);
		if (!clkfile.open(QFile::ReadOnly))
		{
			QString erroInfo = "Open " + CLKFileName + "faild!(QReadClk::readFileData2Vec)";
			ErroTrace(erroInfo);
		}
//读取头文件数据
		tempLine = clkfile.readLine();//读取第一行
		QString flagHeadEnd = "END";
		QString endHeadStr = tempLine.mid(60,20).trimmed();
		while (!endHeadStr.contains(flagHeadEnd,Qt::CaseInsensitive))
		{
			//进行头文件数据读取......
			//此处跳过头文件

			//读取下一行
			tempLine = clkfile.readLine();//读取下一行
			endHeadStr = tempLine.mid(60,20).trimmed();
		}
		tempLine = clkfile.readLine();//读取下一行进入数据区域
//首个文件读取尾部8块数据
		if (i == 0&&ClkFileNames.length() >= 3)
		{
			int flag = 0,i = 1,CharNum = 80;
			QString preStr = "AS";//保存之前的每行前两个字符
			//判断文件是一行60字符还是80字符
			clkfile.seek(clkfile.size() - 80);//有的钟差文件是80有的是60
			tempLine = clkfile.readLine();//读取一行
			//第二个字符不是S就是60字符
			if (tempLine.mid(1,1) != "S")
				CharNum = 60;
			do 
			{
				clkfile.seek(clkfile.size() - i*CharNum);//假设每行一定是80字符
				i++;
				tempLine = clkfile.readLine();
				if (tempLine.mid(0,2) == "AR" && preStr == "AS")
				{
					flag++;
				}
				preStr = tempLine.mid(0,2);
			} while (flag < m_EndHeadNum);
		}

//最后一个文件读取头部8块数据
		bool isEndFile = false;//判别是否是最后一个文件标记
		int endFileBlockNum = 0;//读取最后一个文件数量标记
		if (i == (ClkFileNames.length()-1)&&ClkFileNames.length() >= 3)
		{
			isEndFile = true;
		}
//读取数据区域
		//首次进入时候tempLine AR......
		int Year =0,Month = 0,Day = 0,Hours = 0,Minutes = 0,Week = 0;
		double Seconds = 0;
		while (!clkfile.atEnd())
		{
			//跳过不是钟误差开头的区域
			while(tempLine.mid(0,3) != "AS ")
				tempLine = clkfile.readLine();
			//读取一个历元钟误差
			epochData.MatrixDataGPS.setZero();//用之前最好清零虽然加大计算量但是安全
			epochData.MatrixDataGlonass.setZero();
			epochData.MatrixDataBDS.setZero();
			epochData.MatrixDataGalieo.setZero();
			while (tempLine.mid(0,3) == "AS ")
			{//进入第一个历元
				int PRN = 0;
				char tempSatType = '0';
				tempSatType = *(tempLine.mid(3,1).toLatin1().data());
				//GPS系统
				if (tempSatType == 'G'&&isInSystem(tempSatType))
				{
					//读取年月日时分秒
					Year = tempLine.mid(8,4).toInt();
					Month = tempLine.mid(13,2).toInt();
					Day = tempLine.mid(16,2).toInt();
					Hours = tempLine.mid(19,2).toInt();
					Minutes = tempLine.mid(22,2).toInt();
					Seconds = tempLine.mid(25,9).toDouble();
					epochData.GPSTime = YMD2GPSTime(Year,Month,Day,Hours,Minutes,Seconds,&Week) + WeekOrder.at(i)*604800;//跨周时间加上604800s
					epochData.GPSWeek = Week;
					//读取卫星PRN 钟误差 中误差
					PRN = tempLine.mid(4,2).toInt();//PRN
					epochData.MatrixDataGPS(PRN - 1,0) = PRN;
					epochData.MatrixDataGPS(PRN - 1,1) = tempLine.mid(40,20).toDouble();//钟误差
					epochData.MatrixDataGPS(PRN - 1,2) = tempLine.mid(60,20).toDouble();//中误差
				}
				//Glonass系统
				else if (tempSatType == 'R'&&isInSystem(tempSatType))
				{
					//读取年月日时分秒
					Year = tempLine.mid(8,4).toInt();
					Month = tempLine.mid(13,2).toInt();
					Day = tempLine.mid(16,2).toInt();
					Hours = tempLine.mid(19,2).toInt();
					Minutes = tempLine.mid(22,2).toInt();
					Seconds = tempLine.mid(25,9).toDouble();
					epochData.GPSTime = YMD2GPSTime(Year,Month,Day,Hours,Minutes,Seconds,&Week) + WeekOrder.at(i)*604800;//跨周时间加上604800s
					epochData.GPSWeek = Week;
					//读取卫星PRN 钟误差 中误差
					PRN = tempLine.mid(4,2).toInt();//PRN
					epochData.MatrixDataGlonass(PRN - 1,0) = PRN;
					epochData.MatrixDataGlonass(PRN - 1,1) = tempLine.mid(40,20).toDouble();//钟误差
					epochData.MatrixDataGlonass(PRN - 1,2) = tempLine.mid(60,20).toDouble();//中误差
				}
				//BDS系统
				else if (tempSatType == 'C'&&isInSystem(tempSatType))
				{
					//读取年月日时分秒
					Year = tempLine.mid(8,4).toInt();
					Month = tempLine.mid(13,2).toInt();
					Day = tempLine.mid(16,2).toInt();
					Hours = tempLine.mid(19,2).toInt();
					Minutes = tempLine.mid(22,2).toInt();
					Seconds = tempLine.mid(25,9).toDouble();
					epochData.GPSTime = YMD2GPSTime(Year,Month,Day,Hours,Minutes,Seconds,&Week) + WeekOrder.at(i)*604800;//跨周时间加上604800s
					epochData.GPSWeek = Week;
					//读取卫星PRN 钟误差 中误差
					PRN = tempLine.mid(4,2).toInt();//PRN
					epochData.MatrixDataBDS(PRN - 1,0) = PRN;
					epochData.MatrixDataBDS(PRN - 1,1) = tempLine.mid(40,20).toDouble();//钟误差
					epochData.MatrixDataBDS(PRN - 1,2) = tempLine.mid(60,20).toDouble();//中误差
				}
				//Galieo系统
				else if (tempSatType == 'E'&&isInSystem(tempSatType))
				{
					//读取年月日时分秒
					Year = tempLine.mid(8,4).toInt();
					Month = tempLine.mid(13,2).toInt();
					Day = tempLine.mid(16,2).toInt();
					Hours = tempLine.mid(19,2).toInt();
					Minutes = tempLine.mid(22,2).toInt();
					Seconds = tempLine.mid(25,9).toDouble();
					epochData.GPSTime = YMD2GPSTime(Year,Month,Day,Hours,Minutes,Seconds,&Week) + WeekOrder.at(i)*604800;//跨周时间加上604800s
					epochData.GPSWeek = Week;
					//读取卫星PRN 钟误差 中误差
					PRN = tempLine.mid(4,2).toInt();//PRN
					epochData.MatrixDataGalieo(PRN - 1,0) = PRN;
					epochData.MatrixDataGalieo(PRN - 1,1) = tempLine.mid(40,20).toDouble();//钟误差
					epochData.MatrixDataGalieo(PRN - 1,2) = tempLine.mid(60,20).toDouble();//中误差
				}
				tempLine = clkfile.readLine();//读取一行坐标数据
			}
			m_allEpochData.append(epochData);
			if (isEndFile)
			{
				endFileBlockNum++;
				//终止读取最后一个文件
				if (endFileBlockNum >=m_EndHeadNum)	break;
			}
		}//读取文件数据结束while (!clkfile.atEnd())
		clkfile.close();
	}//for (int i = 0;i < ClkFileNames.length();i++)
	isReadAllData = true;
	//判断钟误差历元间隔确定插值点个数
	CLKData epoch1 = m_allEpochData.at(20);
	CLKData epoch2 = m_allEpochData.at(21);
	if (qAbs(epoch1.GPSTime - epoch2.GPSTime) < 60)
		lagrangeFact = 2;
}

//返回数据
QVector< CLKData > QReadClk::getAllData()
{
	if (IsSigalFile)	
		readAllData();
	else
		readFileData2Vec(m_ClkFileNames);
	return m_allEpochData;
}

//释放所有数据
void QReadClk::releaseAllData()
{
	m_allEpochData.clear();
}

////pX,pY,pZ：lagrangeFact个点坐标；pGPST:lagrangeFact个点GPS周内秒;GPST卫星发射时刻周内秒
void QReadClk::get8Point(int PRN,char SatType,double *pCLKT,double *pGPST,double GPST)
{//获取最邻近的lagrangeFact个点
	//检查是否加速
	int lengthEpoch = m_allEpochData.length();
	//发现GPST在整个历元中的位置
	int GPSTflag = m_lastGPSTimeFlag;
	int numPoint = lagrangeFact / 2;//前后取numPoint 个点
	if (qAbs(m_lastCmpGPSTime - GPST) > 0.3)//大于0.3s说明不在一个历元假设观测历元间隔肯定在1s之上(减少相同历元多次查询位置)
	{
		if (ACCELERATE)	m_lastCmpGPSTime = GPST;
		for (int i = m_lastGPSTimeFlag;i < lengthEpoch;i++)
		{
			CLKData epochData = m_allEpochData.at(i);
			if (epochData.GPSTime >= GPST)
				break; 
			else
				GPSTflag++;
		}
		//两点插值时候由于卫星传播0.00n秒 导致GPSTflag多1 需要减一
		if (numPoint == 1) GPSTflag--;
	}



//前后取 lagrangeFact 点 考虑边界问题

	if (GPSTflag < 0) GPSTflag = 0;
	if (ACCELERATE)	m_lastGPSTimeFlag = GPSTflag;//保存最新的位置

	if ((GPSTflag >= numPoint - 1) && (GPSTflag <= lengthEpoch - numPoint - 1))
	{//在中间位置前面四个包含本身 后面四个不包含本身
		for (int i = 0;i < lagrangeFact;i++)
		{
			CLKData epochData = m_allEpochData.at(GPSTflag - numPoint + 1 + i);
			//判断属于那个系统数据
			switch (SatType)
			{
			case 'G':
				pCLKT[i] = epochData.MatrixDataGPS(PRN - 1,1);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'R':
				pCLKT[i] = epochData.MatrixDataGlonass(PRN - 1,1);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'C':
				pCLKT[i] = epochData.MatrixDataBDS(PRN - 1,1);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'E':
				pCLKT[i] = epochData.MatrixDataGalieo(PRN - 1,1);
				pGPST[i] = epochData.GPSTime;
				break;
			default:
				pCLKT[i] = 0;
				pGPST[i] = 0;
			}
		}
	}
	else if(GPSTflag < numPoint - 1)
	{//在开始位置边界
		for (int i = 0;i < lagrangeFact;i++)
		{
			CLKData epochData = m_allEpochData.at(i);
			//判断属于那个系统数据
			switch (SatType)
			{
			case 'G':
				pCLKT[i] = epochData.MatrixDataGPS(PRN - 1,1);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'R':
				pCLKT[i] = epochData.MatrixDataGlonass(PRN - 1,1);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'C':
				pCLKT[i] = epochData.MatrixDataBDS(PRN - 1,1);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'E':
				pCLKT[i] = epochData.MatrixDataGalieo(PRN - 1,1);
				pGPST[i] = epochData.GPSTime;
				break;
			default:
				pCLKT[i] = 0;
				pGPST[i] = 0;
			}
		}
	}
	else if(GPSTflag > lengthEpoch - numPoint - 1)
	{//在结束位置边界
		for (int i = 0;i < lagrangeFact;i++)
		{
			//debug:2017.07.08
			CLKData epochData = m_allEpochData.at(lengthEpoch - (lagrangeFact-i));
			//判断属于那个系统数据
			switch (SatType)
			{
			case 'G':
				pCLKT[i] = epochData.MatrixDataGPS(PRN - 1,1);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'R':
				pCLKT[i] = epochData.MatrixDataGlonass(PRN - 1,1);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'C':
				pCLKT[i] = epochData.MatrixDataBDS(PRN - 1,1);
				pGPST[i] = epochData.GPSTime;
				break;
			case 'E':
				pCLKT[i] = epochData.MatrixDataGalieo(PRN - 1,1);
				pGPST[i] = epochData.GPSTime;
				break;
			default:
				pCLKT[i] = 0;
				pGPST[i] = 0;
			}
		}
	}
}


//拉格朗日方法
void QReadClk::lagrangeMethod(int PRN,char SatType,double GPST,double *pCLKT)
{//拉格朗日插值 此处选取前后---共8个点做插值 GPST卫星发射时刻周内秒
	*pCLKT = 0;
	//判断是否设置合法系统
	if (!isInSystem(SatType)) 
		return ;
	//判断是否读取数据
	if (!isReadAllData) 
	{
		if (IsSigalFile)
			readAllData();
		else
			readFileData2Vec(m_ClkFileNames);
	}
	double m_CLKT[12] = {0};//保存获取的插值数据
	double m_pGPST[12] = {0};
	get8Point(PRN,SatType,m_CLKT,m_pGPST,GPST);//获得PRN卫星发射时刻最近8或者2个点钟差
	//检验数据完整性（精密钟差插值中数据不能缺失）
	for (int i = 0;i <lagrangeFact;i++)
		if (!m_CLKT[i]) return ;
	//拉格朗日插值
	double sumCLK = 0;//插值后的坐标XYZ
	for (int k = 0; k < lagrangeFact;k++)
	{
		double lk = 1;
		for (int n = 0;n < lagrangeFact;n++)
		{
			if (k == n) continue;
			lk *= (GPST - m_pGPST[n])/(m_pGPST[k] - m_pGPST[n]);
		}
		sumCLK += m_CLKT[k]*lk;
	}
	*pCLKT = sumCLK*M_C;
}



//获得发射时刻卫星钟误差(多系统)
void QReadClk::getStaliteClk(int PRN,char SatType,double GPST,double *pCLKT)
{
	if (IsSigalFile)	
		readAllData();
	else
		readFileData2Vec(m_ClkFileNames);
	//如果位于GPS周第一天GPST时间需要加上604800，读取数据原因
	if (!IsSigalFile&&m_WeekOrder > 0)
	{
		if (GPST < 24*3600)  GPST += 604800;
	}
	//if (GPST < 24*3600)  GPST += 604800;//考虑不全面
	lagrangeMethod(PRN,SatType,GPST,pCLKT);
}

bool QReadClk::setSatlitSys(QString SystemStr)
{
	bool IsGood = QBaseObject::setSatlitSys(SystemStr);
	InitStruct();
	return IsGood;
}