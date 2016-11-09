#include <ioCAN128v.h>
#include <macros.h>
#define uint unsigned int
#define uchar unsigned char 
 
/*定义1602的控制脚*/
#define LCM_RS_1 PORTE|=(1<<0)    				
#define LCM_RS_0 PORTE&=(~(1<<0))
#define LCM_RW_1 PORTE|=(1<<1)
#define LCM_RW_0 PORTE&=(~(1<<1))
#define LCM_E_1 PORTE|=(1<<2)
#define LCM_E_0 PORTE&=(~(1<<2))
unsigned char LCM_Num_Table[]={'0','1','2','3','4','5','6','7','8','9'};
 
#define R1 0.508
#define R2 0.3325
#define RJ 0.01012
#define R6 100.4
#define R7 1.0003
#define VCC 5.027

/********************************************
* 函  数：延时函数
* 参  数：要延时的时间
* 返回值：无
********************************************/ 
void delay_1_us(void)
{
 	 NOP();
	 NOP();
	 NOP();
	 NOP();
}
void delay_n_us(uint n_us)
{
 	 uint cnt_i;
	 for(cnt_i=0;cnt_i<n_us;cnt_i++)
	 {
	  	delay_1_us();
	 }
}
void delay_1_ms(void)
{
 	 uchar cnt_i,cnt_j;
	 for(cnt_i=0;cnt_i<40;cnt_i++)
	 {
	  	for(cnt_j=0;cnt_j<33;cnt_j++)
		{
		
		}
	 }
}
void delay_n_ms(uint n_ms)
{
 	 uint cnt_i;
	 for(cnt_i=0;cnt_i<n_ms;cnt_i++)
	 {
	  	delay_1_ms();
	 }
}

/*LCD显示功能定义*/
uchar LCM_Re_BAC(void)
{
 	uchar status;
	DDRB=0x00;//LCM_Dat为输入
	LCM_RS_0;//选择指令通道
	LCM_RW_1;//选择读操作
	LCM_E_1;//使能线置1
	delay_n_us(1);//等待信号线稳定
	status=PINB;//读入
	LCM_E_0;//使能线置0
	return status;
}
void LCM_Wr_CMD(uchar cmd_dat)
{
 	while(LCM_Re_BAC()>=0x80);//判忙
	DDRB=0xff;//LEC_Dat为输出
	LCM_RS_0;//选择指令通道
	LCM_RW_0;//选择写操作
	LCM_E_1;//使能线置1
	PORTB=cmd_dat;//设置指令数据
	delay_n_us(1);
	LCM_E_0;
}
void LCM_Wr_DAT(uchar dis_dat)
{
 	 while(LCM_Re_BAC()>=0x80);//判忙
	 DDRB=0xff;//LCM_Dat为输出
	 LCM_RS_1;//选择数据通道
	 LCM_RW_0;//选择写操作
	 LCM_E_1;
	 PORTB=dis_dat;
	 delay_n_us(1);
	 LCM_E_0;
}
void LCM_1602_Init(void)
{
 	 LCM_Wr_CMD(0x38);//显示模式设置：16*2显示，5*7点阵，8位数据接口
	 delay_n_ms(5);//延时5ms
	// LCM_Wr_CMD(0x38);
	// delay_n_ms(5);
	// LCM_Wr_CMD(0x38);
	 //delay_n_ms(5);
	 LCM_Wr_CMD(0x0f);//显示模式设置：显示开，有光标，光标闪烁
	 delay_n_ms(5);
	 LCM_Wr_CMD(0x06);//显示模式设置：光标右移，字符不移
	 delay_n_ms(5);
	 LCM_Wr_CMD(0x01);//清屏幕指令，将以前的显示内容清除
	 delay_n_ms(5);
}

/********************************************
* 函  数：ADC0初始化
* 参  数：空
* 返回值：无
********************************************/
void adc0_init(void)
{
  ADCSRA=0x00;//关闭ADC
  ADMUX=0x00;//AREF,内部VREF关闭，数据右对齐，通道0
  ACSR=0x80;//关闭模拟比较器的电源
  ADCSRA=0x87;//使能ADC，单次转换，ADC转换中断禁止，128分频
}

/********************************************
* 函  数：端口初始化函数
* 参  数：空
* 返回值：无
********************************************/
void port_init(void)
{
  DDRA=0xFF;           //继电器控制信号输出
  PORTA=0x00;          //初始值，所有开关都断开
 
  DDRF=0x00;          //采样输入
  PORTF=0x00;
   
  DDRC=0xFF;           //LED警报灯输出
  PORTC=0x01;
  
  DDRE=0xff;         //LCD输出
  PORTE=0xff;
}    

/********************************************
* 函  数：求平均值
* 参  数：Rn[],Rp[]
* 返回值：平均值
********************************************/
float Avg(float a[],int n)
{
 	float average=0;
	int j;
	float sum=a[0],max=a[0],min=a[0];
	for (j=1;j<n;j++)
	{
		if (max<a[j])
			max=a[j];
		if (min>a[j])
			min=a[j];
        sum=sum+a[j];
	}
	average=(sum-min-max)/(n-2);
	return average;
}

/********************************************
* 函  数：采样主函数
* 参  数：空
* 返回值：无
********************************************/
void main(void)
{
 unsigned char temp;
 uint adc_data[4];
 //UR7=adc_data[0]十六进制0-0x03ffH
 //UR8=adc_data[1]十六进制0-0x03ffH，经过OP电路处理后的数据，只求比例，可直接计算
 //URJ1=adc_data[2]十六进制0-0x03ffH，经过OP电路处理后的数据，不可直接计算
 //URJ2=adc_data[3]十六进制0-0x03ffH，经过OP电路处理后的数据，不可直接计算
 float adc_val[4];
 //UR7=adc_val[0]电压值0-5v
 //UR8=adc_val[1]电压值0-5v，经过OP电路处理后的数据，只求比例，可直接计算
 //URJ1=adc_val[2]电压值0-5v，经过OP电路处理后的数据，不可直接计算
 //URJ2=adc_val[3]电压值0-5v，经过OP电路处理后的数据，不可直接计算
 uint i,k;
 float adc_t,UJ_1=0,UJ_2=0;//需要转换得到UJ的实际端电压
 float ratio=0.0;//R+与R-的比值
 float sub=0.0;//sub=UJ_1-UJ_2
 float R_para;//R+与R-并联阻值
 float Rp_float=0,Rn_float=0,R_min=0;
 float UM=0;
 
 float Rp[10],Rn[10],Rpa[10];
 float Rp_avg[5],Rn_avg[5],Rpa_avg[5];
 
 unsigned long Rp_int=0,Rn_int=0;
 uchar Rp_char[7],Rn_char[7],UJ1_char[3];
 
 unsigned long Rp_nextbit,Rn_nextbit;
 
 port_init(); 
 LCM_1602_Init();
 
 while(1)
 {
 for(k=0;k<5;k++)
 {
   for(i=0;i<10;i++)
   {
   /*后面的测量电路暂时不接入，先测量正负绝缘电阻的比值*/
   PORTA=0x01;//PA0=1，控制K5、K6闭合
   //delay_n_ms(50); 
   adc0_init();//开始0通道AD
   ADCSRA|=(1<<ADSC);//开始转换
   while(!(ADCSRA&(1<<ADIF)));//等待AD转换结束
   adc_data[0]=ADCL;//读取低8位
   adc_data[0]|=(ADCH&0x03)<<8;//读取高2位
   adc_val[0]=(float)adc_data[0]*VCC/1024;///10位精度的二进制数0-0x03ffH转换成0~5v的电压值
 
   ADMUX=0x01;//选通道1.开始采样R8电压
   ADCSRA|=(1<<ADSC);//开始转换
   while(!(ADCSRA&(1<<ADIF)));//等待AD转换结束
   adc_data[1]=ADCL;//读取低8位
   adc_data[1]|=(ADCH&0x03)<<8;//读取高2位
   adc_val[1]=(float)adc_data[1]*VCC/1024;///10位精度的二进制数0-0x03ffH转换成0~5v的电压值
 
   PORTA=0x00;//断开K5 K6

   /*比值确定后可以开始双不对称模块*/
   PORTA=0x06;//闭合K3 K4以及K1，开始测桥电阻电压1
   delay_n_ms(150);
   ADMUX=0x02;//通道2
   ADCSRA|=(1<<ADSC);//开始转换
   while(!(ADCSRA&(1<<ADIF)));//等待AD转换结束
   adc_data[2]=ADCL;//读取低8位
   adc_data[2]|=(ADCH&0x03)<<8;//读取高2位
   adc_val[2]=(float)adc_data[2]*VCC/1024;
   UJ_1=adc_val[2]*6-3*VCC;   

   PORTA=0x00;
   PORTA=0x0A;//闭合K3 K4以及K2，开始测桥电阻电压2
   delay_n_ms(150);
   ADMUX=0x02;//通道2
   ADCSRA|=(1<<ADSC);//开始转换
   while(!(ADCSRA&(1<<ADIF)));//等待AD转换结束
   adc_data[3]=ADCL;//读取低8位
   adc_data[3]|=(ADCH&0x03)<<8;//读取高2位
   adc_val[3]=(float)adc_data[3]*VCC/1024;//???
   UJ_2=adc_val[3]*6-3*VCC;
   
   sub=UJ_1-UJ_2;  
   UM=(adc_val[0]+adc_val[1])*((R6+R7)/R7);
   Rpa[i]=(UM*RJ*(R2-R1))/(sub*(R1+R2))-(R1*R2)/(R1+R2)-RJ;
/*为了减小误差*/
   if(adc_val[0]<=adc_val[1])
   {
   		ratio=adc_val[0]/adc_val[1];
        Rn[i]=Rpa[i]*(ratio+1)/ratio;//实际值
        Rp[i]=ratio*Rn[i];//实际值
   }
   else 
   {
	    ratio=adc_val[1]/adc_val[0];
		Rp[i]=Rpa[i]*(ratio+1)/ratio;//实际值
        Rn[i]=ratio*Rp[i];//实际值
   }
 }
   Rp_avg[k]=Avg(Rp,10);
   Rn_avg[k]=Avg(Rn,10);
   Rpa_avg[k]=Avg(Rpa,10);

 }
   Rp_float=(Rp_avg[0]+Rp_avg[1]+Rp_avg[2]+Rp_avg[3]+Rp_avg[4])/5;
   Rn_float=(Rn_avg[0]+Rn_avg[1]+Rn_avg[2]+Rn_avg[3]+Rn_avg[4])/5;
   R_para=(Rpa_avg[0]+Rpa_avg[1]+Rpa_avg[2]+Rpa_avg[3]+Rpa_avg[4])/5;
   if(Rn_float>Rp_float)
   		R_min=Rp_float;
   else
   	    R_min=Rn_float;

   Rp_nextbit=(unsigned long)(Rp_float*10000000)%10;
   if(Rp_nextbit>=5)    Rp_int=(unsigned long)(Rp_float*1000000)+1;	
   else                 Rp_int=(unsigned long)(Rp_float*1000000);	   
   
   Rn_nextbit=(unsigned long)(Rn_float*10000000)%10;
   if(Rn_nextbit>=5)    Rn_int=(unsigned long)(Rn_float*1000000)+1;	
   else                 Rn_int=(unsigned long)(Rn_float*1000000);	
  
   if(Rn_int>9999999) Rn_int=9999999;
   Rn_char[6]=Rn_int/1000000;  //最高位
   Rn_char[5]=(Rn_int%1000000)/100000;
   Rn_char[4]=((Rn_int%1000000)%100000)/10000;
   Rn_char[3]=(((Rn_int%1000000)%100000)%10000)/1000;
   Rn_char[2]=((((Rn_int%1000000)%100000)%10000)%1000)/100;
   Rn_char[1]=((((Rn_int%1000000)%100000)%10000)%1000)%100/10;
   Rn_char[0]=(((((Rn_int%1000000)%100000)%10000)%1000)%100)%10;//最低位
  
   if(Rp_int>9999999) Rp_int=9999999;
   Rp_char[6]=Rp_int/1000000;  //最高位
   Rp_char[5]=(Rp_int%1000000)/100000;
   Rp_char[4]=((Rp_int%1000000)%100000)/10000;
   Rp_char[3]=(((Rp_int%1000000)%100000)%10000)/1000;
   Rp_char[2]=((((Rp_int%1000000)%100000)%10000)%1000)/100;
   Rp_char[1]=((((Rp_int%1000000)%100000)%10000)%1000)%100/10;
   Rp_char[0]=(((((Rp_int%1000000)%100000)%10000)%1000)%100)%10;
 
 	 LCM_Wr_CMD(0x01);//清屏幕指令，将以前的显示内容清除
	 delay_n_ms(5);
	 
		LCM_Wr_CMD(0x80);
		LCM_Wr_DAT('R');
		LCM_Wr_DAT('+');
		LCM_Wr_DAT('=');
		LCM_Wr_DAT(LCM_Num_Table[Rp_char[6]]);
		LCM_Wr_DAT(LCM_Num_Table[Rp_char[5]]);
		LCM_Wr_DAT(LCM_Num_Table[Rp_char[4]]);
		LCM_Wr_DAT(LCM_Num_Table[Rp_char[3]]);
		LCM_Wr_DAT('.');
		LCM_Wr_DAT(LCM_Num_Table[Rp_char[2]]);
		LCM_Wr_DAT(LCM_Num_Table[Rp_char[1]]);
		LCM_Wr_DAT(LCM_Num_Table[Rp_char[0]]);
		LCM_Wr_DAT('K');
		
		if((R_min>0.001)&&(R_min<0.0001*UM))
		{
		 	LCM_Wr_DAT(' ');
			LCM_Wr_DAT('T');
			LCM_Wr_DAT('O');
			LCM_Wr_DAT('O');
			LCM_Wr_CMD(0xcd);
			LCM_Wr_DAT('L');
			LCM_Wr_DAT('O');
			LCM_Wr_DAT('W');
			PORTC=0x04;
		}
		LCM_Wr_CMD(0xc0);
		LCM_Wr_DAT('R');
		LCM_Wr_DAT('-');
		LCM_Wr_DAT('=');
		LCM_Wr_DAT(LCM_Num_Table[Rn_char[6]]);
		LCM_Wr_DAT(LCM_Num_Table[Rn_char[5]]);
		LCM_Wr_DAT(LCM_Num_Table[Rn_char[4]]);
		LCM_Wr_DAT(LCM_Num_Table[Rn_char[3]]);
		LCM_Wr_DAT('.');
		LCM_Wr_DAT(LCM_Num_Table[Rn_char[2]]);
		LCM_Wr_DAT(LCM_Num_Table[Rn_char[1]]);
		LCM_Wr_DAT(LCM_Num_Table[Rn_char[0]]);
		LCM_Wr_DAT('K');
		LCM_Wr_DAT(' ');
		if((R_min>0.0001*UM)&&(R_min<0.0005*UM))
		{
		 	LCM_Wr_DAT('L');
			LCM_Wr_DAT('O');
			LCM_Wr_DAT('W');
			PORTC=0x02;
		}
		if(R_min<0.001)
		{
			LCM_Wr_DAT('R');
			LCM_Wr_DAT('~');
			LCM_Wr_DAT('0');
			PORTC=0x04;
		}
   		delay_n_ms(500);
}
}
