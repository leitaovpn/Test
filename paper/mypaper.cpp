#include<iostream>
#include<math.h>
#include<stack>
#include<queue>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#include<string.h>
#include<fstream>
using namespace std;
//移动设备数
#define PHONE_NUM  30
//基站数
#define BS_NUM    3
//时间片大小
#define t     0.001
//V的大小
#define V     1e+9
//单位bit所需计算量
#define L   737.5
//最大QoE花费(未确定)
#define QoE_cost  1e+10
//噪声功率
#define N   3.98e-21
//到达任务的最大值
#define A_MAX 8000
//时间最大值
#define T_MAX  1000 
//服务器和移动设备的连接情况
int Connect[3][30]={
    {1,1,1,1,1,1,1,1,1,1,0,1,1,0,1,0,0,0,1,0,0,1,0,1,0,1,0,1,0,1},
    {1,1,0,0,0,0,0,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,1,0,1,0,0,1,0,0,1,0,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1}
    };
int Distance[3][30]={
	  {150,150,50,100,50,100,50,100,100,50,1000000,150,100,1000000,150,1000000,1000000,1000000,150,1000000,1000000,100,1000000,100,1000000,100,1000000,100,1000000,100},
    {150,150,1000000,1000000,1000000,1000000,1000000,100,1000000,1000000,50,150,100,50,150,50,100,50,150,50,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000,1000000},
    {150,150,1000000,100,1000000,100,1000000,1000000,100,1000000,1000000,150,1000000,1000000,150,1000000,100,1000000,150,1000000,50,100,50,100,50,100,50,100,50,100}
};
//卸载用户数
int offloading_num=0;
ofstream SaveFile("result.txt", ios::ate);
ofstream OffLoadingFile("offloading1.txt", ios::ate);
ofstream PowerFile("Power1.txt", ios::ate);
ofstream LengthFile("Length1.txt", ios::ate);
//返回最小值
double max(double x,double y){
  return x>y?x:y;
};
//返回最小值
double min(double x,double y){
  return x>y?y:x;
};
//取差的绝对值
double absolute(double x,double y){
   double z;
   z=x-y;
   return z>0?z:-z;
};
//MEC服务器
class BaseStation{
	private:		
	  /*常量*/
  	//最大CPU频率
  	double f_max;
	  //CPU个数
	  double num;
	  //服务器总带宽
	  double Total_B;
	 public:
	 	//服务器在一个时隙内所能提供的计算资源
	 	double C_R;	
	 	//子带带宽
	 	double Sub_B;
	  //覆盖用户数
	  int user;
		//构造函数
		BaseStation();
		//获取子带情况
		void GetSubNet(int server);
};
//MEC服务器
BaseStation::BaseStation(){
	f_max=2.5e+9;
	num=4;
	C_R=f_max*t*4;
	Total_B=1e+7;
};

	//获取子带情况
void BaseStation::GetSubNet(int server){
	user=0;
	for (int i=0; i<PHONE_NUM; i++)
	{
		if(Connect[server][i]==1)
			{
				user++;
			}
	}
	
	Sub_B=Total_B/user;
};

	
//移动设备
class Phone{
	private:
		/*属性*/
		//开关电容
		double k;
		//最大CPU频率
		double f_max;
		//最大发射功率
		double p_max;
		

	public:
		/*变量*/
		//到达任务
		double A;
		//任务队列
		double Q;
		//本地执行的任务
		double Dl;
		//本地CPU频率
		double f;
		//本地CPU功率
		double pl;
		//远程传输的任务;
		double Dr;
		//远程传输功率
		double pr;
    //子带带宽
    double B;
		//服务器上的任务队列长度
		double H;
		//远程执行的任务量
		double Dexe;
		//功率增益
		double F;
		//所选上传链路的服务器
		int server;
		
		//构造函数
		Phone();
		//获取本地执行CPU频率
		void GetLocalExec();
		//选择子信道
		void SeleteSubNet(double the_B,double the_F,int the_server);
		//获取远程传输功率
		void GetTransPower();
		//获取远程执行任务量
		void GetRemoteExec(double the_Dexe);
		//QoE-Cost
		double GetQoECost(double the_pr);
		//获取传输数据量
		double GetRemoteTask(double the_pr);
		//更新用户信息
		void RefreshPhone();
		
};

//移动设备
Phone::Phone(){
	k=1e-27  ;
	f_max=1e+9;
	p_max=0.5;
	A=0;
	Q=0;
	Dl=0;
	f=0;
	pl=0;
	Dr=0;
	pr=0;
	H=0;
	B=0;
	Dexe=0;
	F=0;
};

void Phone::GetLocalExec(){
   double m=Q*t/(3*V*k*L);
   f=min(sqrt(m),f_max);
   Dl=f*t/L;
   pl=k*pow(f,3);
};

void Phone::SeleteSubNet(double the_B,double the_F,int the_server){ 
	  B=the_B;	
    F=the_F;
    server=the_server;
};

double Phone::GetRemoteTask(double the_pr){
	return B*t*log(1+F*the_pr/N)/log(2);
};

double Phone::GetQoECost(double the_pr){
	//本地执行花费
	double Local_Cost=-Q*Dl+V*pl;
	//远程传输花费
	double Trans_Cost=-(Q-H)*GetRemoteTask(the_pr)+V*the_pr;
	//其他花费
	double Other_Cost=Q*A-H*Dexe;
	
	return Local_Cost+Trans_Cost+Other_Cost;
};
                                            
void Phone::GetTransPower(){
	double m=Q-H;
	double n;
	double x;
	double min_p,max_p,mid_p;
	if((Q-Dl)<=0)
		{
			pr=0;
			Dr=0;
			return;
		}
	x=(pow(2,(Q-Dl)/(B*t))-1)*N/F;
	if(m>0)
		{
			n=m*t*B/(V*log(2))-N/F;
			pr=min(min(p_max,max(n,0)),x);
			Dr=GetRemoteTask(pr);
		}
	else
		{
			min_p=0;
			max_p=min(p_max,x);
			if(GetQoECost(min_p)>QoE_cost)
				{
					pr=0;
					Dr=0;
					return;
				}
			if(GetQoECost(max_p)<QoE_cost)
				{
					pr=max_p;
          Dr=GetRemoteTask(pr);
          return;
				}
			
			min_p=0;
			max_p=p_max;	
			while(absolute(min_p,max_p)>0.001)
			{
				mid_p=(min_p+max_p)/2;
				if(GetQoECost(mid_p)<QoE_cost)
					{
						min_p=mid_p;
					}
				else
					{
						max_p=mid_p;
					}
			}
			
			pr=min_p;
			Dr=GetRemoteTask(pr);
		}
};                             

void Phone::RefreshPhone(){
	Q=max(Q-Dl-Dr,0)+A;
	H=max(H-Dexe,0)+Dr;
};

void Phone::GetRemoteExec(double the_Dexe){
	Dexe=the_Dexe;
};

//给用户分配带宽
void SelectUpLink(Phone *phone,BaseStation *BS){
	double the_B,the_F;
	double d0=1;
	double y=rand()%10 + 1;
	double g0=0.0001;
	int server;
    for (int i=0; i<PHONE_NUM; i++)
    {
    	server=-1;
    	the_B=0;
    	for (int j=0; j<BS_NUM; j++)
    		{
    			if((Connect[j][i]==1)&&(BS[j].Sub_B>the_B))
    				{
    					server=j;
    					the_B=BS[j].Sub_B;
    				}
    		}
    	if(server==-1)
    		{
    			the_F=0;
    		}
    	else
    		{
    			the_F=y*g0*pow(d0/Distance[server][i],4);
    		}
    	phone[i].SeleteSubNet(the_B,the_F,server);  	
    }
};

//给卸载的任务分配计算资源
void DistributionCompute(Phone *phone,BaseStation *BS){
	double total;
	for (int i=0; i<PHONE_NUM; i++)
	{
		phone[i].GetRemoteExec(0);
	}
	for (int i=0; i<BS_NUM; i++)
	{
		total=0;
		for (int j=0; j<PHONE_NUM; j++)
		{
			if(Connect[i][j]==1)
				{
					total+=phone[j].H;
				}
			
		}
		
		for (int j=0; j<PHONE_NUM; j++)
		{
			if(Connect[i][j]==1)
				{
					if(total==0)
						{
							phone[j].Dexe+=0;
						}
					else
						{
							phone[j].Dexe+=BS[i].C_R*phone[j].H/total;
						}					
				}
			
		}
	}
  for (int i=0; i<PHONE_NUM; i++)
		{
		phone[i].Dexe=max(phone[i].H,phone[i].Dexe);
		}
	
};
int main()
{
  Phone phone[PHONE_NUM];
  BaseStation BS[BS_NUM];
  for (int i1=0; i1<BS_NUM; i1++)
  {
  	BS[i1].GetSubNet(i1);
  }
  //每个时间段的资源分配和计算卸载
  srand((unsigned)time(NULL));
  for (int t1=0; t1<T_MAX; t1++)
  {
  	SaveFile<<"########################"<<t1<<"#######################"<<endl;
  	offloading_num=0;
  	SelectUpLink(phone,BS);
  	DistributionCompute(phone,BS);
  	SaveFile<<endl<<"Q   ";
    for (int j1=0; j1<PHONE_NUM; j1++)
    {
    	SaveFile<<phone[j1].Q<<"   ";
    }
    
    SaveFile<<endl<<"H   ";
    for (int j1=0; j1<PHONE_NUM; j1++)
    {
    	SaveFile<<phone[j1].H<<"   ";
    }
    
    SaveFile<<endl<<"Dexe   ";
    for (int j1=0; j1<PHONE_NUM; j1++)
    {
    	SaveFile<<phone[j1].Dexe<<"   ";
    }
    
  	for (int i=0; i<PHONE_NUM; i++)
  	{ 
  		phone[i].A=rand()%A_MAX;		
  		phone[i].GetLocalExec();
  		phone[i].GetTransPower();
  		phone[i].RefreshPhone();
  		if(phone[i].pr>0)
  			{
  				offloading_num++;
  			}
   	}
   	
   	SaveFile<<endl<<"Dl   ";
    for (int j1=0; j1<PHONE_NUM; j1++)
    {
    	SaveFile<<phone[j1].Dl<<"   ";
    }
    
     SaveFile<<endl<<"pl    ";
    for (int j1=0; j1<PHONE_NUM; j1++)
    {
    	SaveFile<<phone[j1].pl<<"   ";
    }
    
    SaveFile<<endl<<"Dr   ";
    for (int j1=0; j1<PHONE_NUM; j1++)
    {
    	SaveFile<<phone[j1].Dr<<"   ";
    }
    
    SaveFile<<endl<<"pr    ";
    for (int j1=0; j1<PHONE_NUM; j1++)
    {
    	SaveFile<<phone[j1].pr<<"   ";
    }
    
    SaveFile<<endl<<"B    ";
    for (int j1=0; j1<PHONE_NUM; j1++)
    {
    	SaveFile<<phone[j1].B<<"   ";
    }
    
    SaveFile<<endl<<"F    ";
    for (int j1=0; j1<PHONE_NUM; j1++)
    {
    	SaveFile<<phone[j1].F<<"   ";
    }
    
    SaveFile<<endl;
    double the_p=0;
    double length=0;
    offloading_num=0;
    for (int j1=0; j1<PHONE_NUM; j1++)
    {
    	the_p=the_p+phone[j1].pl+phone[j1].pr;
    	length=length+phone[j1].Q+phone[j1].H;
    	if(phone[j1].pr>0)
    		{
    			offloading_num++;
    		}
    }
    the_p=the_p/PHONE_NUM;
    length=length/PHONE_NUM;
    OffLoadingFile<<offloading_num<<endl;
    PowerFile<<the_p<<endl;
    LengthFile<<length<<endl;
  }
	return 0;
}
