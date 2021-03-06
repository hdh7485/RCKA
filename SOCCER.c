﻿
#define SHOOT 165	//공격수 슈팅 기준값,	먼곳에서 슈팅을 시도하면 이값을 키운다.
#define NEAR 145	//공격수 공 근처의 기준값,	공을 끼고 너무 크게 돌면 이값을 키운다.
#define SHOOT2 180	//수비수 슈팅 기준값,	먼곳에서 슈팅을 시도하면 이값을 키운다.
#define NEAR2 90	//수비수 공 감지 기준값,	가반응거리가 짧으면  이값을 줄인다.
//
// 아래 정의값들은 경기장 테두리의흰색 선의 기준값입니다.
// 경기장에서 직접 측정하여 변경하던지 가변저항을 조정하세요.
//
#define Line0_White 50	
#define Line1_White 50
#define Line2_White 50

//
// 위 기준값을 넘은 경우가 있으면 아래 변수에 기록이 되어짐.
// 선을 감지하고 그에따른 액션을 수행한후에는 아래값을 Clear 시켜주세요.
//
// 0 bit : line 0 detected
// 1 bit : line 1 detected
// 2 bit : line 2 detected
//
unsigned  char LineDetected=0;

//----------------------------------------------------------------------------

#define KP 1
//----------------------------------------------------------------------------

double memComp=1800;

// Eric Han 2016. 1. 29 .Ver.1.29
//
// 보드에 장착된 네개의 LED들을 On/Off 시키는 명령 정의
// 프로그램 작성중에 인디케이팅용으로 사용 가능함.
//
// UART 사용시에는 사용할 수 없음.
//
// RXLEDON , RXLEDOFF
// TXLEDON , TXLEDOFF 
// RX1LEDON , RX1LEDOFF
// TX1LEDON , TX1LEDOFF
//

//
//------------- 여기부터의 변수는 읽기만 하세요. ------------------------
//
// 아래 변수에 센서 값들이 들어옵니다.
// 읽기만 하시고 절대 쓰지 마세요.
//
unsigned int analog[16];	// 0 : 원거리 적외선 0 값
							// 1 : 근거리 적외선 0 값
							// 2 : 원거리 적외선 1 값
							// 3 : 근거리 적외선 1 값
							// 4 : 원거리 적외선 2 값
							// 5 : 근거리 적외선 2 값
							// 6 : 원거리 적외선 3 값
							// 7 : 근거리 적외선 3 값
							// 8 : 원거리 적외선 4 값
							// 9 : 근거리 적외선 4 값
							// 10 : 원거리 적외선 5 값
							// 11 : 근거리 적외선 5 값
							// 12 : 배터리 모니터링
							// 13 : 바닥감지 센서 0
							// 14 : 바닥감지 센서 1
							// 15 : 바닥감지 센서 2
							
int an[16][16];
int an_avg[16];						
//
//	초음파 센서 거리 측정 값(0~3번 초음파)
//							
unsigned int ultra[4];		

//
// 가공되어진 적외선 센서 값
// 원거리용 센서와 근거리용 센서를 조합하여 계산되어진 값.
//
unsigned int ir[12];		//시계방향으로 0시~11시방향의 값

int ball_dir;				//적외선 값이 가장 강하게 보이는 방향.
int max_ir;					//가장 강한 적외선 값.
int last_pos = 0;			//공이 마지막으로 있던 좌우 방향, 왼쪽 = 1, 오른쪽 = 0;
int detect_light = 0;
// 컴파스 센서 값.
double compass;
//11번의 가중치를 늘려서 11의 감지가 더 잘되게 함.
//------------- 여기까지의 변수는 읽기만 하세요. ------------------------


unsigned char menu = 0;

#include "SOCCER2.h"


void WarningDisplay()
{	
	OCR1A = 0;
	OCR1B = 0;
	OCR2B = 0;
	Lcd_Init();
	Lcd_Write_String(LINE1,"-WARNING-");
	Lcd_Write_String(LINE2,"  LOW BATTERY");
	while(1)
		Volt_Display();
}
void MOTORA(int ma)
{
	int tmp = abs(ma);
	if((int)voltage < 95)
	{
		WarningDisplay();
	}
	else
	{
		tmp=tmp*255/100;
		if(tmp>255)	tmp=255;
		
		if(ma<0)	madirB;
		else		madirF;
		OCR1A = tmp;
	}
}
void MOTORB(int mb)
{
	int tmp = abs(mb);
	
	tmp=tmp*255/100;
	if(tmp>255)	tmp=255;
	if((int)voltage < 95)
	{
		WarningDisplay();
	}
	else
	{
		if(mb<0)	mbdirB;
		else		mbdirF;
		OCR1B = tmp;
	}
}
void MOTORC(int mc)
{
	int tmp = abs(mc);
	
	tmp=tmp*255/100;
	if(tmp>255)	tmp=255;
	if((int)voltage < 95)
	{
		WarningDisplay();
	}
	else
	{
		if(mc<0)	mcdirB;
		else		mcdirF;
		OCR2B = tmp;
	}
}

void move(int ma, int mb, int mc)
{
	MOTORA(ma);
	MOTORB(mb);
	MOTORC(mc);	
}


void view_line(void)
{
	Lcd_Clear();
	Lcd_Write_String(LINE1,"LINE");
	while(1)
	{
		Volt_Display();
		Lcd_Cmd(LINE2);
		for(int i=13;i<16; i++)
		{
			DigitDisplay(analog[i]);
			Lcd_Data(' ');
		}
		if(SELECT)
		{
			while(!SELECT)	;
			return;
		}
		delay1ms(200);
	}
}

void view_ir(void)
{
	Lcd_Clear();
	while(1)
	{
		Lcd_Move(0, 0);
		for(int i=0;i<6; i++)
		{
			HEX_Display(analog[i*2+1]);
			if(i != 2) Lcd_Data(' ');
		}
		Lcd_Cmd(LINE2);
		for(int i=0;i<6; i++)
		{
			HEX_Display(analog[i*2]);
			if(i != 2) Lcd_Data(' ');
		}
		if(SELECT)
		{
			while(!SELECT)	;
			return;
		}
		delay1ms(200);
	}
}

void view_long_ir(void)
{
	Lcd_Clear();
	Lcd_Write_String(LINE1,"LONG IR");
	while(1)
	{
		Lcd_Move(0, 9);
		for(int i=0;i<2; i++)
		{
			DigitDisplay(analog[i*2]);
			Lcd_Data(' ');
		}
		Lcd_Cmd(LINE2);
		for(int i=2;i<6; i++)
		{
			DigitDisplay(analog[i*2]);
			Lcd_Data(' ');
		}
		if(SELECT)
		{
			while(!SELECT)	;
			return;
		}
		delay1ms(200);
	}
}

void view_short_ir(void)
{
	Lcd_Clear();
	Lcd_Write_String(LINE1,"SHORT IR");
	while(1)
	{
		Lcd_Move(0, 9);
		for(int i=0;i<2; i++)
		{
			DigitDisplay(analog[i*2+1]);
			Lcd_Data(' ');
		}
		Lcd_Cmd(LINE2);
		for(int i=2;i<6; i++)
		{
			DigitDisplay(analog[i*2+1]);
			Lcd_Data(' ');
		}
		if(SELECT)
		{
			while(!SELECT)	;
			return;
		}
		delay1ms(200);
	}
}

void view_ultra(void)
{
	Lcd_Clear();
	Lcd_Write_String(LINE1,"ULTRA");
	while(1)
	{
		Volt_Display();
		Lcd_Cmd(LINE2);
		for(int i=0;i<4; i++)
		{				
			DigitDisplay((int)((float)ultra[i]*0.34));
			Lcd_Data(' ');
		}
		if(SELECT)
		{
			while(!SELECT)	;
			return;
		}
		delay1ms(200);
	}
}

void view_compass(void)
{
	Lcd_Clear();
	Lcd_Write_String(LINE1,"COMPASS");
	while(1)
	{
		read_compass();
		
		Lcd_Cmd(LINE2);
		AngleDisplay(compass);

		if(SELECT)
		{
			while(!SELECT)	;
			return;
		}
//		delay1ms(200);
	}
}

void menu_display(unsigned char no)
{
	switch(no)
	{
		case 0: Lcd_Write_String(LINE2,"RUN PROGRAM 1   ");
				break;
		case 1: Lcd_Write_String(LINE2,"RUN PROGRAM 2   ");
				break;
		case 2: Lcd_Write_String(LINE2,"VIEW IR         ");
				break;
		case 3: Lcd_Write_String(LINE2,"VIEW ULTRA      ");
				break;
		case 4: Lcd_Write_String(LINE2,"VIEW LINE       ");
				break;
	}
}

void analog_average(int max)
{
	long sum;
	
	for(int i=0; i <12; i++)
	{
		sum=0;
		for(int j=max; j >0; j--)
		{
			an[i][j]=an[i][j-1];		
			sum+=an[i][j];
		}
		an[i][0]=analog[i];
		sum+=an[i][0];
		
		an_avg[i] = (int)(sum/max);
		
	}
}

void compass_move(int ma, int mb, int mc)
{
	int comp;
	read_compass();
	comp = (int)compass / 10;
	comp = comp - 180;
	if (comp > 100)
	{
		move(50, 50, 50);
	}
	else if(comp < -100)
	{
		move(-50, -50, -50);
	}
	else
	{
		move(ma+comp*KP, mb+comp*KP, mc+comp*KP);
	}
}

void dir_move(int input_ball, int power)
{
	switch(input_ball)
	{
		case 0:
			compass_move(power, 0, -power);
			break;
		case 1:
			compass_move(power/2, power/2, -power);
			break;
		case 2:
			compass_move(0, power, -power);
			break;
		case 3:
			compass_move(-power/2, power, -power/2);
			break;
		case 4:
			compass_move(-power, power, 0);
			break;
		case 5:
			compass_move(-power, power/2, power/2);
			break;
		case 6:
			compass_move(-power, 0, power);
			break;
		case 7:
			compass_move(-power/2, -power/2, power);
			break;
		case 8:
			compass_move(0, -power, power);
			break;
		case 9:
			compass_move(power/2,- power, power/2);
			break;
		case 10:
			compass_move(power, -power, 0);
			break;
		case 11:
			compass_move(power, -power/2, -power/2);
			break;
	}
}

void ball_near(int dir, int power)
{
	switch(dir)
	{
		case 0:
			if(last_pos == 1)		//이전에 공이 왼쪽에 있었음.
				dir_move(10, power);
			else                   //이전에 공이 오른쪽에 있었음.
				dir_move(2, power);
			break;
		case 1:
			dir_move(2, power);
			break;
		case 2:
			dir_move(5, power);
			break;
		case 3:
			dir_move(6, power);
			break;
		case 4:
			dir_move(7, power);
			break;
		case 5:
			dir_move(8, power);
			break;
		case 6:
			dir_move(8, power);
			break;
		case 7:
			dir_move(4, power);
			break;
		case 8:
			dir_move(5, power);
			break;
		case 9:
			dir_move(6, power);
			break;
		case 10:
			dir_move(7, power);
			break;
		case 11:
			dir_move(10, power);
			break;
	}
}

void find_ball()
{
	analog_average(2);

	ball_dir = 0;
	max_ir = 0;
	
	for(int i = 0; i < 6; i++)
	{
		ir[i*2] = (an_avg[i*2+1] > 3) || (an_avg[i*2] > 240) ? 128 + an_avg[i*2+1]*1.2 : an_avg[i*2]/2;
		if(ir[i*2] > 255) ir[i*2]=255;
	}
	ir[0] = ir[0]-10;
	ir[2] = ir[2]-10;
	ir[4] = ir[4]-10;

	for(int i = 0; i < 5; i++)
		ir[i*2+1] = (int)((float)(ir[i*2] + ir[i*2+2])/2*1.1);
	ir[11] = (int)((float)(ir[10] + ir[0])/2*1.1);


	for(int i = 0; i < 12; i++)
	{
		if(max_ir < ir[i])
		{
			max_ir = ir[i];
			ball_dir = i;
		}
	}
	Lcd_Move(1,1);
	DigitDisplay(ball_dir);
	Lcd_Data(' ');
	DigitDisplay(max_ir);
}

void find_ball2()
{
	ball_dir = 0;
	max_ir = 0;
	
	for(int i = 0; i < 6; i++)
	{
		ir[i*2] = (analog[i*2+1] > 3) || (analog[i*2] > 240) ? 128 + analog[i*2+1]*1.3 : analog[i*2]/2;
		if(ir[i*2] > 255) ir[i*2]=255;
	}
	for(int i = 0; i < 5; i++)
	ir[i*2+1] = (int)((float)(ir[i*2] + ir[i*2+2])/2*1.1);
	ir[11] = (int)((float)(ir[10] + ir[0])/2*1.1);


	for(int i = 0; i < 12; i++)
	{
		if(max_ir < ir[i])
		{
			max_ir = ir[i];
			ball_dir = i;
		}
	}
	//	Lcd_Move(1,1);
	//	DigitDisplay(ball_dir);
	//	Lcd_Data(' ');
	//	DigitDisplay(max_ir);
}

void view_totalIR(void)
{
	Lcd_Clear();
	Lcd_Write_String(LINE1,"IR");
	while(1)
	{
		find_ball();
		Volt_Display();
		Lcd_Cmd(LINE2);
		DigitDisplay(max_ir);
		Lcd_Data(' ');
		DigitDisplay(ball_dir);
		delay1ms(200);
	}
}


void PROGRAM1(void)//공격 
{
	move(0,0,0);
	Lcd_Clear();
	int ultra_gap = 0;
	LineDetected = 0;

	while(1)
	{
		find_ball();
		if(ball_dir > 6)						//이전에 공이 왼쪽에 있었음.
			last_pos = 1;
		else if(ball_dir >= 1 && ball_dir < 6)	//이전에 공이 오른쪽에 있었음.
			last_pos = 0;
/*
		if((float)ultra[1]*0.34 < 15 && max_ir < 80)
		{
			move(0,0,0);
			delay1ms(100);
			dir_move(9,100);
			delay1ms(800);
			LineDetected=0;
		}
		else if((float)ultra[3]*0.34 < 15 && max_ir < 80)
		{
			move(0,0,0);
			delay1ms(100);
			dir_move(3,100);
			delay1ms(800);
			LineDetected=0;
		}
*/		
		if(LineDetected)
		{
			if(LineDetected & 1)
			{
				move(0,0,0);
				delay1ms(100);
				if(LineDetected & 2)
					dir_move(7, 100);
				else if(LineDetected & 4)
					dir_move(5, 100);
				else
					dir_move(6,100);
				delay1ms(400);
			}
			else if(LineDetected & 6)
			{
				move(0,0,0);
				delay1ms(100);
				if(LineDetected & 2)
					dir_move(9, 100);
				else if(LineDetected & 4)
					dir_move(3, 100);
				delay1ms(400);
			}
			LineDetected = 0;
		}

		if(max_ir > SHOOT && (ball_dir == 0)) // || ball_dir == 1 || ball_dir == 11))			//공이 가까이 있을 때
		{
			ultra_gap = (int)((float)ultra[1]*0.34) - (int)((float)ultra[3]*0.34);
			int comp;
			read_compass();
			comp = (int)compass / 10;
			comp = comp - 180 - ultra_gap;
			
			move(100+comp*KP, comp*KP, -100+comp*KP);
			
			dir_move(0, 100);
			compass_move(100, -ultra_gap*2, -100);
		}
		else
		{
			if (max_ir > NEAR)
			{
				ball_near(ball_dir, 100);
			}
			else if(max_ir > 100)
			{
				dir_move(ball_dir, 100);
			}
			else
				dir_move(ball_dir, 0);
		}
	}
}

void PROGRAM2(void)//수비 
{
	move(0,0,0);
	Lcd_Clear();
	int ultra_gap = 0;
	
	while(1)
	{
		find_ball2();
		if(ball_dir > 6)
			last_pos = 1;
		else if(ball_dir >= 1 && ball_dir < 6)
			last_pos = 0;
			
		if((int)((float)ultra[2]*0.34) < 30)
			dir_move(0, 50);
		else if((float)ultra[1]*0.34 < 15 && max_ir < 80)
		{
			move(0,0,0);
			delay1ms(100);
			dir_move(9,100);
			delay1ms(700);
			LineDetected=0;
		}
		else if((float)ultra[3]*0.34 < 15 && max_ir < 80)
		{
			move(0,0,0);
			delay1ms(100);
			dir_move(3,100);
			delay1ms(700);	
			LineDetected=0;		
		}
		else if(LineDetected)
		{
			move(0,0,0);
			delay1ms(100);
			
			if(LineDetected & 2)
				dir_move(9, 100);
			else if(LineDetected & 4)
				dir_move(3, 100);
				
			delay1ms(400);
			LineDetected = 0;
		}
		else
		{
			if(max_ir > SHOOT2 && (ball_dir == 0)) // || ball_dir == 1 || ball_dir == 11)			//공이 가까이 있을 때
			{
				ultra_gap = (int)((float)ultra[1]*0.34) - (int)((float)ultra[3]*0.34);
				int comp;
				read_compass();
				comp = (int)compass / 10;
				comp = comp - 180 - ultra_gap;
					
				move(100+comp*KP, comp*KP, -100+comp*KP);
					
				dir_move(0, 100);
				for (int i=0;i<100;i++)
				{
					compass_move(100, -ultra_gap*2, -100);
				}
				move(0,0,0);
				delay1ms(100);
				dir_move(6,100);
				delay1ms(200);
				find_ball2();
			}
			if(max_ir > NEAR2)
			{
				dir_move(ball_dir, 100);
			}
			else
			{
				if((int)((float)ultra[1]*0.34) + (int)((float)ultra[3]*0.34) > 40 && (int)((float)ultra[1]*0.34) - (int)((float)ultra[3]*0.34) < 20 && (int)ultra[1] - (int)ultra[3] > -20 && (int)((float)ultra[2]*0.34) > 40)
					dir_move(6, 100);
				else if((int)((float)ultra[1]*0.34) - (int)((float)ultra[3]*0.34) > 10 && (int)((float)ultra[1]*0.34) + (int)((float)ultra[3]*0.34) > 40)
					dir_move(3, 90);
				else if((int)((float)ultra[1]*0.34) - (int)((float)ultra[3]*0.34) < -10 && (int)((float)ultra[1]*0.34) + (int)((float)ultra[3]*0.34) > 40)
					dir_move(9, 90);
				else
					dir_move(0, 0);
			}
		}
	}
}

int main()
{
	init_devices();
	double sumcomp=0;
	
	Lcd_Init();
	
	Lcd_Write_String(LINE1,"RCKA SOCCER V1.0");
	Lcd_Write_String(LINE2,"  REVOLUTION ");
//	Lcd_Write_String(LINE2," ROBOCUP KOREA");
	
	while(!ENTER)	;
	while(ENTER)	;

	for(int i=0; i<16;i++)
		for(int j=0;j<16;j++)
			an[i][j]=0;
		
	Lcd_Clear();
	Lcd_Write_String(LINE1,"RCKA");	
	Lcd_Write_String(LINE2,"RUN PROGRAM 1");

	for(int i=0; i<10; i++)
		read_compass();

	memComp = compass;
	
	while(1)
	{
		delay1ms(200);

		Volt_Display();
		Lcd_Move(0,5);
		read_compass();
		
		AngleDisplay(compass);

		if(SELECT)
		{
			while(SELECT)	;
			menu++;
			if (menu>4)	menu=0;
			menu_display(menu);
		}
		if(ENTER)
		{
			while(ENTER)	;
			switch(menu)
			{
				case 0: PROGRAM1();
						break;
				case 1: PROGRAM2();
						break;
				case 2: view_ir();
						break;
				case 3: view_ultra();
						break;
				case 4: view_line();
						break;
			}
			Lcd_Clear();
			Lcd_Write_String(LINE1,"RCKA");
			menu_display(menu);
		}
	}
	return 0;
}
