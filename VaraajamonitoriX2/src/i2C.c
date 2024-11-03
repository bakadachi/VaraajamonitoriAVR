#include <avr/io.h>
#include <util/delay.h>
#include "i2c.h"

// Pin definitions
#define SDA PD4   // Define SDA pin (change to match your wiring)
#define SCL PD3   // Define SCL pin (change to match your wiring)

// Macros for easier pin control
#define SDA_HIGH() PORTD |= (1 << SDA)
#define SDA_LOW()  PORTD &= ~(1 << SDA)
#define SCL_HIGH() PORTD |= (1 << SCL)
#define SCL_LOW()  PORTD &= ~(1 << SCL)
#define SDA_READ() (PIND & (1 << SDA))  // Read the SDA pin state

// Set SDA pin as input or output
#define SDA_OUTPUT() DDRD |= (1 << SDA)
#define SDA_INPUT()  DDRD &= ~(1 << SDA)

void I2C_Init() {
	// Set SDA and SCL as outputs and set them high (idle state)
    // Configure SDA and SCL (PD3 and PD4) as outputs
    DDRD |= (1 << SDA) | (1 << SCL);

    // Set SDA and SCL high initially (idle state)
    SDA_HIGH();
    SCL_HIGH();
    
    // Enable pull-up resistors if needed
    PORTD |= (1 << SDA) | (1 << SCL);
}

void I2C_Start() {
	SDA_OUTPUT();  // Ensure SDA is set as output
	SDA_HIGH();
	SCL_HIGH();
	_delay_us(10);

	SDA_LOW();     // Pull SDA low while SCL is high to signal START
	_delay_us(10);

	SCL_LOW();     // Pull SCL low to prepare for data transmission
}

void I2C_Stop() {
	SDA_OUTPUT();  // Ensure SDA is set as output
	SCL_LOW();     // Pull SCL low first

	SDA_LOW();     // Pull SDA low
	_delay_us(10);

	SCL_HIGH();    // Pull SCL high
	_delay_us(10);

	SDA_HIGH();    // Pull SDA high while SCL is high to signal STOP
}

uint8_t I2C_Write(uint8_t data) {
	for (uint8_t i = 0; i < 8; i++) {
		if (data & 0x80) {
			SDA_HIGH();
			} else {
			SDA_LOW();
		}
		data <<= 1;

		SCL_HIGH();  // Pulse the clock to signal the data bit
		_delay_us(10);
		SCL_LOW();
		_delay_us(10);
	}

	// Check for acknowledgment
	SDA_INPUT(); // Release SDA to be an input (pull-up)
	SCL_HIGH();
	_delay_us(10);
	uint8_t ack = SDA_READ() == 0;  // ACK is 0
	SCL_LOW();
	SDA_OUTPUT();

	return ack;
}


uint8_t I2C_Read(uint8_t ack) {
	uint8_t data = 0;

	SDA_INPUT();  // Set SDA as input to read data

	for (uint8_t i = 0; i < 8; i++) {
		SCL_HIGH();
		_delay_us(10);

		data <<= 1;
		if (SDA_READ()) {
			data |= 0x01;
		}

		SCL_LOW();
		_delay_us(10);
	}

	// Send ACK or NACK
	SDA_OUTPUT();
	if (ack) {
		SDA_LOW();   // ACK
		} else {
		SDA_HIGH();  // NACK
	}

	SCL_HIGH();
	_delay_us(10);
	SCL_LOW();
	_delay_us(10);

	SDA_INPUT();  // Release SDA after acknowledgment

	return data;
}
