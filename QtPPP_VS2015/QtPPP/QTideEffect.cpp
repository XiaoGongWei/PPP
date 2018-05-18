#include "QTideEffect.h"

const double  QTideEffect::args[][5]={
	{1.40519E-4, 2.0,-2.0, 0.0, 0.00},  /* M2 */
	{1.45444E-4, 0.0, 0.0, 0.0, 0.00},  /* S2 */
	{1.37880E-4, 2.0,-3.0, 1.0, 0.00},  /* N2 */
	{1.45842E-4, 2.0, 0.0, 0.0, 0.00},  /* K2 */
	{0.72921E-4, 1.0, 0.0, 0.0, 0.25},  /* K1 */ 
	{0.67598E-4, 1.0,-2.0, 0.0,-0.25},  /* O1 */
	{0.72523E-4,-1.0, 0.0, 0.0,-0.25},  /* P1 */
	{0.64959E-4, 1.0,-3.0, 1.0,-0.25},  /* Q1 */
	{0.53234E-5, 0.0, 2.0, 0.0, 0.00},  /* Mf */
	{0.26392E-5, 0.0, 1.0,-1.0, 0.00},  /* Mm */
	{0.03982E-5, 2.0, 0.0, 0.0, 0.00}   /* Ssa */
};

void QTideEffect::initVar()
{
	//初始化星体GM参数
	m_GMi[0] = 3.986004415e14;
	m_GMi[1] = 4.9027890e12;
	m_GMi[2] = 1.3271250e20;
	//love和Shadi参数
	loveShida2[0] = 0;loveShida2[1] = 0;
	isgetLoveShida2 = false;
	loveShida3[0] = 0.2920;loveShida3[1] = 0.0150;
	m_SationBLH[0] = 0;m_SationBLH[1] = 0;m_SationBLH[2] = 0;
	//初始化erp_t 数据
	m_erpData.n = 0;
	m_erpData.nmax = 10;
	m_erpData.data = (erpd_t*)malloc(sizeof(erpd_t)*m_erpData.nmax);
	isPoleEffect = true;
	isSolidTide = true;
	isOCEANTide = true;
	m_SecondFlag = -1;
	m_AllTideENU[0] = 0;m_AllTideENU[1] = 0;m_AllTideENU[2] = 0;
	m_epochFlag = 0;
	m_MaxCompution = 50;
	for (int i = 0;i < 3;i++)
	{
		m_pSolidENU[i] = 0;m_pPoleENU[i] = 0;m_pOCEANENU[i] = 0;
		m_sunpos[i] = 0;m_moonpos[i] = 0;
	}
	m_gmst = 0;isGetPos = false;
	for (int i = 0;i < 5;i++)
		m_erpV[i] = 0;
	isReadErp = false;
	isReadOCEAN = false;
	LeapSeconds = 0;
	m_OCEANData.isRead = false;//不可用
	m_StationName = "";
}

//设置太阳、月亮、gmst数据
void QTideEffect::setSunMoonPos(double *psun,double *pmoon,double gmst/* =0 */)
{
	for (int i = 0;i < 3;i++)
	{
		m_sunpos[i] = psun[i];
		m_moonpos[i] = pmoon[i];
	}
	m_gmst = gmst;
	isGetPos = true;
}

//设置站名 用于极潮搜索数据
void QTideEffect::setStationName(QString StationName /* = "" */)
{
	m_StationName = StationName.trimmed();
}

//传入海洋数据,erp文件路径，否则就从当前目录搜索 否则就不适用海洋潮汐和极潮改正
QTideEffect::QTideEffect(QString OCEANFileName,QString erpFileName)
{
	initVar();
	m_erpFileName = erpFileName.trimmed();
	m_OCEANFileName = OCEANFileName.trimmed();
	
}

QTideEffect::~QTideEffect(void)
{
	free(m_erpData.data);
}

bool QTideEffect::readRepFile()
{
	if (isReadErp)	return true;
	//读取erp文件参数
	QDir m_dir(".");
	QStringList m_fliterList;
	//如果存在erp文件则使用 否则就不使用
	m_fliterList.append("*.erp");
	QStringList ErpFileNameList = m_dir.entryList(m_fliterList); 
	QString ErpfileName ="";
	if (!ErpFileNameList.isEmpty())
		ErpfileName = ErpFileNameList.at(0);

	if (!m_erpFileName.isEmpty())
	{//首先判断用户是否传入erp路径
		if (!m_cmpClass.readerp(m_erpFileName.toLatin1().data(),&m_erpData))
			isPoleEffect = false;
		
	}
	else if (!ErpfileName.isEmpty())
	{//若没有传入则读取当前目录erp文件
		if (!m_cmpClass.readerp(ErpfileName.toLatin1().data(),&m_erpData))
			isPoleEffect = false;
	}
	else
	{
		isPoleEffect = false;
	}
	isReadErp = true;
	return true;
}

//double *erpv       O   erp values {xp,yp,ut1_utc,lod} (rad,rad,s,s/d)
bool  QTideEffect::getErpV(gtime_t obsGPST,double *erpV)
{
	if(!m_cmpClass.geterp(&m_erpData,obsGPST,erpV)) return false;
	return true;
}

void QTideEffect::tide_pole(const double *pos, const double *erpv, double *denu)
{
	double xp,yp,cosl,sinl;

	//trace(3,"tide_pole: pos=%.3f %.3f\n",pos[0]*R2D,pos[1]*R2D);
	xp=erpv[0]/AS2R; /* rad -> arcsec */
	yp=erpv[1]/AS2R;
	cosl=cos(pos[1]); sinl=sin(pos[1]);
	denu[0]=  9E-3*sin(pos[0])    *(xp*sinl+yp*cosl);
	denu[1]= -9E-3*cos(2.0*pos[0])*(xp*cosl-yp*sinl);
	denu[2]=-32E-3*sin(2.0*pos[0])*(xp*cosl-yp*sinl);

	//trace(5,"tide_pole : denu=%.3f %.3f %.3f\n",denu[0],denu[1],denu[2]);
}

//提供极潮接口,一天不会变化，只要BLH精确就可以，注意代码减少计算量
void QTideEffect::getPoleTide(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,double *pBLH,double *pTideENU)
{
	pTideENU[0] = 0;pTideENU[1] = 0;pTideENU[2] = 0;
	if (!isPoleEffect) return ;
	double tSeconds = 0;
	int tWeek;
	gtime_t obsGPST;
	pTideENU[0] = 0;pTideENU[1] = 0;pTideENU[2] = 0;
	tSeconds = m_cmpClass.YMD2GPSTime(Year,Month,Day,Hours,Minuts,Seconds,&tWeek);
	obsGPST = m_cmpClass.gpst2time(tWeek,Seconds);
	//得到erp参数
	//if (!isgetErpV)
	if(!getErpV(obsGPST,m_erpV))
		for (int i = 0;i < 5;i++)
			m_erpV[i] = 0;
	//计算极移潮汐
	tide_pole(pBLH,m_erpV,pTideENU);
}

void QTideEffect::subSolidTide(double *sunmoonPos,double *pXYZ,double *pTideXYZ,int flag)
{//求取二次潮
	//求取二次潮
	double esunmoon[3]={0},erec[3]={0};//太阳或月亮、测站的单位矢量
	double lensunmoon = 0,lenrec = 0;
	//求取向量长度
	lensunmoon = m_cmpClass.norm(sunmoonPos,3);
	lenrec = m_cmpClass.norm(pXYZ,3);
	//单位化
	for (int i = 0; i < 3;i++)
	{
		esunmoon[i] = sunmoonPos[i]/lensunmoon;
		erec[i] = pXYZ[i]/lenrec;
	}
	//二次固体潮影响
	if (flag<0||flag>1 ) return;
	double GMe = m_GMi[0],GMj = m_GMi[flag+1];//获取天体引力参数
	double K1 = 0,K2 = 0,K3 = 0,dotsunRec = 0;
	dotsunRec = m_cmpClass.dot(esunmoon,erec,3);
	K1 = GMj*lenrec*lenrec*lenrec*lenrec/(GMe*lensunmoon*lensunmoon*lensunmoon);
	K2 = 3*loveShida2[1]*dotsunRec;
	K3 = 3*(loveShida2[0]/2 - loveShida2[1])*(dotsunRec*dotsunRec) - loveShida2[0]/2;

	pTideXYZ[0] = K1*(K2*esunmoon[0] + K3*erec[0]);
	pTideXYZ[1] = K1*(K2*esunmoon[1] + K3*erec[1]);
	pTideXYZ[2] = K1*(K2*esunmoon[2] + K3*erec[2]);
}

void QTideEffect::getSoildTide(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,double *pXYZ,double *pTideENU,bool isElimate/* = false*/)
{
	pTideENU[0] = 0;pTideENU[1] = 0;pTideENU[2] = 0;
	if (!isSolidTide)	return ; 
	double sunpos[3]={0},moonpos[3] = {0},gmst = 0;
	double pBLH[3] = {0},pmoon2XYZ[3] = {0},psun2XYZ[3]={0},pmoon3XYZ[3] = {0},sinb2 = 0;
	//随着测站计算精度提高BLH也要实时更新（增加了计算量）
	if (m_epochFlag < m_MaxCompution)
	{//更新BLH并且获得love数
		m_epochFlag++;
		m_cmpClass.XYZ2BLH(pXYZ,pBLH);
		m_SationBLH[0] = pBLH[0];m_SationBLH[1] = pBLH[1];m_SationBLH[2] = pBLH[2];
		sinb2 = ( 3*qSin(pBLH[0])*qSin(pBLH[0]) - 1 )/2;
		loveShida2[0] = 0.6078 - 0.0006*sinb2;
		loveShida2[1] = 0.0847 + 0.0002*sinb2;
	}
	if (isGetPos)//外部传入太阳月亮数据
	{
		sunpos[0] = m_sunpos[0];sunpos[1] = m_sunpos[1];sunpos[2] = m_sunpos[2];
		moonpos[0] = m_moonpos[0];moonpos[1] = m_moonpos[1];moonpos[2] = m_moonpos[2];
		gmst = m_gmst;
	}
	else
		m_cmpClass.getSunMoonPos(Year,Month,Day,Hours,Minuts,Seconds,sunpos,moonpos,&gmst);
	//二次固体潮计算
	subSolidTide(moonpos,pXYZ,pmoon2XYZ,0);//月亮
	subSolidTide(sunpos,pXYZ,psun2XYZ,1);//太阳
	//三次固体潮只考虑月亮U和N方向
	double emoon[3] = {0},erec[3] = {0};
	double lenmoon = 0,lenrec = 0;
	double K1 = 0,K2 = 0,K3 = 0,dotmoonRec = 0;
	double GMe = m_GMi[0],GMm = m_GMi[1];//获取天体引力参数
	lenmoon = m_cmpClass.norm(moonpos,3);
	lenrec = m_cmpClass.norm(pXYZ,3);
	//单位化
	for (int i = 0; i < 3;i++)
	{
		emoon[i] = moonpos[i]/lenmoon;
		erec[i] = pXYZ[i]/lenrec;
	}
	dotmoonRec = m_cmpClass.dot(emoon,erec,3);
	K1 = GMm*lenrec*lenrec*lenrec*lenrec*lenrec/(GMe*lenmoon*lenmoon*lenmoon*lenmoon);
	K2 = loveShida3[1]*( 7.5*dotmoonRec*dotmoonRec - 1.5);
	K3 = 2.5*(loveShida3[0] - 3*loveShida3[1])*dotmoonRec*dotmoonRec*dotmoonRec + 1.5*(loveShida3[1] - loveShida3[0])*dotmoonRec;
	pmoon3XYZ[0] = K1*(K3*erec[0] + K2*emoon[0]);
	pmoon3XYZ[1] = K1*(K3*erec[1] + K2*emoon[1]);
	pmoon3XYZ[2] = K1*(K3*erec[2] + K2*emoon[2]);
	//影响结果叠加
	double pTideXYZ[3]={0};
	pTideXYZ[0] = pmoon2XYZ[0] + psun2XYZ[0] + pmoon3XYZ[0];
	pTideXYZ[1] = pmoon2XYZ[1] + psun2XYZ[1] + pmoon3XYZ[1];
	pTideXYZ[2] = pmoon2XYZ[2] + psun2XYZ[2] + pmoon3XYZ[2];	
	/* step2: frequency domain, only K1 radial */
	double sin2l=qSin(2.0*m_SationBLH[0]);
	double du=-0.012*sin2l*qSin(gmst+m_SationBLH[1]);
	pTideXYZ[0] += du*erec[0];pTideXYZ[1] += du*erec[1];pTideXYZ[2] += du*erec[2];
	//恒定潮汐形变的分量消除 需要投影到垂直和北向
	if (isElimate)
	{
		double dltaU = 0,dltaN = 0;
		double  eN[3]= {-qSin(m_SationBLH[0])*qCos(m_SationBLH[1]),-qSin(m_SationBLH[0])*qSin(m_SationBLH[1]),qCos(m_SationBLH[0])};
		dltaU = -0.0603*(3*qSin(m_SationBLH[0])*qSin(m_SationBLH[0]) - 1);
		dltaN = -0.0252*qSin(2*m_SationBLH[0]);
		pTideXYZ[0]-= (dltaU*erec[0] + dltaN*eN[0]);
		pTideXYZ[1]-= (dltaU*erec[1] + dltaN*eN[1]);
		pTideXYZ[2]-= (dltaU*erec[2] + dltaN*eN[2]);
	}
	//转换ENU方向
	//将pTideXYZ转化为到测站坐标 ENU方向
	pTideXYZ[0]+=pXYZ[0];pTideXYZ[1]+=pXYZ[1];pTideXYZ[2]+=pXYZ[2];
	m_cmpClass.XYZ2ENU(pTideXYZ,pTideENU,pXYZ);
	
}

//void 读取海潮文件（只能读取IGS站的数据 其他任意一点不可以读取，也可以把接收机名字替换为临近测站）
bool QTideEffect::readOCEANFile(QString  StationName,OCEANData &oceaData,QString  OCEANFileName)
{
	if (isReadOCEAN) return true;
	OCEANData tempOCEANData;
	tempOCEANData.isRead = false;
	oceaData.isRead = false;
	StationName = StationName.trimmed();//消去文件名空格，以及空格形式的文件名变为空
	StationName = StationName.toUpper();
	if (StationName.isEmpty())
		if (!m_StationName.isEmpty())
			StationName = m_StationName;
		else
		{
			isOCEANTide = false;
			return false;
		}
			
	//找到文件
	//搜索当前目录.dat文件
	QDir m_dir(".");
	QStringList m_fliterList;
	//如果存在erp文件则使用 否则就不使用
	m_fliterList.append("*.blq");
	QStringList OCEANFileNameList = m_dir.entryList(m_fliterList); 
	QString dirOCEANfileName ="";
	if (!OCEANFileNameList.isEmpty())
		dirOCEANfileName = OCEANFileNameList.at(0);
	if (OCEANFileName.isEmpty())
	{
		if (!m_OCEANFileName.isEmpty())
			OCEANFileName = m_OCEANFileName;
		else if (!dirOCEANfileName.isEmpty())
			OCEANFileName = dirOCEANfileName;
		else
			return false;
	}
	m_readOCEANClass.setFileName(OCEANFileName);
	if(!m_readOCEANClass.open(QFile::ReadOnly)) return false;
	//跳过头文件
	QString tempLine="";
	while (!tempLine.contains("END"))
		tempLine = m_readOCEANClass.readLine();
	//读取数据开始符号（$$）
	QString tempStationName;
	while (!m_readOCEANClass.atEnd())
	{
		//每行2-6个自己为空就代表数据将要开始"$$" 长度为4
		while (4 != tempLine.length())
		{
			tempLine = m_readOCEANClass.readLine();
			if (m_readOCEANClass.atEnd()) 
			{
				isOCEANTide = false;
				oceaData.isRead = false;
				break;	
			}
		}
			
		//读取测站数据
		tempLine = m_readOCEANClass.readLine();//读取头文件行
		if (m_readOCEANClass.atEnd()) 
		{
			isOCEANTide = false;
			oceaData.isRead = false;
			break;	
		}
		tempStationName = tempLine.mid(2,4).trimmed().toUpper();
		if (tempStationName != StationName) continue;
		//找到测站
		tempOCEANData.StationName = tempStationName;
		//跳过注释（可以读取测站BLH，此处未读取）
		tempLine = m_readOCEANClass.readLine();//读取注释
		while (tempLine.mid(0,2).contains("$"))
			tempLine = m_readOCEANClass.readLine();//读取注释
		//解析数据
		for (int i = 0;i < 6;i++)
		{
			for (int j = 0;j < 11;j++)
			{
				if (i < 3)
					tempOCEANData.amp[i][j] = tempLine.mid(2+j*7,6).toDouble();
				else
					tempOCEANData.phasedats[i-3][j] = tempLine.mid(2+j*7,6).toDouble();
			}
			tempLine = m_readOCEANClass.readLine();//读取注释
		}
		tempOCEANData.isRead = true;
		break;//读取完数据跳出
	}
	isReadOCEAN = true;//读取了文件，不能确保找到了该测站
	oceaData = tempOCEANData;//保存数据
	if (tempOCEANData.isRead = false)	isOCEANTide = false;
	m_readOCEANClass.close();
	return true;
}

//计算海潮影响
void QTideEffect::getOCEANTide(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,double *pXYZ,double *pTideENU,QString StationName)
{
	pTideENU[0]=0;pTideENU[1]=0;pTideENU[2]=0;
	if (!isOCEANTide)	return ;
	if (!isReadOCEAN)
		if (!readOCEANFile(StationName,m_OCEANData,m_OCEANFileName))
			return ; 
	if (!m_OCEANData.isRead) return ; //判断数据是否读取到了，并且可用
	//参考RTKLIB ARG
	const double ep1975[]={1975,1,1,0,0,0};
	double ep[6] = {0},fday = 0,days = 0,t = 0,t2 = 0,t3 = 0,a[5] = {0},ang = 0,dp[3]={0};
	int i = 0,j = 0;

	/* angular argument: see subroutine arg.f for reference [1] */
	//m_cmpClass.time2epoch(tut,ep);
	//RTKLIB tut使用的UT1需要erp为文件 增加计算量 此处忽略UT1,可以使用getErp函数获取
	ep[0] = Year;ep[1] = Month;ep[2] = Day;ep[3] = Hours;ep[4] = Minuts;ep[5] = Seconds;
	//转化为UTC时间
	if (LeapSeconds <= 0)//跳秒只获得一次 为大于零的数
		LeapSeconds = m_cmpClass.getLeapSecond(Year,Month,Day,Hours,Minuts,Seconds);
	ep[5]-=LeapSeconds;
	fday=ep[3]*3600.0+ep[4]*60.0+ep[5];
	ep[3]=ep[4]=ep[5]=0.0;
	days=m_cmpClass.timediff(m_cmpClass.epoch2time(ep),m_cmpClass.epoch2time(ep1975))/86400.0;
	t=(27392.500528+1.000000035*days)/36525.0;
	t2=t*t; t3=t2*t;

	a[0]=fday;
	a[1]=(279.69668+36000.768930485*t+3.03E-4*t2)*D2R; /* H0 */
	a[2]=(270.434358+481267.88314137*t-0.001133*t2+1.9E-6*t3)*D2R; /* S0 */
	a[3]=(334.329653+4069.0340329577*t+0.010325*t2-1.2E-5*t3)*D2R; /* P0 */
	a[4]=2.0*PI;
	/* displacements by 11 constituents */
	for (i=0;i<11;i++) {
		ang=0.0;
		for (j=0;j<5;j++) ang+=a[j]*args[i][j];
		for (j=0;j<3;j++) dp[j]+=m_OCEANData.amp[j][i]*cos(ang-m_OCEANData.phasedats[j][i]*D2R);
	}
	pTideENU[0]=-dp[1];
	pTideENU[1]=-dp[2];
	pTideENU[2]= dp[0];
}

//获取所有海潮对ENU方向的影响
void QTideEffect::getAllTideEffectENU(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,double *pXYZ,double *pENU,double *psunpos/* =NULL */, double *pmoonpos /* = NULL */,double gmst /* = 0 */,QString StationName /* = "" */)
{
	pENU[0] = 0;pENU[1] = 0;pENU[2] = 0;
	if (psunpos&&pmoonpos)
		setSunMoonPos(psunpos,pmoonpos,gmst);
	//获得固体潮
	getSoildTide(Year,Month,Day,Hours,Minuts,Seconds,pXYZ,m_pSolidENU);//采用默认不考虑永久腐蚀
	//获取海洋潮汐
	if (!StationName.isEmpty())
		getOCEANTide(Year,Month,Day,Hours,Minuts,Seconds,pXYZ,m_pOCEANENU,StationName);
	else if (!m_StationName.isEmpty())
		getOCEANTide(Year,Month,Day,Hours,Minuts,Seconds,pXYZ,m_pOCEANENU,m_StationName);
	//获取极潮
	if (m_epochFlag < m_MaxCompution)
		getPoleTide(Year,Month,Day,Hours,Minuts,Seconds,m_SationBLH,m_pPoleENU);
	//潮汐影响叠加
	pENU[0] = m_pSolidENU[0] + m_pOCEANENU[0] + m_pPoleENU[0];
	pENU[1] = m_pSolidENU[1] + m_pOCEANENU[1] + m_pPoleENU[1];
	pENU[2] = m_pSolidENU[2] + m_pOCEANENU[2] + m_pPoleENU[2];
	
}

double QTideEffect::getAllTideEffect(int Year,int Month,int Day,int Hours,int Minuts,double Seconds,double *pXYZ,double *EA,double *psunpos/* =NULL */, double *pmoonpos /* = NULL */,double gmst /* = 0 */,QString StationName /* = "" */)
{
	double effectDistance = 0;
	if (qAbs(m_SecondFlag - Seconds) > 0.3)//两次只差大于0.3s认为更换历元重新计算太阳坐标
	{//一个历元只计算一次m_AllTideENU，节省计算量
		getAllTideEffectENU(Year,Month,Day,Hours,Minuts,Seconds,pXYZ,m_AllTideENU,psunpos,pmoonpos,gmst,StationName);
		m_SecondFlag = Seconds;
	}
	effectDistance = m_AllTideENU[0]*qCos(EA[0])*qSin(EA[1]) + m_AllTideENU[1]*qCos(EA[0])*qCos(EA[1]) + m_AllTideENU[2]*qSin(EA[0]);
	return effectDistance;
}

void QTideEffect::getAllData()
{
	readRepFile();//初始化时候读取erp文件到变量 m_erpData
	readOCEANFile(m_StationName,m_OCEANData,m_OCEANFileName);
}