/*
 * SOCCER.h
 *
 * Created: 2015-08-06 오후 1:59:58
 * Author: JEON HAKYEONG (전하경)
 *
 */ 

#define ENTER (PINC & 0x08)
#define SELECT (PINC & 0x04)

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

volatile int	COUNTby20us = 0;

unsigned int Pulse_Width_Count=0;
unsigned int ECHO_CNT[4];
unsigned char ECHO_REPLY[4]={0,0,0,0};
unsigned int  Distance[3][4];
unsigned char ADC_SEQ = 0;
unsigned char ADC_VALUE[3][16];
unsigned char ADC_TEMP;

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
	PORTA = 0xFC;
	DDRA = 0x0F;
	PORTB = 0xFB;
	DDRB = 0xFB;	//상위 4비트만 출력
	PORTC = 0x0F; 
	DDRC = 0xE0;
	PORTD = 0x7F;
	DDRD = 0xF3;
}

void timer_init(void)
{
	TCCR0B = 0x02;		//  clock/8  pre-scaler
	TCNT0 = 206;		//  20us Period
	
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
	TIMSK2 = 0x01; //timer0 Overflow Interrupt Enable
}


ISR(TIMER0_OVF_vect)
{
	int i;
	
	TCNT0 = 206;		//  RELOAD.. 20us Period

	PORTD &= 0xFE;
	if (COUNTby20us>0)	COUNTby20us--;
	
	if (ADCSRA & 0x10)
	{
		ADCSRA = ADCSRA | 0x10;
		ADC_VALUE[0][ADC_SEQ] = ADC_VALUE[1][(int)ADC_SEQ];
		ADC_VALUE[1][ADC_SEQ] = (char)ADCL;
		ADC_VALUE[1][ADC_SEQ] = (char)ADCH;
		analog[ADC_SEQ]=(ADC_VALUE[0][ADC_SEQ]+ADC_VALUE[1][ADC_SEQ])/2;
		ADCSRA = ADCSRA | 0x10;
		
		ADC_SEQ++;
		if (ADC_SEQ>15)	ADC_SEQ=0;
		
		PORTA = ADC_SEQ;
		ADCSRA = ADCSRA & 0x3f;
		ADCSRA = ADCSRA | 0xc0;
//		PORTD = ~PORTD;
	}
	
	if (Pulse_Width_Count==0)
	{
		TRIGERON;
	}
	else if (Pulse_Width_Count==1)	TRIGEROFF;
	else if (Pulse_Width_Count < 800)
	{
		if (!ECHO_CNT[0] && ECHO1) ECHO_CNT[0]=Pulse_Width_Count;
		if (!ECHO_CNT[1] && ECHO2) ECHO_CNT[1]=Pulse_Width_Count;
		if (!ECHO_CNT[2] && ECHO3) ECHO_CNT[2]=Pulse_Width_Count;
		if (!ECHO_CNT[3] && ECHO4) ECHO_CNT[3]=Pulse_Width_Count;

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
	}
	else if (Pulse_Width_Count == 800)
	{
		for(i=0; i<4;i++)
		{
			if (!ECHO_REPLY[i]) ECHO_CNT[i]=Pulse_Width_Count;
			Distance[0][i]=Distance[1][i];
			Distance[1][i]=ECHO_CNT[i];
			ultra[i]=(float)(Distance[0][i]+Distance[1][i])*0.34/2;
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
	PORTD |= 0x01;
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


// 입력으로 들어오는 채널의 ADC를 스타트 시킨다.
void startConvertion(unsigned char ch)
{
	ADCSRA = ADCSRA & 0x3f;
//	ADMUX = 0x64 | (ch & 0x0f);
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

