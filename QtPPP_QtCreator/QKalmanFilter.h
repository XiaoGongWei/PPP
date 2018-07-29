#ifndef QKALMANFILTER_H
#define QKALMANFILTER_H

/*
1、矩阵求逆-基于Eigen计算，求逆计算量大
2、优化计算不需要基于Eigen求逆
*/
#include "QGlobalDef.h"


class QKalmanFilter:public QBaseObject
{
//函数部分
public:
	QKalmanFilter(void);
	~QKalmanFilter(void);
	void initVar();//初始化一些参数
	
	//F:状态转移矩阵，Xk_1:前一次滤波数值，Pk_1：前一次滤波误差矩阵，Qk_1：前一次状态转移噪声矩阵，Bk：观测矩阵，
	//Rk:观测噪声矩阵，Lk：观测向量
	void KalmanforStatic(MatrixXd Bk,VectorXd Lk,MatrixXd F,MatrixXd Qw,MatrixXd Rk,VectorXd &Xk_1,MatrixXd &Pk_1);
	void KalmanforStatic(QVector< SatlitData > &preEpoch,QVector< SatlitData > &currEpoch,double *m_ApproxRecPos,VectorXd &Xk_1,MatrixXd &Pk_1);
private:
	void printMatrix(MatrixXd mat);//打印矩阵Debug
	void initKalman(QVector< SatlitData > &currEpoch,MatrixXd &B,VectorXd &L);//卡尔曼初始化
	void changeKalmanPara(QVector< SatlitData > &epochSatlitData,QVector< int >oldPrnFlag );
//变量部分
public:

private:
	bool isInitPara;//判断第一个历元是否初始化
	MatrixXd m_Fk_1,m_Pk_1,m_Qwk_1,m_Rk_1;
	VectorXd m_Xk_1;//分别为dX,dY,dZ,dT(天顶对流层残差),dVt(接收机钟差)，N1,N2...Nm(模糊度)
	bool m_VarChang;//标记 下个滤波时期 矩阵是否发生变化
};

#endif
