#include <atmel_start.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <usart_basic.h>
#include <atomic.h>
#include <usart_basic_example.h>
#include "i2c.h"

#define MAX4820_CS PD1    // Define Chip Select as PD1
#define MOSI  3
#define MISO  4
#define SCK   5

#define PUMP1 0
#define PUMP2 1
#define BUZZ1 2

#define ADC_MAX 1023      // 10-bit ADC maximum value
#define V_REF 4.09        // Reference voltage
#define R_SERIES 1000.0   // Series resistor value in ohms
#define R0 1000.0         // PT1000 resistance at 0°C
#define ALPHA 3.85        // PT1000 temperature coefficient (Ohms/°C)

#define LCD_ADDR 0x3E  // Adjust based on your module's address
#define LCD_RGB_ADDRESS 0x60  // I2C address for RGB backlight

#define WHITE           0
#define RED             1
#define GREEN           2
#define BLUE            3

#define REG_RED         0x04        // pwm2
#define REG_GREEN       0x03        // pwm1
#define REG_BLUE        0x02        // pwm0

#define REG_MODE1       0x00
#define REG_MODE2       0x01
#define REG_OUTPUT      0x08


#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x8DOTS 0x00

// Varaajamonitorin tilat
#define LEPO		0x00
#define LATAUS      0x01
#define PALUU		0x02
#define VIKA        0x03

// Menu
#define LATAUSRAJA  0x00
#define TURVARAJA   0x01
#define PALUURAJA   0x02
#define HYSTEREESI	0x03
#define AVCC        0x04
#define KORJAUS     0x05

// Päävalikko
#define MONITORI    0x00
#define MENU        0x01

// Nappulat
#define NUM_BUTTONS 6
#define DEBOUNCE_COUNT 0  // Number of reads for debouncing

#define BTN_UP     0x00
#define BTN_DOWN   0x01
#define BTN_RIGHT  0x02
#define BTN_LEFT   0x03
#define BTN_SELECT 0x04
#define BTN_RETURN 0x05

// Define button states
typedef enum {
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_HELD,
} ButtonState;

// Button structure
typedef struct {
	uint8_t pin;
	ButtonState state;
	uint8_t buttonWasDown;
	uint8_t debounceCounter;
} Button;

uint8_t tila = 0;
uint8_t display = 0;
uint8_t menu = 0;
uint16_t menuTimeCnt = 0;
uint8_t redrawCnt = 0;
bool refresh = true;
	
// Array to hold button configurations
Button buttons[NUM_BUTTONS] = {
	{PC0, BUTTON_UP, 0},
	{PC1, BUTTON_UP, 0},
	{PC2, BUTTON_UP, 0},
	{PC3, BUTTON_UP, 0},
	{PC4, BUTTON_UP, 0},
	{PC5, BUTTON_UP, 0}
};

// Initialize buttons
void init_buttons() {
	// Set button pins as inputs and enable pull-up resistors
	DDRC &= ~(1 << PC0 | 1 << PC1 | 1 << PC2 | 1 << PC3 | 1 << PC4 | 1 << PC5);
	PORTC |= (1 << PC0 | 1 << PC1 | 1 << PC2 | 1 << PC3 | 1 << PC4 | 1 << PC5);
}

// Function to read buttons with debouncing
void read_buttons() {
	for (int i = 0; i < NUM_BUTTONS; i++) {
		uint8_t pin_state = PINC & (1 << buttons[i].pin);

		if (pin_state == 0) {  // Button pressed (active low)
			if (buttons[i].debounceCounter < DEBOUNCE_COUNT) {
				buttons[i].debounceCounter++;
				} else {
				if (buttons[i].state == BUTTON_UP) {
					buttons[i].state = BUTTON_DOWN;  // Button was just pressed
					} else {
					buttons[i].state = BUTTON_HELD;  // Button is being held
				}
			}
			} else {  // Button released
			if (buttons[i].debounceCounter > 0) {
				buttons[i].debounceCounter--;
				} else {
				if (buttons[i].state == BUTTON_DOWN || buttons[i].state == BUTTON_HELD) {
					buttons[i].state = BUTTON_UP;  // Button was just released
				}
			}
		}
	}
}

void MenuButtons() {
	menuTimeCnt=0;
	if (buttons[BTN_RIGHT].state == BUTTON_DOWN) {
		buttons[BTN_RIGHT].buttonWasDown = true;
	}
	else if (buttons[BTN_RIGHT].state == BUTTON_HELD) {
	}
	else if (buttons[BTN_RIGHT].state == BUTTON_UP) {
		if(buttons[BTN_RIGHT].buttonWasDown){
			display = MENU;
			if((menu < 0x05)){menu++;}else{menu=0x00;}
			refresh = true;
			buttons[BTN_RIGHT].buttonWasDown = false;
		}
	}
		
	if (buttons[BTN_LEFT].state == BUTTON_DOWN) {
		buttons[BTN_LEFT].buttonWasDown = true;
	}
	else if (buttons[BTN_LEFT].state == BUTTON_HELD) {
	}
	else if (buttons[BTN_LEFT].state == BUTTON_UP) {
		if(buttons[BTN_LEFT].buttonWasDown){
			display = MENU;
			if((menu > 0)){menu--;}else{menu=0x05;}
			refresh = true;
			buttons[BTN_LEFT].buttonWasDown = false;
		}
	}
	
	
	if (buttons[BTN_RETURN].state == BUTTON_DOWN) {
		buttons[BTN_RETURN].buttonWasDown = true;
	}
	else if (buttons[BTN_RETURN].state == BUTTON_HELD) {
	}
	else if (buttons[BTN_RETURN].state == BUTTON_UP) {
		if(buttons[BTN_RETURN].buttonWasDown){
			display = MONITORI;
			menu=0x00;
			refresh = true;
			buttons[BTN_RETURN].buttonWasDown = false;
		}
	}
}

void MAX4820_Write(uint8_t data) {
    // Select MAX4820 by setting CS (PD1) low
	PORTB |= (1 << MOSI);
    PORTD &= ~(1 << MAX4820_CS);
	_delay_us(1);


    // SPI manual slow
	for(uint8_t i=0;i<8;i++){
		if(data & (1 << i))PORTB |= (1 << MOSI);else PORTB &= ~(1 << MOSI);
		PORTB |= (1 << SCK);
		_delay_us(1);
		PORTB &= ~(1 << SCK);
		_delay_us(1);
	}
	

	_delay_us(1);
    // Deselect MAX4820 by setting CS (PD1) high
    PORTD |= (1 << MAX4820_CS);
	PORTB &= ~(1 << MOSI);
}



void RGB_Init() {
	I2C_Start();
	I2C_Write(LCD_RGB_ADDRESS << 1);  // Send the address with write flag
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Write(0x00);  // Mode1 register
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Write(0x00);  // Normal operation
	I2C_Stop();
	_delay_us(10);

	I2C_Start();
	I2C_Write(LCD_RGB_ADDRESS << 1);
	_delay_us(10); 
	I2C_Write(0x01);  // Mode2 register
	_delay_us(10); 
	I2C_Write(0x20);  // Outdriver mode
	_delay_us(10); 
	I2C_Stop();
    _delay_us(10);
	
	I2C_Start();
	I2C_Write(LCD_RGB_ADDRESS << 1);
	_delay_us(10); 
	I2C_Write(REG_MODE1);  
	_delay_us(10); 
	I2C_Write(0);  
	_delay_us(10); 
	I2C_Stop();
	_delay_us(10);	
	
	I2C_Start();
	I2C_Write(LCD_RGB_ADDRESS << 1);
	_delay_us(10); 
	I2C_Write(REG_OUTPUT);  
	_delay_us(10); 
	I2C_Write(0xFF);  
	_delay_us(10); 
	I2C_Stop();
	_delay_us(10);
	
	I2C_Start();
	I2C_Write(LCD_RGB_ADDRESS << 1);
	_delay_us(10); 
	I2C_Write(REG_MODE2);  
	_delay_us(10); 
	I2C_Write(0x20);  
	_delay_us(10); 
	I2C_Stop();
	_delay_us(10);
	
	I2C_Start();
	I2C_Write(LCD_RGB_ADDRESS << 1);
	_delay_us(10);
	I2C_Write(REG_RED);
	_delay_us(10);
	I2C_Write(0xFF);
	_delay_us(10);
	I2C_Stop();
	_delay_us(10);
	
	I2C_Start();
	I2C_Write(LCD_RGB_ADDRESS << 1);
	_delay_us(10);
	I2C_Write(REG_GREEN);
	_delay_us(10);
	I2C_Write(0xFF);
	_delay_us(10);
	I2C_Stop();
	_delay_us(10);
	
	I2C_Start();
	I2C_Write(LCD_RGB_ADDRESS << 1);
	_delay_us(10);
	I2C_Write(REG_BLUE);
	_delay_us(10);
	I2C_Write(0xFF);
	_delay_us(10);
	I2C_Stop();
	_delay_us(10);
}

void RGB_SetColor(uint8_t red, uint8_t green, uint8_t blue) {
	I2C_Start();
	I2C_Write(LCD_RGB_ADDRESS << 1);
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Write(0x04);  // Red LED register
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Write(red);   // Red intensity
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Stop();
	_delay_us(10);

	I2C_Start();
	I2C_Write(LCD_RGB_ADDRESS << 1);
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Write(0x03);  // Green LED register
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Write(green); // Green intensity
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Stop();
	_delay_us(10);

	I2C_Start();
	I2C_Write(LCD_RGB_ADDRESS << 1);
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Write(0x02);  // Blue LED register
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Write(blue);  // Blue intensity
	I2C_Stop();
	_delay_us(10);
}


void LCD_Send(uint8_t data, uint8_t mode) {
	// Send high nibble
	I2C_Write(LCD_ADDR << 1);
    _delay_us(10);  // Enable pulse width >450ns
	I2C_Write(data | mode );  // Set Enable High
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Write(data | mode );           // Set Enable Low
	_delay_us(50);  // Command delay >37us

	// Send low nibble
	I2C_Write(LCD_ADDR << 1);
	I2C_Write((data << 4) | mode );  // Set Enable High
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Write((data << 4) | mode );           // Set Enable Low
	_delay_us(50);  // Command delay >37us
}

void LCD_COMMAND(bool RS, uint8_t command) {
	I2C_Start();
	I2C_Write(LCD_ADDR << 1); // Send the address with write bit
	_delay_us(10);  // Enable pulse width >450ns
	if(RS)
		I2C_Write(0b10000000); // Control byte: 0x00 for command mode
	else
		I2C_Write(0x40); // Control byte: 0x00 for command mode
	_delay_us(10);  // Enable pulse width >450ns
	I2C_Write(command);
	I2C_Stop();
	_delay_us(50);
}

void LCD_Init() {
	// LCD initialization sequence
	_delay_ms(3000); // Wait for the display to power up

	
	LCD_COMMAND(true ,LCD_DISPLAYCONTROL | LCD_DISPLAYOFF | LCD_CURSOROFF | LCD_BLINKOFF );
	_delay_ms(5);
	LCD_COMMAND(true ,LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_2LINE | LCD_5x8DOTS);
	_delay_ms(5);
	LCD_COMMAND(true ,LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_2LINE | LCD_5x8DOTS);
	_delay_ms(5);
	LCD_COMMAND(true ,LCD_FUNCTIONSET | LCD_4BITMODE | LCD_2LINE | LCD_2LINE | LCD_5x8DOTS);
	_delay_ms(5);
	LCD_COMMAND(true ,LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF );
	_delay_ms(5);
	LCD_COMMAND(true ,LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF );
	_delay_ms(5);
	LCD_COMMAND(true ,LCD_CLEARDISPLAY);
	_delay_ms(5);
	LCD_COMMAND(true , LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);
	_delay_ms(5);
	LCD_COMMAND(true , 0x0);
	_delay_ms(5);

}

void LCD_PrintChar(char character) {
	LCD_COMMAND(false , character);
	_delay_ms(5);
}

void LCD_PrintString(char* str) {
	while (*str) {
		LCD_PrintChar(*str++);
	}
}

void TrimOrPadString(char *str) {
	int length = strlen(str);

	// Remove any null characters within the first 16 characters
	for (int i = 0; i < 16; i++) {
		if (str[i] == '\0') {
			str[i] = ' ';  // Replace null character with a space
		}
	}

	if (length > 16) {
		// Truncate if the string is longer than 16 characters
		str[16] = '\0';
		} else if (length < 16) {
		// Pad with spaces if the string is shorter than 16 characters
		for (int i = length; i < 16; i++) {
			str[i] = ' ';
		}
		str[16] = '\0'; // Ensure null termination
	}
}


void usart_print(char *_txt, ...)
{
  char buffer[32] = "";
  va_list args;
  va_start(args, _txt);
  vsprintf(buffer, _txt, args);
  va_end(args);
  
  TrimOrPadString(buffer);
  
  for(uint8_t i=0;i < 32;i++){
	if(buffer[i]==NULL)break;
	USART_0_write(buffer[i]);
	
  }
}

void SetTemperatureColor(float temperature) {
	uint8_t red = 0, green = 0, blue = 0;

	if (temperature <= 50.0) {
		// **50°C and below**: Blue to Green
		// Blue (0, 0, 255) to Green (0, 255, 0)
		float t = temperature / 50.0;  // Normalize temperature to range 0.0 - 1.0
		green = (uint8_t)(t * 255);    // Increase green as temperature rises
		blue = 255 - green;            // Decrease blue proportionally
	}
	else if (temperature > 50.0 && temperature <= 65.0) {
		// **50°C to 65°C**: Green to Yellow
		// Green (0, 255, 0) to Yellow (255, 255, 0)
		float t = (temperature - 50.0) / 15.0;  // Normalize temperature to range 0.0 - 1.0
		red = (uint8_t)(t * 255);               // Increase red as temperature rises
		green = 255;                            // Green stays full
		blue = 0;                               // Blue remains off
	}
	else if (temperature > 65.0 && temperature <= 72.0) {
		// **65°C to 72°C**: Yellow to Red
		// Yellow (255, 255, 0) to Red (255, 0, 0)
		float t = (temperature - 65.0) / 7.0;   // Normalize temperature to range 0.0 - 1.0
		red = 255;                              // Red stays full
		green = (uint8_t)(255 * (1.0 - t));     // Decrease green as temperature rises
		blue = 0;                               // Blue remains off
	}
	else {
		// **Above 72°C**: Full Red
		red = 255;
		green = 0;
		blue = 0;
	}

	// Set the RGB backlight color based on calculated values
	RGB_SetColor(red, green, blue);
	
}



int main(void)
{
	/* Initializes MCU, drivers and middleware */
	uint8_t MAXO = 0x00;
	atmel_start_init();
	init_buttons();
	I2C_Init();
	LCD_Init();
	RGB_Init();
	
	// Set PD1 as output for Chip Select
	DDRD |= (1 << MAX4820_CS);
	PORTD |= (1 << MAX4820_CS); // Deselect MAX4820 initially
	
	bool toggle = true;
	//static uint8_t rx[16];
	//char test[17];
    char buffer[17] = "";
	uint8_t scorr1 = 0;
	uint8_t scorr2 = 0;
	float v_ref = V_REF;
	adc_result_t s1 = 0;
    adc_result_t s2 = 0;
	float t1, t2 = 0;

	/* Replace with your application code */
	while (1) {
		s1 = ADC_0_get_conversion(6) - scorr1;
		t1= (s1 / (float)ADC_MAX) * v_ref;
		t1= R_SERIES * (t1 / (v_ref - t1));
		t1= (t1 - R0) / ALPHA;
		
		s2 = ADC_0_get_conversion(7) - scorr2;
		t2= (s2 / (float)ADC_MAX) * v_ref;
		t2= R_SERIES * (t2 / (v_ref - t2));
		t2= (t2 - R0) / ALPHA;
		

		
		// Handling backlight
		switch (tila) {
			case VIKA:
				if(toggle){
						RGB_SetColor(0xFF,  0x00, 0x00);
					}else{
						RGB_SetColor(0xFF,  0xFF, 0x00);
					}
				toggle = ~toggle;
				break;

			default:
				RGB_SetColor(0xFC, 0xFC, 0xFC);
		}
		
		// Handling Pumps
		switch (tila) {
			case LEPO:
			MAXO &= ~(1 << PUMP1);
			MAXO &= ~(1 << PUMP2);
			MAXO &= ~(1 << BUZZ1);
			break;
			case LATAUS:			
			MAXO |= (1 << PUMP1);
			MAXO &= ~(1 << PUMP2);
			MAXO &= ~(1 << BUZZ1);
			break;
			case PALUU:
			MAXO &= ~(1 << PUMP1);
			MAXO |= (1 << PUMP2);
			MAXO &= ~(1 << BUZZ1);
			break;
			case VIKA:;
			MAXO &= ~(1 << PUMP1);
			MAXO &= ~(1 << PUMP2);
			MAXO |= (1 << BUZZ1);
			break;

			default:;
			// code block
		}
		MAX4820_Write(MAXO);
		
		// Rendering screen and menus
		if(refresh)
			{LCD_COMMAND(true ,LCD_CLEARDISPLAY); refresh = false; _delay_us(2000);} // clear when menu changes
		

		read_buttons();	
		switch (display) {
			case MONITORI:
				if (buttons[BTN_UP].state == BUTTON_DOWN) {
					buttons[BTN_UP].buttonWasDown = true;
				}
				else if (buttons[BTN_UP].state == BUTTON_HELD) {
				}
				else if (buttons[BTN_UP].state == BUTTON_UP) {
					if(buttons[BTN_UP].buttonWasDown){
						display = MENU;
						refresh = true;
						buttons[BTN_UP].buttonWasDown = false;
					}
				}

                if(redrawCnt>=10){
				LCD_COMMAND(true , 0x80 | 0x00 );
				snprintf(buffer, 17, "t1 %.1f\xDF""C           ", t1);
				LCD_PrintString(buffer);
				
				LCD_COMMAND(true , 0x80 | 0x40 );
				snprintf(buffer, 17, "t2 %.1f\xDF""C           ", t2);
				//snprintf(buffer, 17, "%i%i%i%i%i%i ", buttons[0].state, buttons[1].state, buttons[2].state, buttons[3].state, buttons[4].state, buttons[5].state);
				LCD_PrintString(buffer);
				redrawCnt=0;
				}
				redrawCnt++;
				break;
				
			case MENU:
			    if(menuTimeCnt==250){menuTimeCnt=0; menu=LATAUSRAJA; display=MONITORI;}else{menuTimeCnt++;}
				switch(menu) {
					case LATAUSRAJA:

						MenuButtons();
						LCD_COMMAND(true , 0x80 | 0x00 );
						snprintf(buffer, 17, "Latauspumppu");
						LCD_PrintString(buffer);
						
						LCD_COMMAND(true , 0x80 | 0x40 );
						snprintf(buffer, 17, "t1 > 70\xDF""C");
						LCD_PrintString(buffer);

						break;
					case PALUURAJA:
						MenuButtons();
						LCD_COMMAND(true , 0x80 | 0x00 );
						snprintf(buffer, 17, "Paluulatauspumppu");
						LCD_PrintString(buffer);
						
						LCD_COMMAND(true , 0x80 | 0x40 );
						snprintf(buffer, 17, " t1 < t2 ");
						LCD_PrintString(buffer);
						break;
					case TURVARAJA:
						MenuButtons();
						LCD_COMMAND(true , 0x80 | 0x00 );
						snprintf(buffer, 17, "Pumppu seis, jos");
						LCD_PrintString(buffer);
						
						LCD_COMMAND(true , 0x80 | 0x40 );
						snprintf(buffer, 17, " t1 >= 90\xDF""C");
						LCD_PrintString(buffer);
						break;
					case HYSTEREESI:
						MenuButtons();
						LCD_COMMAND(true , 0x80 | 0x00 );
						snprintf(buffer, 17, "Hystereesi");
						LCD_PrintString(buffer);
						
						LCD_COMMAND(true , 0x80 | 0x40 );
						snprintf(buffer, 17, " 2\xDF""C");
						LCD_PrintString(buffer);
						break;
					case AVCC:
						MenuButtons();
						LCD_COMMAND(true , 0x80 | 0x00 );
						snprintf(buffer, 17, "AVCC");
						LCD_PrintString(buffer);
						
						LCD_COMMAND(true , 0x80 | 0x40 );
						snprintf(buffer, 17, " 5.0V");
						LCD_PrintString(buffer);
					break;
					case KORJAUS:
						MenuButtons();
						LCD_COMMAND(true , 0x80 | 0x00 );
						snprintf(buffer, 17, "Lampotilakorjaus");
						LCD_PrintString(buffer);
						
						LCD_COMMAND(true , 0x80 | 0x40 );
						snprintf(buffer, 17, " 0.0\xDF""C");
						LCD_PrintString(buffer);
						break;
					default:break;
				}
			default:;
			// code block
		}
		
		
		

		

		
	    /*/ENABLE_INTERRUPTS();
        USART_0_enable_tx();
		if(USART_0_is_tx_ready()){
			usart_print("Varaajamonitori lämpötilat: \r\0");
			usart_print("S1: %f S2: %f \r\0", t1, t2);
			usart_print("S1: %i S2: %i \r\0", s1, s2);
		}
		//DISABLE_INTERRUPTS();*/
		
		_delay_ms(50);
	}
}
