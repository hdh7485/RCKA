/*
 * SOCCER.h
 *
 * Created: 2015-08-06 오후 1:59:58
 * Author: JEON HAKYEONG (전하경)
 *
 */ 
#define KP 0.5

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
// 아래 정의값들은 경기장 테두리의흰색 선의 기준값입니다.
// 경기장에서 직접 측정하여 변경하던지 가변저항을 조정하세요.
//
#define Line0_White 150
#define Line1_White 150
#define Line2_White 150

//
// 위 기준값을 넘은 경우가 있으면 아래 변수에 기록이 되어짐.
// 선을 감지하고 그에따른 액션을 수행한후에는 아래값을 Clear 시켜주세요.
//
// 0 bit : line 0 detected
// 1 bit : line 1 detected
// 2 bit : line 2 detected
//
unsigned  char LineDetected=0;

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

// 컴파스 센서 값.
double compass;

//------------- 여기까지의 변수는 읽기만 하세요. ------------------------


unsigned char menu = 0;

#include "SOCCER.h"


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

void compass_move(int ma, int mb, int mc)
{
	int comp;
	read_compass();
	comp = (int)compass / 10;
	comp = comp - 180;
	//Lcd_Cmd(LINE1);
	//AngleDisplay(compass);
	//Lcd_Cmd(LINE2);
	//DigitDisplay(comp);
	move(ma+comp*KP, mb+comp*KP, mc+comp*KP);
}

void dir_move(int dir, int power)
{
	switch(dir)
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
		delay1ms(200);
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
		case 2: Lcd_Write_String(LINE2,"VIEW COMPASS    ");
		break;
		case 3: Lcd_Write_String(LINE2,"VIEW ULTRA      ");
		break;
		case 4: Lcd_Write_String(LINE2,"VIEW SHORT IR   ");
		break;
		case 5: Lcd_Write_String(LINE2,"VIEW LONG IR    ");
		break;
		case 6: Lcd_Write_String(LINE2,"VIEW LINE SENSOR");
		break;
	}
}

void find_ball()
{	
	ball_dir = 0;
	max_ir = 0;
	for(int i = 0; i < 6; i++)
	ir[i*2] = analog[i*2] > 240? 128 + analog[i*2+1]/2 : analog[i*2]/2;
	for(int i = 0; i < 5; i++)
	ir[i*2+1] = (int)((float)(ir[i*2] + ir[i*2+2])/2*1.20);
	ir[11] = (int)((float)(ir[10] + ir[0])/2*1.20);
	for(int i = 0; i < 12; i++)
	{
		if(max_ir < ir[i])
		{
			max_ir = ir[i];
			ball_dir = i;
		}
	}	
}

void following_ball()
{
	int ultra_gap = 0;
	while(1)
	{
/*
		Lcd_Cmd(LINE1);
		DigitDisplay(ball_dir);
		DigitDisplay(max_ir);
*/		
		if(max_ir > 160 && ball_dir == 0)
		{
			ultra_gap = ultra[1] - ultra[3];
			compass_move(100, -ultra_gap*3, -100);
		}
		else
		{
			dir_move(ball_dir, 100);
		}
	}
}

void PROGRAM1(void)
{
	Lcd_Clear();
	while(1)
	{
		for (int i = 0; i < 12; i++)
		{
			for(int j=0; j<4; j++)
			{
				dir_move(i, 100);
				delay1ms(1);
			}
		}
		if(SELECT)
		{
			while(!SELECT);
			return;
		}
	}
	move(0,0,0);
}
void PROGRAM2(void)
{
	move(0,0,0);
	Lcd_Clear();
	while(1)
	{
		find_ball();
		following_ball();
	}
}

int main()
{
	init_devices();
	Lcd_Init();
	
	Lcd_Write_String(LINE1,"RCKA SOCCER V1.0");
	Lcd_Write_String(LINE2," ROBOCUP KOREA");
	
	while(!ENTER)	;
	while(ENTER)	;

	IIC_WRITE(SLA_W,0x02,0x00);
	delay1ms(200);
	IIC_WRITE(SLA_W,0x00,0x58);
	delay1ms(200);
	IIC_WRITE(SLA_W,0x02,0x00);
	delay1ms(200);



	Lcd_Clear();
	Lcd_Write_String(LINE1,"RCKA");	
	Lcd_Write_String(LINE2,"RUN PROGRAM 1");
	
	while(1)
	{
		delay1ms(200);

		Volt_Display();
		
		if(SELECT)
		{
			while(SELECT)	;
			menu++;
			if (menu>6)	menu=0;
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
				case 2: view_compass();
						break;
				case 3: view_ultra();
						break;
				case 4: view_short_ir();
						break;
				case 5: view_long_ir();;
						break;
				case 6: view_line();;
						break;
			}
			Lcd_Clear();
			Lcd_Write_String(LINE1,"RCKA");
			menu_display(menu);
		}
	}
	return 0;
}