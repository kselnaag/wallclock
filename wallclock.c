#define	F_CPU 8000000UL
#define __AVR_ATmega8__

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#ifndef CLEARBIT
#define CLEARBIT(data, bit) (data) &= ~(1 << (bit))
#endif

#ifndef SETBIT
#define SETBIT(data, bit) (data) |= (1 << (bit))
#endif

#ifndef CHECKBIT
#define CHECKBIT(data, bit) ((data) & (1 << (bit)))
#endif

#ifndef INVERTBIT
#define INVERTBIT(data, bit) (data) ^= (1 << (bit))
#endif

#define s08	signed char
#define s16	signed int

#define u08	unsigned char
#define u16	unsigned int

typedef enum {FALSE = 0, TRUE = !FALSE} BOOL;

//watch states
#define MODE_IDLE	0
#define NEXT_SEC	1
#define BUTT_SHORT	2
#define BUTT_LONG	3
#define MODE_MIN	4
#define MODE_HOUR	5

u08 g_STATE = 0; // MODE_IDLE
u08 g_isBlink = 0;
u08 g_secs = 50, g_mins = 59, g_hours = 23;

u08 arr_Ind_LEN = 14;
u08 arr_Ind[] = {
	 0b00000011 // 0
	,0b10011111 // 1
	,0b00100101 // 2
	,0b00001101 // 3
	,0b10011001 // 4
	,0b01001001 // 5
	,0b01000001 // 6
	,0b00011111 // 7
	,0b00000001 // 8
	,0b00001001 // 9
	,0b11111101 // -
	,0b10111111 // >|
	,0b11111011 // |<
	,0b11111111 // empty
};

/////// Prototypes //////////////////////////////
void state_init(void);
void ports_init(void);
void RTCInit(void);
void disp_send(u08 arr_data[], u08 arr_data_len);
void WaitForNextTick(void);
//////////////////////////////////////////////////


void state_init(void) {
	GICR	= (1<<INT1)|(1<<INT0)|(0<<IVSEL)|(0<<IVCE);
	TIMSK	= (0<<OCIE2)|(1<<TOIE2)|(0<<TICIE1)|(0<<OCIE1A)|(0<<OCIE1B)|(0<<TOIE1)|(0<<TOIE0);
	SFIOR	= (0<<ACME)|(0<<PUD)|(0<<PSR2)|(0<<PSR10);
	MCUCR	= (1<<SE)|(0<<SM2)|(0<<SM1)|(0<<SM0)|(1<<ISC11)|(0<<ISC10)|(1<<ISC01)|(1<<ISC00); //int1 - V, int0 - ^
	
	TCCR0	= (0<<CS02)|(0<<CS01)|(0<<CS00);
	TCNT0	= 0;
	
	TCCR1A	= (0<<COM1A1)|(0<<COM1A0)|(0<<COM1B1)|(0<<COM1B0)|(0<<FOC1A)|(0<<FOC1B)|(0<<WGM11)|(0<<WGM10);
	TCCR1B	= (0<<ICNC1)|(0<<ICES1)|(0<<WGM13)|(0<<WGM12)|(0<<CS12)|(0<<CS11)|(1<<CS10);
	TCNT1H	= 0; 
	TCNT1L	= 0;
	OCR1AH	= 0;
	OCR1AL	= 0;
	OCR1BH	= 0;
	OCR1BL	= 0;
	ICR1H	= 0;
	ICR1L	= 0;
	
	ACSR	= (1<<ACD)|(0<<ACBG)|(0<<ACO)|(0<<ACI)|(0<<ACIE)|(0<<ACIC)|(0<<ACIS1)|(0<<ACIS0); 
	ADMUX	= (0<<REFS1)|(0<<REFS0)|(0<<ADLAR)|(0<<MUX3)|(0<<MUX2)|(0<<MUX1)|(0<<MUX0); 
	ADCSRA	= (0<<ADEN)|(0<<ADSC)|(0<<ADFR)|(0<<ADIF)|(0<<ADIE)|(0<<ADPS2)|(0<<ADPS1)|(0<<ADPS0);
}

void ports_init(void) {
	/*
	PORTA	= 0b00000000;
	DDRA	= 0b00000000;
	*/
	PORTB	= 0b00000000;
	DDRB	= 0b00000000;
	
	PORTC	= 0b00000000;
	DDRC	= 0b00000011;
	
	PORTD	= 0b00000000;
	DDRD	= 0b00000000;
}

ISR(TIMER2_OVF_vect) {		
	SETBIT(g_STATE, NEXT_SEC);
	if (g_secs > 58) {
		g_secs = 00;
		if (g_mins > 58) {
			g_mins = 00;
			if (g_hours > 22) g_hours = 00;
			else g_hours++;
		} else g_mins++;
	} else g_secs++;
	g_isBlink = ~g_isBlink;
}

ISR(INT0_vect) {
	if (!CHECKBIT(g_STATE, BUTT_LONG)) {
		SETBIT(g_STATE, BUTT_SHORT);
		if CHECKBIT(g_STATE, MODE_MIN) {
			if (g_mins > 58) g_mins = 00;
			else g_mins++;
		}
		if CHECKBIT(g_STATE, MODE_HOUR) {
			if (g_hours > 22) g_hours = 00;
			else g_hours++;
		}
	} else CLEARBIT(g_STATE, BUTT_LONG);
	g_secs = 0;
	CLEARBIT(GIFR, INTF0);
}

ISR(INT1_vect) {
	SETBIT(g_STATE, BUTT_LONG);
	
	if CHECKBIT(g_STATE, MODE_IDLE) {
		CLEARBIT(g_STATE, MODE_IDLE);
		SETBIT(g_STATE, MODE_MIN);
	} else {
		if CHECKBIT(g_STATE, MODE_MIN) {
			CLEARBIT(g_STATE, MODE_MIN);
			SETBIT(g_STATE, MODE_HOUR);
		} else {
			if CHECKBIT(g_STATE, MODE_HOUR) {
				CLEARBIT(g_STATE, MODE_HOUR);
				SETBIT(g_STATE, MODE_IDLE);
			}
		}
	}
	CLEARBIT(GIFR, INTF1);
}

void disp_send(u08 arr_data[], u08 arr_data_len) {
	u08 i,j;
	for(j=0; j<arr_data_len; j++) {
		for(i=0; i<8; i++) {
			CLEARBIT(PORTC, PC1);
			if (CHECKBIT(arr_data[j], i)) SETBIT(PORTC, PC0);
			else CLEARBIT(PORTC, PC0);
			SETBIT(PORTC, PC1);
		}
	}
}

void RTCInit(void) {  //Timer2 init acording datasheet
	_delay_ms(1000);
	CLEARBIT(TIMSK, OCIE2);
	CLEARBIT(TIMSK, TOIE2);
	ASSR	= (1<<AS2)|(0<<TCN2UB)|(0<<OCR2UB)|(0<<TCR2UB);
	TCNT2	= 0;
	OCR2	= 0;
	TCCR2	= (0<<FOC2)|(0<<WGM20)|(0<<COM21)|(0<<COM20)|(0<<WGM21)|(1<<CS22)|(0<<CS21)|(1<<CS20);
	while ( CHECKBIT(ASSR, TCR2UB) || CHECKBIT(ASSR, TCN2UB) || CHECKBIT(ASSR, OCR2UB) );
	CLEARBIT(TIFR, OCF2);
	CLEARBIT(TIFR, TOV2);
	SETBIT(TIMSK, TOIE2);
}

void WaitForNextTick(void) {
	set_sleep_mode(SLEEP_MODE_IDLE);
	sei();
	sleep_mode();
	cli();
}

int main(void) {	
	cli();
	state_init();
	ports_init();
	RTCInit();
	
	SETBIT(g_STATE, MODE_IDLE);
	u08 arr_Disp[5] = { arr_Ind[2], arr_Ind[3], arr_Ind[10], arr_Ind[5], arr_Ind[9] };
	
	while(1) {
		
		arr_Disp[0] = arr_Ind[g_hours/10];
		arr_Disp[1] = arr_Ind[g_hours%10];
		
		arr_Disp[3] = arr_Ind[g_mins/10];
		arr_Disp[4] = arr_Ind[g_mins%10];
		
		if (g_isBlink) {
			if CHECKBIT(g_STATE, MODE_IDLE)	arr_Disp[2] = arr_Ind[10];
			if CHECKBIT(g_STATE, MODE_MIN)	arr_Disp[2] = arr_Ind[11];
			if CHECKBIT(g_STATE, MODE_HOUR)	arr_Disp[2] = arr_Ind[12];
		}
		else arr_Disp[2] = arr_Ind[13];
		
		disp_send(arr_Disp, 5);
		
		CLEARBIT(g_STATE, BUTT_SHORT);
		CLEARBIT(g_STATE, NEXT_SEC);
		
		WaitForNextTick();
	}
	
	return 0;
}


/*

ATmega8 (0x1E9307)

IntRC/1MHz(DEFAULT)
FUSEH: 0xD9 (1101 1001)
FUSEL: 0xE1 (1110 0001)

IntRC/4MGz
FUSEH: 0xD9 (1101 1001)
FUSEL: 0xD3 (1101 0011)

ExtQuartz/4MGz
FUSEH: 0xD9 (1101 1001)
FUSEL: 0xEF (1110 1111)



(DEFAULT)
FUSEH:
S8535C WDTON SPIEN CKOPT     EESAVE BOOTSZ1 BOOTSZ2 BOOTRST
   1      1    0     1          1      0       0       1

FUSEL:
BODLVL BODEN SUT1 SUT0       CKSEL3 CKSEL2 CKSEL1 CKSEL0
  1      1     1   0           0      0      0      1


*/