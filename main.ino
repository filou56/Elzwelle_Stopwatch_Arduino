#include <util/atomic.h>
#include <I2C_RTC.h>
#include <Wire.h>
#include "util.h"

#define DEFAULT_SAMPLE_WIDTH   50

#define SECONDS_PER_DAY		86400
#define SECONDS_PER_HOUR	 3600

static DS3231 RTC;

volatile uint16_t 	sync_counter;
volatile uint32_t	millis_counter;
volatile uint32_t	unix_epoch;
volatile bool		tick;

volatile uint32_t   trail_start;
volatile uint32_t	lead_start;
volatile uint32_t	width_start;
volatile bool		tick_start;

volatile uint32_t   trail_finish;
volatile uint32_t	lead_finish;
volatile uint32_t	width_finish;
volatile bool		tick_finish;

volatile uint32_t	sample_width;

uint8_t	timezone	= 1;
uint8_t dst			= 0;
bool	print_stamp	= true;

ISR (TIMER1_COMPA_vect){  			// Interrupt Service Routine...
	PORTD = PORTD | (1<<PD4);
}

void sync_isr() {
		sync_counter = TCNT1;
		TCNT1 = 0;
		TCNT2 = 0;
		PORTD = PORTD & ~(1<<PD4);
		millis_counter += 1000;
		tick = true;
}

ISR(PCINT0_vect) {
	volatile static bool pending_finish = false;

	delayMicroseconds(10);

	if ( !(PINB & 0b00000001) && !pending_finish ) {
		lead_finish = millis_counter + TCNT1;
		pending_finish  = true;
	} else if ( (PINB & 0b00000001) && pending_finish ) {
		trail_finish = millis_counter + TCNT1;
		pending_finish = false;
		if ( (trail_finish - lead_finish) > sample_width ) {
			tick_finish = true;
		}
	}
}

ISR(PCINT1_vect) {
	volatile static bool pending_start  = false;

	delayMicroseconds(10);

	if ( !(PINC & 0b00000001) && !pending_start ) {
		lead_start = millis_counter + TCNT1;
		pending_start  = true;
	} else if ( (PINC & 0b00000001) && pending_start ) {
		trail_start = millis_counter + TCNT1;
		pending_start = false;
		if ( (trail_start - lead_start) > sample_width ) {
			tick_start = true;
		}
	}
}

void setup() {
	Serial.begin(57600);

	// disable global interrupts
  	cli();

    // 			1kHz an D3								// WGM22/WGM21/WGM20 all set -> Mode 7, fast PWM
    TCCR2A  = (1<<COM2B1) + (1<<WGM21) + (1<<WGM20); 	// Set OC2B at bottom, clear OC2B at compare match
    TCCR2B  = (1<<CS22)   + (1<<WGM22); 				// prescaler = 64;
    OCR2A   = 249;										// 1kHz
    OCR2B   = 124;
    DDRD   |= (1<<PD3);									// Arduino NANO D3

    //         Count 1kHz from Timer2					// External Input on Arduino D5
    TCCR1A  = 0; ;
    TCCR1B  = (1<<CS12) + (1<<CS11) + (1<<CS10);
    OCR1A   = 999;
    TCNT1	= 0;

    DDRD   |= (1<<PD4);									// Arduino NANO D4
    TIMSK1  = (1<<OCIE1A); 								// interrupt when Compare Match with OCR1A

	// Start Pinchange Int8 on PC0 / Analog 0
	PCMSK1 |= bit(0);
	PCICR  |= bit(1);

	// Finish Pinchange Int0 on PB0 / Digital 8
	PCMSK0 |= bit(0);
	PCICR  |= bit(0);

	trail_start    = 0;
	lead_start	   = 0;
	width_start    = 300;
	tick_start	   = false;

	trail_finish  = 0;
	lead_finish   = 0;
	width_finish  = 300;
	tick_finish	  = false;

	sample_width  = DEFAULT_SAMPLE_WIDTH;

  	// enable global interrupts
  	sei();

  	pinMode(2, INPUT_PULLUP);
  	attachInterrupt(digitalPinToInterrupt(2), sync_isr, RISING);

	RTC.begin();
//	RTC.setHourMode(CLOCK_H24);
	RTC.enableSqwePin();

	unix_epoch = RTC.getEpoch(true);

}

void printCentis(uint32_t stamp) {

	uint32_t millis = stamp % 1000;
	if ( millis < 100 )
		Serial.print('0');
	if ( millis < 10 ) {
		Serial.print('0');
	}
	Serial.print(millis);
}

void loop() {
	char 	 c;
	uint32_t stamp;

	// ------------- Serial command interpreter -----------

	if (Serial.available() > 0) {
		c = Serial.read();
		Serial.print(c);
		switch (tolower(c)) {
		case 'e':							// Set Unix epoch timestamp from linux date +%s
			unix_epoch  = Serial.parseInt();
//			unix_epoch += (uint32_t(dst + timezone) * SECONDS_PER_HOUR);
			RTC.setEpoch(unix_epoch);
			Serial.print(unix_epoch);
			break;
		case 'w':
			sample_width = Serial.parseInt();
			if ( sample_width < 10   ) sample_width = 10;
			if ( sample_width > 5000 ) sample_width = 5000;
			Serial.print(sample_width);
			break;
		case 'r':							// Read real time clock
			Serial.print(RTC.getDay());
			Serial.print(".");
			Serial.print(RTC.getMonth());
			Serial.print(".");
			Serial.print(RTC.getYear());
			Serial.print(" ");
			Serial.print(RTC.getHours());
			Serial.print(":");
			Serial.print(RTC.getMinutes());
			Serial.print(":");
			Serial.print(RTC.getSeconds());
			break;
		case 's':							// Read sync counter
			Serial.print(sync_counter);
			break;
		case 'm':							// Read millis counter
			Serial.print(millis_counter);
			break;
		case 'p':							// Toogle print flag
			print_stamp = !print_stamp;
			Serial.print('!');
			break;
		default:
			Serial.print('?');
		}
		Serial.println();
	} else {

		if (tick_start) {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				stamp = lead_start;
			}

			Serial.print('S');
			Serial.print(unix_epoch + stamp / 1000);
			Serial.print(".");
			printCentis(stamp);
			Serial.print('\n');
			tick_start = false;
		}

		if (tick_finish) {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				stamp = lead_finish;
			}

			Serial.print('F');
			Serial.print(unix_epoch + stamp / 1000);
			Serial.print(".");
			printCentis(stamp);
			Serial.print('\n');
			tick_finish = false;
		}

		if (print_stamp && tick) {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				stamp = millis_counter;
				tick = false;
			}

			Serial.print('#');
			Serial.print(unix_epoch + stamp / 1000);
			Serial.print(".");
			Serial.print(stamp % 1000);
			Serial.print('\n');

		}
	}
}
