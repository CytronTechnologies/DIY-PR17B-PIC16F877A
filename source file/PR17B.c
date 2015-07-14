//=========================================================================
//	Author			: Cytron Technologies
//	Project			: Voice Recording and Playback: ISD1790
//	Project description     : Use ISD1790 chip to record and playback voice
//                                This project use PIC16F877A.Compatible with
//                                MPLAB IDE with HITECH C compiler
//                                using MPLABX with HITECH C compiler v 9.80/v9.83 and XC8 compiler.
//=========================================================================

//===========================================================================
//	include
//===========================================================================
#if defined(__XC8)
   #include <xc.h>
   #pragma config CONFIG = 0x3F32
#else
#include <htc.h>                        //include the PIC microchip header file

//===========================================================================
// 	configuration
//============================================================================
__CONFIG (0x3F32);
//FOSC = HS        // Oscillator Selection bits (HS oscillator)
//WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
//PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
//BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
//LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
//CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
//WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
//CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)



#endif

//	define
//==========================================================================
#define	LCD_DATA		PORTB
#define RS			RD6     //Pin RS at 2x16 LCD
#define E			RD7     //Pin E at 2x16 LCD
#define CS			RE1		
#define ENTER			RD4     //Push button Enter
#define SELECT	 		RD5     //Push button Select


//	function prototype		(every function must have a function prototype)
//==========================================================================
void Init(void);
void delay(unsigned long data);
void tmr_delay(unsigned short lenght);
void send_config(unsigned char data);
void send_char(unsigned char data);
void lcd_goto(unsigned char data);
void lcd_clr(void);
void send_string(const char *s);
void e_pulse(void);
void spi_initialize(void);
unsigned char uc_send_receive_spi(unsigned char uc_data);
void Spi(void);
void Stand(void);
void Play(void);
void Record(void);
void message1p(void);
void message2p(void);
void message1r(void);
void message2r(void);
void Play_s(void);
void Record_s(void);
void Forward(void);
void start(void);
void stop(void);
void Erase(void);
void g_erase(void);
void erase(void);
void Volume(void);
void msg_1(void);
void mic(void);
void analog(void);
void location(void);
void Option(void);

//	global variable
//==========================================================================

unsigned char status;
unsigned char msg;
unsigned char pointer;
const unsigned char SPI [] = {"1.SPI MODE"};
const unsigned char STD[] = {"2.STANDALONE"};
const unsigned char ERASE [] = {"3.ERASE"};
const unsigned char *mode [4] = {&SPI[0],&STD[0],&ERASE[0]};




//	main function(main function of the program)
//==========================================================================
void main(void)
{
	unsigned char j=0;

	Init();
	lcd_clr();	
	lcd_goto(0);					// clear the LCD screen
	send_string("Select mode");     // display "select mode"
	lcd_goto(20);                   // move to 2nd line
	send_string(mode[j]);           // display string according to the mode

	while(1)
	{
		if(	SELECT == 0)                       	// check if user press select button
		{
			while(SELECT == 0) continue;		// wait button to be released
			j++;
			if(j>2) j =0;
			lcd_goto(20);					// start display at 20
			send_string(mode[j]);			// display string depend on mode
			send_string("            ");	// space to overwrite long words
		}
		if(ENTER == 0)
		{
			while(ENTER == 0)continue;              // wait button to release
			switch(j)                               // check what is the current mode, execute the mode
			{

				case 1 :	Stand();        // mode 2 : standalone
							break;
				case 2 :	Erase();
							break;
				default : 	Spi();          // mode 1 : spi
							j=0;
			}
		}
	}
}

//	functions
//==========================================================================
void Init(void)
{
	TRISA = 0b00000000;         // configure Port A as output
	TRISB = 0b00000000;         // configure Port B as output
	TRISC = 0b00000000;         // configure Port C as output
	TRISD = 0b00110000;         // configure RD5 and RD4 as input, else are output
	TRISE = 0x00;               // configure Port E as output
	PORTA = 0;                  // clear Port A
	PORTD = 0;                  // clear Port D
	PORTE = 0;                  // clear Port E
	ADCON1 = 0b00000110;        //set all portA as digital I/O
	RD3 = 1;                    //initilaize the isd play, record, ft and fwd as 5V initially
	RA0 = 1;
	RC0 = 1;
	RE2 = 1;
	//configure lcd
	send_config(0b00000001);            //clear display at lcd
	send_config(0b00000010);            //lcd return to home
	send_config(0b00000110);            //entry mode-cursor increase 1
	send_config(0b00001100);            //display on, cursor off and cursor blink off
	send_config(0b00111000);            //function set


	//tmr 0 configuration
	T0CS = 0;			//set timer 0 clock source to internal instruction clock cycle
	PSA = 0;			//assign prescaler to timer 0 module
	PS2 = 1;                        //set timer 0 prescaler (bit 2)
	PS1 = 1;			//set timer 0 prescaler	(bit 1)
	PS0 = 1;			//set timer 0 prescaler	(bit 0)

	spi_initialize();		// initialize SPI mode

	//reset ISD1790
	CS = 0;                                 //start receiving spi command
	uc_send_receive_spi(0b10000000);        //turn on spi
	uc_send_receive_spi(0b00000000);        //stop receiving spi command
	CS = 1;	

	tmr_delay(10);                          //delay 100 milisecond

	CS = 0;							
	uc_send_receive_spi(0b11000000);        //reset device
	uc_send_receive_spi(0b00000000);
	CS = 1;		

	tmr_delay(10);                          //delay 100 milisecond
	
	CS = 0;							
	uc_send_receive_spi(0b11100000);        //turn off spi
	uc_send_receive_spi(0b00000000);
	CS = 1;
}


void spi_initialize(void)
{
	// Set the SDO as output. SDI is automatically controlled by the SPI module.
	TRISC5 = 0;
	
	// Input data sampled at middle of data output time.
	SMP = 0;
	
	// Select SPI mode 1, 1.
	CKE = 0;                    // Output data changes on transition from idle to active clock state.
	CKP = 1;                    // Idle state for clock is a high level.
	
	// SPI Master mode, clock = FOSC/4.
	SSPM3 = 0;
	SSPM2 = 0;
	SSPM1 = 0;
	SSPM0 = 0;
	
	// Clear the Write Collision Detect bit.
	WCOL = 0;
	
	// Enable the MSSP module.
	SSPEN = 1;
}



unsigned char uc_send_receive_spi(unsigned char uc_data)
{
	
	// Send the data
	SSPBUF = uc_data;
	
	// Wait for the SPI module to finish sending / receiving.
	while(BF == 0);
	
	// Return the received data.
	return SSPBUF;
}

void delay(unsigned long data)		//short delay		
{
	for( ;data>0;data-=1);          //delay lenght according to the given value
}

void tmr_delay(unsigned short lenght)   	//delay using timer 0
{											//delay lenght = given value X 10ms
	for( ;lenght>0;lenght-=1)				//loop
	{
		TMR0=0;								//clear timer 0 value
		while(TMR0<195);					//wait timer 0 value to reach 195 (10ms)
	}	
}

void send_config(unsigned char data)
{
	RS=0;				//clear rs into config mode
	LCD_DATA=data;
	delay(50);
	e_pulse();
}

void send_char(unsigned char data)
{
	RS=1;				//set rs into write mode
	LCD_DATA=data;
	delay(50);
	e_pulse();
}

void e_pulse(void)
{
	E=1;
	delay(50);
	E=0;
	delay(50);
}

void lcd_goto(unsigned char data)
{
 	if(data<16)
	{
	 	send_config(0x80+data);
	}
	else
	{
	 	data=data-20;
		send_config(0xc0+data);
	}
}

void lcd_clr(void)
{
 	send_config(0x01);
	delay(50);	
}

void send_string(const char *s)
{          
	//unsigned char i=0;
  	while (s && *s)send_char (*s++);

}

void Spi(void)
{	char i =1;
	CS = 0;							
	uc_send_receive_spi(0b10000000);	//turn on spi
	uc_send_receive_spi(0b00000000);
	CS = 1;	
	tmr_delay(10);
	RD3 = 1;
	lcd_clr();	
	lcd_goto(0);					// clear lcd
	send_string("Switch mode"); 	// display string
	lcd_goto(20);					// lcd goto 2nd line
	send_string("1. PLAY   ");	
	while(1)						// loop forever
	{
		if(SELECT == 0)                            	// check if user press select button
		{			
			while(SELECT == 0) continue;            // wait button to release
			i++;                                    // increment counter
			lcd_goto(20);                           // go to second line
			switch (i)								// check current value of i
			{
				case 2 :    send_string("2. RECORD");		// mode 2: record
							break;                  		// break out from switch
				
				default:  	send_string("1. PLAY   ");	// if not 2 , set it back to 1 and display "1. PLAY   "
							i =1;
			}
		}
		if(ENTER == 0)                                  //check if user enter any mode
		{
			while(ENTER == 0) continue;                 // wait button to release
			switch (i)                                  // execute the current mode
			{
				case 2 : Record();            // execute record mode
							break;
				default : Play();             // default mode: play mode and set back counter to 1
							i = 1;
			}
			
		}
	}
}

void Stand(void)
{
	char i =1;
	CS = 0;							
	uc_send_receive_spi(0b11100000);		//turn off spi
	uc_send_receive_spi(0b00000000);
	CS = 1;
	
	lcd_clr();	
	lcd_goto(0);					// clear lcd
	send_string("Switch mode");		// display string 
	lcd_goto(20);					// lcd goto 2nd line
	send_string("1. Forward");		// mode 1 : Forward
	while(1)						// loop forever
	{
		if(SELECT == 0)				// check if select button has been pressed
		{
			while(SELECT == 0) continue;	// wait button to release
			i++;							// increment counter
			lcd_goto(20);					// go to second line
			switch (i)						// check current value of i
			{
				case 2 : 	send_string("2. Play    ");			// display mode 2
							break;								// break out from switch
				case 3 :    send_string("3. Record  ");			// display mode 3
							break;								// break out from switch
				default:  	send_string("1. Forward ");			// if not 2 and 3 , set it back to 1 and display "1. PLAY   "
							i =1;
			}
		}
		if(ENTER == 0)                              // check if user enter any mode
		{
			while(ENTER == 0) continue;             // wait button to release
			switch (i)
			{
				case 2 : Play_s();                  // execute mode 2
							break;
				case 3 : Option();                  // execute mode 3
							break;
				default : Forward();                // execute default mode
							i = 1;
			}
			
		}
	}
}

void Play(void)
{
	char i =1;
	lcd_clr();	
	lcd_goto(0);                            // clear lcd
	send_string("Location");                // display string
	lcd_goto(20);                           // lcd goto 2nd line
	send_string("1. Message 1   ");         // default mode
	while(1)
	{
		if(SELECT == 0)
		{
			while(SELECT == 0) continue;
			i++;
			lcd_goto(20);
			switch (i)											// check current value of i
			{
				case 2 : 	send_string("2. Message 2");		// display second mode
							break;								// break out from switch
				
				default:  	send_string("1. Message 1   "); 	// if not 2 , set it back to 1 and display "1. PLAY   "
							i =1;
			}
		}
			if(ENTER == 0)                          // check if user enter any mode
			{
			while(ENTER == 0) continue;				// wait button to release
			switch (i)
			{
				case 2 : message2p();				// execute mode 2
							break;
				default : message1p();				// execute default mode
							i = 1;
			}
			
		}		
}
}

void Record(void)
{
	char i =1;
	lcd_clr();	
	lcd_goto(0);						// clear lcd
	send_string("Option");				// display string 
	lcd_goto(20);						// lcd goto 2nd line
	send_string("1. Analog Input");		// display string
	while(1)							// loop forever
	{
		if(SELECT == 0)
		{
			while(SELECT == 0) continue;
			i++;
			lcd_goto(20);
			switch (i)											// check current value of i
			{
				case 2 : 	send_string("2. MIC         ");				
							break;								// break out from switch
				default:  	send_string("1. Analog Input");		// if not 2 , set it back to 1 and display "1. PLAY   "
							i =1;
			}
		}
			if(ENTER == 0)					// check if user enter any mode
			{
			while(ENTER == 0) continue;		// wait button to release
			switch (i)
			{
				case 2 : mic();				// execute mode 2
							break;
				default : analog();			// execute default mode
							i = 1;
			}
			
		}		
}
}

void message1p(void)
{
	CS = 0;
	uc_send_receive_spi(0b00000001);            //send spi command by LSB first
	uc_send_receive_spi(0b00000000);
	uc_send_receive_spi(0b00001000);            //starting address of message 1 to be played
	uc_send_receive_spi(0b00000000);
	uc_send_receive_spi(0b01101110);
	uc_send_receive_spi(0b10000000);            //ending address of message 1 to be played
	uc_send_receive_spi(0b00000000);
	CS = 1;
	lcd_clr();                          // clear LCD
	lcd_goto(0);                        // go to first line
	send_string("**playing the");       // display string
	lcd_goto(20);                       // go to second line
	send_string("first message**");     // display string
	tmr_delay(4600);                    // delay 46 second
	lcd_clr();                          // clear LCD
	lcd_goto(0);                        // go to first line
	send_string("***halt***");          // display string
}

void message2p(void)
{
	CS = 0;
	uc_send_receive_spi(0b00000001);            // send spi command by LSB first
	uc_send_receive_spi(0b00000000);
	uc_send_receive_spi(0b11101110);            // starting address of message 2 to be played
	uc_send_receive_spi(0b10000000);
	uc_send_receive_spi(0b10111011);            // ending address of message 2 to be played
	uc_send_receive_spi(0b01000000);
	uc_send_receive_spi(0b00000000);
	CS = 1;
	lcd_clr();                          // clear LCD
	lcd_goto(0);                        // go to first line
	send_string("**playing the");       // display string
	lcd_goto(20);                       // go to second line
	send_string("second message**");    // display string
	tmr_delay(4600);                    // delay 46 second
	lcd_clr();                          // clear LCD
	lcd_goto(0);                        // go to first line
	send_string("***halt***");          // display string
}

void location(void)
{

	char i =1;
	lcd_clr();	
	lcd_goto(0);                        // clear lcd
	send_string("Location");            // display string
	lcd_goto(20);                       // lcd goto 2nd line
	send_string("1. Message 1");        // display string
	while(1)                            // loop forever
	{
		if(SELECT == 0)
		{
			while(SELECT == 0) continue;
			i++;
			lcd_goto(20);
			switch (i)										// check current value of i
			{
				case 2 : 	send_string("2. Message 2");				
							break;                  		// break out from switch
				
				default:  	send_string("1. Message 1");	// if not 2 , set it back to 1 and display "1. PLAY   "
							i =1;
			}
		}
			if(ENTER == 0)                          // check if user enter any mode
			{
			while(ENTER == 0) continue;             // wait button to release
			switch (i)
			{
				case 2 : message2r();               // execute mode 2
							break;
				default : message1r();              // execute default mode
							i = 1;
			}
			
		}		
}
}

void message1r(void)
{	
	CS = 0;
	uc_send_receive_spi(0b10000001);            //send spi command by LSB first
	uc_send_receive_spi(0b00000000);
	uc_send_receive_spi(0b00001000);            //starting address of message 1 to be recorded
	uc_send_receive_spi(0b00000000);
	uc_send_receive_spi(0b01101110);            //ending address of message 1 to be recorded
	uc_send_receive_spi(0b10000000);
	uc_send_receive_spi(0b00000000);
	CS = 1;
	lcd_clr();                          // clear LCD
	lcd_goto(0);                        // go to first line
	send_string("**recording the");     // display string
	lcd_goto(20);                       // go to second line
	send_string("first message**");     // display string
	tmr_delay(4600);                    // delay 46 second
	lcd_clr();                          // clear LCD
	lcd_goto(0);                        // go to first line
	send_string("***halt***");          // display string
	RD3 =1;
}

void message2r(void)
{
	CS = 0;
	uc_send_receive_spi(0b10000001);	//send spi command by LSB first
	uc_send_receive_spi(0b00000000);
	uc_send_receive_spi(0b11101110);	//starting address of message 2 to be recorded
	uc_send_receive_spi(0b10000000);
	uc_send_receive_spi(0b10111011);	//ending address of message 2 to be recorded
	uc_send_receive_spi(0b01000000);
	uc_send_receive_spi(0b00000000);
	CS = 1;
	lcd_clr();                      		// clear LCD
	lcd_goto(0);							// go to first line
	send_string("**recording the");			// display string
	lcd_goto(20);							// go to second line
	send_string("second message**");		// display string
	tmr_delay(4600);						// delay 46 second
	lcd_clr();								// clear LCD
	lcd_goto(0);							// go to first line
	send_string("***halt***");				// display string
	RD3 = 1;
}

void analog(void)
{
	CS = 0;							
	uc_send_receive_spi(0b11100000);		//turn off spi
	uc_send_receive_spi(0b00000000);
	CS = 1;
	tmr_delay(10);

	CS = 0;							
	uc_send_receive_spi(0b00100000);		//clear interrupt
	uc_send_receive_spi(0b00000000);
	CS = 1;		

	tmr_delay(10);

	CS = 0;							
	uc_send_receive_spi(0b10000000);		//turn on spi
	uc_send_receive_spi(0b00000000);
	CS = 1;	
	tmr_delay(10);

	RD3 = 0;
	//clear interrupt
	CS = 0;							
	uc_send_receive_spi(0b00100000);		//clear interrupt
	uc_send_receive_spi(0b00000000);
	CS = 1;		

	tmr_delay(10);

	//analog input
	CS = 0;
	uc_send_receive_spi(0b10100010);		//analog input
	uc_send_receive_spi(0b00000000);
	uc_send_receive_spi(0b00100000);
	CS = 1;
	
	tmr_delay(10);							//delay 100 milisecond

	location();
}

void mic(void)
{
	CS = 0;							
	uc_send_receive_spi(0b11100000);		//turn off spi
	uc_send_receive_spi(0b00000000);
	CS = 1;
	tmr_delay(10);

	CS = 0;							
	uc_send_receive_spi(0b10000000);		//turn on spi
	uc_send_receive_spi(0b00000000);
	CS = 1;	
	tmr_delay(10);

	RD3 = 1;
	location();
}


void Forward(void)
{
	RC0 = 0;						// pull low to ISD forward pin
	lcd_goto(20);					// go to second line
	send_string("**forward**");		// display string	
	tmr_delay(300);					// delay 3 second
	RC0 = 1;                        // pull high to ISD forward pin
}

void Play_s(void)
{
	RE2 = 0;						// pull low to ISD play pin
	lcd_goto(20);                   // go to second line
	send_string("**play**   ");		// display string
	tmr_delay(100);					// delay 1 second
	RE2 =1;							// pull high to ISD play pin
}

void Option(void)
{
	char i =1;
	lcd_clr();	
	lcd_goto(0);						// clear lcd
	send_string("Option");				// display string 
	lcd_goto(20);						// lcd goto 2nd line
	send_string("1. Analog Input");		// display string
	while(1)							// loop forever
	{
		if(SELECT == 0)
		{
			while(SELECT == 0) continue;
			i++;
			lcd_goto(20);
			switch (i)                                            	// check current value of i
			{
				case 2 : 	send_string("2. MIC         ");				
							break;                     				// break out from switch
				default:  	send_string("1. Analog Input");    		// if not 2 , set it back to 1 and display "1. PLAY   "
							i =1;
			}
		}
			if(ENTER == 0)							// check if user enter any mode
			{
			while(ENTER == 0) continue;				// wait button to release
			switch (i)
			{
				case 2 : RD3 = 1; Record_s();		// execute mode 2
							break;
				default : RD3 = 0; Record_s();		// execute default mode
							i = 1;
			}
			
		}		
}
}

void Record_s(void)
{
	while(1)                                    // loop forever
	{
		while(ENTER == 1)continue;              // wait button to press
		if(ENTER == 0)                          // if enter is pressed
		
		{
			while(ENTER == 0) continue;             // wait button to release
			RA0 = 0;                                // pull low to ISD record pin
			lcd_clr();                              // clear LCD
			lcd_goto(0);                            // go to first line
			send_string("**record**");              // display string
			lcd_goto(20);                           // go to second line
			send_string("select to stop");          // display string
		}
		while(SELECT ==1)continue;               // wait select button to be pressed
		if(SELECT == 0)                          // if select button is pressed
		{	
			while(SELECT == 0)continue;      // wait button to release
			RA0 = 1;                         // pull high to ISD record pin
			lcd_clr();                       // clear LCD
			lcd_goto(0);                     // go to first line
			send_string("**stop ");          // display string
			lcd_goto(20);                    // go to second line
			send_string("recording**");      // display string
			while(1){}                       // loop forever
		}
	}
}


void Erase(void)
{
	char i =1;
	lcd_clr();	
	lcd_goto(0);                        // clear lcd
	send_string("Decision");            // display string
	lcd_goto(20);                       // lcd goto 2nd line
	send_string("1. Erase       ");     // display string
	while(1)
	{
		if(SELECT == 0)                         // check if select button is pressed
		{
			while(SELECT == 0) continue;        // wait button to release
			i++;                                // increment counter
			lcd_goto(20);                       // go to second line
			switch (i)                          // check current value of n
			{
				case 2 : 	send_string("2. Global Erase");				
							break;                        			// break out from switch
				
				default:  	send_string("1. Erase       ");       	// if not 2 , set it back to 1 and display "1. PLAY   "
							i =1;
			}
		}
			if(ENTER == 0)                                // check if user enter any mode
			{
			while(ENTER == 0) continue;                   // wait button to release
			switch (i)
			{
				case 2 : g_erase();                // execute second mode
							break;
				default : erase();                 // execute default mode
							i = 1;
			}
			
		}		
}
}

void erase(void)
{
	CS = 0;							
	uc_send_receive_spi(0b10000000);	//turn on spi
	uc_send_receive_spi(0b00000000);
	CS = 1;
	tmr_delay(10);						//delay 100 milisecond	

	CS = 0;
	uc_send_receive_spi(0b01000010);	//send spi command by LSB first
	uc_send_receive_spi(0b00000000);
	CS = 1;
	tmr_delay(100);					//delay 1 second
	lcd_clr();						// clear LCD
	lcd_goto(20);					// go to second line
	send_string("Erased");			// display string
	while(1);						// loop forever
	
}

void g_erase(void)
{
	CS = 0;							
	uc_send_receive_spi(0b10000000);	// turn on spi
	uc_send_receive_spi(0b00000000);
	CS = 1;
	tmr_delay(10);						// delay 100 milisecond	

	CS = 0;
	uc_send_receive_spi(0b11000010);    // send spi command by LSB first
	uc_send_receive_spi(0b00000000);
	CS = 1;
	tmr_delay(100);						// delay 1 second
	lcd_clr();							// clear LCD
	lcd_goto(20);						// go to second line
	send_string("Memory Cleared");		// display string
	while(1);							// loop forever
	
}

