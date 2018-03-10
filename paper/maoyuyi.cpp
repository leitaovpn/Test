/*
* 论文的比较算法
*Mao YuYi 
*  Created on: 2017年8月13日
*      Author: leitao
*/
#include<iostream>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include<string.h>
#include<fstream>
using namespace std;
#define  NUM  10        //用户数
#define CORES 4         //服务器CPU个数
#define  LOOP 1000       //循环次数
#define t 0.001          //时间间隔
#define V 1e+9            //李雅普诺夫系数
#define min_bw 0.001        //最小带宽分配
#define W 1000000          //带宽
#define w_s 0.8          //服务器权重
#define THE_K 1e-27    //表示10的-27次方
#define g0 -40       //路径损耗常数
#define q 4          //路径损耗指数
#define d0 1          //参考距离
/*
    移动设备{
    value 移动设备对应在MEC服务器的Task buff的价值；
    Q 本地队列；
    T 远程队列；
    L 每bit所需周期数；
    dl 本地执行的任务
    ds offloading的任务
    fu 本地CPU频率
    fs 服务器分配的CPU频率
    pwr 移动设备传输功率
    A 在每个时间段到达移动设备的任务
    a 分配的带宽
    w 移动设备权重
    F 信道收益
    k_u 移动设备CPU系数
    fu_max CPU最大频率
    p_max 移动设备最大传输功率
    d    移动设备与MEC服务器的距离
    Y  小区间信道衰落信道功率收益
  }
*/
typedef struct Mobile{
  double value;
  double Q;
  double T;
  double L;
  double dl;
  double dr;
  double ds;
  double fu;
  double fs;
  double pwr;
  double A;
  double a;
  double w;
  double F;
  double k_u;
  double fu_max;
  double p_max;
  double d;
  double Y;
}Mobile;
/*
服务器CPU{
fs_max CPU最大频率；
fs CPU频率；
k_s CPU系数；
}
*/
typedef struct ServerCore{
  double fs_max;
  double fs;
  double k_s;
}ServerCore;
int NO_Offloading=0;      //没有Offloading的用户数

double N=pow(10,-17.4)/1000;       //小区间内干扰

double S_R=0;            //服务器提供的计算资源

int mValue[NUM];

ofstream SaveFile("my.txt", ios::ate);
ofstream SigleFile("sigle.txt", ios::ate);
ofstream QoEFile("QoE.txt", ios::ate);	
ofstream OffLoadingFile("offloading.txt", ios::ate);	
ofstream PowerFile("Power.txt", ios::ate);
ofstream LengthFile("Length.txt", ios::ate);
double max(double x,double y){
  return x>y?x:y;
}
double min(double x,double y){
  return x>y?y:x;
}
double absolute(double x,double y){
   double z;
   z=x-y;
   return z>0?z:-z;
}
//获取服务器提供的计算资源
void GetServerResource(ServerCore *server){
    S_R=0;
    for (int  i = 0; i < CORES; i++) {
      //i表示第i个服务器CPU
      S_R+=server[i].fs;
    }
}
//本地CPU频率
double getFu(Mobile mobile){
    return sqrt((mobile.Q*t)/(3*mobile.k_u*mobile.w*V*mobile.L));
}
//解决SP1问题
void DoSP1(Mobile *mobile) {
  for (int i = 0; i < NUM; i++) {
    if(mobile[i].w!=0){
      mobile[i].fu=min(mobile[i].fu_max,getFu(mobile[i]));
    }
    else{
      mobile[i].fu=mobile[i].fu_max;
    }
    mobile[i].dl=t*mobile[i].fu/mobile[i].L;
  //  cout<<"mobile["<<i<<"].fu="<<mobile[i].fu<<endl;
  //  cout<<"mobile["<<i<<"].dl="<<mobile[i].dl<<endl;
  }
}
//获得对应带宽的传输功率
double getPpwr(Mobile mobile,double a){
  double M;
  if(mobile.w==0){
    return mobile.p_max;
  }
  else{
    M=(mobile.Q-mobile.T)*t/(log(2)*V*mobile.w)-N/mobile.F;
    return min(a*W*max(M,0),mobile.p_max);
  }
}
//拉格朗日系数
double getLM(Mobile mobile,double a){
  double M; //倒数值
  double p; //传输功率
  p=mobile.p_max;
  M=W*t*(log(1+mobile.F*p/(a*W*N))/log(2)-mobile.F*p/((a*W*N+mobile.F*p)*log(2)));
  double l1=log(1+mobile.F*p/(a*W*N))/log(2);
  double l2=mobile.F*p/((a*W*N+mobile.F*p)*log(2));
  return (mobile.Q - mobile.T) * M;
}
//PWB的导数
double getDaoshu(Mobile mobile,double a,double LM,double p)
{
	double  M;
	M=W*t*(log(1+mobile.F*p/(a*W*N))/log(2)-mobile.F*p/((a*W*N+mobile.F*p)*log(2)));
	return -(mobile.Q-mobile.T)*M+LM;
}
//根据拉格朗日系数得到分配带宽
double getRoot(Mobile mobile,double LM){
  int LOOP_MAX=100,loop=0;
  double a_max,a_min,a_mid=0;
  double M;
  double p;
  double x=0.001;  //精度
  //p=getPpwr(mobile,mobile.a);
  p=mobile.p_max;
  a_min=min_bw;
  a_max=1-NO_Offloading*min_bw;
  //传输功率
  do
  {
  	loop++;
  	a_mid=(a_min+a_max)/2;
  	if(getDaoshu(mobile,a_mid,LM,p)>0)
  		{
  			a_max=a_mid;
  		}
  	else
  		{
  			a_min=a_mid;
  		}
  	
  }while(absolute(a_max,a_min)>x&&loop<LOOP_MAX);
  return a_mid;
}

//已分配的带宽资源
double GetA(Mobile *mobile){
  double BW_L=0;   //已分配的带宽
  for (int i = 0; i < NUM; i++) {
     if(mobile[i].Q > mobile[i].T)
         BW_L+=mobile[i].a;
  }
  return BW_L;
}
//解决Pbw问题
void DoPbw(Mobile *mobile) {
  double x=0.001; //精度
  int LOOP_MAX=200,loop=0;    //最大循环次数
  double BW_R = 1 - NO_Offloading*min_bw;    //可分配的带宽资源
  double lm_l=0,lm_u=0,lm_mid,a_min,a_max;
  a_max=BW_R;
  a_min=min_bw;
  SigleFile<<"a_max="<<a_max<<" a_min="<<a_min<<endl;
  double l1,l2;
  double m;
  //初始化二分法界限
  for (int i = 0; i < NUM; i++) {
  	if(mobile[i].Q>mobile[i].T)
  	{
      l1=getLM(mobile[i],a_max);
      l2=getLM(mobile[i],a_min);
      if(lm_l<l1)
      	{
      		lm_l=l1;
      	}
      	
      if(lm_u<l2)
      	{
      		lm_u=l2;
      	}
      SigleFile<<"第"<<i<<"个移动设备"<<" l1="<<l1<<"  l2="<<l2<<endl;
    }
  }
  SigleFile<<"NO_Offloading="<<NO_Offloading<<"  lm_l="<<lm_l<<"  lm_u="<<lm_u<<endl;
  //确定不属于Nc的移动设备的带宽和传输功率的分配情况
  for (int i=0; i<NUM; i++)
  {
  	if (mobile[i].Q<=mobile[i].T) 
  	{
  		mobile[i].a=min_bw;
  	}
  }
  
  //确定属于Nc的移动设备的带宽和传输功率的分配情况
  while (absolute(GetA(mobile),BW_R)>=x&&loop<=LOOP_MAX) {
    //如果精度达到要求或者循环次数达到限制退出循环
       loop++;
       lm_mid=(lm_l+lm_u)/2;
       
       //求解每个属于Nc用户的带宽分配方案
       for(int i=0;i<NUM;i++)
       {
       	  m=mobile[i].Q-mobile[i].T;
       	  if(m>0)
       	  	{
       	  		mobile[i].a=max(min_bw,getRoot(mobile[i],lm_mid));
       	  	}
       }
       
       if(GetA(mobile)>BW_R)
       	{
       		lm_l=lm_mid;
       	}
       else
       	{
     			lm_u=lm_mid;
       	}
  }
}
//解决Ppwr问题
void DoPpwr(Mobile *mobile){
  for (int  i = 0; i < NUM; i++) {
    //求出第i个移动设备的传输功率
    if (mobile[i].Q > mobile[i].T) {
      mobile[i].pwr=max(min(getPpwr(mobile[i],mobile[i].a),mobile[i].p_max),0);
    }
    else
    	{
    		mobile[i].pwr=0;
    	}
  }
}

//解决SP2问题
void DoSP2(Mobile *mobile) {
   DoPbw(mobile);
   DoPpwr(mobile);
   for (int i = 0; i < NUM; i++) {
     //更新第i个用户的dl,ds,dr
     if(mobile[i].a==min_bw){
       mobile[i].dr=0;
       mobile[i].pwr=0;
     }else{
       mobile[i].dr= mobile[i].a*W*t*log(1 + (mobile[i].F*mobile[i].pwr)/(mobile[i].a*N*W))/log(2);
      // cout<<mobile[i].a*N*W<<endl;cout<<"mobile["<<i<<"].a="<<mobile[i].a<<"  mobile["<<i<<"].pwr="<<mobile[i].pwr<<"  mobile["<<i<<"].dr="<<mobile[i].dr<<"  mobile["<<i<<"].F="<<mobile[i].F<<endl;
     }


   }
}

//计算移动设备的价值,并从大到小排序
void GetMaxValue(Mobile *mobile){
//  Mobile m;
  int m;
  for (int i=0; i<NUM; i++)
  {
  	mValue[i]=i;
  }
  for (int i = 0; i < NUM; i++) {
    //移动设备i的价值
    mobile[i].value=mobile[i].T/mobile[i].L;
  }
  //冒泡排序
  for (int i = 0; i < NUM; i++) {
    for (int j = 0; j < NUM-i-1; j++) {
      if (mobile[mValue[j]].value < mobile[mValue[j+1]].value) {
//          m=mobile[j];
//          mobile[j]=mobile[j+1];
//          mobile[j+1]=m;
            m=mValue[j];
            mValue[j]=mValue[j+1];
            mValue[j+1]=m;
      }
    }
  }
}
//计算资源最大能满足的移动设备数
int getNser(Mobile *mobile,ServerCore *server){
  double s=0; //执行的任务
  int m=0;
  int i;
  for (i = 0; i < NUM; i++) {
  	m=mValue[i];
    s+=mobile[m].T*mobile[m].L;
    if (s>S_R*t) {
      return i;
    }
  }
  return i;
}
//已分配的计算资源
double getResource(Mobile *mobile,int nser){
  double s=0;
    for (int i = 0; i < nser; i++) {
      //i表示移动设备编号
      s+=mobile[mValue[i]].fs;
    }
    return s;
}
//延迟改进算法
void DO_delay_improved(Mobile *mobile,ServerCore *server) {
  int nser;
  if (mobile[mValue[0]].T*mobile[mValue[0]].L >= S_R*t) 
  {
    mobile[mValue[0]].ds=S_R*t/mobile[mValue[0]].L;
    mobile[mValue[0]].fs=S_R;
    for (int i = 1; i < NUM; i++) {
       mobile[mValue[i]].ds=0;
       mobile[mValue[i]].fs=0;
    }
    return;
  }
  else
  {
    nser=getNser(mobile,server);
    for (int i = 0; i < nser; i++) {
      mobile[mValue[i]].ds=mobile[mValue[i]].T;
      mobile[mValue[i]].fs=mobile[mValue[i]].ds*mobile[mValue[i]].L/t;
    }
    mobile[mValue[nser]].fs=S_R-getResource(mobile,nser);
    mobile[mValue[nser]].ds=min(mobile[mValue[nser]].fs*t/mobile[mValue[nser]].L,mobile[mValue[nser]].T);
    for (int j = nser+1; j < NUM; j++) {
      mobile[mValue[j]].ds=0;
      mobile[mValue[j]].fs=0;
    }
  }
}
//解决SP3问题
void DoSP3(Mobile *mobile,ServerCore *server) {
  double f;
  for (int i = 0; i < CORES; i++) {
    //求出第i个CPU的频率
    if (w_s==0) {
      server[i].fs=server[i].fs_max;
    }
    else
    {
    	f=sqrt((mobile[mValue[0]].T*t) / (3*V*w_s*mobile[mValue[0]].L*server[i].k_s));
      server[i].fs=min(server[i].fs_max,f);
    }	
  }
  
  //得到分配的计算资源
  GetServerResource(server);
  //延迟改进版本
  DO_delay_improved(mobile,server);
}
//解决Popt问题
void DoPopt(Mobile *mobile,ServerCore *server) {
  DoSP1(mobile);
  DoSP2(mobile);
  DoSP3(mobile,server);
}
//在该时间段内不offloading的用户数
void GetNo_Offloading(Mobile *mobile) {
  NO_Offloading=0;
  for (int i = 0; i < NUM; i++) {
     if(mobile[i].Q <= mobile[i].T)
     {++NO_Offloading;}
  }
}
//QoECost
double GetQoECost(Mobile mobile){
	double pl=mobile.k_u*pow(mobile.fu,3);
	//本地执行花费
	double Local_Cost=-mobile.Q*mobile.dl+V*pl;
	//远程传输花费
	double Trans_Cost=-(mobile.Q-mobile.T)*mobile.dr+V*mobile.pwr;
	//其他花费
	double Other_Cost=mobile.Q*mobile.A-mobile.T*mobile.ds;
	
	return Local_Cost+Trans_Cost+Other_Cost;
};
void init(Mobile *mobile,ServerCore *server){
  for (int i = 0; i < NUM; i++) {
    mobile[i].value=0;
    mobile[i].dl=0;
    mobile[i].dr=0;
    mobile[i].ds=0;
    mobile[i].fu=0;
    mobile[i].fs=0;
    mobile[i].a=min_bw;
    mobile[i].pwr=0;
    mobile[i].Y=rand()%10 + 1;
    mobile[i].F=mobile[i].Y*pow(10,g0/10)*pow(d0/mobile[i].d,q);
  }
}
int main() {
  Mobile *mobile =(Mobile *)malloc(sizeof(Mobile)*NUM);
  ServerCore *server =(ServerCore *)malloc(sizeof(ServerCore)*CORES);
  //初始化用户
  for ( int i=0; i < NUM  ;i++) {
    mobile[i].value=0;
    mobile[i].Q=0;
    mobile[i].T=0;
    mobile[i].L=737.5;
    mobile[i].dl=0;
    mobile[i].dr=0;
    mobile[i].ds=0;
    mobile[i].fu=0;
    mobile[i].fs=0;
    mobile[i].A=0;
    mobile[i].a=min_bw;
    mobile[i].w=1;
    mobile[i].pwr=0;
    mobile[i].F=0;
    mobile[i].k_u=THE_K;
    mobile[i].fu_max=1000000000;
    mobile[i].p_max=0.5;
    mobile[i].d=150;
    mobile[i].Y=0;
  }
  for (int i = 0; i < CORES; i++) {
    //初始化服务器
    server[i].fs_max=2500000000;
    server[i].fs=0;
    server[i].k_s=THE_K;
  }
  srand((unsigned)time(NULL));
  for (int i = 0; i <LOOP ; i++) {
    SaveFile<<"#####################第"<<i<<"个时间片#######################"<<endl;
    SigleFile<<"#####################第"<<i<<"个时间片#######################"<<endl;
    SaveFile<<"Q   ";
    for (int j1=0; j1<NUM; j1++)
    {
    	SaveFile<<mobile[j1].Q<<"   ";
    }
    SaveFile<<endl<<"T   ";
    for (int j1=0; j1<NUM; j1++)
    {
    	SaveFile<<mobile[j1].T<<"   ";
    }
    
    //在每个时间片段内
    //初始化
    init(mobile,server);
    //不产生offloading的移动设备
    GetNo_Offloading(mobile);
    //计算移动设备价值并从大到小排序
    GetMaxValue(mobile);
    //调用算法
    DoPopt(mobile,server);
    SaveFile<<endl<<"Dl   ";
    for (int j1=0; j1<NUM; j1++)
    {
    	SaveFile<<mobile[j1].dl<<"   ";
    }
    SaveFile<<endl<<"Dr   ";
    for (int j1=0; j1<NUM; j1++)
    {
    	SaveFile<<mobile[j1].dr<<"   ";
    }
    SaveFile<<endl<<"a*W   ";
    for (int j1=0; j1<NUM; j1++)
    {
    	SaveFile<<mobile[j1].a*W<<"   ";
    }
    SaveFile<<endl<<"pr   ";
    for (int j1=0; j1<NUM; j1++)
    {
    	SaveFile<<mobile[j1].pwr<<"   ";
    }
    SaveFile<<endl<<"F   ";
    for (int j1=0; j1<NUM; j1++)
    {
    	SaveFile<<mobile[j1].F<<"   ";
    }
    SaveFile<<endl<<"ds   ";
    for (int j1=0; j1<NUM; j1++)
    {
    	SaveFile<<mobile[j1].ds<<"   ";
    }
    SaveFile<<endl;
    
    for (int j1=0; j1<NUM; j1++)
    {
    	QoEFile<<GetQoECost(mobile[j1])<<"   ";
    }
    QoEFile<<endl;
    int m=0;
    double the_p=0;
    double length=0;
    for (int j = 0; j < NUM; j++) {
      //循环更新每个用户的队列长度
      //cout<<"mobile["<<j<<"].pwr="<<mobile[j].pwr<<"  mobile["<<j<<"].Q="<<mobile[j].Q<<"  mobile["<<j<<"].T="<<mobile[j].T<<endl;
      mobile[j].A=rand()%8000;
      mobile[j].Q=max(mobile[j].Q-mobile[j].dl-mobile[j].dr,0) + mobile[j].A;
      mobile[j].T=max(mobile[j].T-mobile[j].ds,0)+min(max(mobile[j].Q-mobile[j].dl,0),mobile[j].dr);
      //cout<<"A="<<mobile[j].A<<"  dl="<<mobile[j].dl<<"  dr="<<mobile[j].dr<<"  ds="<<mobile[j].ds<<"  Q="<<mobile[j].Q<<"  T="<<mobile[j].T;
      //cout<<"  fu="<<mobile[j].fu<<"  fs="<<mobile[j].fs<<"  fu="<<mobile[j].fs<<"  pwr="<<mobile[j].pwr<<endl;
      //cout<<"mobile["<<j<<"].pwr="<<mobile[j].pwr<<endl;
      the_p=the_p+pow(mobile[j].fu,3)*THE_K+mobile[j].pwr;
      length=length+mobile[j].Q+mobile[j].T;
      if(mobile[j].pwr>0)
      	{
      		m++;
      	}      
    }
    the_p=the_p/NUM;
    length=length/NUM;
    m=m*3;
    OffLoadingFile<<m<<endl;
    PowerFile<<the_p<<endl;
    LengthFile<<length<<endl;
    
    //cout<<m<<endl;
    //cout<<"**********************************************"<<endl;
 
  }
  SaveFile.close();
  return 0;
}
