#include "QKalmanFilter.h"


QKalmanFilter::QKalmanFilter(void)
{
	initVar();
}


QKalmanFilter::~QKalmanFilter(void)
{
}

void QKalmanFilter::initVar()
{
	isInitPara = true;//首次历元初始化只一次
	m_VarChang = false;
}

//打印矩阵for Debug
void QKalmanFilter::printMatrix(MatrixXd mat)
{
	qDebug()<<"Print Matrix......";
	for (int i = 0; i < mat.rows();i++)
	{
		for (int j = 0;j< mat.cols();j++)
		{
			qDebug()<<mat(i,j);
		}
		qDebug()<<"___________________";
	}
}

//初始化Kalman
void QKalmanFilter::initKalman(QVector< SatlitData > &currEpoch,MatrixXd &B,VectorXd &L)
{
	int epochLenLB = currEpoch.length();
	//Fk_1初始化
	m_Fk_1.resize(5+epochLenLB,5+epochLenLB);
	m_Fk_1.setZero();
	m_Fk_1.setIdentity(5+epochLenLB,5+epochLenLB);
	//Fk_1(3,3) = 0;//静态PPP只有钟差为0

	//Xk_1初始化,最小二乘初始化
	MatrixXd B1,L1,X1;
	m_Xk_1.resize(epochLenLB+5);
	m_Xk_1.setZero();
	m_Xk_1 = (B.transpose()*B).inverse()*B.transpose()*L;
	//初始化状态协方差矩阵Pk_1初始化
	m_Pk_1.resize(5+epochLenLB,5+epochLenLB);
	m_Pk_1.setZero();
	m_Pk_1(0,0) = 1e5;m_Pk_1(1,1) = 1e5;m_Pk_1(2,2) = 1e5;
	m_Pk_1(3,3) = 0.5; m_Pk_1(4,4) = 1e5;
	for (int i = 0;i < currEpoch.length();i++)	m_Pk_1(5+i,5+i) = 1e6;
	//Qk_1系统噪声初始化
	m_Qwk_1.resize(5+epochLenLB,5+epochLenLB);
	m_Qwk_1.setZero();
	m_Qwk_1(3,3) = 3e-8;//天顶对流层残差方差	
	m_Qwk_1(4,4) = 1e+6;
	//Rk_1初始化在判断卫星数目没有变化处
	isInitPara = false;//此后不在初始化
}

//改变Kalman参数 大小
void QKalmanFilter::changeKalmanPara( QVector< SatlitData > &epochSatlitData,QVector< int >oldPrnFlag )
{
	int epochLenLB = epochSatlitData.length();
	m_Fk_1.resize(5+epochLenLB,5+epochLenLB);
	m_Fk_1.setZero();
	m_Fk_1.setIdentity(5+epochLenLB,5+epochLenLB);
	//Fk_1(4,4) = 0;//静态PPP只有钟差为0
	//Xk_1变化
	VectorXd tempXk_1 = m_Xk_1;
	m_Xk_1.resize(epochLenLB+5);
	m_Xk_1.setZero();
	//Xk.resize(epochLenLB+5);
	for (int i = 0;i < 5;i++)
		m_Xk_1(i) = tempXk_1(i);
	for (int i = 0;i<epochLenLB;i++)
	{
		if (oldPrnFlag.at(i)!=-1)//保存老的卫星模糊度
			m_Xk_1(5+i) = tempXk_1(oldPrnFlag.at(i)+5);
		else
		{//新的卫星模糊度计算
			SatlitData oneStalit = epochSatlitData.at(i);
			m_Xk_1(5+i) = (oneStalit.PP3 - oneStalit.LL3)/M_GetLamta3(oneStalit.Frq[0],oneStalit.Frq[1]);
		}
	}
	//Qk_1系统噪声不会更新，系统噪声不可测量
	m_Qwk_1.resize(5+epochLenLB,5+epochLenLB);
	m_Qwk_1.setZero();
	m_Qwk_1(3,3) = 3e-8;//天顶对流层残差方差	
	m_Qwk_1(4,4) = 1e+6;
	//重置Rk_1观测噪声矩阵(已在外侧重置 此处不需要重复重置)
	//保存状态协方差矩阵Pk_1增加或减少(此处比较复杂，主要思想是取出老卫星数据，新卫星数据初始化)
	MatrixXd tempPk_1 = m_Pk_1;
	m_Pk_1.resize(5+epochLenLB,5+epochLenLB);
	m_Pk_1.setZero();
	//如果卫星数目改变
	for (int i = 0;i < 5;i++)
		for (int j = 0;j < 5;j++)
			m_Pk_1(i,j) = tempPk_1(i,j);

	for (int n = 0; n < epochLenLB;n++)
	{
		int flag = oldPrnFlag.at(n);
		if ( flag != -1)//说明：上一个历元含有本颗卫星数据，需要从tempPk_1取出
		{
			flag+=5;//本颗卫星在原始数据tempPk_1中的行数
			for (int i = 0;i < tempPk_1.cols();i++)
			{//从tempPk_1取出并跳过oldPrnFlag为-1的数据
				if (i < 5)
				{
					m_Pk_1(n+5,i) = tempPk_1(flag,i);
					m_Pk_1(i,n+5) = tempPk_1(i,flag);
				}
				else
				{
					int findCols = i - 5,saveFlag = -1;
					//查找数据在老链表是否存在，以及将要保存位置
					for (int m = 0;m < oldPrnFlag.length();m++)
					{
						if (findCols == oldPrnFlag.at(m))
						{
							saveFlag = m;
							break;
						}
					}
					if (saveFlag!=-1)
					{
						m_Pk_1(n+5,saveFlag+5) = tempPk_1(flag,i);
						//Pk_1(saveFlag+5,n+5) = tempPk_1(i,flag);
					}

				}//if (i < 5)
			}//for (int i = 0;i < tempPk_1.cols();i++)

		}
		else
		{
			m_Pk_1(n+5,n+5) = 1e20;
			for (int i = 0;i < 5;i++)
			{
				m_Pk_1(n+5,i) = 1;
				m_Pk_1(i,n+5) = 1;
			}
		}
	}//Pk_1保存数据完毕

	m_VarChang = true;
}


//第一个版本
void QKalmanFilter::KalmanforStatic(MatrixXd Bk,VectorXd Lk,MatrixXd F,MatrixXd Qwk,MatrixXd Rk,VectorXd &tXk_1,MatrixXd &tPk_1)
{
	//时间更新
	VectorXd Xkk_1 = F*tXk_1,Vk;
	MatrixXd Pkk_1 = F*tPk_1*F.transpose() + Qwk,I,tempKB,Kk;
	//计算增益矩阵
	Kk = (Pkk_1*Bk.transpose())*((Bk*Pkk_1*Bk.transpose() + Rk).inverse());
	//滤波更新
	Vk = Lk - Bk*Xkk_1;
	//更新X
	tXk_1 = Xkk_1 + Kk*Vk;

	tempKB = Kk*Bk;
	I.resize(tempKB.rows(),tempKB.cols());
	I.setIdentity();
	//更新P
	tPk_1 = (I - tempKB)*Pkk_1;
	//printMatrix(tPk_1);
	//tPk_1 = 0.5*(tPk_1 + tPk_1.transpose());	//(理论上应该加上但是加上后 或出现跳动。改变了原始数据形式)
	//printMatrix(tPk_1);
}

//第二个版本
void QKalmanFilter::KalmanforStatic(QVector< SatlitData > &preEpoch,QVector< SatlitData > &currEpoch,double *m_ApproxRecPos,VectorXd &X,MatrixXd &P)
{
	//组合伪距载波方程 L = BX
	int preEpochLen = preEpoch.length();
	int epochLenLB = currEpoch.length();
	MatrixXd B(2*epochLenLB,epochLenLB+5);
	VectorXd L(2*epochLenLB);
	for (int i = 0; i < epochLenLB;i++)
	{
		SatlitData oneSatlit = currEpoch.at(i);
		double li = 0,mi = 0,ni = 0,p0 = 0,dltaX = 0,dltaY = 0,dltaZ = 0;
		dltaX = oneSatlit.X - m_ApproxRecPos[0];
		dltaY = oneSatlit.Y - m_ApproxRecPos[1];
		dltaZ = oneSatlit.Z - m_ApproxRecPos[2];
		p0 = qSqrt(dltaX*dltaX+dltaY*dltaY+dltaZ*dltaZ);
		li = dltaX/p0;mi = dltaY/p0;ni = dltaZ/p0;
		//计算B矩阵
		//L3载波矩阵
		B(i,0) = li;B(i,1) = mi;B(i,2) = ni;B(i,3) = -oneSatlit.StaTropMap;B(i,4) = -1;
		for (int n = 0;n < epochLenLB;n++)//后面对角线部分全部初始化Lamta3的波长，其余为0
			if (i == n)	
				B(i,5+n) = M_GetLamta3(oneSatlit.Frq[0],oneSatlit.Frq[1]);//LL3波长
			else 
				B(i,5+n) = 0;
		
		//P3伪距码矩阵
		B(i+epochLenLB,0) = li;B(i+epochLenLB,1) = mi;B(i+epochLenLB,2) = ni;B(i+epochLenLB,3) = -oneSatlit.StaTropMap;B(i+epochLenLB,4) = -1;
		for (int n = 0;n < epochLenLB;n++)//后面部分全部为0
			B(i+epochLenLB,5+n) = 0;

		//计算L矩阵
		double dlta = 0;//各项那个改正
		dlta =  - oneSatlit.StaClock + oneSatlit.SatTrop - oneSatlit.Relativty - 
			oneSatlit.Sagnac - oneSatlit.TideEffect - oneSatlit.AntHeight - oneSatlit.SatOffset;
			//载波L
		L(i) = p0 - oneSatlit.LL3 + dlta;
		//伪距码L
		L(i+epochLenLB) = p0 - oneSatlit.PP3 + dlta;
	}//B,L计算完毕


	if (isInitPara) initKalman(currEpoch,B,L);//第一个历元初始化

	//判断卫星个数是否发生变化（前后两个历元对比）
	QVector< int > oldPrnFlag;//相比上一个历元相同卫星所在位置，未找到用-1表示
	int oldSatLen = 0;
	for (int i = 0;i < epochLenLB;i++)
	{//循环前后历元卫星检查是否完全相等
		SatlitData epochSatlit = currEpoch.at(i);
		bool Isfind = false;//标记是否找到上个历元存在
		for (int j = 0;j < preEpochLen;j++)
		{
			SatlitData preEpochSatlit = preEpoch.at(j);
			if (epochSatlit.PRN == preEpochSatlit.PRN&&epochSatlit.SatType == preEpochSatlit.SatType)
			{
				oldPrnFlag.append(j);
				Isfind = true;
				oldSatLen++;
				break;
			}
		}
		if (!Isfind)	oldPrnFlag.append(-1);
	}

	////增加或者减少n颗卫星
	if (preEpochLen!=0&&(oldSatLen!=preEpochLen||epochLenLB!=preEpochLen))
		changeKalmanPara(currEpoch,oldPrnFlag);//更新所有kalman参数数据大小

		
	//更新Rk_1（此时卫星数目没有变化）
	m_Rk_1.resize(2*epochLenLB,2*epochLenLB);
	m_Rk_1.setZero();
	for (int i = 0;i < epochLenLB;i++)
	{
		SatlitData oneSatlit = currEpoch.at(i);
		m_Rk_1(i,i) = 3.6e-5/oneSatlit.SatWight;//伪距方程的权 倒数(噪声小)
		m_Rk_1(i+epochLenLB,i+epochLenLB) = 9e-2/oneSatlit.SatWight;//载波权 倒数(噪声大)
	}

	
	if (m_VarChang)
	{
		KalmanforStatic(B,L,m_Fk_1,m_Qwk_1,m_Rk_1,m_Xk_1,m_Pk_1);
		m_VarChang = false;
	}

	//采用第一版本Kalman滤波
	KalmanforStatic(B,L,m_Fk_1,m_Qwk_1,m_Rk_1,m_Xk_1,m_Pk_1);
	X = m_Xk_1;//保存本历元计算结果（不包含初始化数据）
	/*for (int i = 0;i < X.size();i++)
	{
	qDebug()<<X[i];
	}*/
	P = m_Pk_1;
	
}