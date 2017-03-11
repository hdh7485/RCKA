/*
 * SOCCER.h
 *
 * Created: 2015-08-06 오후 1:59:58
 * Author: JEON HAKYEONG (전하경)
 *
 */ 

#define LINE1 0x80
#define LINE2 0xC0

void Lcd_Chk_Busy()
{
/*
	char busy = 1;
	char in;		
	PORTB = 0x0B;
	DDRB = 0x0B;	//상위 4비트만 출력
	PORTB=0x02;

	while(busy==1)
	{
		PORTB=0x0A;
		delay20us(3);
		if ((PINB & 0x80) ==0)	busy = 0;
		PORTB=(PORTB & 0xF7);
		delay20us(3);
		PORTB=0x0A;
		delay20us(3);
		PORTB=(PORTB & 0xF5);
		delay20us(3);
	}
	PORTB = 0xFB;
	DDRB = 0xFB;	//상위 4비트만 출력	
*/
}

void Lcd_Cmd(char cmd)
{
	char tmp = cmd;
	delay20us(3);
	PORTB=(cmd&0xF0) | 0x08;
	PORTB=(PORTB & 0xF7);
	PORTB=((tmp<<4)&0xF0) | 0x08;
	PORTB=(PORTB & 0xF7);
}

static void Lcd_Data(char ch)
{
	char tmp=ch;
	
	delay20us(3);
	PORTB=(ch&0xF0) | 0x09;
	PORTB=(PORTB & 0xF7);
	PORTB=((tmp<<4)&0xF0) | 0x09;
	PORTB=(PORTB & 0xF7);
}

void Lcd_Init()
{
	delay1ms(50);
	PORTB=0x28;					//4bit mode set , 1 nibble writing one time.
	PORTB=(PORTB & 0xF7);	//
	Lcd_Cmd(0x28);					//4bit mode set , 2 nibble writing.
	Lcd_Cmd(0x28);					//4bit mode set , one more...
	Lcd_Cmd(0x0C);	
	Lcd_Cmd(0x01);
	delay1ms(2);
	Lcd_Cmd(0x06);
}

void Lcd_Move(char line, char pos)
{
	pos=(line<<6)+pos;
	pos |= 0x80;
	Lcd_Cmd(pos);
}

void Lcd_Write_String(char d_line, char *lcd_str)
{
	Lcd_Cmd(d_line);
	while(*lcd_str != '\0')
	{
		Lcd_Data(*lcd_str);
		lcd_str++;
	}
}

void DigitDisplay(unsigned int x)
{
	Lcd_Data((int)((x%1000)/100)+'0');
	Lcd_Data((int)((x%100)/10)+'0');
	Lcd_Data((int)((x%10))+'0');
}

void Lcd_Clear()
{
	Lcd_Cmd(0x01);
	delay1ms(2);
}

void Volt_Display(void)
{
	int volt = ((float)analog[12]*0.625 -2.5);
	
	Lcd_Move(0, 11);
	Lcd_Data((int)((volt%1000)/100)+'0');
	Lcd_Data((int)((volt%100)/10)+'0');
	Lcd_Data('.');
	Lcd_Data((int)((volt%10))+'0');
	Lcd_Data('V');
}