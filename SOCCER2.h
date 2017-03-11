/*
 * SOCCER.h
 *
 * Created: 2015-08-06 ?¤í›„ 1:59:58
 * Author: JEON HAKYEONG (?„í•˜ê²?
 *
 * History
 *
 * 2015-10-28
 *	Debugging & Optimizing Header File.
 *	Change Timer Interrupt 0 Period 20us -> 40us
 *
 * 2015-10-31
 *	Debugging & Optimizing Header File.
 *	Change Timer Interrupt 0 Period 40us -> 20us
 *  Remove All Arithmetic commands in Timer Interrupt Routine
 *
 * 2015-11-01
 *	PWM OFF/BREAK mode Change (Forward & Backward : Break Mode)
 *
 *
 * 2016-02-21
 *	Change Compass Sensor Measurement mode (continuous -> single MODE)
 *
 * 2016-06-07
 *	Change L_IR2 and S_IR2 (change position S_IR2 to front)
 *
 * 2017-03-11
 *	Change some register and resistance.
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

#define IIC_Start()			(TWCR=(1<<TWINT)|(1<<TWSTA)|(1<<TWEN))		
#define IIC_Stop()			(TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN))		
#define IIC_Wait()			{while(!(TWCR&(1<<TWINT)));}				
#define IIC_TestAck()		(TWSR&0xf8)									
#define IIC_SetAck			(TWCR|=(1<<TWEA))							
#define IIC_SetNoAck		(TWCR&=~(1<<TWEA))							
#define IIC_Twi()			(TWCR=(1<<TWINT)|(1<<TWEN))				    
#define IIC_Write8Bit(x)	{TWDR=(x);TWCR=(1<<TWINT)|(1<<TWEN);}		
	
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

#define TRIGERON (PORTD |= 0x80)
#define TRIGEROFF (PORTD &= 0x7F)

#define ECHO1 (PINC & 0x10)
#define ECHO2 (PINA & 0x20) 
#define ECHO3 (PINA & 0x40)
#define ECHO4 (PINA & 0x80)

#define	voltage  (int)((float)analog[12]*0.625 -2.5)

#define madirF (MotorDir &= 0xDF)
#define madirB (MotorDir |= 0x20)
#define mbdirF (MotorDir &= 0xBF)
#define mbdirB (MotorDir |= 0x40)
#define mcdirF (MotorDir &= 0x7F)
#define mcdirB (MotorDir |= 0x80)

unsigned char MotorDir=0;

volatile int	COUNTby20us = 0;

unsigned int Pulse_Width_Count=0;
unsigned int ECHO_CNT[4];
unsigned char ECHO_REPLY[4]={0,0,0,0};
unsigned int  Distance[3][4];
unsigned char ADC_SEQ = 0;
unsigned char ADC_VALUE[3][16];
unsigned char ADC_TEMP;

unsigned char IIC_WRITE(unsigned char add, unsigned char reg, unsigned char data);

void delay20us(int us)
{
	COUNTby20us=us;
	while (COUNTby20us > 0)	;
}

void delay1ms(int ms)
{
	while(ms>0)
	{
		delay20us(50);
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
	TCCR0B = 0x02;		// Timer 0 : clock/8  pre-scaler
	
	TCCR1A = ((1 << COM1A1) | (1 << COM1A0) | (1 << COM1B1)  | (1 << COM1B0) | (1 << WGM10));	// Phase correct PWM Mode 610HZ PWM GENERATION
	TCCR1B = ((1 << CS11) | (1 << CS10));														// Clock/64
		
	TCCR2A = ((1 << COM2B1) | (1 << COM2B0) | (1 << WGM20) );	// Phase correct PWM Mode 610HZ PWM GENERATION
	TCCR2B = ((1 << CS22));										// Clock/64
	
	OCR1A = 0;
	OCR1B = 0;
	OCR2B = 0;
	
	TCNT0 = 206;		//  Timer 0 Over-Flow Interrupt Periods are 20us.
	TIMSK0 = 0x01; //timer0 Overflow Interrupt Enable
	
	TWBR=17;		//SET IIC 400KHz @ 20MHz X-Tal
//	TWBR=42;		//SET IIC 200KHz @ 20MHz X-Tal
//	TWBR=92;		//SET IIC 100KHz @ 20MHz X-Tal
}


ISR(TIMER0_OVF_vect)
{
	unsigned char dirtemp=MotorDir;
	
	TCNT0 += 206;	//  RELOAD.. 20us Period

	if (COUNTby20us>0)	COUNTby20us--;

	if(TCNT1 >= OCR1A)	dirtemp &= 0xDF;
	if(TCNT1 >= OCR1B)	dirtemp &= 0xBF;
	if(TCNT2 >= OCR2B)	dirtemp &= 0x7F;
	
	PORTC = (PORTC & 0x1F) | dirtemp;
	

	if (ADCSRA & 0x10)
	{
		ADCSRA = ADCSRA | 0x10;
		if(ADC_SEQ == 2)
			analog[3]=ADCH;
		else if(ADC_SEQ == 3)
			analog[2]=ADCH;
		else
			analog[ADC_SEQ]=ADCH;

		if (ADC_SEQ == 13)
		{
			if(analog[ADC_SEQ]>=Line0_White)
			LineDetected |= 0x01;
		}
		if(!(LineDetected & 0xFE))
		{
			if (ADC_SEQ == 14)
			{
				if(analog[ADC_SEQ]>=Line1_White)
				LineDetected |= 0x02;
			}
			else if (ADC_SEQ == 15)
			{
				if(analog[ADC_SEQ]>=Line2_White)
				LineDetected |= 0x04;
			}
		}
		ADCSRA = ADCSRA | 0x10;
		
		ADC_SEQ++;
		if (ADC_SEQ>15)		ADC_SEQ=0;
		
		PORTA = ADC_SEQ;
		ADCSRA = ADCSRA & 0x3f;
		ADCSRA = ADCSRA | 0xc0;
		
	}

	if (Pulse_Width_Count==0)
		TRIGERON;
	else if(Pulse_Width_Count==1)
		TRIGEROFF;
	else if (Pulse_Width_Count < 800)
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
	else if (Pulse_Width_Count == 800)	// Stop Measurement after 16ms (Distance : 272cm)
	{
			if (!ECHO_REPLY[0]) ECHO_CNT[0]=Pulse_Width_Count;
			ultra[0]=ECHO_CNT[0];
			if (!ECHO_REPLY[1]) ECHO_CNT[1]=Pulse_Width_Count;
			ultra[1]=ECHO_CNT[1];
			if (!ECHO_REPLY[2]) ECHO_CNT[2]=Pulse_Width_Count;
			ultra[2]=ECHO_CNT[2];
			if (!ECHO_REPLY[3]) ECHO_CNT[3]=Pulse_Width_Count;
			ultra[3]=ECHO_CNT[3];
	}	

	Pulse_Width_Count++;
	if (Pulse_Width_Count > 1000)	// Wait till 20ms, Restart Ultra-Sonic Measurement
	{
		Pulse_Width_Count=0;

		ECHO_CNT[0]=0;
		ECHO_CNT[1]=0;
		ECHO_CNT[2]=0;
		ECHO_CNT[3]=0;
		ECHO_REPLY[0]=0;
		ECHO_REPLY[1]=0;
		ECHO_REPLY[2]=0;
		ECHO_REPLY[3]=0;
	}
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
// ?…ë ¥?¼ë¡œ ?¤ì–´?¤ëŠ” ì±„ë„??ADCë¥??¤í????œí‚¨??
void startConvertion(unsigned char ch)
{
	ADCSRA = ADCSRA & 0x3f;
	ADCSRA = ADCSRA | 0xc0;
}

// startConvertion() ?„ì— ?˜í–‰?˜ë©° ì»¨í„°????ê°’ì„ ë¦¬í„´?œë‹¤.
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
	DDRB = 0x0B;	//?ìœ„ 4ë¹„íŠ¸ë§?ì¶œë ¥
	PORTB=0x02;

	while(busy==1)
	{
		PORTB=0x0A;
		delay20us(4);
		if ((PINB & 0x80) ==0)	busy = 0;
		PORTB=(PORTB & 0xF7);
		delay20us(4);
		PORTB=0x0A;
		delay20us(4);
		PORTB=(PORTB & 0xF5);
		delay20us(4);
	}
	PORTB = 0xFB;
	DDRB = 0xFB;	//?ìœ„ 4ë¹„íŠ¸ë§?ì¶œë ¥	
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
	PORTB=0x28;				//4bit mode set , 1 nibble writing one time.
	PORTB=(PORTB & 0xF7);	//
	Lcd_Cmd(0x28);			//4bit mode set , 2 nibble writing.
	Lcd_Cmd(0x28);			//4bit mode set , one more...
	Lcd_Cmd(0x0C);	
	Lcd_Cmd(0x01);
	delay1ms(3);
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

void HEX_Display(unsigned char x)
{
	unsigned char d=x;
	
	x=(x>>4)&0x0F;
	if(x>9)	Lcd_Data(x-10+'A');
	else Lcd_Data(x+'0');
	x=d & 0x0F;
	if(x>9)	Lcd_Data(x-10+'A');
	else Lcd_Data(x+'0');
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
	Lcd_Move(0, 11);
	Lcd_Data((int)((voltage%1000)/100)+'0');
	Lcd_Data((int)((voltage%100)/10)+'0');
	Lcd_Data('.');
	Lcd_Data((int)((voltage%10))+'0');
	Lcd_Data('V');
}

unsigned char IIC_WRITE(unsigned char add, unsigned char reg, unsigned char data)
{
	unsigned char error_code=0;

	IIC_Start();
	IIC_Wait();
	if ((TWSR & 0xF8) != START)
	{
		Lcd_Write_String(LINE2,"START ERROR");
		error_code=TWSR & 0xF8;
		goto write_error;
	}
	TWDR = add;
	IIC_Twi();
	PORTD &= 0xFE;
	IIC_Wait();
	if ((TWSR & 0xF8) != MT_SLA_ACK)
	{
		Lcd_Write_String(LINE2,"MT_SLA_ACK ERROR");
		error_code=TWSR & 0xF8;
		goto write_error;
	}
	PORTD |= 0x01;	
	
	TWDR = reg;
	IIC_Twi();
	IIC_Wait();
	if ((TWSR & 0xF8) !=MT_DATA_ACK)
	{
		Lcd_Write_String(LINE2,"MT_DATA_ACK ERROR");
		error_code=TWSR & 0xF8;
		goto write_error;
	}
	TWDR = data;
	IIC_Twi();
	IIC_Wait();
	if ((TWSR & 0xF8) !=MT_DATA_ACK)
	{
		Lcd_Write_String(LINE2,"MT_DATA_ACK ERROR");
		error_code=TWSR & 0xF8;
		goto write_error;
	}
	
write_error	:

	IIC_Stop();
	return error_code;
}

unsigned char IIC_READ(unsigned char w_add,unsigned char r_add, unsigned char reg)
{
	unsigned char temp=0;
	
	IIC_Start();
	IIC_Wait();
	if ((TWSR & 0xF8) != START)
	{
		Lcd_Write_String(LINE2,"START ERROR");
		temp=TWSR & 0xF8;
		goto read_error;
	}
	TWDR = w_add;
	IIC_Twi();
	IIC_Wait();
	if ((TWSR & 0xF8) != MT_SLA_ACK)
	{
		Lcd_Write_String(LINE2,"MT_SLA_ACK ERROR");
		temp=TWSR & 0xF8;
		goto read_error;
	}
	
	TWDR = reg;
	IIC_Twi();
	IIC_Wait();
	if ((TWSR & 0xF8) !=MT_DATA_ACK)
	{
		Lcd_Write_String(LINE2,"MT_DATA_ACK ERROR");
		temp=TWSR & 0xF8;
		goto read_error;
	}

	IIC_Start();
	IIC_Wait();
	if ((TWSR & 0xF8) != RE_START)
	{
		Lcd_Write_String(LINE2,"RESTART ERROR");
		temp=TWSR & 0xF8;
		goto read_error;
	}

	TWDR = r_add;
	IIC_Twi();
	IIC_Wait();
	if ((TWSR & 0xF8) !=MR_SLA_ACK)
	{
		Lcd_Write_String(LINE2,"MR_SLA_ACK ERROR");
		temp=TWSR & 0xF8;
		goto read_error;
	}
			
	IIC_Twi();
	IIC_Wait();
	
	if ((TWSR & 0xF8) !=MR_DATA_NOACK)
	{
		Lcd_Write_String(LINE2,"MR_DATA_NOACK ERROR");
		temp=TWSR & 0xF8;
		goto read_error;
	}
	temp=TWDR;
	
read_error:
	IIC_Stop();
	return temp;
}


void read_compass(void)
{
	int16_t x,y;
	
	IIC_WRITE(SLA_W,0x02,0x01);
	delay20us(1);
	delay1ms(6);

	x=IIC_READ(SLA_W,SLA_R,0x03);
	delay20us(1);
	x=x<<8 | IIC_READ(SLA_W,SLA_R,0x04);
	delay20us(1);
	y=IIC_READ(SLA_W,SLA_R,0x07);
	delay20us(1);
	y=y<<8 | IIC_READ(SLA_W,SLA_R,0x08);
//	y=IIC_READ(SLA_W,SLA_R,0x05);
//	delay20us(1);
//	y=y<<8 | IIC_READ(SLA_W,SLA_R,0x06);

	compass = (atan2((double)y,(double)x) * 180 / 3.14159265 + 180)*10; // angle in degrees
	
	compass -= memComp+1800;
	if(compass<0) compass+=3600;
	if(compass>3599) compass-=3600;
	
}