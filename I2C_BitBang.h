/*
 * I2C_BitBang.h
 *
 * Created: 1/4/2023 10:13:05 AM
 *  Author: aparady
 */ 


#ifndef I2C_BITBANG_H_
#define I2C_BITBANG_H_

#define I2C_delay asm("nop") // Adjust this based on clock speed to change SCL frequency

// Define what pin is SDA and what pin is SCL
#define SDA_HIGH (PORTE |= (1<<PORTE0))
#define SCL_HIGH (PORTE |= (1<<PORTE1))

#define SDA_LOW (PORTE &= ~(1<<PORTE0))
#define SCL_LOW (PORTE &= ~(1<<PORTE1))

#define SDA_READ_ON (DDRE &= ~(1<<DDE0))
#define SDA_READ_OFF (DDRE |= (1<<DDE0))

#define SCL_READ_ON (DDRE &= ~(1<<DDE1))
#define SCL_READ_OFF (DDRE |= (1<<DDE1))

uint8_t read_data = 0; // Returned data from slave device

uint8_t bit_convert[8] = {1, 2, 4, 8, 16, 32, 64, 128}; // Conversion factors for binary value without shifting to allow for a constant conversion speed

void I2C_start(){
	// Start from known state
	SDA_HIGH;
	I2C_delay;
	SCL_HIGH;
	I2C_delay;
	
	// Follow standard I2C start condition
	SDA_LOW;
	I2C_delay;
	SCL_LOW;
	I2C_delay;
}

void I2C_stop(){
	// Follow standard I2C stop condition
	SDA_LOW;
	I2C_delay;
	SCL_HIGH;
	I2C_delay;
	SDA_HIGH;
	I2C_delay;
}

void I2C_sendACK(){
	// Send ACK to tell slave to keep transmitting
	SDA_READ_OFF;
	I2C_delay;
	SDA_LOW;
	I2C_delay;
	SCL_HIGH;
	I2C_delay;
	SCL_LOW;
	I2C_delay;
	SDA_LOW;
	I2C_delay;
}

void I2C_sendNACK(){
	// Send ACK to tell slave to stop transmitting
	SDA_READ_OFF;
	I2C_delay;
	SDA_HIGH;
	I2C_delay;
	SCL_HIGH;
	I2C_delay;
	SCL_LOW;
	I2C_delay;
	SDA_LOW;
	I2C_delay;
}

uint8_t I2C_write(uint8_t Data){
	// Go through all eight bits and force SDA to be high or low depending on if bit is set
	for(int bits = 7; bits >= 0; bits--){
		(Data & 0x80) ? SDA_HIGH : SDA_LOW; // If data in last bit is set/1, then force SDA high. Otherwise leave SDA low and clock it out.
		Data <<= 1; // Shift data left by one bit
		I2C_delay;
		
		// Clock data out
		SCL_HIGH;
		I2C_delay;
		SCL_LOW;
		I2C_delay;
	}
	// Check for ACK or NACK
	SDA_LOW;
	I2C_delay;
	SDA_READ_ON;
	I2C_delay;
	SCL_HIGH;
	I2C_delay;
	uint8_t ACK = ((PINE & (1 << PORTE0)) >> PORTE0); // Check if ACK was received by converting pin read to binary, 0 = ACK, 1 = NACK
	SDA_READ_OFF;
	I2C_delay;
	SCL_LOW;
	I2C_delay;
	return ACK;
}

uint8_t I2C_read(uint8_t ack){
	// Prepare transmission lines for slave data
	read_data = 0;
	//SDA_HIGH;
	//I2C_delay;
	SDA_READ_ON;
	I2C_delay;
	for(int bits = 7; bits >= 0; bits--){
		// Clock bits from slave device
		SCL_HIGH;
		I2C_delay;
		read_data = read_data + ((((PINE & (1 << PORTE0)) >> PORTE0)) * bit_convert[bits]); // Convert pin read to binary and multiply it by number of bits to create a full hex byte
		SCL_LOW;
		I2C_delay;
	}
	
	if (ack == 1){
		I2C_sendACK(); // Send ACK if more data needs to be transferred. Otherwise send NACK to end transmission
		}else{
		I2C_sendNACK();
	}
	read_data = (((read_data / 16) * 10) + (read_data % 16)); // Convert received data from hex to decimal
	return read_data;
}


#endif /* I2C_BITBANG_H_ */