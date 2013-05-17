
#define READ    0x01    // I2C READ bit

#define I2CPIN  P1IN
#define I2CDDR  P1DIR
#define I2CPORT	P1OUT
#define I2CREN 	P1REN

#define SDA	0
#define SCL	1

// change slave address for your deice, mine is a PCF8563 rtc
#define SLAVE 0xa2

// bitbang timing we may need for faster clocks or slower buses
//
//#define QDEL	_delay_us(5)
//#define HDEL	_delay_us(10)
//#define QDEL	brief_pause(1)
//#define HDEL	brief_pause(2)
//#define QDEL	asm("nop")
//#define HDEL	asm("nop")
#define QDEL	;
#define HDEL	;

#define I2C_SDA_LO      I2CPORT &= ~(1<<SDA)
#define I2C_SDA_HI      I2CPORT |= (1<<SDA)

#define I2C_SCL_LO      I2CPORT &= ~(1<<SCL); 
#define I2C_SCL_HI      I2CPORT |= (1<<SCL); 

/*______________________________________________________________________
static void __inline__ brief_pause(register uint16_t n) {
    __asm__ __volatile__ (
                "1: \n"
                " dec      %[n] \n"
                " jne      1b \n"
        : [n] "+r"(n));

}*/ 
//___________________________________________________
#define I2C_SCL_TOGGLE  HDEL; I2C_SCL_HI; HDEL; I2C_SCL_LO
#define I2C_START       I2C_SDA_LO; QDEL; I2C_SCL_LO
#define I2C_STOP        HDEL; I2C_SCL_HI; QDEL; I2C_SDA_HI; HDEL
//___________________________________________________
uint8_t i2c_putbyte(uint8_t b) {
    char i;

    for (i=7;i>=0;i--) {
		if (b & 0x80)
        	I2C_SDA_HI;
        else
            I2C_SDA_LO;		// set data bit
        I2C_SCL_TOGGLE;		// clock HI, delay, then LO
		b <<= 1;
    }//for

    I2C_SDA_HI;				// leave SDA HI
	I2CREN |= (1<<SDA);
	I2CDDR &= ~(1<<SDA);	// make SDA input w/ pullup to read ack

	HDEL;
    I2C_SCL_HI;				// clock back up
  	
  	b = I2CPIN & (1<<SDA);  // get the ACK bit

	HDEL;
    I2C_SCL_LO;	// not really ??

	I2CDDR |= (1<<SDA); // change direction back to output

	HDEL;

    return (b == 0);            // return ACK value
}
//___________________________________________________
uint8_t i2c_getbyte(uint8_t ack) {
    char i;
    uint8_t b=0x00;

	I2CDDR &= ~(1<<SDA);
	I2CREN |= (1<<SDA);
    I2C_SDA_HI;			// make sure pullups are ativated

    for (i=7;i>=0;i--) {
		HDEL;
        I2C_SCL_HI;		// clock HI
        b <<= 1;
        if (I2CPIN & (1<<SDA)) b |= 1;

		HDEL;
    	I2C_SCL_LO;		// clock LO
    }//for

	I2CDDR |= (1<<SDA); // change direction to output on SDA line
  
	if (ack)
		I2C_SDA_LO;		// set ACK, more to read
	else
		I2C_SDA_HI;		// set NAK
    I2C_SCL_TOGGLE;		// clock pulse
    I2C_SDA_HI;			// leave with SDA HI
	
    return b;			// return received byte
}

/*
//___________________________________________________
void i2c_init(){
	I2CDDR  |= ((1<<SDA) | (1<<SCL));
	I2CPORT |= ((1<<SDA) | (1<<SCL));
}

//___________________________________________________
uint8_t i2c_exists() {
	I2C_START;
	return i2c_putbyte(SLAVE);
}
*/

//___________________________________________________
void i2c_snd_rcv(uint8_t cnt, uint8_t sub, uint8_t *data) {
//void __attribute__ ((section (".infomembcd"))) i2c_snd_rcv(uint8_t cnt, uint8_t sub, uint8_t *data) {

	//return;
	// u can move the following two lines to form a i2c_init()
	// i need it here since between i2c comms i need the same pins
	// to drive leds
	I2CDDR  |= ((1<<SDA) | (1<<SCL));
	I2CPORT |= ((1<<SDA) | (1<<SCL));

	I2C_START;					// do start transition
	i2c_putbyte(SLAVE);			// send DEVICE address
	i2c_putbyte(sub++);   		// and the subaddress

	if (cnt&0x80) {
		cnt &= ~0x80;
		HDEL;
		I2C_SCL_HI;      		// do a repeated START
		I2C_START;          	// transition

		i2c_putbyte(SLAVE | READ);// resend DEVICE, with READ bit set
		while (cnt)
			*data-- = i2c_getbyte(--cnt);
	}//if
	else {
		while (cnt) {
			i2c_putbyte(*data--);
			cnt--;
		}//while
	}//else

	I2C_SDA_LO;             // clear data line and
	I2C_STOP;               // send STOP transition

}

