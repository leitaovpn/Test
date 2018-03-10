/*
* 论文的比较算法
*Joint Task offloading and Resource Allocation 
*  Created on: 2017年8月13日
*      Author: leitao
*/
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>
#include<stdbool.h>
//用户
struct user{
	double d; //用户卸载任务的大小
	double c;//用户卸载任务的计算量
	double f;//用户CPU的频率
	double k;//基于CPU架构的能量系数
	double t;//卸载任务在本地的执行时间
	double E;//卸载任务在本地执行消耗的能量
	double bt;//用户对时间的重视程度
	double be;//用户对能量消耗的重视程度
	double like;//资源提供者对用户的偏好
  double P;//总的上传功率
  double p;//用户的卸载任务上传功率，当 u不属于Uoff 时，p=0
};
//MEC服务器
struct server{
	double f;//服务器的总的计算资源
	double B;//服务器带宽
	double N;//将服务器分为N个子带
	double W;//子带的带宽
};
//卸载决策
struct TaskOffloading{
	int x;//该决策是否被执行，=1执行，否则 不执行
	double h;//该任务卸载决策的上行链路收益
	double I;//小区间内的干扰
	double Y;//信号加干扰与噪声比
};
int num_u=30,num_s=3,num_j=10;
struct user U[60];
struct server S[60];
struct TaskOffloading TO[60][60][60];
double f[60][60];  //服务器分配给用户的卸载任务的计算资源
//用户与服务器的可连接情况，=1表示可连接，反之则表示不可连接。
double Connect[60][60];
//服务器和用户之间的距离
double Distance[60][60];
double q = 100;  //背景噪声方差
//返回1，表示用户u将任务卸载到服务器s上
int getXus(int u,int s)
{
    int xus=0;
    for(int j=0;j<num_j;j++)
    {
       xus+=TO[u][s][j].x;
    }
    return xus;
}
//返回1，表示u将任务卸载到某台服务器上
int getX(int u)
{
    int x=0;
    for(int s=0;s<num_s;s++)
    {
       x+=getXus(u,s);
    }
    return x;
}
//返回true，表示用户u是卸载到服务器s的用户
bool isOfUs(int u,int s)
{
  if(getXus(u,s)==1)
    {return true;}
  return false;
}
//返回true，表示用户u是卸载用户
bool isOfUoff(int u)
{
    if(getX(u)==1)
       {return true;}
    return false;
}
//初始化用户信息
void userInit()
{
    for (int i = 0; i<num_u; i++)
	{
		U[i].d = 0.42;
		U[i].c = 1000;
		U[i].f = 1000;
    U[i].P = 20;
		U[i].k = 0.000000005;
		U[i].bt = 0.2;
		U[i].be = 1 - U[i].bt;
		U[i].like = 1;
    U[i].p = 0;
		U[i].t = U[i].c / U[i].f;
		U[i].E = U[i].k*U[i].f*U[i].f*U[i].c;
	}
}
//初始化服务器信息
void serverInit()
{
     for (int i = 0; i<num_s; i++)
	{
		S[i].B = 20;
		S[i].f = 20000;
		S[i].N = num_j;
		S[i].W = S[i].B / S[i].N;
	}
}
//初始化决策信息
void decisionInit()
{
  double rad;
	srand((int)time(0));
	for (int i = 0; i<num_u; i++)
	{
		for (int j = 0; j<num_s; j++)
		{
			rad = 140.7 + 36.7*log10(Distance[i][j]);
			for (int k = 0; k<num_j; k++)
			{
				TO[i][j][k].x = 0;
				TO[i][j][k].h = pow(10,rad/10);
				TO[i][j][k].I = 0;
				TO[i][j][k].Y = 0;
			}

		}
	}
}
//初始化参数
void setInit()
{
   userInit();
   serverInit();
   decisionInit();
}
//小区间内干扰
void Interference(int u,int s,int j)
{
   double IF=0;
   for(int w=0;w<num_s;w++)
   {
      if(w!=s)
      {
         for(int k=0;k<num_u;k++)
         {
           if(isOfUs(k,w))
            {
            	/*把dB转换为W*/
              IF+=TO[k][w][j].x*U[k].p*TO[k][w][j].h;
            }
         }
      }
   }
   TO[u][s][j].I=IF;
}
//信号干扰加噪声比
void SINR()
{
    double signal=0;
    for(int u=0;u<num_u;u++)
    {
       for(int s=0;s<num_s;s++)
       {
           for(int j=0;j<num_j;j++)
           {
             if(isOfUs(u,s))
             {
                 Interference(u,s,j);
                 signal=U[u].p*TO[u][s][j].h;
                 TO[u][s][j].Y=signal/(TO[u][s][j].I+0.001*pow(10,q/10));
             }
           }
        }
     }
}
//小区间内干扰近似值
double similarInterference(int u,int s,int j)
{
   double IF=0;
   for(int w=0;w<num_s;w++)
   {
      if(w!=s)
      {
         for(int k=0;k<num_u;k++)
         {
            if(isOfUs(k,w))
            {
              IF+=TO[k][w][j].x*U[k].P*TO[k][w][j].h;
            }
         }
      }
   }
   return IF;
}
//求UPA中的Vus
double getVus(int u,int s)
{
   double V=0;
   if(isOfUs(u,s))
   {
     for(int j=0;j<num_j;j++)
     {
       V+=TO[u][s][j].h/(similarInterference(u,s,j)+q);
     }
     //printf("V[%d][%d]=%f \n",u,s,V);
   }
   return V;
}
//UPA的一阶导数是否大于0
bool ifResultLzero(int u,int s,double p,double Vus)
{
    double Yu,Qu;
    Qu=(U[u].like*U[u].bt*U[u].d)/(U[u].t*S[s].W);
    Yu=(U[u].like*U[u].be*U[u].d)/(U[u].E*S[s].W);

    double L1,L2;
    L1=Vus*(Qu+Yu*p);
    L2=(1+Vus*p)*log(2);

    double M1,M2;
    M1=Yu*log(1+Vus*p)/log(2);
    M2=L1/L2;
    
    if(M1>M2)
    {return true;}

    return false;
}
//UPA
void saveUPA()
{
   // printf("##############   saveUPA()    ########################\n");
    double p;
    double p1,p2;
    double x1=0.001;
     double Vus;
    for(int u=0;u<num_u;u++)
    {
       p=0;
       for(int s=0;s<num_s;s++)
       {
         if(isOfUs(u,s))
         {
            Vus=getVus(u,s);
            if(!ifResultLzero(u,s,U[u].P,Vus))
            {
               p=U[u].P;
              // printf("first U[%d]=%f \n",u,p);
            }
            else
            {
              p1=0,p2=U[u].P;
              while((p2-p1)>x1)
              {
                 p=(p1+p2)/2;
                 if(ifResultLzero(u,s,p,Vus))
                 {p2=p;}
                 else
                 {p1=p;}
              }
              p=(p1+p2)/2;
              //printf("last U[%d]=%f \n",u,p);
            }
         break;
         }
       }
       U[u].p=p;

    }
    SINR();
}
//CRA参数
double getNu(int u)
{
    return U[u].like*U[u].bt*U[u].f;
}
//服务器s分配给用户u的计算资源
double getFus(int u,int s)
{
   double v1=0,v2=0;
   if(isOfUs(u,s))
   {
      v1=S[s].f*sqrt(getNu(u));
      for(int i=0;i<num_u;i++)
      { 
        if(isOfUs(i,s))
          {v2+=sqrt(getNu(i));}
      }
      return v1/v2;
   }
   return 0;
}
//CRA
void saveCRA()
{
   for(int u=0;u<num_u;u++)
   {
      for(int s=0;s<num_s;s++)
      {
         f[u][s]=getFus(u,s);
      }
   }
}
//用户u与服务器s之间的干扰
double getYus(int u,int s)
{
   double Y=0;
   for(int j=0;j<num_j;j++)
   {

      Y+=TO[u][s][j].Y;
   }

   return Y;
}
//系统整体效益
double SysGain()
{
   saveUPA();
   saveCRA();
   double L=0;
   for(int u=0;u<num_u;u++)
   {
      if(isOfUoff(u))
      {
         L+=U[u].like*(U[u].bt+U[u].be);
      }
   }

   double F=0,mylog,Qu,Yu;
   for(int u=0;u<num_u;u++)
   {
      for(int s=0;s<num_s;s++)
      {
        if(isOfUs(u,s))
        {
          mylog=log(1+getYus(u,s))/log(2);
          Qu=(U[u].like*U[u].bt*U[u].d)/(U[u].t*S[s].W);
          Yu=(U[u].like*U[u].be*U[u].d)/(U[u].E*S[s].W);
          F+=(Qu+Yu*U[u].p)/mylog;
        }
      }
   }

   double V=0;
   for(int u=0;u<num_u;u++)
   {
      
      for(int s=0;s<num_s;s++)
      {
         if(isOfUs(u,s))
         {
            V+=getNu(u)/f[u][s];           
         }
      }
   }
   return L-F-V;
}
//移除一个卸载策略
void myremove(int u,int s,int j)
{
  if(TO[u][s][j].x==1)
  {
    TO[u][s][j].x=0;
  }
}
//添加一个策略
void myexchange(int u,int s,int j)
{  if(TO[u][s][j].x==0)
   {
    for(int w=0;w<num_s;w++)
    {
       for(int i=0;i<num_j;i++)
       {
          myremove(u,w,i);
       }
    }
    for(int v=0;v<num_u;v++)
    {
       myremove(v,s,j);
    }
    TO[u][s][j].x=1;
   }
}
int backup[60][60][60];
//备份
void BackUp()
{
    for(int u=0;u<num_u;u++)
    {
     for(int s=0;s<num_s;s++)
     {
         for(int j=0;j<num_j;j++)
         {
           backup[u][s][j]=TO[u][s][j].x;
         }
     }
   }
}
//还原
void BackDown()
{
     for(int u=0;u<num_u;u++)
    {
     for(int s=0;s<num_s;s++)
     {
         for(int j=0;j<num_j;j++)
         {
          TO[u][s][j].x= backup[u][s][j];
         }
     }
   }
}
//找到第一个决策
void FindMax()
{
  int u1=0,s1=0,j1=0;
  double TheGain=0,sysGain=0;
  //TheGain=SysGain();
  for(int u=0;u<num_u;u++)
  {
     for(int s=0;s<num_s;s++)
    {
    	if(Connect[u][s]==1)
    	{
         for(int j=0;j<num_j;j++)
        {
        	
        	  TO[u][s][j].x=1;
            sysGain=SysGain();
            if(TheGain<sysGain)
            {
               TheGain=sysGain;
               u1=u;
               s1=s;
               j1=j;
            }
          TO[u][s][j].x=0;
        }
      }  
    }
  }
  TO[u1][s1][j1].x=1;
}
//是否移除一个决策，系统效益增加
bool ifRemoveDicesion(double Increate)
{
     double thedicesion,nextdicesion;  //当前决策的系统收益、下一个决策的系统收益
     for(int u=0;u<num_u;u++)
     {
        for(int s=0;s<num_s;s++)
        {
        	if(Connect[u][s]==1)
        	{
            for(int j=0;j<num_j;j++)
            {
              if(TO[u][s][j].x==1)
              {
                 BackUp();
                 thedicesion=SysGain();
                 myremove(u,s,j);
                 nextdicesion=SysGain();
                 if(nextdicesion>(Increate*thedicesion))
                 {
                   return true;
                 }
                 else
                 {
                   BackDown();
                 }
              }
            }
          } 
        }
     }
     return false;
}
//是否添加一个决策，系统效益增加
bool ifExchangeDicesion(double Increate)
{
     double thedicesion,nextdicesion;  //当前决策的系统收益、下一个决策的系统收益
     for(int u=0;u<num_u;u++)
     {
        for(int s=0;s<num_s;s++)
        {
        	if(Connect[u][s]==1)
        		{
              for(int j=0;j<num_j;j++)
              {
                  if(TO[u][s][j].x==0)
                 {
                    BackUp();
                    thedicesion=SysGain();
                    myexchange(u,s,j);
                    nextdicesion=SysGain();
                    if(nextdicesion>(Increate*thedicesion))
                   {
                      return true;
                   }
                  else
                   {
                      BackDown();
                   }
                 }
              }
            }  
        }
     }
     return false;
}
//当前JTORA
void getJTORA()
{
    for(int u=0;u<num_u;u++)
    {
    	for(int s=0;s<num_s;s++)
    	{
    		for(int j=0;j<num_j;j++)
    		{
    			if(TO[u][s][j].x==1)
    			{
    				printf("将用户u=%d的任务通过j=%d子带卸载到服务器s=%d上,",u,j,s);
                                printf("服务器分配功率为：%f ,",f[u][s]);
                                printf("传输功率为：%f \n",U[u].p);
                                
    			}
    		}
    	}
    }
}
//TO问题，求卸载决策
void TaskOffloadingScheduling()
{
     double Increate=(1+5/36);   //增长系数
     setInit();
     FindMax();
     while(1)
     {
         printf("######################################################### \n");
         printf("系统收益：%f \n",SysGain());
         getJTORA();
         if(ifRemoveDicesion(Increate))
         {
          printf("\n");
          continue;
         }
         else if(ifExchangeDicesion(Increate))
         {
          printf("\n");
          continue;
         }
         else
         {return ;}

     }
}
int main()
{
  TaskOffloadingScheduling();
	return 0;
}

