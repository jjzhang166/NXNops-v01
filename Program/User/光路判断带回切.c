/******************************************************************************

                  版权所有 (C), 2001-2013, 桂林恒毅金宇通信技术有限公司

 ******************************************************************************
  文 件 名   : optics_judge.c
  版 本 号   :
  作    者   :
  生成日期   : 2013年4月26日
  最近修改   :
  功能描述   :
  函数列表   :
******************************************************************************/
#include "config.h"
#include "main.h"
#include "optics_judge.h"
#include "ADC.h"

uint8  OldState[CHANNEL_NUM]= {0};                      //光路通断历史状态
uint8  Optics_Bypass_flag[CHANNEL_NUM]= {1,1,1,1};            //光功率判断切换控制标示:  1表示要切换到旁路 ,0表示要切换到主路

uint32 BackOSW_Time_Tick[CHANNEL_NUM]= {0};             //回切计时变量
uint32 BackAuto_Time_Tick[CHANNEL_NUM]= {0};            //返回自动模式计时变量


/*****************************************************************************
**函 数 名: optics_judge
**功能描述: 把第 ch1 通道的光功率 和第 ch2 通道的光功率判断比较,
             判断是否低于各自通道的切换门限, 再根据 旁路条件 和 回切条件 判断
             是否符合切换光开关的条件，对Optics_Bypass_flag变量置 1或置 0。
**输入参数: uint8 Link_num  链路号
             uint8 ch1  第一个通道
             uint8 ch2  第二个通道
**输出参数: 无
**返 回 值: 无
*****************************************************************************/
void optics_judge(uint8 Link_num , uint8 ch1 ,uint8 ch2 )
{
    if(EPROM.Autoflag[Link_num] == 1 )                  //自动模式
    {
        ChannelLED(Link_num*4,1);                       //点亮自动模式指示灯
        BackAuto_Time_Tick[Link_num] = 0;	            //屏蔽 返回自动模式延时

        //*********************************************************************************/
        //***************************** R1通，R2断****************************************/
        //*********************************************************************************/
        if((power[ch1] > EPROM.q_power[ch1]) && (power[ch2] <= EPROM.q_power[ch2]))
        {
            if(OldState[Link_num] != 2)
            {
                BackOSW_Time_Tick[Link_num] = 0;		            //屏蔽 回切延时
                //switchTime = 0;		                            //屏蔽 切换延时
                //****************************************************************************/
                //旁路条件判断：
                //1.R1, R2都无光才旁路
                //2.只要R1无光就旁路
                //3.只要R2无光就旁路
                //4.R1, R2任一无光旁路
                //****************************************************************************/
                if( EPROM.OSW_Condition[Link_num]==3 || EPROM.OSW_Condition[Link_num]==4 )
                {
                    Optics_Bypass_flag[Link_num]=1;
                }
                //****************************************************************************/
                //回切条件判断：
                //1.R1, R2都有光才回切
                //2.只要R1有光就回切
                //3.只要R2有光就回切
                //4.R1, R2任一有光回切
                //防止与旁路条件冲突（3-2，3-4，4-2，4-4）
                //****************************************************************************/
                else if( EPROM.BackOSW_Condition[Link_num]==2 || EPROM.BackOSW_Condition[Link_num]==4 )     //回切功能
                {
                    if(EPROM.accessflag[Link_num]==1)
                    {
                        if((EPROM.BackOSW_Delay[Link_num] >= 0) && (BackOSW_Time_Tick[Link_num] == 0))
                        {
                            BackOSW_Time_Tick[Link_num] = 1;    //启动计时
                        }
                    }
                }
                OldState[Link_num] = 2;
            }

            if(BackOSW_Time_Tick[Link_num] >= (EPROM.BackOSW_Delay[Link_num] *1000 + 1) )//回切延时 时间到了
            {
                BackOSW_Time_Tick[Link_num] =0;
                Optics_Bypass_flag[Link_num]=0;
            }
        }

        //*********************************************************************************/
        //********************************* R1断，R2通************************************/
        //*********************************************************************************/
        else if((power[ch1] <= EPROM.q_power[ch1]) && (power[ch2] > EPROM.q_power[ch2]))
        {
            if(OldState[Link_num] != 1)
            {
                BackOSW_Time_Tick[Link_num] = 0;		//屏蔽 回切延时
                //switchTime = 0;		                //屏蔽 切换延时
                //****************************************************************************/
                //旁路条件判断：
                //1.R1、R2都无光才旁路
                //2.只要R1无光就旁路
                //3.只要R2无光就旁路
                //4.R1、R2任一无光旁路
                //****************************************************************************/
                if( EPROM.OSW_Condition[Link_num]==2 || EPROM.OSW_Condition[Link_num]==4 )
                {
                    Optics_Bypass_flag[Link_num]=1;
                }
                //****************************************************************************/
                //回切条件判断：
                //1.R1、R2都有光才回切
                //2.只要R1有光就回切
                //3.只要R2有光就回切
                //4.R1、R2任一有光回切
                //防止与旁路条件冲突（2-3，2-4，4-3，4-4）
                //****************************************************************************/
                else if( EPROM.BackOSW_Condition[Link_num]==3 || EPROM.BackOSW_Condition[Link_num]==4 )
                {
                    if(EPROM.accessflag[Link_num]==1)
                    {
                        if( (EPROM.BackOSW_Delay[Link_num] >= 0) && (BackOSW_Time_Tick[Link_num] == 0))
                        {
                            BackOSW_Time_Tick[Link_num] = 1; //启动计时
                        }
                    }
                }
                OldState[Link_num] = 1;
            }
            if( BackOSW_Time_Tick[Link_num]  >= (EPROM.BackOSW_Delay[Link_num] *1000 + 1) )//回切延时 时间到了
            {
                BackOSW_Time_Tick[Link_num] =0;
                Optics_Bypass_flag[Link_num]=0;
            }
        }
        //*********************************************************************************/
        //************************************ R1断，R2断  *******************************/
        //*********************************************************************************/
        else if((power[ch1] <= EPROM.q_power[ch1]) && (power[ch2] <= EPROM.q_power[ch2]))
        {
            if(OldState[Link_num] != 0)
            {
                BackOSW_Time_Tick[Link_num] = 0;		//屏蔽 回切延时
                //switchTime = 0;		                //屏蔽 切换延时
                //****************************************************************************/
                //旁路条件判断：
                //1.R1、R2都无光才旁路
                //2.只要R1无光就旁路
                //3.只要R2无光就旁路
                //4.R1、R2任一无光旁路
                //****************************************************************************/
                Optics_Bypass_flag[Link_num]=1;

                OldState[Link_num] = 0;
            }
        }

        //*********************************************************************************/
        //************************************ R1通，R2通  *******************************/
        //*********************************************************************************/
        else if((power[ch1] > EPROM.q_power[ch1]) && (power[ch2] > EPROM.q_power[ch2]))
        {
            if(OldState[Link_num] != 3)
            {
                BackOSW_Time_Tick[Link_num] = 0;		//屏蔽 回切延时
                //switchTime = 0;		                //屏蔽 切换延时

                //回切功能
                if( EPROM.accessflag[Link_num]==1 )
                {
                    if((EPROM.BackOSW_Delay[Link_num] >= 0) && (BackOSW_Time_Tick[Link_num] == 0))
                    {
                        BackOSW_Time_Tick[Link_num] = 1;  //启动计时
                    }
                }
                OldState[Link_num] = 3;
            }
            if(BackOSW_Time_Tick[Link_num] >= (EPROM.BackOSW_Delay[Link_num] *1000 + 1) )
            {
                BackOSW_Time_Tick[Link_num] =0;
                Optics_Bypass_flag[Link_num]=0;
            }
        }
    }

    //*********************************************************************************/
    //*************************************手动模式************************************/
    //*********************************************************************************/
    else if(EPROM.Autoflag[Link_num] == 0)
    {
        ChannelLED(Link_num*4,0);               //灭自动模式指示灯
        BEE_OFF;                                //关蜂鸣器
        BackOSW_Time_Tick[Link_num]   = 0;      //屏蔽 回切延时
//      switchTime = 0;                         //屏蔽 切换延时

        if( (EPROM.Auto_Manual_delay[Link_num] > 0) && (BackAuto_Time_Tick[Link_num] == 0) )
        {
            BackAuto_Time_Tick[Link_num] = 1;   //启动计时
        }

        if( BackAuto_Time_Tick[Link_num] >= (EPROM.Auto_Manual_delay[Link_num] *60000 + 1) )
        {
            BackAuto_Time_Tick[Link_num] =0;
            EPROM.Autoflag[Link_num] = 1;
            Save_To_EPROM((uint8 *)&EPROM.Autoflag[Link_num], 1);

            BEE_ON;                     //开蜂鸣器
            OSTimeDly(50);
            BEE_OFF;                    //关蜂鸣器
        }
    }
}

