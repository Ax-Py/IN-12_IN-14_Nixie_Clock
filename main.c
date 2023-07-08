/*
 * File:   main.c
 * Author: aparady
 *
 * Created on February 9, 2023, 10:23 AM
 */


#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "I2C_BitBang.h"
//#include "UART.h"

FUSES = {
	.low = 0x62, // LOW {SUT_CKSEL=INTRCOSC_8MHZ_6CK_14CK_65MS, CKOUT=CLEAR, CKDIV8=SET}
	.high = 0xD9, // HIGH {BOOTRST=CLEAR, BOOTSZ=2048W_3800, EESAVE=CLEAR, WDTON=CLEAR, SPIEN=SET, JTAGEN=CLEAR, OCDEN=CLEAR}
	.extended = 0xF7, // EXTENDED {BODLEVEL=DISABLED, CFD=CFD_DISABLED}
};

LOCKBITS = 0xFF; // {LB=NO_LOCK, BLB0=NO_LOCK, BLB1=NO_LOCK}

// Define array that contains the output pin configuration in decimal form for each tube (T1,T2,T3,T4) row then column (UPDATED 2/27/2023)
uint8_t digit_array[13][4] = {{6,96,6,6},       // 0 G
							  {34,68,34,34},    // 1 G
							  {130,65,130,130},	// 2 G
							  {192,3,192,192},	// 3 G
                              {96,6,96,96},	    // 4 G
                              {80,10,80,80},	// 5 G
                              {72,18,72,72},	// 6 G
                              {68,34,68,68},	// 7 G
                              {65,130,65,65},	// 8 G
                              {3,192,3,3},	    // 9 G
                              {18,72,18,18},    // C1 G
							  {10,80,10,10},	// C2 G
                              {0,0,0,0}	        // Reserved for blanking
							 };

int8_t RTC_hour = 0; // Register that stores the raw value of the hours register of the RTC
int8_t RTC_minute = 0; // Register that stores the raw value of the minutes register of the RTC
uint8_t RTC_second = 0; // Register that stores the raw value of the seconds register of the RTC

uint16_t button_counter = 0; // Button de-bounce counter/timer
uint8_t button_state = 0; // Prevents counter from rolling over and triggering a command more than once
uint8_t menu_state = 0; // Store current menu state to access functions such as changing time and resetting seconds to zero
uint8_t mil_flag = 0; // 12/24 hour selection flag, defaults to 12 hour time
uint8_t read_flag = 0; // RTC sample flag. 1 = sample taken

// Convert decimal values to hex values that the RTC will accept e.g. 10d = 16d/10h, 23d = 35d/23h
uint8_t Dec2Hex(uint8_t value){
	return ((value / 10) * 0x10) + (value % 10);
}

// Anti cathode poisoning routine
void anti_poison(){
	for (int poison_digit = 0; poison_digit < 12; poison_digit++){
		PORTD = digit_array[poison_digit][0]; // Tube 1
		PORTC = digit_array[poison_digit][1]; // Tube 2
		PORTA = digit_array[poison_digit][2]; // Tube 3
		PORTB = digit_array[poison_digit][3]; // Tube 4
		_delay_ms(1200); // Dwell on digit for 0.5 seconds (6 seconds total) to ensure that the digit has reached full brightness
	}
}

// Forces a reading from RTC registers
void RTC_read(){
	I2C_start();
	I2C_write(0x68 << 1);
	I2C_write(0x00);
	
	I2C_start();
	I2C_write((0x68 << 1) + 1);
	
	RTC_second = I2C_read(1);
	RTC_minute = I2C_read(1);
	RTC_hour = I2C_read(0);
	
	I2C_stop();
    
    switch(menu_state){
        case 0: // Display all digits and only toggle mil_flag
            if (mil_flag == 0){
                // Check if PM indicator should be set or not
                if (RTC_hour >= 12){
                    PORTE |= (1<<PORTE3); // Set PM indicator
                }else{
                    PORTE &= ~(1<<PORTE3); // Clear PM indicator
                }
                
                // If the time is greater than 12, then subtract 12 to change 24hr time to 12hr time
                if (RTC_hour > 12){
                    PORTD = digit_array[(RTC_hour - 12) / 10][0]; // Tube 1
                    PORTC = digit_array[(RTC_hour - 12) % 10][1]; // Tube 2
                }else{
                   PORTD = digit_array[(RTC_hour) / 10][0]; // Tube 1
                   PORTC = digit_array[(RTC_hour) % 10][1]; // Tube 2 
                }
                
                // If time is 12AM then display 12 rather than 0 for 12hr time
                if (RTC_hour == 0){
                    PORTE &= ~(1<<PORTE3); // Clear PM indicator
                    PORTD = digit_array[1][0]; // Tube 1
                    PORTC = digit_array[2][1]; // Tube 2
                }
                
            // Display 24hr time and make sure that the PM indicator is off
            }else{
                PORTE &= ~(1<<PORTE3); // Clear PM indicator
                PORTD = digit_array[RTC_hour / 10][0]; // Tube 1
                PORTC = digit_array[RTC_hour % 10][1]; // Tube 2
            }
            
            // Display minutes
            PORTA = digit_array[RTC_minute / 10][2]; // Tube 3
            PORTB = digit_array[RTC_minute % 10][3]; // Tube 4
            break;
            
        case 64: // Display hours only
            if (mil_flag == 0){
                // Check if PM indicator should be set or not
                if (RTC_hour >= 12){
                    PORTE |= (1<<PORTE3); // Set PM indicator
                }else{
                    PORTE &= ~(1<<PORTE3); // Clear PM indicator
                }
                
                // If the time is greater than 12, then subtract 12 to change 24hr time to 12hr time
                if (RTC_hour > 12){
                    PORTD = digit_array[(RTC_hour - 12) / 10][0]; // Tube 1
                    PORTC = digit_array[(RTC_hour - 12) % 10][1]; // Tube 2
                }else{
                   PORTD = digit_array[(RTC_hour) / 10][0]; // Tube 1
                   PORTC = digit_array[(RTC_hour) % 10][1]; // Tube 2 
                }
                
                // If time is 12AM then display 12 rather than 0 for 12hr time
                if (RTC_hour == 0){
                    PORTE &= ~(1<<PORTE3); // Clear PM indicator
                    PORTD = digit_array[1][0]; // Tube 1
                    PORTC = digit_array[2][1]; // Tube 2
                }
                
            // Display 24hr time and make sure that the PM indicator is off
            }else{
                PORTE &= ~(1<<PORTE3); // Clear PM indicator
                PORTD = digit_array[RTC_hour / 10][0]; // Tube 1
                PORTC = digit_array[RTC_hour % 10][1]; // Tube 2
            }
            
            // Blank minutes
            PORTA = digit_array[12][2]; // Tube 3
            PORTB = digit_array[12][3]; // Tube 4
            break;
            
        case 128: // Display minutes only
            PORTD = digit_array[12][0]; // Tube 1
            PORTC = digit_array[12][1]; // Tube 2
            PORTA = digit_array[RTC_minute / 10][2]; // Tube 3
            PORTB = digit_array[RTC_minute % 10][3]; // Tube 4
            break;
            
        case 192: // Display seconds only
            PORTD = digit_array[12][0]; // Tube 1
            PORTC = digit_array[12][1]; // Tube 2
            PORTA = digit_array[RTC_second / 10][2]; // Tube 3
            PORTB = digit_array[RTC_second % 10][3]; // Tube 4
            break;
            
        default: // do nothing
            break;
    }
}

void RTC_set(uint8_t address, uint8_t operator){
    I2C_start();
	I2C_write(0x68 << 1);
	I2C_write(address);
    
    switch(address){
		case 0x00: // Adjust seconds (ADDR = 0x00)
            I2C_write(Dec2Hex(0));
            break;
            
		case 0x01: // Adjust minutes (ADDR = 0x01)
            if(operator == 1){
                if(RTC_minute < 59){
                    RTC_minute++;
                    I2C_write(Dec2Hex(RTC_minute));
                }else{
                    RTC_minute = 0;
                    I2C_write(Dec2Hex(RTC_minute));
                }
                
            }else if(operator == 0){
                if(RTC_minute > 0){
                    RTC_minute--;
                    I2C_write(Dec2Hex(RTC_minute));
                }else{
                    RTC_minute = 59;
                    I2C_write(Dec2Hex(RTC_minute));
                }
            }
            break;
		
		case 0x02: // Adjust hours (ADDR = 0x02)
            if(operator == 1){
                if(RTC_hour < 23){
                    RTC_hour++;
                    I2C_write(Dec2Hex(RTC_hour));
                }else{
                    RTC_hour = 0;
                    I2C_write(Dec2Hex(RTC_hour));
                }
                
            }else if(operator == 0){
                if(RTC_hour > 0){
                    RTC_hour--;
                    I2C_write(Dec2Hex(RTC_hour));
                }else{
                    RTC_hour = 23;
                    I2C_write(Dec2Hex(RTC_hour));
                }
            }
            break;
		
		default:
            break;
	}
	I2C_stop();
}

void read_inputs(){
	if(!(PINE & 0b1000000) || !(PINE & 0b0000100)){ // Check if PE6 or PE2 is low
		button_counter++; // Increment de-bounce counter
		
		if(button_counter == 2500 && button_state == 0){ // Check if de-bounce counter reached settling time and only act if a prior button press has not occurred
			
			switch(PINE & 0b1000100){ // Choose outcome based on the value of the PIN register at the time of press
				
				case 0: // Both pressed
                    menu_state += 64;
                    RTC_read();
				break;
				
				case 4: // PE6 pressed
                    switch(menu_state){
                        case 0:
                            mil_flag = !mil_flag;
                            RTC_read();
                            break;
                        case 64:
                            RTC_set(0x02, 1);
                            RTC_read();
                            break;
                        case 128:
                            RTC_set(0x01, 1);
                            RTC_read();
                            break;
                        case 192:
                            RTC_set(0x00, 1);
                            RTC_read();
                            break;
                    }
				break;
				
				case 64: // PE2 pressed
                    switch(menu_state){
                        case 0:
                            mil_flag = !mil_flag;
                            RTC_read();
                            break;
                        case 64:
                            RTC_set(0x02, 0);
                            RTC_read();
                            break;
                        case 128:
                            RTC_set(0x01, 0);
                            RTC_read();
                            break;
                        case 192:
                            RTC_set(0x00, 0);
                            RTC_read();
                            break;
                    }
				break;
				
				default: // Handler for any possible errors
				break;
			}
			button_state = 1; // Prevent action from occurring again until button is released
			button_counter = 0; // Reset delay counter
		}
	}
	else{
		button_counter = 0; // Reset delay counter
		button_state = 0; // Reset button state to non active state
	}
}

int main(void) {
    DDRA = 0b11111111; // Set all of PORTA to outputs
    DDRB = 0b11111111; // Set all of PORTB to outputs
    DDRC = 0b11111111; // Set all of PORTC to outputs
    DDRD = 0b11111111; // Set all of PORTD to outputs
    DDRE = 0b0101011; // Set all of PORTE except PE0(SDA), PE1(SCL), PE3(PM), and PE5(ALM) to inputs
    
    PORTA = 0b00000000; // Set all of PORTA LOW
    PORTB = 0b00000000; // Set all of PORTB LOW
    PORTC = 0b00000000; // Set all of PORTC LOW
    PORTD = 0b00000000; // Set all of PORTD LOW
    PORTE = 0b1000111; // Set pull ups on PE0(SDA), PE1(SCL), PE2(B1), and PE6(B2), set rest of pins LOW
    
    _delay_ms(1);
    
    // Initialize RTC to known state
	I2C_start();
	I2C_write(0x68 << 1);
	I2C_write(0x00);
	I2C_write(Dec2Hex(0));
	I2C_write(Dec2Hex(0));
	I2C_write(Dec2Hex(0));
	I2C_stop();
	
	// Force 1Hz SQW on INT pin
	I2C_start();
	I2C_write(0x68 << 1);
	I2C_write(0x0E);
	I2C_write(0x40);
	I2C_stop();
    
    while (1) {
        // Read one sample from RTC when 1Hz SQW is high on PE4/AREF
		if(PINE & 0b0010000 && read_flag == 0){
			RTC_read();
            
			read_flag = 1;
		}
		// Reset read flag when 1Hz SQW is low on PE4/AREF
		else if (!(PINE & 0b0010000)){
			read_flag = 0;
            
            // If it is 2AM/PM, run the anti-poisoning routine to ensure consistent brightness among digits and prolong tube life
            if (((RTC_hour == 2 || RTC_hour == 14) && RTC_minute == 0) && menu_state == 0){
                // Turn off PM indicator
                PORTE &= ~(1<<PORTE3);
                // Begin wear-leveling routine
                anti_poison();
            }
		}
		
		read_inputs();
    }
}
