#include "QReadAnt.h"


QReadAnt::QReadAnt(QString AntFileName,QString AnTypeName,double ObvJD)
{
	initVar();
	if (!AnTypeName.isEmpty())		m_AntFileName = AntFileName;
	m_AntType = AnTypeName;
	m_ObeservTime = ObvJD;
}

QReadAnt::~QReadAnt(void)
{
}

//初始化变量
void QReadAnt::initVar()
{
	m_AntFileName = "antmod.atx";
	isReadAllData = false;
	isReadHead = false;
	isReadSatData = false;
	isReadRecvData = false;
	m_ObeservTime = 0;
	m_RecvData.IsSat = false;
	m_RecvData.isNan = true;//初始化假设可以找到天线
	m_RecvData.ValidJD = 0;
	m_RecvData.EndJD = 999999999;
	m_pi = 3.1415926535897932385;
	for (int i = 0;i < 3;i++)
	{
		m_sunpos[i] = 0;
		m_moonpos[i] = 0;
	}
	m_gmst = 0;
	m_sunSecFlag = -1;
}

//设置儒略日时间
void QReadAnt::setObsJD(QString AnTypeName,double ObsJD)
{
	m_AntType = AnTypeName;
	m_ObeservTime = ObsJD;
}

//计算儒略日
double QReadAnt::computeJD(int Year,int Month,int Day,int HoursInt,int Minutes,int Seconds)
{//计算儒略日
	double Hours = HoursInt + ((Minutes * 60) + Seconds)/3600.0;
	//Get JD
	double JD = 0.0;
	if(Month<=2)
		JD = (int)(365.25*(Year-1)) + (int)(30.6001*(Month+12+1)) + Day + Hours/24.0 + 1720981.5;
	else
		JD = (int)(365.25*(Year)) + (int)(30.6001*(Month+1)) + Day + Hours/24.0 + 1720981.5;
	return JD;
}

//打开文件
bool QReadAnt::openFiles(QString AntFileName)
{
	if (!AntFileName.isEmpty())
	{
		m_ReadFileClass.setFileName(AntFileName);
		if (!m_ReadFileClass.open(QFile::ReadOnly))//如果打开文件失败则......
		{
			m_ReadFileClass.setFileName("antmod.atx");//到当前目录打开
			if (!m_ReadFileClass.open(QFile::ReadOnly))
			{
				isReadAllData = true;
				return false;
			}
		}
		return true;
	}
	return false;
}

//读取头文件
bool QReadAnt::readFileAntHead()
{
	if (isReadHead)
		return true;
	m_tempLine = m_ReadFileClass.readLine();//读取第一行
	QString flagHeadEnd = "HEADER";
	QString endHeadStr = m_tempLine.mid(60,20).trimmed();
	while (!endHeadStr.contains(flagHeadEnd,Qt::CaseInsensitive))
	{
		//进行头文件数据读取......
		//此处跳过头文件

		//读取下一行
		m_tempLine = m_ReadFileClass.readLine();//读取下一行
		endHeadStr = m_tempLine.mid(60,20).trimmed();
	}
	m_tempLine = m_ReadFileClass.readLine();//读取下一行进入数据区域
	isReadHead = true;
	return true;
}

//读取卫星+接收机天线数据
bool QReadAnt::getAllData()
{
	if (m_AntFileName.isEmpty())
		isReadAllData = true;
	if (isReadAllData) return true;
	openFiles(m_AntFileName);
	readSatliData();
	readRecvData();
	isReadAllData = true;
	m_ReadFileClass.close();
	return true;
}

//读取接收机天线数据
bool QReadAnt::readRecvData()
{
	if (!isReadHead) readFileAntHead();
	if (isReadRecvData) return true;
	isReadRecvData = true;//只读取一次接收机天线数据（搜索不到不再多次搜索）
	long int currPos = m_ReadFileClass.pos();//保存当前指针位置
	//搜索天线类型
	while(!m_tempLine.mid(0,20).contains(m_AntType))
	{
		m_tempLine = m_ReadFileClass.readLine();
		if (m_ReadFileClass.atEnd())
		{//防止读取文件末尾死循环，并且标记false 表示未找到天线
			m_RecvData.isNan = false;
			return false;
		}
	}
	//读取接收机天线数据
	while (!m_tempLine.mid(60).contains("END OF ANTENNA"))
	{
		if (m_tempLine.mid(60).contains("TYPE / SERIAL NO"))//TYPE / SERIAL NO
		{
			m_RecvData.StrAntType = m_tempLine.mid(0,20);
			m_RecvData.SatliCNN = m_tempLine.mid(20,20);
			m_tempLine = m_ReadFileClass.readLine();//读取下一行数据

		}
		else if (m_tempLine.mid(60).contains("DAZI"))//DAZI
		{
			m_RecvData.DAZI = m_tempLine.mid(0,8).toDouble();
			m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
		}
		else if (m_tempLine.mid(60).contains("ZEN1 / ZEN2 / DZEN"))//ZEN1 / ZEN2 / DZEN
		{
			m_RecvData.ZEN_12N[0] = m_tempLine.mid(2,6).toDouble();
			m_RecvData.ZEN_12N[1] = m_tempLine.mid(8,6).toDouble();
			m_RecvData.ZEN_12N[2] = m_tempLine.mid(14,6).toDouble();
			m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
		}
		else if (m_tempLine.mid(60).contains("FREQUENCIES"))//# OF FREQUENCIES    
		{
			m_RecvData.NumFrqu = m_tempLine.mid(0,6).toDouble();
			m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
		}
		else if (m_tempLine.mid(60).contains("VALID FROM"))//VALID FROM
		{
			int Year = m_tempLine.mid(0,6).toInt();
			int Month = m_tempLine.mid(6,6).toInt();
			int Day = m_tempLine.mid(12,6).toInt();
			int Hour = m_tempLine.mid(18,6).toInt();
			int Minuts = m_tempLine.mid(24,6).toInt();
			int Seconds = m_tempLine.mid(30,13).toDouble();
			m_RecvData.ValidJD = computeJD(Year,Month,Day,Hour,Minuts,Seconds);
			m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
		}
		else if (m_tempLine.mid(60).contains("VALID UNTIL"))//VALID UNTIL
		{
			int Year = m_tempLine.mid(0,6).toInt();
			int Month = m_tempLine.mid(6,6).toInt();
			int Day = m_tempLine.mid(12,6).toInt();
			int Hour = m_tempLine.mid(18,6).toInt();
			int Minuts = m_tempLine.mid(24,6).toInt();
			int Seconds = m_tempLine.mid(30,13).toDouble();
			m_RecvData.EndJD = computeJD(Year,Month,Day,Hour,Minuts,Seconds);
			m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
		}
		else if (m_tempLine.mid(60).contains("START OF FREQUENCY"))//START OF FREQUENCY
		{//读取接收机PCO和PCV
			FrqData tempFrqData;
			tempFrqData.PCO[0] = 0;tempFrqData.PCO[1] = 0;tempFrqData.PCO[2] = 0;
			tempFrqData.FrqFlag = m_tempLine.mid(3,3);
			m_tempLine = m_ReadFileClass.readLine();
			if (m_tempLine.mid(60).contains("NORTH / EAST / UP"))//NORTH / EAST / UP
			{
				tempFrqData.PCO[0] = m_tempLine.mid(0,10).toDouble();
				tempFrqData.PCO[1] = m_tempLine.mid(10,10).toDouble();
				tempFrqData.PCO[2] = m_tempLine.mid(20,10).toDouble();
			}
			//读取NOZAI数据
			m_tempLine = m_ReadFileClass.readLine();
			int Hang = 0,lie = 0;
			if (m_RecvData.DAZI !=  0)
				Hang = (int)(360/m_RecvData.DAZI) + 1;
			lie = (int)((m_RecvData.ZEN_12N[1] - m_RecvData.ZEN_12N[0])/m_RecvData.ZEN_12N[2]) + 1;
			tempFrqData.Hang = Hang;
			tempFrqData.Lie = lie;
			if (m_tempLine.mid(0,8).contains("NOAZI"))
				for (int i = 0;i < lie;i++)
					tempFrqData.PCVNoAZI.append(m_tempLine.mid(8+i*8,8).toDouble());
			//Hang 不等于0则读取ZAI数据
			if (tempFrqData.Hang != 0)
			{
				for (int i = 0;i < tempFrqData.Hang;i++)
				{
					m_tempLine = m_ReadFileClass.readLine();//读取一行数据
					for (int j = 0;j < tempFrqData.Lie;j++)
					{
						tempFrqData.PCVAZI.append(m_tempLine.mid(8+j*8,8).toDouble());
					}
				}
			}
			m_RecvData.PCOPCV.append(tempFrqData);
			m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
		}
		else if (m_tempLine.mid(60).contains("END OF FREQUENCY"))//END OF FREQUENCY
		{
			m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
		}
		else
		{
			m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
		}
	}

	m_ReadFileClass.seek(currPos);//设置为起始搜索坐标
	return true;
}

//读取卫星系统天线数据
bool QReadAnt::readSatliData()
{
	if (!isReadHead) readFileAntHead();
	if(isReadSatData) return true;
	isReadSatData = true;//只读取一次卫星天线（搜索不到不再多次搜索）
	long int currPos = m_ReadFileClass.pos();//保存当前指针位置
	//找到开始读取数据
	while(!m_tempLine.mid(60).contains("START OF ANTENNA"))
		m_tempLine = m_ReadFileClass.readLine();
	//读取卫星天线数据
	int SatNum = 0;//记录保存有效的卫星个数 一定小于MaxSatlitNum
	char tempSatType = '0';
	bool IsGPS = false;//判断PRN是否进入离开GPS区域，再次离开时候中断 防止死循环
	while (!m_ReadFileClass.atEnd())
	{
		AntDataType tempSatData;
		tempSatData.IsSat = true;
		tempSatData.ValidJD = 0;
		tempSatData.EndJD = m_ObeservTime+1;
		//读取一个卫星天线数据
		while (!m_tempLine.mid(60).contains("END OF ANTENNA"))
		{
			if (m_tempLine.mid(60).contains("START OF ANTENNA"))//START OF ANTENNA
			{
				m_tempLine = m_ReadFileClass.readLine();
			} 
			else if (m_tempLine.mid(60).contains("TYPE / SERIAL NO"))//TYPE / SERIAL NO
			{
				tempSatData.StrAntType = m_tempLine.mid(0,20);
				tempSatData.SatliCNN = m_tempLine.mid(20,20).trimmed();//卫星系统的PRN
				m_tempLine = m_ReadFileClass.readLine();//读取下一行数据

			}
			else if (m_tempLine.mid(60).contains("DAZI"))//DAZI
			{
				tempSatData.DAZI = m_tempLine.mid(0,8).toDouble();
				m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
			}
			else if (m_tempLine.mid(60).contains("ZEN1 / ZEN2 / DZEN"))//ZEN1 / ZEN2 / DZEN
			{
				tempSatData.ZEN_12N[0] = m_tempLine.mid(2,6).toDouble();
				tempSatData.ZEN_12N[1] = m_tempLine.mid(8,6).toDouble();
				tempSatData.ZEN_12N[2] = m_tempLine.mid(14,6).toDouble();
				m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
			}
			else if (m_tempLine.mid(60).contains("FREQUENCIES"))//# OF FREQUENCIES    
			{
				tempSatData.NumFrqu = m_tempLine.mid(0,6).toInt();
				m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
			}
			else if (m_tempLine.mid(60).contains("VALID FROM"))//VALID FROM
			{
				int Year = m_tempLine.mid(0,6).toInt();
				int Month = m_tempLine.mid(6,6).toInt();
				int Day = m_tempLine.mid(12,6).toInt();
				int Hour = m_tempLine.mid(18,6).toInt();
				int Minuts = m_tempLine.mid(24,6).toInt();
				int Seconds = m_tempLine.mid(30,13).toDouble();
				tempSatData.ValidJD = computeJD(Year,Month,Day,Hour,Minuts,Seconds);
				m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
			}
			else if (m_tempLine.mid(60).contains("VALID UNTIL"))//VALID UNTIL
			{
				int Year = m_tempLine.mid(0,6).toInt();
				int Month = m_tempLine.mid(6,6).toInt();
				int Day = m_tempLine.mid(12,6).toInt();
				int Hour = m_tempLine.mid(18,6).toInt();
				int Minuts = m_tempLine.mid(24,6).toInt();
				int Seconds = m_tempLine.mid(30,13).toDouble();
				tempSatData.EndJD = computeJD(Year,Month,Day,Hour,Minuts,Seconds);
				m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
			}
			else if (m_tempLine.mid(60).contains("START OF FREQUENCY"))//START OF FREQUENCY
			{//读取卫星的PCO和PCV
				FrqData tempFrqData;
				tempFrqData.PCO[0] = 0;tempFrqData.PCO[1] = 0;tempFrqData.PCO[2] = 0;
				tempFrqData.FrqFlag = m_tempLine.mid(3,3);
				m_tempLine = m_ReadFileClass.readLine();
				if (m_tempLine.mid(60).contains("NORTH / EAST / UP"))//NORTH / EAST / UP
				{
					tempFrqData.PCO[0] = m_tempLine.mid(0,10).toDouble();
					tempFrqData.PCO[1] = m_tempLine.mid(10,10).toDouble();
					tempFrqData.PCO[2] = m_tempLine.mid(20,10).toDouble();
				}
				//读取NOZAI数据
				m_tempLine = m_ReadFileClass.readLine();
				int Hang = 0,lie = 0;
				if (tempSatData.DAZI !=  0)
					Hang = (int)(360/tempSatData.DAZI) + 1;
				lie = (int)((tempSatData.ZEN_12N[1] - tempSatData.ZEN_12N[0])/tempSatData.ZEN_12N[2]) + 1;
				tempFrqData.Hang = Hang;
				tempFrqData.Lie = lie;
				if (m_tempLine.mid(0,8).contains("NOAZI"))
					for (int i = 0;i < lie;i++)
						tempFrqData.PCVNoAZI.append(m_tempLine.mid(8+i*8,8).toDouble());
				//Hang 不等于0则读取ZAI数据
				if (tempFrqData.Hang != 0)
				{
					for (int i = 0;i < tempFrqData.Hang;i++)
					{
						m_tempLine = m_ReadFileClass.readLine();//读取一行数据
						for (int j = 0;j < tempFrqData.Lie;j++)
						{
							tempFrqData.PCVAZI.append(m_tempLine.mid(8+j*8,8).toDouble());
						}
					}
				}
				tempSatData.PCOPCV.append(tempFrqData);
				m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
			}
			else if (m_tempLine.mid(60).contains("END OF FREQUENCY"))//END OF FREQUENCY
			{
				m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
			}
			else
			{
				m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
			}//if elseif 结构
			if (m_ReadFileClass.atEnd())	break;
		}//while (!m_tempLine.mid(60).contains("END OF ANTENNA"))

		//判断是否是有效的数据
		tempSatType = *(tempSatData.SatliCNN.mid(0,1).toLatin1().data());
		if (tempSatData.ValidJD < m_ObeservTime&&tempSatData.EndJD > m_ObeservTime&&isInSystem(tempSatType))
		{
			m_SatData.append(tempSatData);
		}
		//不在四系统卫星天线区域跳出
		if(tempSatType!='G'&&tempSatType!='R'&&tempSatType!='C'&&tempSatType!='E')
			break;
		m_tempLine = m_ReadFileClass.readLine();//读取下一行数据
	}//while (!m_ReadFileClass.atEnd())
	m_ReadFileClass.seek(currPos);
	return true;
}

//双线性插值计算PCV
bool QReadAnt::getPCV(const AntDataType &tempAntData,char SatType,double *PCV12,double Ztop,double AZI/* = 0*/)
{
	QVector< double > XZen;//方位角相当于插值X坐标
	QVector< double > XAzi;//高度角相当于插值X坐标
	int flagZ = 0,flagA = 0;//插值左侧（较小）最邻近
	for (double i = tempAntData.ZEN_12N[0];i <= tempAntData.ZEN_12N[1];i+=tempAntData.ZEN_12N[2])
	{
		XZen.append(i*m_pi/180);
		if (i*m_pi/180 < Ztop)
			flagZ++;
	}
	flagZ--;
	if (tempAntData.DAZI !=0)
	{//存在矩阵形式的PCV数值表
		for (int i = 0;i <= 360;i+=tempAntData.DAZI)
		{
			XAzi.append(i*m_pi/180);
			if (i*m_pi/180 < AZI)
				flagA++;
		}
		flagA--;
	}
	
	/*计算PCV12(计算接收机天线改正需要，但是计算BDS和Galieo卫星天线，!!!!!!!!!!!!!!!
	不存在01和02,但卫星的PCO和PCV任何频段都一样也可以计算正确),不太完美*/
	FrqData PCOPCV1,PCOPCV2;
	QString destFlag1 = QString(SatType) + "01",destFlag2 = QString(SatType) + "02";
	bool isFind1 = false,isFind2 = false;
	for (int i = 0;i < tempAntData.PCOPCV.length();i++)
	{//此处只适用于GPS的L1和L2 以及GLONASS的G1和G2
		QString LFlag = tempAntData.PCOPCV.at(i).FrqFlag.trimmed();

		if (LFlag == destFlag1)
		{
			PCOPCV1 = tempAntData.PCOPCV.at(i);
			isFind1 = true;
		}
		else if (LFlag == destFlag2)
		{
			PCOPCV2 = tempAntData.PCOPCV.at(i);
			isFind2 = true;
		}
	}
	if (!(isFind1&&isFind2))
	{
		PCV12[0] = 0;
		PCV12[1] = 0;
		return false;
	}
	//计算PCV1
	if (PCOPCV1.PCVAZI.length() == 0)
	{//单线性插值
		if (flagZ < XZen.length() - 1)
		{
			double x1 = XZen.at(flagZ),x2 = XZen.at(flagZ+1);
			double y1 = PCOPCV1.PCVNoAZI.at(flagZ),y2 = PCOPCV1.PCVNoAZI.at(flagZ+1);
			PCV12[0] = y1+(Ztop - x1)*(y2-y1)/(x2 -x1);
		}
		else
			PCV12[0] = PCOPCV1.PCVNoAZI.at(flagZ);
	}
	else
	{//双线性插值
		if (flagZ >= XZen.length() - 1)		flagZ--;//判断越界即高度角大于90度一般不会
		if (flagA >= XAzi.length() - 1)		flagA--;//判断越界即方位角大于360度一般不会
		double x1 = XZen.at(flagZ),x2 = XZen.at(flagZ+1);
		double y1 = XAzi.at(flagA),y2 = XAzi.at(flagA+1);

		int HangNum = 0,LieNum = 0;//计算矩阵行列个数
		if (tempAntData.DAZI !=  0)
			HangNum = (int)(360/m_RecvData.DAZI) + 1;
		LieNum = (int)((tempAntData.ZEN_12N[1] - tempAntData.ZEN_12N[0])/tempAntData.ZEN_12N[2]) + 1;
		double z11 = PCOPCV1.PCVAZI.at(LieNum*flagA+flagZ),z12 = PCOPCV1.PCVAZI.at(LieNum*flagA+flagZ+1),
			z21 = PCOPCV1.PCVAZI.at(LieNum*(flagA+1)+flagZ),z22 = PCOPCV1.PCVAZI.at(LieNum*(flagA+1)+flagZ+1);

		double z1_2 = z11 + (Ztop - x1)*(z12 - z11)/(x2 - x1),
			   z2_2 = z21 + (Ztop - x1)*(z22 - z21)/(x2 - x1);

		PCV12[0] = z1_2 + (AZI - y1)*(z2_2 - z1_2)/(y2 - y1);
	}
	//计算PCV2
	if (PCOPCV2.PCVAZI.length() == 0)
	{//单线性插值
		if (flagZ < XZen.length() - 1)
		{
			double x1 = XZen.at(flagZ),x2 = XZen.at(flagZ+1);
			double y1 = PCOPCV2.PCVNoAZI.at(flagZ),y2 = PCOPCV2.PCVNoAZI.at(flagZ+1);
			PCV12[1] = y1+(Ztop - x1)*(y2-y1)/(x2 -x1);
		}
		else
			PCV12[1] = PCOPCV2.PCVNoAZI.at(flagZ);
		
	}
	else
	{//双线性插值
		if (flagZ >= XZen.length() - 1)		flagZ--;//判断越界即高度角大于90度一般不会
		if (flagA >= XAzi.length() - 1)		flagA--;//判断越界即方位角大于360度一般不会
		double x1 = XZen.at(flagZ),x2 = XZen.at(flagZ+1);
		double y1 = XAzi.at(flagA),y2 = XAzi.at(flagA+1);
		int HangNum = 0,LieNum = 0;//计算矩阵行列个数
		if (tempAntData.DAZI !=  0)
			HangNum = (int)(360/m_RecvData.DAZI) + 1;
		LieNum = (int)((tempAntData.ZEN_12N[1] - tempAntData.ZEN_12N[0])/tempAntData.ZEN_12N[2]) + 1;
		double z11 = PCOPCV2.PCVAZI.at(LieNum*flagA+flagZ),z12 = PCOPCV2.PCVAZI.at(LieNum*flagA+flagZ+1),
			z21 = PCOPCV2.PCVAZI.at(LieNum*(flagA+1)+flagZ),z22 = PCOPCV2.PCVAZI.at(LieNum*(flagA+1)+flagZ+1);
		double z1_2 = z11 + (Ztop - x1)*(z12 - z11)/(x2 - x1),
			z2_2 = z21 + (Ztop - x1)*(z22 - z21)/(x2 - x1);
		PCV12[1] = z1_2 + (AZI - y1)*(z2_2 - z1_2)/(y2 - y1);

	}
	return true;
}

//计算接收机天线的改正，将PCO和PCV改正到卫星信号方向
bool QReadAnt::getRecvL12(double E,double A,char SatType,double &L1Offset,double &L2Offset)
{
	L1Offset = 0;L2Offset = 0;
	if (!isInSystem(SatType))	return false;
	if (!m_RecvData.isNan) return false; 
	if (!isReadRecvData) readRecvData(); 
	FrqData PCOPCV1,PCOPCV2;
	QString destFlag1 = QString(SatType) + "01",destFlag2 = QString(SatType) + "02";
	bool IsFind1 = false,IsFind2 = false;
	for (int i = 0;i < m_RecvData.PCOPCV.length();i++)
	{/*此处只适用于GPS的L1和L2 以及GLONASS的G1和G2  但是计算BDS和Galieo卫星天线，!!!!!!!!!!!!!!!
		不存在01和02,但卫星的PCO和PCV任何频段都一样也可以计算正确),不太完美*/
		QString LFlag = m_RecvData.PCOPCV.at(i).FrqFlag.trimmed();
		
		if (LFlag == destFlag1)
		{
			PCOPCV1 = m_RecvData.PCOPCV.at(i);
			IsFind1 = true;
		}
		else if (LFlag == destFlag2)
		{
			PCOPCV2 = m_RecvData.PCOPCV.at(i);
			IsFind2 = true;
		}
	}
	if (!(IsFind1&&IsFind2))	return false;

	//获取PCO
	double *PCO1 = PCOPCV1.PCO,*PCO2 = PCOPCV2.PCO;
	//下面计算PCV
	double PCV12[2]={0};
	double Ztop = m_pi/2 - E;//高度角转换为天顶角
	getPCV(m_RecvData,SatType,PCV12,Ztop,A);
	//计算L1 L2在卫星信号入射方向的偏移量mm
	L1Offset = -PCV12[0] + PCO1[0]*qCos(E)*qCos(A)+PCO1[1]*qCos(E)*qSin(A)+PCO1[2]*qSin(E);
	L2Offset = -PCV12[1] + PCO2[0]*qCos(E)*qCos(A)+PCO2[1]*qCos(E)*qSin(A)+PCO2[2]*qSin(E);
	L1Offset = L1Offset/1000;
	L2Offset = L2Offset/1000;//返回米
	return true;
}

//计算接收机天线的改正，将PCO和PCV改正到卫星信号方向
double QReadAnt::getSatOffSet(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,int PRN,char SatType,double *StaPos,double *RecPos)
{
	if (!isReadSatData) return 0.0;//判断是否读取了数据
	if (!isInSystem(SatType))	return 0.0;	//判断是否在设定的系统内
	if (qAbs(m_sunSecFlag - Seconds) > 0.8)//两次只差大于0.8s认为更换历元重新计算太阳坐标
	{//每个历元计算一次太阳坐标 不需要重复 大量计算
		m_CmpClass.getSunMoonPos(Year,Month,Day,Hours,Minuts,Seconds,m_sunpos,m_moonpos,&m_gmst);//一个历元只需计算一次 不需要多次计算
		m_sunSecFlag = Seconds;
	}
	//计算星固系坐标轴在惯性系中单位向量表示ex,ey,ez方向轴要明确
	double ex[3]={0},ey[3]={0},ez[3]={0};
	double lenStaPos = qSqrt(m_CmpClass.InnerVector(StaPos,StaPos));//计算卫星向量长度
	//计算卫星与太阳叉积
	double vectSunSat[3] = {0},lenSunSta = 0.0;
	m_CmpClass.OutVector(StaPos,m_sunpos,vectSunSat);
	lenSunSta = qSqrt(m_CmpClass.InnerVector(vectSunSat,vectSunSat));
	//计算X轴长度向量
	double vectEZ[3] = {0},lenEZ = 0;
	m_CmpClass.OutVector(StaPos,vectSunSat,vectEZ);
	lenEZ = qSqrt(m_CmpClass.InnerVector(vectEZ,vectEZ));
	for (int i = 0;i < 3;i++)
	{
		ez[i] = -StaPos[i]/lenStaPos;
		ey[i] = -vectSunSat[i]/lenSunSta;
		ex[i] = -vectEZ[i]/lenEZ;
	}
	//获取卫星的PCO以及PCV(需要判断卫星的类型)
	AntDataType m_SatTemp;
	int tempPRN = 0;
	char tempSatType = '0';
	bool Isfind = false;
	for (int i = 0;i < m_SatData.length();i++)
	{//遍历查询所需卫星天线数据
		m_SatTemp = m_SatData.at(i);
		tempPRN = m_SatTemp.SatliCNN.mid(1,2).toInt();
		tempSatType = *(m_SatTemp.SatliCNN.mid(0,1).toLatin1().data());
		if (tempPRN == PRN&&tempSatType == SatType)
		{
			Isfind = true;
			break;
		}
	}
	//未找到该颗卫星天线数据
	if (!Isfind)	return 0.0;
	//PCO转换到地固坐标系ecef
	double PCOL1ecef[3] = {0},PCOL1[3] = {0};
	for (int i = 0;i < 3;i++)
	{
		PCOL1[i] = m_SatTemp.PCOPCV.at(0).PCO[i];
	}

	for (int i = 0;i < 3;i++)
	{
		PCOL1ecef[i] = ex[i]*PCOL1[0] + ey[i]*PCOL1[1] + ez[i]*PCOL1[2];
	}
	//计算卫星与接收机近似坐标向量
	double sat2recV[3] = {RecPos[0] - StaPos[0],RecPos[1] - StaPos[1],RecPos[2] - StaPos[2]};
	double lenSat2RecV = qSqrt(m_CmpClass.InnerVector(sat2recV,sat2recV));
	double sat2recVE[3] = {sat2recV[0]/lenSat2RecV,sat2recV[1]/lenSat2RecV,sat2recV[2]/lenSat2RecV};
	double AntHigPCOL1 = m_CmpClass.InnerVector(PCOL1ecef,sat2recVE); 

	//下面计算PCV
	double PCVAngle = qAcos(m_CmpClass.InnerVector(ez,sat2recVE));
	if (PCVAngle < 0||PCVAngle > 0.244346095)
	{
		PCVAngle = m_pi - PCVAngle;
	}
	double PCV12[2]={0}; 
	getPCV(m_SatTemp,SatType,PCV12,PCVAngle);

 	return (AntHigPCOL1 - PCV12[0])/1000;//返回单位（m）
}