#include "QPPPModel.h"

//初始化操作
void QPPPModel::initVar()
{
	for (int i = 0;i < 3;i++)
		m_ApproxRecPos[0] = 0;
	m_OFileName = "";
	multReadOFile = 1000;
	m_leapSeconds = 0;
}

//析构函数
QPPPModel::~QPPPModel()
{

}

//为空的构造函数
QPPPModel::QPPPModel(void)
{
	initVar();
}

//构造函数
QPPPModel::QPPPModel(QString OFileName,QStringList Sp3FileNames,QStringList ClkFileNames,QString ErpFileName,QString BlqFileName,QString AtxFileName,QString GrdFileName)
	:m_ReadOFileClass(OFileName),m_ReadSP3Class(Sp3FileNames),m_ReadClkClass(ClkFileNames),m_ReadTropClass(GrdFileName,"GMF"),
	m_ReadAntClass(AtxFileName),m_TideEffectClass(BlqFileName,ErpFileName)
{
//初始化变量
	initVar();

//保存文件名
	m_OFileName = OFileName;
	m_Sp3FileNames = Sp3FileNames;
	m_ClkFileNames = ClkFileNames;
//各种类的设置
	int obsTime[5] = {0};
	double Seconds = 0,ObsJD = 0;
	m_ReadOFileClass.getApproXYZ(m_ApproxRecPos);//获得O文件近似坐标
	m_ReadOFileClass.getFistObsTime(obsTime,Seconds);//获得起始观测时刻
	ObsJD = qCmpGpsT.computeJD(obsTime[0],obsTime[1],obsTime[2],obsTime[3],obsTime[4],Seconds);
	m_ReadAntClass.setObsJD(m_ReadOFileClass.getAntType(),ObsJD);//设置天线有效时间
	m_TideEffectClass.setStationName(m_ReadOFileClass.getMakerName());//设置海潮需要测站名字
//获取跳秒
	m_leapSeconds = qCmpGpsT.getLeapSecond(obsTime[0],obsTime[1],obsTime[2],obsTime[3],obsTime[4],Seconds);
//设置多系统数据
	//设置文件系统 SystemStr:"G"(开启GPS系统);"GR":(开启GPS+GLONASS系统);"GRCE"(全部开启)等
	//其中GPS,GLONASS,BDS,Galieo分别使用：字母G,R,C,E
	//QString SysStr = "GRCE";
	QString SysStr = "G";
	setSatlitSys(SysStr);

//读取需要用到的计算文件模块（耗时）
	m_ReadAntClass.getAllData();//读取卫星和接收机所有数据
	m_TideEffectClass.getAllData();//读取潮汐数据
	m_ReadTropClass.getAllData();//读取grd文件便于后面对流层计算
	m_ReadSP3Class.getAllData();//读取整个SP3文件
	m_ReadClkClass.getAllData();//读取钟误差文件便于后面计算
	
}

//设置文件系统 SystemStr:"G"(开启GPS系统);"GR":(开启GPS+GLONASS系统);"GRCE"(全部开启)等
//其中GPS,GLONASS,BDS,Galieo分别使用：字母G,R,C,E
bool QPPPModel::setSatlitSys(QString SystemStr)
{
	bool IsGood = QBaseObject::setSatlitSys(SystemStr);
	//设置读取O文件
	m_ReadOFileClass.setSatlitSys(SystemStr);
	//设置读取Sp3
	m_ReadSP3Class.setSatlitSys(SystemStr);
	//设置读取Clk文件
	m_ReadClkClass.setSatlitSys(SystemStr);
	//设置接收机卫星天线系统
	m_ReadAntClass.setSatlitSys(SystemStr);
	//设置kalman滤波的系统
	m_KalmanClass.setSatlitSys(SystemStr);
	return IsGood;
}

//从SP3数据数据获取本历元卫星的坐标
void QPPPModel::getSP3Pos(double GPST,int PRN,char SatType,double *p_XYZ,double *pdXYZ)
{
	m_ReadSP3Class.getPrcisePoint(PRN,SatType,GPST,p_XYZ,pdXYZ);
}

//从CLK数据中获取钟误差
void QPPPModel::getCLKData(int PRN,char SatType,double GPST,double *pCLKT)
{
	m_ReadClkClass.getStaliteClk(PRN,SatType,GPST,pCLKT);
}

//地球自传改正
double QPPPModel::getSagnac(double X,double Y,double Z,double *approxRecvXYZ)
{//计算地球自传改正
	double dltaP = M_We*((X - approxRecvXYZ[0])*Y - (Y - approxRecvXYZ[1])*X)/M_C;
	return -dltaP;//返回相反数使得p = p' + dltaP;可以直接相加
}

//计算相对论效应
double QPPPModel::getRelativty(double *pSatXYZ,double *pRecXYZ,double *pSatdXYZ)
{
	/*double c = 299792458.0;
	double dltaP = -2*(pSatXYZ[0]*pSatdXYZ[0] + pSatdXYZ[1]*pdXYZ[1] + pSatXYZ[2]*pSatdXYZ[2]) / c;*/
	double b[3] = {0},a = 0,R = 0,Rs = 0,Rr = 0,v_light = 299792458.0,GM=3.9860047e14,dltaP = 0;
	b[0] = pRecXYZ[0] - pSatXYZ[0];
	b[1] = pRecXYZ[1] - pSatXYZ[1];
	b[2] = pRecXYZ[2] - pSatXYZ[2];
	a = pSatXYZ[0]*pSatdXYZ[0] + pSatXYZ[1]*pSatdXYZ[1] + pSatXYZ[2]*pSatdXYZ[2];
	R=qCmpGpsT.norm(b,3);
	Rs = qCmpGpsT.norm(pSatXYZ,3);
	Rr = qCmpGpsT.norm(pRecXYZ,3);
	dltaP=-2*a/M_C + (2*M_GM/qPow(M_C,2))*qLn((Rs+Rr+R)/(Rs+Rr-R));
	return dltaP;//m
}

//计算EA，E：卫星高度角,A：方位角
void QPPPModel::getSatEA(double X,double Y,double Z,double *approxRecvXYZ,double *EA)
{//计算EA//出现BUG 由于计算XYZ转BLH时候 L（大地经度）竟然相反当y < 0 ,x > 0.L = -atan(y/x)错误 应该是 L = -atan(-y/x)
	double pSAZ[3] = {0};
	qCmpGpsT.XYZ2SAE(X,Y,Z,pSAZ,approxRecvXYZ);//出现Bug
	EA[0] = (PI/2 - pSAZ[2])*360/(2*PI);
	EA[1] = pSAZ[1]*360/(2*PI);
}

//使用Sass模型 还有其他可以使用的模型以及投影函数 以及GPT2模型
double QPPPModel::getTropDelay(double MJD,int TDay,double E,double *pBLH,double *mf)
{
	double GPT2_Trop = m_ReadTropClass.getGPT2SasstaMDelay(MJD,TDay,E,pBLH,mf);//GPT2模型 只返回干延迟估计和湿延迟函数
	//double UNB3m_Trop = m_ReadTropClass.getUNB3mDelay(pBLH,TDay,E,mf);//UNB3M模型 只返回干延迟估计和湿延迟函数
	return GPT2_Trop;
}

//计算接收机L1和L2相位中心改正PCO+PCV,L1Offset和L2Offset代表视线方向的距离改正
bool QPPPModel::getRecvOffset(double *EA,char SatType,double &L1Offset,double &L2Offset)
{
	if (m_ReadAntClass.getRecvL12(EA[0],EA[1],SatType,L1Offset,L2Offset))
		return true;
	else
	{
		L1Offset = 0; L2Offset = 0;
		return false;
	}
}

//计算卫星PCO+PCV改正，因为卫星 G1和G2频率一样所以两个波段改正是一样的；StaPos和RecPos，卫星和接收机WGS84坐标（单位m）
double QPPPModel::getSatlitOffset(int Year,int Month,int Day,int Hours,int Minutes,double Seconds,int PRN,char SatType,double *StaPos,double *RecPos)
{
	return m_ReadAntClass.getSatOffSet(Year,Month,Day,Hours,Minutes,Seconds,PRN,SatType,StaPos,RecPos);//pXYZ保存了卫星坐标
}

//计算潮汐在视线方向的改正（单位m）
double QPPPModel::getTideEffect(int Year,int Month,int Day,int Hours,int Minutes,double Seconds,double *pXYZ,double *EA,double *psunpos/* =NULL */, double *pmoonpos /* = NULL */,double gmst /* = 0 */,QString StationName /* = "" */)
{
	return m_TideEffectClass.getAllTideEffect(Year,Month,Day,Hours,Minutes,Seconds,pXYZ,EA,psunpos,pmoonpos,gmst,StationName);
}

//SatPos和RecPos代表卫星和接收机WGS84坐标 返回周（单位周）范围[-0.5 +0.5]
double QPPPModel::getWindup(int Year,int Month,int Day,int Hours,int Minutes,double Seconds,double *StaPos,double *RecPos,double &phw,double *psunpos)
{
	return m_WinUpClass.getWindUp(Year,Month,Day,Hours,Minutes,Seconds,StaPos,RecPos,phw,psunpos);
}

//检测周跳：返回pLP是三维数组，第一个是W-M组合（N2-N1 < 5） 第二个数电离层残差（<0.3） 第三个是（lamt2*N2-lamt1*N1 < 5）
bool QPPPModel::CycleSlip(const SatlitData &oneSatlitData,double *pLP)
{//
	if (oneSatlitData.L1*oneSatlitData.L2*oneSatlitData.C1*oneSatlitData.P2 == 0)//判断是否是双频数据
		return false; 
	double F1 = oneSatlitData.Frq[0],F2 = oneSatlitData.Frq[1];//获取本颗卫星的频率
	double Lamta1 = M_C/F1,Lamta2 = M_C/F2;

	double NL12 = ((F1-F2)/(F1+F2))*(oneSatlitData.C1/Lamta1 + oneSatlitData.P2/Lamta2) - (oneSatlitData.L1 - oneSatlitData.L2);
	double IocL = Lamta1*oneSatlitData.L1 - Lamta2*oneSatlitData.L2;
	double IocLP = IocL+(oneSatlitData.C1 - oneSatlitData.P2); 
	pLP[0] = NL12;
	pLP[1] =IocL;
	pLP[2] = IocLP;
	return true;
}

//保存前一个历元的相位缠绕
double QPPPModel::getPreEpochWindUp(QVector< SatlitData > &prevEpochSatlitData,int PRN,char SatType)
{
	int preEopchLen = prevEpochSatlitData.length();
	if (0 == preEopchLen) return 0;

	for (int i = 0;i < preEopchLen;i++)
	{
		SatlitData oneSatalite = prevEpochSatlitData.at(i);
		if (PRN == oneSatalite.PRN&&oneSatalite.SatType == SatType)
			return oneSatalite.AntWindup;
	}
	return 0;
}

//筛选不缺失数据，检测周跳，质量高（高度角，测距码等）的卫星
void QPPPModel::getGoodSatlite(QVector< SatlitData > &prevEpochSatlitData,QVector< SatlitData > &epochSatlitData,double eleAngle)
{
	//周跳检测
	QVector< int > CycleFlag;//记录周跳位置
	int preEpochLen = prevEpochSatlitData.length();
	int epochLen = epochSatlitData.length();
	CycleFlag.resize(epochLen);
	for (int i = 0;i < epochLen;i++) CycleFlag[i] = 0;
	for (int i = 0;i < epochLen;i++)
	{
		SatlitData epochData = epochSatlitData.at(i);
		//数据不为 0 
		if (!(epochData.L1&&epochData.L2&&epochData.C1&&epochData.P2))
			CycleFlag[i] = -1;
		//各项改正不为零
		if (!(epochData.X&&epochData.Y&&epochData.Z&&epochData.StaClock&&epochData.SatTrop))
			CycleFlag[i] = -1;
		//质量控制（高度角 伪距差值）
		if (epochData.EA[0] < eleAngle || (epochData.C1 - epochData.P2) > 50)
			CycleFlag[i] = -1;
		//周跳检测
		for (int j = 0;j < preEpochLen;j++)
		{
			SatlitData preEpochData = prevEpochSatlitData.at(j);
			if (epochData.PRN == preEpochData.PRN&&epochData.SatType == preEpochData.SatType)
			{//需要判断系统
				double epochLP[3]={0},preEpochLP[3]={0},diffLP[3]={0};
				CycleSlip(epochData,epochLP);
				CycleSlip(preEpochData,preEpochLP);
				for (int n = 0;n < 3;n++)
					diffLP[n] = qAbs(epochLP[n] - preEpochLP[n]);
				//根据经验确定周跳阈值吧
				if (diffLP[0] > 3.5||diffLP[1] > 0.3||diffLP[2] > 3.5||qAbs(epochData.AntWindup - preEpochData.AntWindup) > 0.3)
				{//发生周跳
					CycleFlag[i] = -1;//保存周跳卫星标志
				}
				break;
			}
			else
			{
				continue;
			}
		}
	}
	//删除低质量和周跳卫星
	QVector< SatlitData > tempEpochSatlitData;
	for (int i = 0;i < epochLen;i++)
	{
		if (CycleFlag.at(i) != -1)
		{
			tempEpochSatlitData.append(epochSatlitData.at(i));
		}
	}
	epochSatlitData = tempEpochSatlitData;
		
}

//读取各项改正之后的.ppp文件（自定义输出文件的格式,）
void QPPPModel::readPPPFile(QString pppFileName,QVector< QVector< SatlitData > > &allEpochData)
{
	QFile pppFile(pppFileName);
	if (!pppFile.open(QFile::ReadOnly))
	{
		QString erroInfo = "Open " + pppFileName + "faild!";
		ErroTrace(erroInfo);
		return ;
	}
	QString tempLine = "";
	//读取头文件
	bool endFileHead = false;//头文件结束标志
	do 
	{//此处为了试验只读取部分重要数据，以后需要完善
		tempLine = pppFile.readLine();//读取一行
		if (tempLine.indexOf("APPROX_POSITION") >= 0)
		{//读取近似坐标
			m_ApproxRecPos[0] = tempLine.mid(16,15).toDouble();
			m_ApproxRecPos[1] = tempLine.mid(32,15).toDouble();
			m_ApproxRecPos[2] = tempLine.mid(48,15).toDouble();
		}
		else if (tempLine.indexOf("PRN") >= 0)
		{
			endFileHead = true;//头文件结束
		}
	} while (!endFileHead);
	//读取数据部分
	bool endDataFlag = false;//整个文件读取结束标志
	do 
	{//读取每一个历元数据
		QVector < SatlitData > epochStaliData;//保存每个历元数据
		tempLine = pppFile.readLine();//数据段起始标志
		if (tempLine.indexOf("Number") >= 0)
		{
			double SatlitNum = tempLine.mid(17,3).toDouble();
			for (int i = 0;i < SatlitNum;i++)
			{//读取卫星各项改正
				SatlitData tempSatlitData;//存储单颗卫星数据
				//读取一行数据
				tempLine = pppFile.readLine();
				//读取PRN
				tempSatlitData.PRN = tempLine.mid(1,2).toInt();
				tempSatlitData.SatType = *(tempLine.mid(0,1).toLatin1().data());
				//计算频率
				if (tempSatlitData.SatType == 'G')
				{
					tempSatlitData.Frq[0] = M_F1;
					tempSatlitData.Frq[1] = M_F2;
				}
				else if(tempSatlitData.SatType == 'R')
				{
					tempSatlitData.Frq[0] = M_GLONASSF1(g_GlonassK[tempSatlitData.PRN - 1]);
					tempSatlitData.Frq[1] = M_GLONASSF2(g_GlonassK[tempSatlitData.PRN - 1]);
				}
				else
				{
					tempSatlitData.Frq[0] = 0;
					tempSatlitData.Frq[1] = 0;
				}
				double F1 = tempSatlitData.Frq[0],F2 = tempSatlitData.Frq[1];
				double Lamta1 = M_C/F1,Lamta2 = M_C/F2;
				double alpha1 = (F1*F1)/(F1*F1 - F2*F2),alpha2 = (F2*F2)/(F1*F1 - F2*F2);

				//读取坐标
				tempSatlitData.X = tempLine.mid(6,15).toDouble();
				tempSatlitData.Y = tempLine.mid(22,15).toDouble();
				tempSatlitData.Z = tempLine.mid(38,15).toDouble();
				//读取卫星钟误差
				tempSatlitData.StaClock = tempLine.mid(54,15).toDouble();
				//读取卫星高度角
				tempSatlitData.EA[0] = tempLine.mid(70,15).toDouble();
				tempSatlitData.EA[1] = tempLine.mid(86,15).toDouble();
				tempSatlitData.SatWight = (qSin(tempSatlitData.EA[0]*PI/180)*qSin(tempSatlitData.EA[0]*PI/180));//设定卫星的权重
				//读取卫星载波和伪距码
				tempSatlitData.C1 = tempLine.mid(102,15).toDouble();
				tempSatlitData.P2 = tempLine.mid(118,15).toDouble();
				tempSatlitData.L1 = tempLine.mid(134,15).toDouble();
				tempSatlitData.L2 = tempLine.mid(150,15).toDouble();
				//读取对流层延迟改正
				tempSatlitData.SatTrop = tempLine.mid(166,15).toDouble();
				tempSatlitData.StaTropMap = tempLine.mid(182,15).toDouble();
				//读取相对论改正
				tempSatlitData.Relativty = tempLine.mid(198,15).toDouble();;
				//读取地球自传改正
				tempSatlitData.Sagnac = tempLine.mid(214,15).toDouble();
				//读取潮汐影响改正
				tempSatlitData.TideEffect = tempLine.mid(230,15).toDouble();
				//读取接收机天线高改正
				tempSatlitData.AntHeight = tempLine.mid(246,15).toDouble();
				//读取卫星天线高改正
				tempSatlitData.SatOffset = tempLine.mid(262,15).toDouble();
				//读取接收机相位中心改正
				tempSatlitData.L1Offset = tempLine.mid(278,15).toDouble();
				tempSatlitData.L2Offset = tempLine.mid(294,15).toDouble();
				//读取windup改正
				tempSatlitData.AntWindup = tempLine.mid(310,15).toDouble();
				//计算LL3和PP3（修改为多系统）
				tempSatlitData.LL3 = alpha1*(tempSatlitData.L1 + tempSatlitData.L1Offset - tempSatlitData.AntWindup)*Lamta1 - alpha2*(tempSatlitData.L2 + tempSatlitData.L2Offset - tempSatlitData.AntWindup)*Lamta2;//消去电离层载波LL3
				tempSatlitData.PP3 = alpha1*(tempSatlitData.C1 + Lamta1*tempSatlitData.L1Offset) - alpha2*(tempSatlitData.P2 + Lamta2 *tempSatlitData.L2Offset);//消去电离层载波PP3
				//保存每一颗卫星数据
				epochStaliData.append(tempSatlitData);
			}
			allEpochData.append(epochStaliData);//保存每个历元数据

		}//if (tempLine.indexOf("satellite") >= 0)

		if (pppFile.atEnd())
		{
			endDataFlag = true;
		}

	} while (!endDataFlag);
}

//读取.ppp文件运行kalman滤波
void QPPPModel::Run(QString pppFileName)
{
	QVector < QVector < SatlitData > > allEpochData;
	readPPPFile(pppFileName,allEpochData);//读取.ppp所有数据
	QVector < SatlitData > prevEpochSatlitData,epochSatlitData;//存储上一个历元的卫星数据，周跳检测使用
	RecivePos epochRecivePos;
	for (int i = 0; i < allEpochData.length();i++)
	{
		if (!epochSatlitData.isEmpty())
		{
			prevEpochSatlitData = epochSatlitData;//保存上个历元数据
			epochSatlitData.clear();
		}
		epochSatlitData = allEpochData.at(i);//获取本历元数据
		m_writeFileClass.allPPPSatlitData.append(epochSatlitData);//保存数据写入.sp3文件
		//监测卫星质量和周跳
		getGoodSatlite(prevEpochSatlitData,epochSatlitData);
		//Kalman滤波
		MatrixXd P;
		VectorXd X;//分别为dX,dY,dZ,dT(天顶对流层残差),dVt(接收机钟差)，N1,N2...Nm(模糊度)
		m_KalmanClass.KalmanforStatic(prevEpochSatlitData,epochSatlitData,m_ApproxRecPos,X,P);

		int epochLen = epochSatlitData.length();
		double pENU[3] = {0};
		qCmpGpsT.XYZ2ENU(X(0)+m_ApproxRecPos[0],X(1)+m_ApproxRecPos[1],X(2)+m_ApproxRecPos[2],pENU,m_ApproxRecPos);
		qDebug()<<"____Print ENU____";
		qDebug()<<epochLen;
		for (int i = 0;i < 3;i++)
		{
			qDebug()<<pENU[i];
		}
		qDebug()<<"______END X______";
		//保存接收机数据写入到文件
		epochRecivePos.Year = 0;
		epochRecivePos.Month = 0;
		epochRecivePos.Day = 0;
		epochRecivePos.Hours = 0;
		epochRecivePos.Minutes = 0;
		epochRecivePos.Seconds = 0;
		epochRecivePos.totolEpochStalitNum = epochLen;
		epochRecivePos.dX = pENU[0];
		epochRecivePos.dY = pENU[1];
		epochRecivePos.dZ = pENU[2];
		m_writeFileClass.allReciverPos.append(epochRecivePos);
	}//所有历元计算完毕

	//写入结果到文件
	m_writeFileClass.writeRecivePos2Txt("Kalman.txt");
	m_writeFileClass.writePPP2Txt("Kalman.ppp");
}

int FlagN = 0;
//读取O文件，sp3文件，clk文件,以及各项误差计算，Kalman滤波 ......................
void QPPPModel::Run()
{	
	//外部初始化固定变量 加快计算速度
	double p_HEN[3] = {0};//获取天线高
	m_ReadOFileClass.getAntHEN(p_HEN);

	//逐个历元遍历数据 读取O文件数据
	QVector < SatlitData > prevEpochSatlitData;//存储上一个历元的卫星数据，周跳检测使用（放在最上面否则读取multReadOFile个历元，在读取就会生命周期到期）
	while (!m_ReadOFileClass.isEnd())
	{
		QVector< QVector < SatlitData > > multepochSatlitData;//存储多个历元数据
		m_ReadOFileClass.getMultEpochData(multepochSatlitData,multReadOFile);//读取multReadOFile个历元
//多个历元循环计算
		for (int n = 0; n < multepochSatlitData.length();n++)
		{
			qDebug()<<FlagN++;
			QVector< SatlitData > epochSatlitData;//临时存储每个历元卫星未计算数据
			QVector< SatlitData > epochResultSatlitData;// 存储每个历元卫星计算完各项改正数据
			
			epochSatlitData = multepochSatlitData.at(n);
			GPSPosTime epochTime = epochSatlitData.at(0).UTCTime;//获取观测时刻（历元每个卫星都存储了观测时间）
			//Debug
			RecivePos epochRecivePos;
			qDebug()<<epochTime.Hours<<":"<<epochTime.Minutes<<":"
				<<epochTime.Seconds<<" SatalitNum= "<<epochSatlitData.length();
			epochRecivePos.Year = epochTime.Year;epochRecivePos.Month = epochTime.Month;
			epochRecivePos.Day = epochTime.Day;epochRecivePos.Hours = epochTime.Hours;
			epochRecivePos.Minutes = epochTime.Minutes;epochRecivePos.Seconds = epochTime.Seconds;
//一个历元循环开始
			for (int i = 0;i < epochSatlitData.length();i++)
			{
				
				SatlitData tempSatlitData = epochSatlitData.at(i);//存储计算的各项改正卫星数据

				//求取GPS时
				double m_PrnGpst = qCmpGpsT.YMD2GPSTime(epochTime.Year,epochTime.Month,epochTime.Day,
					epochTime.Hours,epochTime.Minutes,epochTime.Seconds);
				//从CLK文件读取卫星钟误差
				double stalitClock = 0;//单位m
				getCLKData(tempSatlitData.PRN,tempSatlitData.SatType,m_PrnGpst - tempSatlitData.P2/M_C,&stalitClock);
				tempSatlitData.StaClock = stalitClock;
				//从SP3数据数据获取本历元卫星的坐标
				double pXYZ[3] = {0},pdXYZ[3] = {0};//单位m
				getSP3Pos(m_PrnGpst - tempSatlitData.P2/M_C - tempSatlitData.StaClock/M_C,tempSatlitData.PRN,tempSatlitData.SatType,pXYZ,pdXYZ);//获取卫星发射时刻的精密星历坐标(这里需要减去卫星的钟误差 tempSatlitData.StaClock/M_C 否则导致收敛差距20cm)
				tempSatlitData.X = pXYZ[0];tempSatlitData.Y = pXYZ[1];tempSatlitData.Z = pXYZ[2];
//测试一下精密星历和钟差状态以及载波、伪距是否异常
				if (!(tempSatlitData.X&&tempSatlitData.Y&&tempSatlitData.Z&&tempSatlitData.StaClock&&tempSatlitData.L1&&tempSatlitData.L2&&tempSatlitData.C1&&tempSatlitData.P2))
				{
					epochResultSatlitData.append(tempSatlitData);
					continue;
				}
//计算波长（主要针对多系统）
				double F1 = tempSatlitData.Frq[0],F2 = tempSatlitData.Frq[1];
				double Lamta1 = M_C/F1,Lamta2 = M_C/F2;
				double alpha1 = (F1*F1)/(F1*F1 - F2*F2),alpha2 = (F2*F2)/(F1*F1 - F2*F2);
				//计算相对论改正
				double relative = 0;
				relative = getRelativty(pXYZ,m_ApproxRecPos,pdXYZ);
				tempSatlitData.Relativty = relative;
				//计算卫星高坐度角(随着接收机近似标变化)
				double EA[2]={0};
				getSatEA(tempSatlitData.X,tempSatlitData.Y,tempSatlitData.Z,m_ApproxRecPos,EA);
				tempSatlitData.EA[0] = EA[0];tempSatlitData.EA[1] = EA[1];
				EA[0] = EA[0]*PI/180;EA[1] = EA[1]*PI/180;//转到弧度方便下面计算
				tempSatlitData.SatWight = (qSin(EA[0])*qSin(EA[0]));//设定卫星的权重
				//计算地球自传改正
				double earthW = 0;
				earthW = getSagnac(tempSatlitData.X,tempSatlitData.Y,tempSatlitData.Z,m_ApproxRecPos);
				tempSatlitData.Sagnac = earthW;
				//计算对流层干延迟
				double TropDelay = 0,TropMap = 0;
				double MJD = qCmpGpsT.computeJD(epochTime.Year,epochTime.Month,epochTime.Day,
					epochTime.Hours,epochTime.Minutes,epochTime.Seconds) - 2400000.5;//简化的儒略日
				//计算保存年积日
				double TDay = qCmpGpsT.YearAccDay(epochTime.Year,epochTime.Month,epochTime.Day);
				double p_BLH[3] = {0},mf = 0;
				qCmpGpsT.XYZ2BLH(m_ApproxRecPos[0],m_ApproxRecPos[1],m_ApproxRecPos[2],p_BLH);
				TropDelay = getTropDelay(MJD,TDay,EA[0],p_BLH,&mf);
				tempSatlitData.SatTrop = TropDelay;
				tempSatlitData.StaTropMap = mf;
				//计算天线高偏移改正 Antenna Height
				tempSatlitData.AntHeight = p_HEN[0]*qSin(EA[0]);
				//接收机L1 L2偏移改正
				double L1Offset = 0,L2Offset = 0;
				getRecvOffset(EA,tempSatlitData.SatType,L1Offset,L2Offset);
				tempSatlitData.L1Offset = L1Offset/Lamta1;
				tempSatlitData.L2Offset = L2Offset/Lamta2;
				//卫星天线相位中心改正
				double SatAnt = 0.0;
				SatAnt = getSatlitOffset(epochTime.Year,epochTime.Month,epochTime.Day,
					epochTime.Hours,epochTime.Minutes,epochTime.Seconds - tempSatlitData.P2/M_C,
					tempSatlitData.PRN,tempSatlitData.SatType,pXYZ,m_ApproxRecPos);//pXYZ保存了卫星坐标
				tempSatlitData.SatOffset = SatAnt;
				//计算潮汐改正
				double pENU[3] = {0},effctDistance = 0;
				effctDistance = getTideEffect(epochTime.Year,epochTime.Month,epochTime.Day,
					epochTime.Hours,epochTime.Minutes,epochTime.Seconds,m_ApproxRecPos,EA,
					m_ReadAntClass.m_sunpos,m_ReadAntClass.m_moonpos,m_ReadAntClass.m_gmst);
				tempSatlitData.TideEffect = effctDistance;
				//计算天线相位缠绕
				double AntWindup = 0,preAntWindup = 0;
				//查找前一个历元 是否存在本颗卫星 存在取出存入preAntWindup 否则preAntWindup=0
				preAntWindup = getPreEpochWindUp(prevEpochSatlitData,tempSatlitData.PRN,tempSatlitData.SatType);//获取前一个历元的WindUp
				AntWindup = getWindup(epochTime.Year,epochTime.Month,epochTime.Day,
					epochTime.Hours,epochTime.Minutes,epochTime.Seconds - tempSatlitData.P2/M_C,
					pXYZ,m_ApproxRecPos,preAntWindup,m_ReadAntClass.m_sunpos);
				tempSatlitData.AntWindup = AntWindup;

				//计算消除电离层伪距和载波组合(此处吸收了接收机载波偏转和WindUp)
				tempSatlitData.LL3 = alpha1*(tempSatlitData.L1 + tempSatlitData.L1Offset - tempSatlitData.AntWindup)*Lamta1 - alpha2*(tempSatlitData.L2 + tempSatlitData.L2Offset - tempSatlitData.AntWindup)*Lamta2;//消去电离层载波LL3
				tempSatlitData.PP3 = alpha1*(tempSatlitData.C1 + Lamta1*tempSatlitData.L1Offset) - alpha2*(tempSatlitData.P2 + Lamta2 *tempSatlitData.L2Offset);//消去电离层载波PP3

				//保存一个历元卫星数据 
				epochResultSatlitData.append(tempSatlitData);
			}
//一个历元结束for (int i = 0;i < epochSatlitData.length();i++)

			m_writeFileClass.allPPPSatlitData.append(epochResultSatlitData);
			//监测卫星质量和周跳
			getGoodSatlite(prevEpochSatlitData,epochResultSatlitData);
			
			//Kalman滤波
			MatrixXd P;
			VectorXd X;//分别为dX,dY,dZ,dT(天顶对流层残差),dVt(接收机钟差)，N1,N2...Nm(模糊度)
			m_KalmanClass.KalmanforStatic(prevEpochSatlitData,epochResultSatlitData,m_ApproxRecPos,X,P);
			//保存上个历元卫星数据
			prevEpochSatlitData = epochResultSatlitData;

			//输出计算结果
			int epochLen = epochResultSatlitData.length();
			double pENU[3] = {0};
			qCmpGpsT.XYZ2ENU(X(0)+m_ApproxRecPos[0],X(1)+m_ApproxRecPos[1],X(2)+m_ApproxRecPos[2],pENU,m_ApproxRecPos);
			qDebug()<<"____Print ENU____";
			qDebug()<<epochLen;
			for (int i = 0;i < 3;i++)
			{
				qDebug()<<pENU[i];
			}

			qDebug()<<"______END________";
//保存接收机数据写入到文件
			//存储坐标数据
			epochRecivePos.totolEpochStalitNum = epochLen;
			epochRecivePos.dX = X[0];
			epochRecivePos.dY = X[1];
			epochRecivePos.dZ = X[2];
			m_writeFileClass.allReciverPos.append(epochRecivePos);
			//保存接收机钟差
			ClockData epochRecClock;
			epochRecClock.UTCTime.Year = epochRecivePos.Year;epochRecClock.UTCTime.Month = epochRecivePos.Month;epochRecClock.UTCTime.Day = epochRecivePos.Day;
			epochRecClock.UTCTime.Hours = epochRecivePos.Hours;epochRecClock.UTCTime.Minutes = epochRecivePos.Minutes;epochRecClock.UTCTime.Seconds = epochRecivePos.Seconds;
			epochRecClock.ZTD_W = X(3);//存储湿延迟
			epochRecClock.clockData = X(4);//存储接收机钟差
			m_writeFileClass.allClock.append(epochRecClock);
			//保存卫星模糊度
			Ambiguity oneSatAmb;
			for (int i = 0;i < epochResultSatlitData.length();i++)
			{
				SatlitData oneSat = epochSatlitData.at(i);
				oneSatAmb.PRN = oneSat.PRN;
				oneSatAmb.SatType = oneSat.SatType;
				oneSatAmb.UTCTime = epochRecClock.UTCTime;
				oneSatAmb.isIntAmb = false;
				oneSatAmb.Amb = X(i+5);
				oneSatAmb.epochNum = FlagN;
				m_writeFileClass.allAmbiguity.append(oneSatAmb);
			}
		}
//多个历元结束for (int n = 0; n < multepochSatlitData.length();n++)
		
	}
//所有历元结束while (!m_ReadOFileClass.isEnd())

	//写入结果到文件
	m_writeFileClass.WriteEpochPRN(".//Product//Epoch_PRN.txt");
	m_writeFileClass.writeRecivePos2Txt(".//Product//Kalman.txt");
	m_writeFileClass.writePPP2Txt(".//Product//Kalman.ppp");
	m_writeFileClass.writeClockZTDW2Txt(".//Product//ZTDW_Clock.txt");
	m_writeFileClass.writeAmbiguity2Txt();//路径为.//Ambiguity//
}

