#include "QReadOFile.h"

//初始化变量
void QReadOFile::initVar()
{
	m_OfileName = "";
	tempLine = "";
	
	isReadHead = false;
//头文件信息
	RinexVersion = 0;
	FileIdType = 0;
	SatelliteSys = 0;
	PGM = "";
	RUNBY = "";
	CreatFileDate = "";
	CommentInfo = "";
	MarkerName = "";
	ObserverNames = "";
	Agency = "";
	ReciverREC = "";
	ReciverType = "";
	ReciverVers = "";
	AntNumber = "";
	AntType = "";
	for (int i = 0; i < 3; i++)
	{
		ApproxXYZ[i] = -1;
		AntHEN[i] = -1;
	}
	FactL12[0] = -1; FactL12[1] = -1;
	TypeObservNum = -1;
	IntervalSeconds = -1;
	for (int i = 0; i < 5; i++)
	{
		YMDHM[i] = 0;
	}
	ObsSeconds = 0;
	SateSystemOTime = "";
	for (int i = 0;i < 5;i++)
		for (int j = 0;j < 20;j++)
			CPflagVer3[i][j] = -1;
	for (int j = 0;j < 20;j++)
		CPflags[j] = -1;
	QString str1 = "   G 1 A 2";
	matchHead.setPattern(".*[GRSCE].*");//简单匹配头文件（2.X）（可以进行优化）
}

QReadOFile::QReadOFile()
{
	initVar();
}

//类的初始化
QReadOFile::QReadOFile(QString OfileName)
{
	initVar();
	if (OfileName.trimmed().isEmpty())
		ErroTrace("File Name is Empty!(QReadOFile::QReadOFile(QString OfileName))");
	m_OfileName = OfileName;

	//读取文件头部信息并解析
	getHeadInf();//此处并未关闭文件
	baseYear = (int)(YMDHM[0]/100)*100;

}

QReadOFile::~QReadOFile(void)
{
}

//计算约化儒略日（本类未用到）
double QReadOFile::getMJD(int Year,int Month,int Day,int HoursInt,int Minutes,int Seconds)
{
	double Hours = HoursInt + ((Minutes * 60) + Seconds)/3600.0;
	//Get JD
	double JD = 0.0;
	if(Month<=2)
		JD = (int)(365.25*(Year-1)) + (int)(30.6001*(Month+12+1)) + Day + Hours/24.0 + 1720981.5;
	else
		JD = (int)(365.25*(Year)) + (int)(30.6001*(Month+1)) + Day + Hours/24.0 + 1720981.5;
	return (JD - 2400000.5);
}

//读取头文件到变量（各个版本O文件 头文件相似，此处并未关闭文件）
void QReadOFile::getHeadInf()
{
	if (isReadHead)	return ;
	//打开文件
	m_readOFileClass.setFileName(m_OfileName);
	if (!m_readOFileClass.open(QFile::ReadOnly))
		ErroTrace("Open file bad!(QReadOFile::QReadOFile(QString OfileName))");
	//读取头文件
	
	while (!m_readOFileClass.atEnd())
	{
		tempLine = m_readOFileClass.readLine();
		if (tempLine.contains("END OF HEADER",Qt::CaseInsensitive))
			break;
		if (tempLine.contains("RINEX VERSION",Qt::CaseInsensitive))
		{

			RinexVersion =  tempLine.mid(0,20).trimmed().toDouble();
			FileIdType = tempLine.at(20);
			SatelliteSys = tempLine.at(40);
		} 
		else if (tempLine.contains("PGM / RUN BY / DATE",Qt::CaseInsensitive))
		{
			PGM = tempLine.mid(0,20).trimmed();
			RUNBY = tempLine.mid(20,20).trimmed();
			CreatFileDate = tempLine.mid(40,20).trimmed();
		}
		else if (tempLine.contains("COMMENT",Qt::CaseInsensitive))
		{
			CommentInfo+=tempLine.mid(0,60).trimmed() + "\n";
		}
		else if (tempLine.contains("MARKER NAME",Qt::CaseInsensitive))
		{
			MarkerName = tempLine.mid(0,60).trimmed().toUpper();
		}
		else if (tempLine.contains("MARKER NUMBER"))
		{
			MarkerNumber = tempLine.mid(0,60).trimmed();
		}
		else if (tempLine.contains("OBSERVER / AGENCY",Qt::CaseInsensitive))
		{
			ObserverNames = tempLine.mid(0,20).trimmed();
			Agency = tempLine.mid(20,40).trimmed();
		}
		else if (tempLine.contains("REC # / TYPE / VERS"))
		{
			ReciverREC = tempLine.mid(0,20).trimmed();
			ReciverType = tempLine.mid(20,20).trimmed();
			ReciverVers = tempLine.mid(40,20).trimmed();
		}
		else if (tempLine.contains("ANT # / TYPE",Qt::CaseInsensitive))
		{
			AntNumber = tempLine.mid(0,20).trimmed();
			AntType = tempLine.mid(20,20).trimmed();
		}
		else if (tempLine.contains("APPROX POSITION XYZ",Qt::CaseInsensitive))
		{
			ApproxXYZ[0] = tempLine.mid(0,14).trimmed().toDouble();
			ApproxXYZ[1] = tempLine.mid(14,14).trimmed().toDouble();	
			ApproxXYZ[2] = tempLine.mid(28,14).trimmed().toDouble();
		}
		else if (tempLine.contains("ANTENNA: DELTA H/E/N",Qt::CaseInsensitive))
		{
			AntHEN[0] = tempLine.mid(0,14).trimmed().toDouble();
			AntHEN[1] = tempLine.mid(14,14).trimmed().toDouble();	
			AntHEN[2] = tempLine.mid(28,14).trimmed().toDouble();
		}
		else if (tempLine.contains("WAVELENGTH FACT L1/2",Qt::CaseInsensitive))
		{
			FactL12[0] = tempLine.mid(0,6).trimmed().toInt();
			FactL12[1] = tempLine.mid(6,6).trimmed().toInt();
		}
		else if (tempLine.contains("TYPES OF OBSERV",Qt::CaseInsensitive))
		{//由于全局变量Static的使用导致该函数出现一个很大的Bug 禁止使用全局变量 在main函数中也不会消失
			if (RinexVersion > 3) continue;
			if(TypeObservNum < 1)
				TypeObservNum = tempLine.left(6).trimmed().toInt();
			if (TypeObservNum != 0 )
			{
				for (int i = 0;i < 9;i++)
				{
					QString tempObserVar = "";
					tempObserVar = tempLine.left(6*i + 12).right(6).trimmed();
					if (tempObserVar.isEmpty())
						break;
					ObservVarsNames.append(tempObserVar);
				}
			}
			else
			{
				ErroTrace("Lack TYPES OF OBSERV!(QReadOFile::QReadOFile(QString OfileName))");
			}

		}//# / TYPES OF OBSERV if结束
		else if (tempLine.contains("SYS / # / OBS TYPES",Qt::CaseInsensitive))
		{//renix 3.0增加的头文件
			if (RinexVersion < 3)	continue;
			obsVarNamesVer3 tempObsType;
			tempObsType.SatType = tempLine.mid(0, 1);
			tempObsType.obsNum3ver = tempLine.mid(3, 3).trimmed().toInt();
			QString obstypeName = "";
			int flag = 0;
			for (int i = 0; i < tempObsType.obsNum3ver;i++)
			{
				obstypeName = tempLine.mid(6 + flag * 4, 4).trimmed();
				flag++;
				tempObsType.obsNames3ver.append(obstypeName);
				//超出13解析下一行
				if (flag > 12)
				{
					tempLine = m_readOFileClass.readLine();
					flag = 0;
				}
				
			}
			m_obsVarNamesVer3.append(tempObsType);//保存本系统数据
		}
		else if (tempLine.contains("INTERVAL",Qt::CaseInsensitive))
		{
			IntervalSeconds = tempLine.mid(0,10).trimmed().toDouble();
		}
		else if (tempLine.contains("TIME OF FIRST OBS",Qt::CaseInsensitive))
		{
			for (int i = 0;i < 5;i++)
			{
				YMDHM[i] = tempLine.left((i+1)*6).right(6).trimmed().toInt();
			}
			ObsSeconds = tempLine.left(43).right(13).trimmed().toDouble();
			SateSystemOTime = tempLine.left(51).right(8).trimmed();
		}
	}
	isReadHead = true;
	//读取完头文件进行立刻解析
	if (RinexVersion < 3)
	{
		getLPFlag2();//获得观测值位置
	}
	else if (RinexVersion >= 3)
	{
		getLPFlag3();
	}
}

//获取载波和测距码所在位置（仅适用于版本2.X）
void QReadOFile::getLPFlag2()
{
	for (int i = 0; i < TypeObservNum; i++)
	{
		if (ObservVarsNames.at(i).contains("L1"))
			CPflags[0] = i;
		else if (ObservVarsNames.at(i).contains("L2"))
			CPflags[1] = i;
		else if (ObservVarsNames.at(i).contains("C1"))
			CPflags[2] = i;
		else if (ObservVarsNames.at(i).contains("P2"))
			CPflags[3] = i;
		else if (ObservVarsNames.at(i).contains("P1"))
			CPflags[4] = i;
	}
}

//获取载波和测距码所在位置（仅适用于版本3.X）
void QReadOFile::getLPFlag3()
{
	QVector< obsVarNamesVer3 > tempChangeVar3;
	tempChangeVar3 = m_obsVarNamesVer3;
	for (int i= 0;i < tempChangeVar3.length();i++)
	{
		obsVarNamesVer3 tempObsType = m_obsVarNamesVer3.at(i);
		QString obsType = "";
		int LFlag = 0;
		for (int k = 0;k < sizeof(tempChangeVar3[i].CPflags)/sizeof(tempChangeVar3[i].CPflags[0]);k++)//初始化-1
			tempChangeVar3[i].CPflags[k] = -1;
		//找到“靠前”两个不同的L频段码
		for (int j = 0;j < tempObsType.obsNames3ver.length();j++)
		{
			obsType = tempObsType.obsNames3ver.at(j);
			if (obsType.mid(0,1).contains("L"))
			{
				LFlag = obsType.mid(1,1).toInt();
				if (tempChangeVar3[i].CPflags[0] < 0)
				{
					tempChangeVar3[i].CPflags[0] = j;
					tempChangeVar3[i].CPflags[4] = obsType.mid(1,1).toInt();
				}
				else if (tempChangeVar3[i].CPflags[1] < 0&&LFlag != tempChangeVar3[i].CPflags[4])
				{
					tempChangeVar3[i].CPflags[1] = j;
					tempChangeVar3[i].CPflags[5] = obsType.mid(1,1).toInt();
					break;
				}
					
			}
			
		}
		//找到对应的L的C码
		for (int j = 0;j < tempObsType.obsNames3ver.length();j++)
		{
			obsType = tempObsType.obsNames3ver.at(j);
			if (obsType.mid(0,1).contains("C"))
			{
				LFlag = obsType.mid(1,1).toInt();
				if (tempChangeVar3[i].CPflags[2] < 0&&LFlag == tempChangeVar3[i].CPflags[4])
				{
					tempChangeVar3[i].CPflags[2] = j;
					tempChangeVar3[i].CPflags[6] = LFlag;
				}
				else if (tempChangeVar3[i].CPflags[3] < 0&&LFlag == tempChangeVar3[i].CPflags[5])
				{
					tempChangeVar3[i].CPflags[3] = j;
					tempChangeVar3[i].CPflags[7] = LFlag;
					break;
				}

			}
		}
 	}//所有系统解析完
	//3.X解析测距码和载波位置
	int LenHang = sizeof(CPflagVer3[0])/sizeof(CPflagVer3[0][0]);
	for (int i = 0;i < tempChangeVar3.length();i++)
	{
		obsVarNamesVer3 tempObsType = tempChangeVar3.at(i);
		if (tempObsType.SatType == "G")
		{
			for(int i = 0;i < LenHang;i++)
				CPflagVer3[0][i] = tempObsType.CPflags[i];
		}
		else if (tempObsType.SatType == "R")
		{
			for(int i = 0;i < LenHang;i++)
				CPflagVer3[1][i] = tempObsType.CPflags[i];
		}
		else if (tempObsType.SatType == "C")
		{
			for(int i = 0;i < LenHang;i++)
				CPflagVer3[2][i] = tempObsType.CPflags[i];
		}
		else if (tempObsType.SatType == "S")
		{
			for(int i = 0;i < LenHang;i++)
				CPflagVer3[3][i] = tempObsType.CPflags[i];
		}
		else if (tempObsType.SatType == "E")
		{
			for(int i = 0;i < LenHang;i++)
				CPflagVer3[4][i] = tempObsType.CPflags[i];
		}
	}
	m_obsVarNamesVer3 = tempChangeVar3;
}

//将字符转数据行，解析到卫星数据，获取卫星测距码和载波数据
bool QReadOFile::getOneSatlitData(QString &dataString,SatlitData &oneSatlite)
{//读取Rinex 2.X和3.X观测文件都可以用
	if (dataString.isEmpty())	return false;
	int *CPflagVers,flag = 0;
	//按照版本获取卫星测距码和载波数据位置
	if (RinexVersion >= 3)
	{
		if (oneSatlite.SatType == 'G')
			flag = 0;
		else if (oneSatlite.SatType == 'R')
			flag = 1;
		else if (oneSatlite.SatType == 'C')
			flag = 2;
		else if (oneSatlite.SatType == 'S')
			flag = 3;
		else if (oneSatlite.SatType == 'E')
			flag = 4;
		CPflagVers = CPflagVer3[flag];
	}
	else if (RinexVersion < 3)
	{
		CPflagVers = CPflags;
	}
	//读取载波测距码
	if (CPflagVers[0] != -1)
		oneSatlite.L1 = dataString.mid(CPflagVers[0]*16,14).trimmed().toDouble();
	if(CPflagVers[1] != -1)
		oneSatlite.L2 = dataString.mid(CPflagVers[1]*16,14).trimmed().toDouble();
	if (CPflagVers[2] != -1)
		oneSatlite.C1 = dataString.mid(CPflagVers[2]*16,14).trimmed().toDouble();
	if (CPflagVers[3] != -1)
		oneSatlite.P2 = dataString.mid(CPflagVers[3]*16,14).trimmed().toDouble();
	return true;
}

void QReadOFile::getFrequency(SatlitData &oneSatlite)
{//debug:2017.07.08
	if (RinexVersion >= 3)
		getFrequencyVer3(oneSatlite);
	else if (RinexVersion < 3)
		getFrequencyVer2(oneSatlite);
	return ;
}
//根据卫星PRN，类型获取L1和L2的频率
void QReadOFile::getFrequencyVer2(SatlitData &oneSatlite)
{//debug:2017.07.08
	if (oneSatlite.SatType == 'G')
	{//有Bug：如果GPS频率C2 C1
		oneSatlite.Frq[0] = M_F1;
		oneSatlite.Frq[1] = M_F2;
	}
	else if (oneSatlite.SatType == 'R')
	{
		oneSatlite.Frq[0] = M_GLONASSF1(g_GlonassK[oneSatlite.PRN - 1]);
		oneSatlite.Frq[1] = M_GLONASSF2(g_GlonassK[oneSatlite.PRN - 1]);
	} 
	else if (oneSatlite.SatType == 'C')
	{
		oneSatlite.Frq[0] = g_BDSFrq[1];
		oneSatlite.Frq[1] = g_BDSFrq[2];
	}
	else if (oneSatlite.SatType == 'E')
	{
		oneSatlite.Frq[0] = g_GalieoFrq[1];
		oneSatlite.Frq[1] = g_GalieoFrq[2];
	}
	else
	{
		oneSatlite.Frq[0] = 0;
		oneSatlite.Frq[1] = 0;
	}
}

//根据卫星PRN，类型获取L1和L2的频率
void QReadOFile::getFrequencyVer3(SatlitData &oneSatlite)
{//debug:2017.07.08
	if (oneSatlite.SatType == 'G')
	{//有Bug：如果GPS频率C2 C1
		if(CPflagVer3[0][4] != -1&&CPflagVer3[0][5] != -1)
		{
			oneSatlite.Frq[0] = g_GPSFrq[CPflagVer3[0][4] - 1];
			oneSatlite.Frq[1] = g_GPSFrq[CPflagVer3[0][5] - 1];
		}
		else
		{
			oneSatlite.Frq[0] = 0;
			oneSatlite.Frq[1] = 0;
		}
	}
	else if (oneSatlite.SatType == 'R')
	{
		oneSatlite.Frq[0] = M_GLONASSF1(g_GlonassK[oneSatlite.PRN - 1]);
		oneSatlite.Frq[1] = M_GLONASSF2(g_GlonassK[oneSatlite.PRN - 1]);
	} 
	else if (oneSatlite.SatType == 'C')
	{
		int flag1 = CPflagVer3[2][4] - 1,flag2 = CPflagVer3[2][5] - 1;
		//转换
		if (flag1 == 6)	flag1 = 2;
		if (flag2 == 6)	flag2 = 2;
		if (flag1 == 5)	flag1 = 3;
		if (flag2 == 5)	flag2 = 3;
		if (flag1 > 3||flag2 > 3)
			ErroTrace("BDS Frqence erro!(QReadOFile::getFrequencyVer3)");
		oneSatlite.Frq[0] = g_BDSFrq[flag1];
		oneSatlite.Frq[1] = g_BDSFrq[flag2];
	}
	else if (oneSatlite.SatType == 'E')
	{
		int flag1 = CPflagVer3[4][4] - 1,flag2 = CPflagVer3[4][5] - 1;
		//转换
		if (flag1 > 1)	flag1 -= 3;
		if (flag2 > 1)	flag2 -= 3;
		if (flag1 > 4||flag2 > 4)
			ErroTrace("Galieo Frqence erro!(QReadOFile::getFrequency)");
		oneSatlite.Frq[0] = g_GalieoFrq[flag1];
		oneSatlite.Frq[1] = g_GalieoFrq[flag2];
	}
	else
	{
		oneSatlite.Frq[0] = 0;
		oneSatlite.Frq[1] = 0;
	}
}

//读取Rinex 3.X观测文件
void QReadOFile::readEpochVer3(QVector< SatlitData > &epochData)
{
	if (!isReadHead) getHeadInf();
	tempLine = m_readOFileClass.readLine();
	//进入数据区域读取
	while (!m_readOFileClass.atEnd())
	{//读取数据段
		if (0 != tempLine.mid(0,1).compare(">")) continue;//找到数据头部> 2015 05 02 00 00  0.0000000  0 28
		//读取头部数据
		SatlitData oneSatlit;//一颗卫星数据
		int epochStalitNum = 0;
		memset(&oneSatlit,0,sizeof(SatlitData));//使用memset初始化（安全一些，可能减少效率，可以删除）
		oneSatlit.UTCTime.Year = tempLine.mid(1,5).trimmed().toInt();
		oneSatlit.UTCTime.Month = tempLine.mid(6,3).trimmed().toInt();
		oneSatlit.UTCTime.Day = tempLine.mid(9,3).trimmed().toInt();
		oneSatlit.UTCTime.Hours = tempLine.mid(12,3).trimmed().toInt();
		oneSatlit.UTCTime.Minutes = tempLine.mid(15,3).trimmed().toInt();
		oneSatlit.UTCTime.Seconds = tempLine.mid(18,11).trimmed().toDouble();
		oneSatlit.EpochFlag = tempLine.mid(29,3).trimmed().toInt();
		epochStalitNum = tempLine.mid(32,3).trimmed().toInt();//本历元卫星数目
		//读取卫星数据
		char tempSatType = '0';
		QString dataString = "";
		for (int i = 0;i < epochStalitNum;i++)
		{
			tempLine = m_readOFileClass.readLine(); 
			tempSatType = *(tempLine.mid(0,1).toLatin1().data());//类型
			if (isInSystem(tempSatType))
			{
				oneSatlit.SatType = tempSatType;//类型
				oneSatlit.PRN = tempLine.mid(1,2).toInt();//PRN
				dataString = tempLine.mid(3);
				getOneSatlitData(dataString,oneSatlit);//获取卫星测距码和载波数据
				getFrequency(oneSatlit);//获取频率
				epochData.append(oneSatlit);//保存历元中的一颗卫星
			}
		}
		break;
	}
}

//读取Rinex 2.X观测文件
void QReadOFile::readEpochVer2(QVector< SatlitData > &epochData)
{
	if (!isReadHead) getHeadInf();
	tempLine = m_readOFileClass.readLine();
	//进入数据区域读取
	while (!m_readOFileClass.atEnd())
	{//读取数据段
		if (!matchHead.exactMatch(tempLine)) //找到数据头部>  10  4 10  0  1 30.0000000  0  9G 2G29G12G 4G13G17G10G 5G30
		{
			tempLine = m_readOFileClass.readLine();
			continue;
		}
		//读取头部数据
		SatlitData oneSatlit;//一颗卫星数据
		int epochStalitNum = 0;
		memset(&oneSatlit,0,sizeof(SatlitData));//使用memset初始化（安全一些，可能减少效率，可以删除）
		oneSatlit.UTCTime.Year = tempLine.mid(0,3).trimmed().toInt() + baseYear;
		oneSatlit.UTCTime.Month = tempLine.mid(3,3).trimmed().toInt();
		oneSatlit.UTCTime.Day = tempLine.mid(6,3).trimmed().toInt();
		oneSatlit.UTCTime.Hours = tempLine.mid(9,3).trimmed().toInt();
		oneSatlit.UTCTime.Minutes = tempLine.mid(12,3).trimmed().toInt();
		oneSatlit.UTCTime.Seconds = tempLine.mid(15,11).trimmed().toDouble();
		oneSatlit.EpochFlag = tempLine.mid(26,3).trimmed().toInt();
		epochStalitNum = tempLine.mid(29,3).trimmed().toInt();//本历元卫星数目
		//判断形势（首先解析头文件）
		int headHang = epochStalitNum/12,headHangLast = epochStalitNum%12,tempPrn = 0;
		QVector< char > saveType;//保存解析的头文件类型
		QVector< int > savePRN;//保存解析的PRN
		QString tempSatName = "";//一个卫星的名字
		char tempCh = '0';
		for(int i = 0;i < headHang;i++)
		{
			for(int j = 0;j < 12;j++)
			{//一行最多存储12个卫星
				tempSatName = tempLine.mid(32+j*3,3);
				tempCh = *(tempSatName.mid(0,1).toLatin1().data());
				tempPrn = tempSatName.mid(1,2).trimmed().toInt();
				saveType.append(tempCh);
				savePRN.append(tempPrn);
			}
			tempLine = m_readOFileClass.readLine();
		}

		for(int i = 0;i < headHangLast;i++)
		{//读取不够一行的卫星（包括第一行不够一行）
			tempSatName = tempLine.mid(32+i*3,3);
			tempCh = *(tempSatName.mid(0,1).toLatin1().data());
			tempPrn = tempSatName.mid(1,2).trimmed().toInt();
			saveType.append(tempCh);
			savePRN.append(tempPrn);
		}
		if (headHangLast > 0)
		{//读取剩余进入数据行，区别于整数行
			tempLine = m_readOFileClass.readLine();
		}
		if (saveType.length() != epochStalitNum||savePRN.length()!=epochStalitNum)
			ErroTrace("Data erro!(QReadOFile::readEpochVer2)");

		//读取卫星数据
		QString dataString = "";
		int dataHang = (TypeObservNum/5.0 - TypeObservNum/5 == 0)?(TypeObservNum/5):(TypeObservNum/5 + 1);//观测数据行数等价于ceil(TypeObservNum)函数
		for (int i = 0;i < epochStalitNum;i++)
		{
			tempCh = saveType.at(i);
			tempPrn = savePRN.at(i);
			dataString = tempLine;//保存一行数据
			oneSatlit.SatType = tempCh;//类型
			oneSatlit.PRN = tempPrn;//PRN
			for (int j = 0;j < dataHang - 1;j++)//读取多余一行的数据
				dataString += m_readOFileClass.readLine(); 
			if (isInSystem(tempCh))
			{
				getOneSatlitData(dataString,oneSatlit);//获取卫星测距码和载波数据
				getFrequency(oneSatlit);//获取频率
				epochData.append(oneSatlit);//保存历元中的一颗卫星
			}
			if (i != epochStalitNum - 1)
			{//最后一颗卫星不要读取，留作本函数最上面去读取
				tempLine = m_readOFileClass.readLine();
			}
			
		}
		break;
	}
}

//读取一个历元数据（可根据版本进行扩展）
void QReadOFile::getEpochData(QVector< SatlitData > &epochData)
{
	if (RinexVersion >= 3)
		readEpochVer3(epochData);
	else if (RinexVersion < 3)
		readEpochVer2(epochData);
	return ;
}

//读取epochNum个历元数据(提前到文件底部可能不足epochNum个)
void QReadOFile::getMultEpochData(QVector< QVector< SatlitData > >&multEpochData,int epochNum)
{
	if (epochNum < 1) return ;
	int i = 0;
	while(!m_readOFileClass.atEnd())
	{
		if (i++ >= epochNum) break;
		QVector< SatlitData > epochData;
		getEpochData(epochData);
		multEpochData.append(epochData);
	}
}

//判断是否到达文件尾部（结束）
bool QReadOFile::isEnd()
{
	return m_readOFileClass.atEnd();
}

////////////////////////////下面获取一些头文件信息///////////////////////////////

//获取头文件注释信息
QString QReadOFile::getComment()
{
	return CommentInfo;
}

//获取近似坐标
void QReadOFile::getApproXYZ(double* AppXYZ)
{
	for (int i = 0;i < 3;i++)
		AppXYZ[i] = ApproxXYZ[i];
}

//获取天线的HEN改正
void QReadOFile::getAntHEN(double* m_AntHEM)
{
	for (int i = 0;i < 3;i++)
		m_AntHEM[i] = AntHEN[i];
}

//获得起始观测历元时间
void QReadOFile::getFistObsTime(int* m_YMDHM,double &Seconds)
{
	Seconds = ObsSeconds;
	for (int i = 0;i < 5;i++)
		m_YMDHM[i] = YMDHM[i];
}

//获得天线标志点名字
QString QReadOFile::getMakerName()
{
	return MarkerName;
}

//获取观测间隔（单位s）
double QReadOFile::getInterval()
{
	return IntervalSeconds;
}

//获取接收机天线类型
QString QReadOFile::getAntType()
{
	return AntType;

}

//获取接收机类型
QString QReadOFile::getReciveType()
{
	return ReciverType;
}