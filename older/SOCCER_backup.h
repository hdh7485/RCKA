/*
 * SOCCER.h
 *
 * Created: 2015-08-06 오후 1:59:58
 * Author: JEON HAKYEONG (전하경)
 *
 * History
 *
 * 2015-10-28
 *	Debugging & Optimizing Header File.
 *	Change Timer Interrupt 0 Period 20us -> 40us
 *
 */ 

//#define F_CPU 20000000				//CPU CLOCK is 20MHz

#define RXLEDON (PORTD &= 0xFE)
#define RXLEDOFF (PORTD |= 0x01)
#define TXLEDON (PORTD &= 0xFD)
#define TXLEDOFF (PORTD |= 0x02)
#define RX1LEDON (PORTD &= 0xF7)
#define RX1LEDOFF (PORTD |= 0x08)
#define TX1LEDON (PORTD &= 0xFB)
#define TX1LEDOFF (PORTD |= 0x04)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define LINE1 0x80
#define LINE2 0xC0

#define SLA_W 0x3C
#define SLA_R 0x3D

#define START			0x08
#define RE_START		0x10
#define MT_SLA_ACK		0x18
#define MT_SLA_NOACK 	0x20
#define MT_DATA_ACK		0x28
#define MT_DATA_NOACK	0x30
#define MR_SLA_ACK		0x40
#define MR_SLA_NOACK	0x48
#define MR_DATA_ACK		0x50
#define MR_DATA_NOACK	0x58


#define ENTER (~PINC & 0x08)
#define SELECT (~PINC & 0x04)

#define madirF (PORTC &= 0xDF)
#define madirB (PORTC |= 0x20)
#define mbdirF (PORTC &= 0xBF)
#define mbdirB (PORTC |= 0x40)
#define mcdirF (PORTC &= 0x7F)
#define mcdirB (PORTC |= 0x80)

#define TRIGERON (PORTD |= 0x80)
#define TRIGEROFF (PORTD &= 0x7F)

#define ECHO1 (PINC & 0x10)
#define ECHO2 (PINA & 0x20) 
#define ECHO3 (PINA & 0x40)
#define ECHO4 (PINA & 0x80)

volatile int	COUNTby40us = 0;
volatile unsigned int voltage = 120;

unsigned int Pulse_Width_Count=0;
unsigned int ECHO_CNT[4];
unsigned char ECHO_REPLY[4]={0,0,0,0};
unsigned int  Distance[3][4];
unsigned char ADC_SEQ = 0;
unsigned char ADC_VALUE[3][16];
unsigned char ADC_TEMP;

void delay40us(int us)
{
	COUNTby40us=us;
	while (COUNTby40us > 0)	;
}

void delay1ms(int ms)
{
	while(ms>0)
	{
		delay40us(25);
		ms--;
	}
}

void port_init(void)
{
	PORTA = 0xF0;
	DDRA = 0x0F;
	PORTB = 0xFF;
	DDRB = 0xFB;	
	PORTC = 0x1C; 
	DDRC = 0xE3;
	PORTD = 0x7F;
	DDRD = 0xFF;
}

void timer_init(void)
{
	TCCR0B = 0x02;		//  clock/8  pre-scaler
	TCNT0 = 156;		//  Timer 0 Over-Flow Interrupt Periods are 40us.
	
	TCCR1A = ((1 << COM1A1) | (1 << COM1A0) | (1 << COM1B1)  | (1 << COM1B0) | (1 << WGM10)); // 1KHZ PWM GENERATION
	TCCR1B = ((1 << CS11) | (1 << CS10));
//	TCCR1B = ((1 << CS12));
		
	TCCR2A = ((1 << COM2B1) | (1 << COM2B0) | (1 << WGM20) ); // 1KHZ PWM GENERATION
	TCCR2B = ((1 << CS22));
//	TCCR2B = ((1 << CS22) | (1 << CS21));
	
	OCR1A = 0;   // 20000 * 0.05 most left
	OCR1B = 0;   // 20000 * 0.05 most left
	OCR2B = 0;   // 20000 * 0.05 most left
	
	TIMSK0 = 0x01; //timer0 Overflow Interrupt Enable
	
	TWBR=17;		//SET IIC 400KHz @ 20MHz X-Tal
}


ISR(TIMER0_OVF_vect)
{
	int i;
	
	TCNT0 += 156;	//  RELOAD.. 40us Period

	if (COUNTby40us>0)	COUNTby40us--;
	
	if (ADCSRA & 0x10)
	{
		ADCSRA = ADCSRA | 0x10;
		ADC_VALUE[0][ADC_SEQ] = ADC_VALUE[1][(int)ADC_SEQ];
//		ADC_VALUE[1][ADC_SEQ] = (char)ADCL;
		ADC_VALUE[1][ADC_SEQ] = (char)ADCH;
		analog[ADC_SEQ]=(ADC_VALUE[0][ADC_SEQ]+ADC_VALUE[1][ADC_SEQ])/2;
		
		if(ADC_SEQ<12)
		{
			if(ADC_SEQ%2)
			{
				ir[ADC_SEQ-1] = analog[ADC_SEQ-1] > 240 ? 128 + analog[ADC_SEQ]/2 : analog[ADC_SEQ-1]/2;
				if(ADC_SEQ==1)
					ir[11] = (int)((float)(ir[10] + ir[0])/2*1.20);
				else
					ir[ADC_SEQ-2] = (int)((float)(ir[ADC_SEQ-1] + ir[ADC_SEQ-3])/2*1.20);
			}
		}
		else if (ADC_SEQ == 12)
		{
			voltage = ((float)analog[12]*0.625 -2.5);
			ball_dir = 0;
			max_ir = 0;
			for(i = 0; i < 12; i++)
			{
				if(max_ir < ir[i])
				{
					max_ir = ir[i];
					ball_dir = i;
				}
			}
		}
		else if (ADC_SEQ == 13)
		{
			if(analog[ADC_SEQ]>=Line0_White)
				LineDetected |= 0x01;
		}
		else if (ADC_SEQ == 14)
		{
			if(analog[ADC_SEQ]>=Line1_White)
			LineDetected |= 0x02;
		}
		else if (ADC_SEQ == 15)
		{
			if(analog[ADC_SEQ]>=Line2_White)
			LineDetected |= 0x04;
		}

/*
		if(ADC_SEQ == 15)
		{
			ball_dir = 0;
			max_ir = 0;
			for(int i = 0; i < 6; i++)
				ir[i*2] = analog[i*2] > 240 ? 128 + analog[i*2+1]/2 : analog[i*2]/2;
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
*/		
		ADCSRA = ADCSRA | 0x10;
		
		ADC_SEQ++;
		if (ADC_SEQ>15)	ADC_SEQ=0;
		
		PORTA = ADC_SEQ;
		ADCSRA = ADCSRA & 0x3f;
		ADCSRA = ADCSRA | 0xc0;
		
	}
	
	if (Pulse_Width_Count==0)
	{
		TRIGERON;
	}
	else if(Pulse_Width_Count==1)
	{
		TRIGEROFF;
	}
	else if (Pulse_Width_Count < 400)
	{
		if (ECHO_CNT[0] && !ECHO_REPLY[0] && !ECHO1)
		{
			ECHO_CNT[0]=Pulse_Width_Count-ECHO_CNT[0];
			ECHO_REPLY[0]=1;
		}
		if (ECHO_CNT[1] && !ECHO_REPLY[1] && !ECHO2)
		{
			ECHO_CNT[1]=Pulse_Width_Count-ECHO_CNT[1];
			ECHO_REPLY[1]=1;
		}
		if (ECHO_CNT[2] && !ECHO_REPLY[2] && !ECHO3)
		{
			ECHO_CNT[2]=Pulse_Width_Count-ECHO_CNT[2];
			ECHO_REPLY[2]=1;
		}
		if (ECHO_CNT[3] && !ECHO_REPLY[3] && !ECHO4)
		{
			ECHO_CNT[3]=Pulse_Width_Count-ECHO_CNT[3];
			ECHO_REPLY[3]=1;
		}
		
		if (!ECHO_CNT[0] && ECHO1) ECHO_CNT[0]=Pulse_Width_Count;
		if (!ECHO_CNT[1] && ECHO2) ECHO_CNT[1]=Pulse_Width_Count;
		if (!ECHO_CNT[2] && ECHO3) ECHO_CNT[2]=Pulse_Width_Count;
		if (!ECHO_CNT[3] && ECHO4) ECHO_CNT[3]=Pulse_Width_Count;
	}
	else if (Pulse_Width_Count == 400)
	{
		for(i=0; i<4;i++)
		{
			if (!ECHO_REPLY[i]) ECHO_CNT[i]=Pulse_Width_Count;
			Distance[0][i]=Distance[1][i];
			Distance[1][i]=Distance[2][i];
			Distance[2][i]=ECHO_CNT[i];
			ultra[i]=(float)(Distance[0][i]+Distance[1][i]+Distance[2][i])*0.68/3;
//			ultra[i]=(float)ECHO_CNT[i]*0.68;
		}
	}
	
	Pulse_Width_Count++;
	if (Pulse_Width_Count > 1000)
	{
		Pulse_Width_Count=0;
		for(i=0; i<4;i++)
		{
			ECHO_CNT[i]=0;
			ECHO_REPLY[i]=0;
		}
	}
}

ISR(TIMER2_OVF_vect)
{
	/*
//	PORTD &= 0xFE;

	for (int i=0;i<16;i++)
	{
		PORTA = i;
		ADCSRA = ADCSRA & 0x3f;
		ADCSRA = ADCSRA | 0xc0;
		while(!(ADCSRA & 0x10))	;

		ADCSRA = ADCSRA | 0x10;
		ADC_VALUE[0][(int)ADC_SEQ] = ADC_VALUE[1][(int)ADC_SEQ];
		ADC_VALUE[1][(int)ADC_SEQ] = ADC_VALUE[2][(int)ADC_SEQ];
		ADC_VALUE[2][(int)ADC_SEQ] = (char)ADCH;
		ADCSRA = ADCSRA | 0x10;
	
	}

//	PORTD |= 0x01;
*/
}


void adc_init(void)
{
	ADC_SEQ=0;
	PORTA = ADC_SEQ;
	
	ADCSRA = 0x00;			//disable adc
	ADMUX  = 0x64;			//select adc CH 4
	ACSR   = 0x80;			//Analog Comparator Disable
	ADCSRA = 0x85;			//ADC CLOCK Pre-Scaler set to 5 (divide factor is 32) 
	ADCSRA = ADCSRA & 0x3f;	//ADC disable
	ADCSRA = ADCSRA | 0xc0;	//ADC START
}

void echo_clear(void)
{
	for(int i=0; i<4;i++)
	{
		ECHO_CNT[i]=0;
		ECHO_REPLY[i]=0;
	}
}


void init_devices(void)
{
	cli(); //disable all interrupts

	MCUCR = 0x00;
	EIMSK = 0x00;

	echo_clear();
	
	port_init();
	adc_init();
	timer_init();


	sei(); //enable interrupts
}


/*
// 입력으로 들어오는 채널의 ADC를 스타트 시킨다.
void startConvertion(unsigned char ch)
{
	ADCSRA = ADCSRA & 0x3f;
	ADCSRA = ADCSRA | 0xc0;
}

// startConvertion() 후에 수행되며 컨터팅 된 값을 리턴한다.
unsigned char readConvertData(void)
{
	volatile unsigned char temp;
	while((ADCSRA & 0x10)==0);
	ADCSRA = ADCSRA | 0x10;
	temp = ADCL;
	temp = ADCH;
	ADCSRA = ADCSRA | 0x10;
	return temp;
}
*/

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
		delay40us(2);
		if ((PINB & 0x80) ==0)	busy = 0;
		PORTB=(PORTB & 0xF7);
		delay40us(2);
		PORTB=0x0A;
		delay40us(2);
		PORTB=(PORTB & 0xF5);
		delay40us(2);
	}
	PORTB = 0xFB;
	DDRB = 0xFB;	//상위 4비트만 출력	
*/
}

void Lcd_Cmd(char cmd)
{
	char tmp = cmd;
	delay40us(2);
	PORTB=(cmd&0xF0) | 0x08;
	PORTB=(PORTB & 0xF7);
	PORTB=((tmp<<4)&0xF0) | 0x08;
	PORTB=(PORTB & 0xF7);
}

static void Lcd_Data(char ch)
{
	char tmp=ch;
	
	delay40us(2);
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
void AngleDisplay(unsigned int x)
{
	Lcd_Data((int)((x%10000)/1000)+'0');
	Lcd_Data((int)((x%1000)/100)+'0');
	Lcd_Data((int)((x%100)/10)+'0');
	Lcd_Data('.');
	Lcd_Data((int)((x%10))+'0');
}

void Lcd_Clear()
{
	Lcd_Cmd(0x01);
	delay1ms(2);
}

void Volt_Display(void)
{
	//int volt = ((float)analog[12]*0.625 -2.5);
	
	Lcd_Move(0, 11);
	Lcd_Data((int)((voltage%1000)/100)+'0');
	Lcd_Data((int)((voltage%100)/10)+'0');
	Lcd_Data('.');
	Lcd_Data((int)((voltage%10))+'0');
	Lcd_Data('V');
}

void IIC_WRITE(unsigned char add, unsigned char reg, unsigned char data)
{
	int error_code=0;

//	PORTD &= 0xFE;
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while (!(TWCR &	(1<<TWINT)));
	if ((TWSR & 0xF8) != START)
	{
		Lcd_Write_String(LINE2,"START ERROR");
		error_code=TWSR & 0xF8;
		goto write_error;
	}
	TWDR = add;
	TWCR = (1<<TWINT) |	(1<<TWEN);
//	PORTD |= 0x01;
	PORTD &= 0xFE;
	while (!(TWCR &	(1<<TWINT)));
	if ((TWSR & 0xF8) != MT_SLA_ACK)
	{
		Lcd_Write_String(LINE2,"MT_SLA_ACK ERROR");
		error_code=TWSR & 0xF8;
		goto write_error;
	}
	PORTD |= 0x01;	
	
	TWDR = reg;
	TWCR = (1<<TWINT) |	(1<<TWEN);
	while (!(TWCR &	(1<<TWINT)));
	if ((TWSR & 0xF8) !=MT_DATA_ACK)
	{
		Lcd_Write_String(LINE2,"MT_DATA_ACK ERROR");
		error_code=TWSR & 0xF8;
		goto write_error;
	}
	TWDR = data;
	TWCR = (1<<TWINT) |	(1<<TWEN);
	while (!(TWCR &	(1<<TWINT)));
	if ((TWSR & 0xF8) !=MT_DATA_ACK)
	{
		Lcd_Write_String(LINE2,"MT_DATA_ACK ERROR");
		error_code=TWSR & 0xF8;
		goto write_error;
	}
	TWCR =(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	return;
	
write_error	:

	TWCR =(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
//	return error_code;
}

unsigned char IIC_READ(unsigned char w_add,unsigned char r_add, unsigned char reg)
{
	unsigned char temp=0;
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	while (!(TWCR &	(1<<TWINT)));
	if ((TWSR & 0xF8) != START)
	{
		Lcd_Write_String(LINE2,"START ERROR");
		temp=TWSR & 0xF8;
		goto read_error;
	}
	TWDR = w_add;
	TWCR = (1<<TWINT) |	(1<<TWEN);
	while (!(TWCR &	(1<<TWINT)));
	if ((TWSR & 0xF8) != MT_SLA_ACK)
	{
		Lcd_Write_String(LINE2,"MT_SLA_ACK ERROR");
		temp=TWSR & 0xF8;
		goto read_error;
	}
	
	TWDR = reg;
	TWCR = (1<<TWINT) |	(1<<TWEN);
	while (!(TWCR &	(1<<TWINT)));
	if ((TWSR & 0xF8) !=MT_DATA_ACK)
	{
		Lcd_Write_String(LINE2,"MT_DATA_ACK ERROR");
		temp=TWSR & 0xF8;
		goto read_error;
	}
			
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);		//REPEAT START
	while (!(TWCR &	(1<<TWINT)));
	if ((TWSR & 0xF8) != RE_START)
	{
		Lcd_Write_String(LINE2,"RESTART ERROR");
		temp=TWSR & 0xF8;
		goto read_error;
	}
	TWDR = r_add;
	TWCR = (1<<TWINT) |	(1<<TWEN);
	while (!(TWCR &	(1<<TWINT)));
	if ((TWSR & 0xF8) !=MR_SLA_ACK)
	{
		Lcd_Write_String(LINE2,"MR_SLA_ACK ERROR");
		temp=TWSR & 0xF8;
		goto read_error;
	}
			
	TWCR = (1<<TWINT) |	(1<<TWEN);
				
	while (!(TWCR &	(1<<TWINT)));
	if ((TWSR & 0xF8) !=MR_DATA_NOACK)
	{
		Lcd_Write_String(LINE2,"MR_DATA_NOACK ERROR");
		temp=TWSR & 0xF8;
		goto read_error;
	}
	temp=TWDR;
	
read_error:
	TWCR =(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
	return temp;
}

void read_compass(void)
{
	int x,y;
	
	IIC_WRITE(SLA_W,0x02,0x00);
	delay1ms(70);
	x=IIC_READ(SLA_W,SLA_R,0x03);
	delay40us(2);
	x=(x<<8) + IIC_READ(SLA_W,SLA_R,0x04);
	delay40us(2);
	y=IIC_READ(SLA_W,SLA_R,0x07);
	delay40us(2);
	y=(y<<8) + IIC_READ(SLA_W,SLA_R,0x08);
	delay40us(2);
	compass = (atan2((double)y,(double)x) * (180 / 3.14159265) + 180)*10; // angle in degrees
}