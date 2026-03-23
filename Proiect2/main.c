#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// ------------------------------------------------------------------------
// DEFINES ?i MACRO-URI pentru butoane & auto-repeat
// ------------------------------------------------------------------------
#define LONG_PRESS_TIME       1000  // 1 secund? (în ms)
#define AUTO_REPEAT_PERIOD    250   // 0.25s => 4 pe secund?

// Pinii butoanelor (fostele INT0, INT1, INT2):
//   - PD2 => buton OK   (în codul vechi: ISR(INT0_vect))
//   - PD3 => buton NEXT (în codul vechi: ISR(INT1_vect))
//   - PB2 => buton BACK (în codul vechi: ISR(INT2_vect))
//
// Presupunem c? un buton este "ap?sat" când pinul este LOW (0 logic).
#define IS_OK_PRESSED    ( (PIND & (1 << PD2)) == 0 )
#define IS_NEXT_PRESSED  ( (PIND & (1 << PD3)) == 0 )
#define IS_BACK_PRESSED  ( (PINB & (1 << PB2)) == 0 )

// ------------------------------------------------------------------------
// MACROURI cu logica veche din ISR-urile INT0, INT1, INT2
// (f?r? s? modific?m nimic din logica intern?)
// ------------------------------------------------------------------------

// Echivalentul vechiului ISR(INT0_vect) => OK/ENTER
#define PROCESS_OK_BUTTON()                                                 \
do {                                                                        \
	if (meniu3 == 0 && meniu2 == 0)                                         \
	{                                                                       \
		meniu2 = 1;                                                         \
	}                                                                       \
	else if (meniu3 == 0 && meniu2 != 0)                                    \
	{                                                                       \
		meniu3 = 1;                                                         \
	}                                                                       \
	else if (meniu3 == 1)                                                  \
	{                                                                       \
		/* Salvare parametrii */                                           \
		switch (meniu)                                                      \
		{                                                                   \
			case 1: /* MENIU 1: RGB */                                      \
			switch(meniu2)                                             \
			{                                                           \
				case 1: OCR0  = Dimm*((Red   * 255) / 100)/100;  break; \
				case 2: OCR1A = Dimm*((Green * 255) / 100)/100; break; \
				case 3: OCR1B = Dimm*((Blue  * 255) / 100)/100;  break; \
			}                                                           \
			break;                                                          \
			\
			case 2: /* MENIU 2: Dimm */                                     \
			OCR0  = Dimm*((Red   * 255) / 100)/100;                     \
			OCR1A = Dimm*((Green * 255) / 100)/100;                     \
			OCR1B = Dimm*((Blue  * 255) / 100)/100;                     \
			break;                                                          \
			\
			case 3: /* MENIU 3: Dimmer ON/OFF */                            \
			switch(meniu2)                                             \
			{                                                           \
				case 1: /* ON */                                        \
				dimmer_enabled = 1;                                 \
				break;                                                  \
				case 2: /* OFF */                                       \
				{                                                       \
					dimmer_enabled = 0;                                 \
					OCR0  = Dimm*((Red   * 255) / 100)/100;             \
					OCR1A = Dimm*((Green * 255) / 100)/100;             \
					OCR1B = Dimm*((Blue  * 255) / 100)/100;             \
				}                                                       \
				break;                                                  \
			}                                                           \
			break;                                                          \
			\
			case 4: /* MENIU 4: Strobe ON/OFF */                            \
			switch(meniu2)                                             \
			{                                                           \
				case 1: /* ON */                                        \
				strobe_enabled = 1;                                 \
				break;                                                  \
				case 2: /* OFF */                                       \
				strobe_enabled = 0;                                 \
				break;                                                  \
			}                                                           \
			break;                                                          \
		}                                                                   \
	}                                                                       \
} while(0)

// Echivalentul vechiului ISR(INT1_vect) => NEXT
#define PROCESS_NEXT_BUTTON()                                       \
do {                                                                \
	if (meniu3)                                                     \
	{                                                               \
		switch (meniu)                                             \
		{                                                           \
			case 1: /* RGB */                                       \
			if (meniu2 == 1) Red   = (Red   + 1) % 101;         \
			if (meniu2 == 2) Green = (Green + 1) % 101;         \
			if (meniu2 == 3) Blue  = (Blue  + 1) % 101;         \
			break;                                                  \
			case 2: /* Dimm */                                      \
			Dimm = (Dimm + 1) % 101;                            \
			break;                                                  \
			case 3: /* Dimmer */                                    \
			Dimmer = (Dimmer + 1) % 101;                        \
			break;                                                  \
			case 4: /* Strobe (gol) */                              \
			break;                                                  \
		}                                                           \
	}                                                               \
	else if (meniu2 != 0)                                          \
	{                                                               \
		meniu2++;                                                  \
		switch(meniu)                                              \
		{                                                           \
			case 1: if (meniu2 > 3) meniu2 = 1; break;             \
			case 2: if (meniu2 > 1) meniu2 = 1; break;             \
			case 3: if (meniu2 > 2) meniu2 = 1; break;             \
			case 4: if (meniu2 > 2) meniu2 = 1; break;             \
		}                                                           \
	}                                                               \
	else                                                           \
	{                                                               \
		meniu++;                                                   \
		if (meniu > 4) meniu = 1;                                  \
	}                                                               \
} while(0)

// Echivalentul vechiului ISR(INT2_vect) => BACK
#define PROCESS_BACK_BUTTON()                                   \
do {                                                            \
	if (meniu3)                                                 \
	{                                                           \
		meniu3 = 0;                                             \
	}                                                           \
	else if (meniu2 != 0)                                       \
	{                                                           \
		meniu2 = 0;                                             \
	}                                                           \
	else                                                        \
	{                                                           \
		if (meniu > 1)                                          \
		{                                                       \
			meniu = 1;                                          \
		}                                                       \
	}                                                           \
} while(0)

// ------------------------------------------------------------------------
// VARIABILE GLOBALE
// ------------------------------------------------------------------------
int meniu = 0, etaj = 0, meniu2 = 0, meniu3 = 0;
int digit;
int Red = 100, Blue = 50 , Green = 50, Dimm = 100 , Strobe = 0, Dimmer = 0;
int strobe_enabled = 0; // 0 = OFF, 1 = ON
int ms = 0;
int val;
int DimmADC;
int dimmer_enabled = 0; // 0 = OFF, 1 = ON

// Pentru ADC
unsigned char adc_l, adc_h;

// ------------------------------------------------------------------------
// VARIABILE pentru DETEC?IA ap?s?rii (f?r? întreruperi externe) & auto-repeat
// ------------------------------------------------------------------------
volatile uint8_t  oldOkState      = 0;
volatile uint16_t okPressTime     = 0;
volatile uint8_t  okRepeating     = 0;
volatile uint16_t okRepeatTick    = 0;

volatile uint8_t  oldNextState    = 0;
volatile uint16_t nextPressTime   = 0;
volatile uint8_t  nextRepeating   = 0;
volatile uint16_t nextRepeatTick  = 0;

volatile uint8_t  oldBackState    = 0;
volatile uint16_t backPressTime   = 0;
volatile uint8_t  backRepeating   = 0;
volatile uint16_t backRepeatTick  = 0;

// ------------------------------------------------------------------------
// Ini?ializ?ri hardware
// ------------------------------------------------------------------------
void init_PWM0()
{
	TCCR0 = 0b01101011; // Fast PWM, neinversat
	DDRB |= (1 << 3);
	OCR0 = 128;
}

void init_PWM1()
{
	TCCR1A = 0b10100010;
	TCCR1B = 0b00011011;
	ICR1   = 255;
	OCR1A  = 128;
	OCR1B  = 128;

	DDRD |= (1 << 5);
	DDRD |= (1 << 4);
}

void init_timer2()
{
	TCCR2 |= 0b00001011; // prescaler + CTC
	OCR2   = 125;        // => ~1ms la 8MHz (prescaler 64 => 8e6/64 =125k => 1ms)
	TIMSK |= 0b10000000; // OCIE2 = 1
}

void init_adc(void)
{
	ADMUX  = 0b01000000;  // AVCC + aliniere la dreapta, canal 0 by default
	ADCSRA = 0b10000111;  // ADEN=1, prescaler=128 => 62.5kHz
}

int readADC(char ch)
{
	ADMUX  = (ADMUX & 0b11100000) | (ch & 0b00000111);
	ADCSRA |= (1 << 6);  // ADSC start
	while (ADCSRA & (1 << 6));
	return ADC;
}

void display(int p, char c )
{
	PORTA &= 0b11110000;
	PORTC &= 0b00000000;

	switch (c) {
		case 0:  PORTC |= 0b00111111; break; // 0
		case 1:  PORTC |= 0b00000110; break; // 1
		case 2:  PORTC |= 0b01011011; break; // 2
		case 3:  PORTC |= 0b01001111; break; // 3
		case 4:  PORTC |= 0b01100110; break; // 4
		case 5:  PORTC |= 0b01101101; break; // 5
		case 6:  PORTC |= 0b01111101; break; // 6
		case 7:  PORTC |= 0b00000111; break; // 7
		case 8:  PORTC |= 0b01111111; break; // 8
		case 9:  PORTC |= 0b01100111; break; // 9
		case 'A': PORTC |= 0b01110111; break;
		case 'B': PORTC |= 0b01111100; break;
		case 'C': PORTC |= 0b00111001; break;
		case 'D': PORTC |= 0b01011110; break;
		case 'E': PORTC |= 0b01111001; break;
		case 'F': PORTC |= 0b01110001; break;
		case 'r': PORTC |= 0b01010000; break;
		case 'g': PORTC |= 0b01101111; break;
		case 'b': PORTC |= 0b01111100; break;
		case 'f': PORTC |= 0b01110001; break;
		case 'a': PORTC |= 0b01110111; break;
		case 'd': PORTC |= 0b01011110; break;
		case 'e': PORTC |= 0b01111001; break;
		case 's': PORTC |= 0b01101101; break;
		case 't': PORTC |= 0b01111000; break;
		case 'i': PORTC |= 0b00000100; break;
		case 'm': PORTC |= 0b01010100; break;
		case 'H': PORTC |= 0b01110110; break;
		case 'L': PORTC |= 0b00111000; break;
		case 'P': PORTC |= 0b01110011; break;
		case 'U': PORTC |= 0b00111110; break;
		case '-': PORTC |= 0b01000000; break;
		default:
		PORTC |= 0b00000000; break;
	}

	switch (p) {
		case 1: PORTA |= 0b00000001; break;
		case 2: PORTA |= 0b00000010; break;
		case 3: PORTA |= 0b00000100; break;
		case 4: PORTA |= 0b00001000; break;
	}
}

// ------------------------------------------------------------------------
// ISR(TIMER2_COMP_vect): gener?m "tick" de 1ms
//   - Cronometrare strobe, dimmer
//   - Detec?ie butoane (OK, NEXT, BACK) + auto-repeat
// ------------------------------------------------------------------------
ISR(TIMER2_COMP_vect)
{
	ms++;
	if (ms >= 1000) {
		ms = 0;
	}

	// ------------------ STROBE ------------------
	if (strobe_enabled)
	{
		if (ms <= 100)
		{
			TCCR0  |= (1 << 5);
			TCCR1A |= (1 << 7) | (1 << 5);
		}
		else
		{
			TCCR0  &= ~(1 << 5);
			TCCR1A &= ~((1 << 7) | (1 << 5));
		}
	}
	else
	{
		TCCR0  |= (1 << 5);
		TCCR1A |= (1 << 7) | (1 << 5);
	}

	// ------------------ DIMMER ------------------
	if (dimmer_enabled)
	{
		if (ms == 500)
		{
			val     = readADC(6);
			DimmADC = (val / 10);

			OCR0  = DimmADC * ((Red   * 255) / 100) / 100;
			OCR1A = DimmADC * ((Green * 255) / 100) / 100;
			OCR1B = DimmADC * ((Blue  * 255) / 100) / 100;
		}
	}

	// ------------------ AFI?ARE pe 7seg (meniuri) ------------------
	switch (meniu)
	{
		case 1: // RGB
		{
			switch (meniu2)
			{
				case 1: // Red
				{
					if (meniu3)
					{
						digit++;
						switch (digit)
						{
							case 1: display(4, Red / 1000 % 10);   break;
							case 2: display(3, Red / 100  % 10);   break;
							case 3: display(2, Red / 10   % 10);   break;
							case 4: display(1, Red        % 10);   digit = 0; break;
							default: digit = 0; break;
						}
					}
					else
					{
						digit++;
						switch (digit)
						{
							case 1: display(4, ' '); break;
							case 2: display(3, 'r'); break;
							case 3: display(2, 'e'); break;
							case 4: display(1, 'd'); digit = 0; break;
							default: digit = 0; break;
						}
					}
					break;
				}
				case 2: // Green
				{
					if (meniu3)
					{
						digit++;
						switch (digit)
						{
							case 1: display(4, Green / 1000 % 10); break;
							case 2: display(3, Green / 100  % 10); break;
							case 3: display(2, Green / 10   % 10); break;
							case 4: display(1, Green        % 10); digit = 0; break;
							default: digit = 0; break;
						}
					}
					else
					{
						digit++;
						switch (digit)
						{
							case 1: display(4, 'g'); break;
							case 2: display(3, 'r'); break;
							case 3: display(2, 'e'); break;
							case 4: display(1, 'e'); digit = 0; break;
							default: digit = 0; break;
						}
					}
					break;
				}
				case 3: // Blue
				{
					if (meniu3)
					{
						digit++;
						switch (digit)
						{
							case 1: display(4, Blue / 1000 % 10); break;
							case 2: display(3, Blue / 100  % 10); break;
							case 3: display(2, Blue / 10   % 10); break;
							case 4: display(1, Blue        % 10); digit = 0; break;
							default: digit = 0; break;
						}
					}
					else
					{
						digit++;
						switch (digit)
						{
							case 1: display(4, 'b'); break;
							case 2: display(3, 'L'); break;
							case 3: display(2, 'U'); break;
							case 4: display(1, 'e'); digit = 0; break;
							default: digit = 0; break;
						}
					}
					break;
				}
				default:
				{
					digit++;
					switch (digit)
					{
						case 1: display(4, ' '); break;
						case 2: display(3, 'r'); break;
						case 3: display(2, 'g'); break;
						case 4: display(1, 'b'); digit = 0; break;
						default: digit = 0; break;
					}
					break;
				}
			}
			break;
		}

		case 2: // Dimm
		{
			if(meniu2)
			{
				if(meniu3)
				{
					digit++;
					switch (digit)
					{
						case 1: display(4, ' ');               break;
						case 2: display(3, Dimm / 100 % 10);   break;
						case 3: display(2, Dimm / 10  % 10);   break;
						case 4: display(1, Dimm       % 10);   digit = 0; break;
						default: digit = 0; break;
					}
				}
				else
				{
					digit++;
					switch (digit)
					{
						case 1: display(4, 'd'); break;
						case 2: display(3, 'i'); break;
						case 3: display(2, 'm'); break;
						case 4: display(1,  1 ); digit = 0; break;
						default: digit = 0; break;
					}
				}
			}
			else
			{
				digit++;
				switch (digit)
				{
					case 1: display(4, 'd'); break;
					case 2: display(3, 'i'); break;
					case 3: display(2, 'm'); break;
					case 4: display(1, 'm'); digit = 0; break;
					default: digit = 0; break;
				}
			}
			break;
		}

		case 3: // Dimmer / dAmC
		{
			switch(meniu2)
			{
				case 1: // ON
				{
					digit++;
					switch (digit)
					{
						case 1: display(4, ' ');  break;
						case 2: display(3, ' ');  break;
						case 3: display(2, 0);    break;
						case 4: display(1, 'm'); digit = 0; break;
						default: digit = 0; break;
					}
					break;
				}
				case 2: // OFF
				{
					digit++;
					switch (digit)
					{
						case 1: display(4, ' '); break;
						case 2: display(3, 0);   break;
						case 3: display(2, 'f'); break;
						case 4: display(1, 'f'); digit = 0; break;
						default: digit = 0; break;
					}
					break;
				}
				default:
				{
					digit++;
					switch (digit)
					{
						case 1: display(4, 'd'); break;
						case 2: display(3, 'A'); break;
						case 3: display(2, 'm'); break;
						case 4: display(1, 'C'); digit = 0; break;
						default: digit = 0; break;
					}
					break;
				}
			}
			break;
		}

		case 4: // Strobe
		{
			switch(meniu2)
			{
				case 1: // ON
				{
					digit++;
					switch (digit)
					{
						case 1: display(4, ' '); break;
						case 2: display(3, ' '); break;
						case 3: display(2, 0);   break;
						case 4: display(1, 'm'); digit = 0; break;
						default: digit = 0; break;
					}
					break;
				}
				case 2: // OFF
				{
					digit++;
					switch (digit)
					{
						case 1: display(4, ' '); break;
						case 2: display(3, 0);   break;
						case 3: display(2, 'f'); break;
						case 4: display(1, 'f'); digit = 0; break;
						default: digit = 0; break;
					}
					break;
				}
				default:
				{
					digit++;
					switch (digit)
					{
						case 1: display(4, 's'); break;
						case 2: display(3, 't'); break;
						case 3: display(2, 'r'); break;
						case 4: display(1, 'b'); digit = 0; break;
						default: digit = 0; break;
					}
					break;
				}
			}
			break;
		}

		default:
		{
			meniu = 1;
			break;
		}
	}

	// ----------------------------------------------------------------
	//         LOGICA nou?: detec?ie butoane (OK, NEXT, BACK)
	//         ?i auto-repeat (f?r? întreruperi externe)
	// ----------------------------------------------------------------

	// ======================== BUTON OK ========================
	uint8_t currentOkState = (IS_OK_PRESSED ? 1 : 0);
	if (currentOkState != oldOkState)
	{
		// schimbare stare (0->1 sau 1->0)
		if (currentOkState == 1)
		{
			// buton ABIA ap?sat
			okPressTime     = 0;
			okRepeating     = 0;
			okRepeatTick    = 0;
		}
		else
		{
			// buton ELIBERAT
			if (okPressTime < LONG_PRESS_TIME)
			{
				// click scurt
				PROCESS_OK_BUTTON();
			}
		}
	}
	else
	{
		// stare neschimbat?
		if (currentOkState == 1)
		{
			// men?inut ap?sat
			okPressTime++;
			if ((okPressTime >= LONG_PRESS_TIME) && (okRepeating == 0))
			{
				// la atingerea 1s => prima repetare
				PROCESS_OK_BUTTON();
				okRepeating  = 1;
				okRepeatTick = 0;
			}
			else if (okRepeating)
			{
				okRepeatTick++;
				if (okRepeatTick >= AUTO_REPEAT_PERIOD)
				{
					PROCESS_OK_BUTTON();
					okRepeatTick = 0;
				}
			}
		}
	}
	oldOkState = currentOkState;

	// ======================== BUTON NEXT ========================
	uint8_t currentNextState = (IS_NEXT_PRESSED ? 1 : 0);
	if (currentNextState != oldNextState)
	{
		if (currentNextState == 1)
		{
			nextPressTime   = 0;
			nextRepeating   = 0;
			nextRepeatTick  = 0;
		}
		else
		{
			if (nextPressTime < LONG_PRESS_TIME)
			{
				PROCESS_NEXT_BUTTON();
			}
		}
	}
	else
	{
		if (currentNextState == 1)
		{
			nextPressTime++;
			if ((nextPressTime >= LONG_PRESS_TIME) && (nextRepeating == 0))
			{
				PROCESS_NEXT_BUTTON();
				nextRepeating  = 1;
				nextRepeatTick = 0;
			}
			else if (nextRepeating)
			{
				nextRepeatTick++;
				if (nextRepeatTick >= AUTO_REPEAT_PERIOD)
				{
					PROCESS_NEXT_BUTTON();
					nextRepeatTick = 0;
				}
			}
		}
	}
	oldNextState = currentNextState;

	// ======================== BUTON BACK ========================
	uint8_t currentBackState = (IS_BACK_PRESSED ? 1 : 0);
	if (currentBackState != oldBackState)
	{
		if (currentBackState == 1)
		{
			backPressTime   = 0;
			backRepeating   = 0;
			backRepeatTick  = 0;
		}
		else
		{
			if (backPressTime < LONG_PRESS_TIME)
			{
				PROCESS_BACK_BUTTON();
			}
		}
	}
	else
	{
		if (currentBackState == 1)
		{
			backPressTime++;
			if ((backPressTime >= LONG_PRESS_TIME) && (backRepeating == 0))
			{
				PROCESS_BACK_BUTTON();
				backRepeating  = 1;
				backRepeatTick = 0;
			}
			else if (backRepeating)
			{
				backRepeatTick++;
				if (backRepeatTick >= AUTO_REPEAT_PERIOD)
				{
					PROCESS_BACK_BUTTON();
					backRepeatTick = 0;
				}
			}
		}
	}
	oldBackState = currentBackState;
}

// ------------------------------------------------------------------------
// main()
// ------------------------------------------------------------------------
int main(void)
{
	// Porturi pentru 7seg
	DDRA |= 0b11110000;
	DDRC |= 0b11111111;

	// Nu mai ini?ializ?m întreruperile externe
	// init_int0();
	// init_int1();
	// init_int2();

	init_PWM0();
	init_PWM1();
	init_timer2();
	init_adc();

	// Ini?ializare PWM la valorile Dimm, Red, Green, Blue curente
	OCR0  = Dimm*((Red   * 255) / 100)/100;
	OCR1A = Dimm*((Green * 255) / 100)/100;
	OCR1B = Dimm*((Blue  * 255) / 100)/100;

	// Activ?m întreruperile globale (pentru Timer2!)
	SREG |= (1 << 7);

	while(1)
	{
		// Program principal "gol" (toat? logica de butoane & strobe/dimmer e în ISR)
	}
}
