#include <atmel_start.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>

union fixedpoint {
	struct {
		unsigned char fraction;
		unsigned char whole;
	} parts;
	int value;
};

union fixedpoint squarewave( union fixedpoint phase ) {
	union fixedpoint result;
	if( phase.value < 0 ) {
		result.value = 0x8000;
	} else {
		result.value = 0x7FFF;
	}
	return result;
}

union fixedpoint sawwave( union fixedpoint phase ) {
	return phase;
}

union fixedpoint trianglewave( union fixedpoint phase ) {
	union fixedpoint result;
	if( phase.value < 0 ) {
		result.value = (phase.value + 0x4000) * 2;
	} else {
		result.value = 0x7FFF - ( phase.value * 2 );
	}
	return result;
}

union fixedpoint sinewave( union fixedpoint phase ) {
	return trianglewave( phase );
}

volatile union fixedpoint phase;
volatile int filtered;
volatile unsigned int step = 200;
volatile signed char sample = 0;
volatile int ratio = 230;
volatile int time = 0;

ISR(TCA0_OVF_vect) {
	PORTA_OUTSET = 0b10;
	DAC0.DATA = sample + 128;
	phase.value += step;
	union fixedpoint sq = sawwave( phase );
	int ratio2 = 256 - ratio;
	filtered = ( filtered * ratio + (sq.value >> 8) * ratio2 ) >> 8;
	sample = filtered;
	//sample = square.parts.whole + 128;
	//int ratio2 = 256 - ratio;
	//filtered = ( filtered * ratio + (phase.value >> 8) * ratio2 ) >> 8;
	//sample = filtered + 128;
	//sample = phase.parts.whole;
	time ++;
	TCA0.SINGLE.INTFLAGS = 1; // reset interrupt
}

int main(void)
{
	atmel_start_init();
	
	phase.value = 0;
	filtered = 0;

	PORTA_DIR = 0xFF;
	PORTA.DIRSET = 1 << 1;
	PORTA_OUT = 0x00;
	
	// 256 tick interval
	TCA0.SINGLE.CTRLA = 0x0 << 1; // no scaling
	TCA0.SINGLE.CTRLB = 0;
	TCA0.SINGLE.PER = 256;
	TCA0.SINGLE.INTCTRL = 1; // enable overflow interrupt
	SREG |= 1 << 7; // enable global interrupts
	TCA0.SINGLE.CTRLA |= 1; // enable timer

	PORTA.DIRSET = (1<<6);
	VREF.CTRLA |= VREF_DAC0REFSEL_2V5_gc; // set DAC0REFSEL bits
	DAC0.DATA = 0;
	DAC0.CTRLA = 0b01000001; // enable output, enable DAC
	
	int step = 1;
	for( ;; ) {
		PORTA_OUTCLR = 0b10;
		if( (time & 0b11111111111) == 0 ) {
			ratio = ratio + step;
			if( ratio == 255 ) {
				step = -1;
			} else if( ratio == 190 ) {
				step = 1;
			}
		}
	}

}
