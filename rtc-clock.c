//******************************************************************************
//  MSP431F20xx Demo - led multiplexing counter
//
//  Description; 
//
//  ACLK = 32khz, MCLK = SMCLK = default DCO
//
//		  MSP430G2xx1 + PCF8563
/*

   +=====================================================+
   |  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  |
   |  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  |
   |  .  .  .  .  .  o  .  o  c  .  .  .  .  .  .  .  .  |
   |  .  .  .  .  .  .  .  .  .  i  .  .  .  i  +[]+  .  |	i alarm interrupt from rtc
   |  .  .  +--+--+-(0)-A--F-(1)(2)-B--+--+--+--+--+-    |
   |  .  .  |    |- b6 b7 CK IO a7 a6|   |-         |    |
   |  .  .  |    |+ a0 a1 a2 a3 a4 a5|   |A  C     +|    |
   |  .  .  |    -+--+--+--+--+--+--+    -+--+--+--+-    |
   |  .  .  +--------E--D-(.)-C--G-(3)-------+  .  .  .  |
   |  .  .  c  .  .  .  .  .  .  .  .  .  .  .  .  .  .  |
   |  .  .  o--B--o  .  .  c  oBB+  .  .  .  .  .  .  .  |
   |  .  .  .  .  .  1  2-Buz-o  .  .  .  1  2  .  .  .  |  1,2 i2c comm w/ rtc
   +=====================================================+

c chung (www.simpleavr.com)

May 2013
. clean-up
. code exceed 2k now w/ newer mspgcc and CCS, i made the following changes
. alter some macros for newer compilers
. "borrow" infomem b,c,d to put some of my constants to free up code flash
  be sure to include "--section-start=.infomembcd=0x01040" when linking
. works for mspgcc as we are just over 76 bytes
. failed for CCS as we need almost 300 bytes (infomem has only 64x3 usable bytes)
. compiled under cygwin and ubuntu / mint
. get this into github

Nov 2010

code provided as is, no warranty

you cannot use code for commercial purpose w/o my permission
nice if you give credit, mention my site if you adopt much of my code

test build under linux ubuntu 10.04 w/ msp430-gcc and flash w/ msgdebug
not test under windows CCS or IAR


     MSP430G2211 or similar
   ----------------- 
  |                 |
  |                 |                  PCF8563      /|\
  |                 |              ---------------   |
  |                 |             |          Vcc 8|---
  |                 | /---------->|5 SDA          |  
  |                 | | /---------|6 SCL         1|--|[]|--.
  |                 | | | /-------|3 Int          |  Xtal  |
  |                 | | | |       |              2|________.
  |                 | | | |       |          Gnd 4|---
  |                 | | | |        ---------------   |
  |                 | | | |                         ---
  |                 | | | |    4 x 7 segment module ///
  |                 | | | |   ------------------------
  |           A P2.7|-+-+-+->|    __a_                |
  |           B P1.6|-+-+-+->|   |    |b              |
  |           C P1.3|-+-+-+->|  f|    |               |
  |           D P1.1|-+-o-+->|   |__g_|  ....  . .... |
  |           E P1.0|-o---+->|   |    |c              |
  |    Digit0+F P2.6|-o---+->|  e|    |               |
  |           G P1.4|-+---+->|   |__d_ o.             |
  |    Digit1+. P1.2|-+-o-+->|                        |
  |                 | | | |   ------------------------  /|\
  |                 | | | |      ^   ^   ^   ^      _o_  |
  |            Reset|-o-+-+------+---+---+---+------o o---
  |                 |   \-+----------/   |   |            
  |    Digit2   P1.7|-----o--------------/   |
  |    Digit3   P1.5|------------------------/
   -----------------
                                     
   * google "PCF8563 backup battery" for image if u need one

    ___a__
   |      |        (0) A  F (1)(2) B 
  f|      | b      -+--+--+--+--+--+ 
    ___g__        |                 |
  e|      | c     |Pin1             |
   |      |        -+--+--+--+--+--+ 
    ___d__          E  D  .  C  G (3)



best view w/ vim "set ts=4 sw=4"

*/

//#include "signal.h"
//#include  <msp430x20x2.h>
#include  <msp430.h>
#include  <stdlib.h>

#define TASSEL__ACLK	TASSEL_1
#define TASSEL__SMCLK	TASSEL_2
#define MC__UP			MC_1 

#ifdef MSP430
#else


typedef unsigned char	uint8_t;
typedef unsigned int	uint16_t;
typedef int  			int16_t;

#endif

#include  "i2c.h"

#define SEG_A_P1	0x00
#define SEG_B_P1	(1<<6)
#define SEG_C_P1	(1<<3)
#define SEG_D_P1	(1<<1)
#define SEG_E_P1	(1<<0)
#define SEG_F_P1	0x00
#define SEG_G_P1	(1<<4)
#define SEG_d_P1	(1<<2)
#define DIGIT_0_P1	0x00
#define DIGIT_1_P1	(1<<2)
#define DIGIT_2_P1	(1<<7)
#define DIGIT_3_P1	(1<<5)

#define SEG_A_P2	(1<<7)
#define SEG_B_P2	0x00
#define SEG_C_P2	0x00
#define SEG_D_P2	0x00
#define SEG_E_P2	0x00
#define SEG_F_P2	(1<<6)
#define SEG_G_P2	0x00
#define SEG_d_P2	0x00
#define DIGIT_0_P2	(1<<6)
#define DIGIT_1_P2	0x00
#define DIGIT_2_P2	0x00
#define DIGIT_3_P2	0x00

#define SEGS_STAY(v) \
   (((v & (1<<7)) ? 1 : 0) +\
    ((v & (1<<6)) ? 1 : 0) +\
	((v & (1<<5)) ? 1 : 0) +\
	((v & (1<<4)) ? 1 : 0) +\
	((v & (1<<3)) ? 1 : 0) +\
	((v & (1<<2)) ? 1 : 0) +\
	((v & (1<<1)) ? 1 : 0) +\
	((v & (1<<0)) ? 1 : 0)) | 0x20

#define SEGS_PORT_DET(p, v) \
   (((v & (1<<7)) ? SEG_d_P##p : 0) |	\
    ((v & (1<<6)) ? SEG_A_P##p : 0) |	\
	((v & (1<<5)) ? SEG_B_P##p : 0) |	\
	((v & (1<<4)) ? SEG_C_P##p : 0) |	\
	((v & (1<<3)) ? SEG_D_P##p : 0) |	\
	((v & (1<<2)) ? SEG_E_P##p : 0) |	\
	((v & (1<<1)) ? SEG_F_P##p : 0) |	\
	((v & (1<<0)) ? SEG_G_P##p : 0))

#define SEGS_PORT(v)	{SEGS_STAY(v),SEGS_PORT_DET(1, v),SEGS_PORT_DET(2, v)}
#define SEGS_1 (SEG_A_P1|SEG_B_P1|SEG_C_P1|SEG_D_P1|SEG_E_P1|SEG_F_P1|SEG_G_P1)
#define SEGS_2 (SEG_A_P2|SEG_B_P2|SEG_C_P2|SEG_D_P2|SEG_E_P2|SEG_F_P2|SEG_G_P2|SEG_d_P2)

#define DIGITS_1 (DIGIT_0_P1|DIGIT_1_P1|DIGIT_2_P1|DIGIT_3_P1)
#define DIGITS_2 (DIGIT_0_P2|DIGIT_1_P2|DIGIT_2_P2|DIGIT_3_P2)

#define USED_1 (SEGS_1|DIGITS_1)
#define USED_2 (SEGS_2|DIGITS_2)

/*
       ___a__
      |      |
     f|      | b
       ___g__
     e|      | c
      |      |
       ___d__
*/
//_____________________ abc defg
#define LTR_0 0x7e	// 0111 1110
#define LTR_1 0x30	// 0011 0000
#define LTR_2 0x6d	// 0110 1101
#define LTR_3 0x79	// 0111 1001
#define LTR_4 0x33	// 0011 0011
#define LTR_5 0x5b	// 0101 1011
#define LTR_6 0x5f	// 0101 1111
#define LTR_7 0x70	// 0111 0000
#define LTR_8 0x7f	// 0111 1111
#define LTR_9 0x7b	// 0111 1011
#define BLANK 0x00	// 0000 0000
#define BAR_1 0x40	// 0100 0000
#define BAR_2 0x01	// 0000 0001
#define BAR_3 (BAR_1|BAR_2)
#define LTRdg 0x63	// 0110 0011
#define LTR_C 0x4e	// 0100 1110

#define LTR_c 0x4e	// 0000 1101
#define LTR_A 0x77	// 0111 0111
#define LTR_b 0x1f	// 0001 1111
#define LTR_J 0x3c	// 0011 1100
#define LTR_L 0x0e	// 0000 1110
#define LTR_S 0x5b	// 0101 1011
#define LTR_E 0x4f	// 0100 1111
#define LTR_t 0x0f	// 0000 1111
#define LTR_n 0x15	// 0001 0101
#define LTR_N 0x76	// 0111 0110
#define LTR_d 0x3d	// 0011 1101
#define LTR_i 0x10	// 0001 0000
#define LTR_H 0x37	// 0011 0111
#define LTR_h 0x17	// 0001 0111
#define LTR_r 0x05	// 0000 0101
#define LTR_o 0x1d	// 0001 1101
#define LTR_f 0x47	// 0100 0111
#define LTR_u 0x1c	// 0001 1100
#define LTR_U 0x3e	// 0011 1110
#define LTRml 0x66	// 0110 0110
#define LTRmr 0x72	// 0111 0010
#define LTR__ 0x00	// 0000 0000
#define LTRmw 0x90	// 1001 0000

//#pragma DATA_SECTION(digit2ports, ".infoBCD")
//static const uint8_t digit2ports[][3] = { 
static __attribute__ ((section (".infomembcd"))) const uint8_t digit2ports[][3] = { 
	SEGS_PORT(LTR_0), SEGS_PORT(LTR_1), SEGS_PORT(LTR_2), SEGS_PORT(LTR_3),
	SEGS_PORT(LTR_4), SEGS_PORT(LTR_5), SEGS_PORT(LTR_6), SEGS_PORT(LTR_7),
	SEGS_PORT(LTR_8), SEGS_PORT(LTR_9), SEGS_PORT(BLANK), SEGS_PORT(LTR_o),
	SEGS_PORT(BAR_2), SEGS_PORT(LTR_f), SEGS_PORT(LTR_C), SEGS_PORT(LTR_t), 

	SEGS_PORT(LTR_h), SEGS_PORT(LTR_n), SEGS_PORT(LTR_r), SEGS_PORT(LTR_d), 
	SEGS_PORT(LTR_i), SEGS_PORT(LTR_E), SEGS_PORT(LTR_A), SEGS_PORT(LTR_u),
};

enum {
	POS_0, POS_1, POS_2, POS_3, POS_4, POS_5, POS_6, POS_7,	// 00000...00111
	POS_8, POS_9, POS__, POS_o, POSb2, POS_f, POS_C, POS_t, // 01000...01111
	POS_h, POS_n, POS_r, POS_d, POS_i, POS_E, POS_A, POS_u, // 10000...10111
};

#define SPOS_WDAY	5

//#pragma DATA_SECTION(digit2ports, ".infoBCD")
//uint16_t menu_desc[] = {
static __attribute__ ((section (".infomembcd"))) const uint16_t menu_desc[] = { 
	(POS__<<10)| (POS__<<5)| POS__,		// 01010 01010 01010 = 00101001 01001010 = 0x294a
	(POS_2<<10)| (POS_4<<5)| POS_h,		// 00010 00100 10000 = 00001000 10010000 = 0x0890
	(POS_1<<10)| (POS_2<<5)| POS_h,

	(POS__<<10)| (POS_0<<5)| POS_n,
	(POS_o<<10)| (POS_f<<5)| POS_f,

	(POS_5<<10)| (POS_u<<5)| POS_n,
	(POS_n<<10)| (POS_o<<5)| POS_n | 0x8000,	// need extra
	(POS_t<<10)| (POS_u<<5)| POS_E,
	(POS_u<<10)| (POS_E<<5)| POS_d | 0x8000,
	(POS_t<<10)| (POS_h<<5)| POS_u,
	(POS_f<<10)| (POS_r<<5)| POS_i,
	(POS_5<<10)| (POS_A<<5)| POS_t,
};

#define LONG_HOLD	2000

#define ST_HOLD		0x80
#define ST_PRESSED	0x40
#define ST_BUTTON   (ST_HOLD|ST_PRESSED)
#define ST_READ  	0x20
#define ST_UPDATE	0x10
#define ST_REFRESH	0x08
#define ST_BUZZ     0x04
#define ST_SETUP   	0x02

#define BUZZ_PINP	(1<<4)
#define BUZZ_PINN	(1<<3)
#define CNTR_PINP	(1<<1)
#define CNTR_PINN	(1<<3)

#define OPT_ALARM	4
#define OPT_CNTR	1

uint8_t output[3 * 4];
volatile uint8_t stacked=1;


//_________ these are the full rtc register default values (our default)
//          day (am/pm flags) and dow (dimmer) alarms can be used as storage
enum             {  tmr, tctl,  clk, aday, adow,  ahr, amin, year,  mon,  dow,  day, hour,  min,  sec, ctl2, ctl1, };
uint8_t time[] = { 0xaa, 0x00, 0x00, 0x81, 0x81, 0x00, 0x00, 0x10, 0x11, 0x02, 0x10, 0x12, 0x30, 0x61, 0x00, 0x00, };
//_________ what to show in each display mode
const uint8_t show[] = { 
	(tmr<<4)|tmr,		// blank
	(hour<<4)|min, 		// hhmm
	(ctl2<<4)|sec, 		// seconds
	(mon<<4)|day, 		// mmdd
	(ahr<<4)|amin, 		// alarm
	(ctl2<<4)|year,		// year
	};

uint8_t range[] = { 0x00, 0x24, 0x00, 0x13, 0x24, };

uint8_t bcd_chk(uint8_t bcd, uint8_t adv) {
	if ((bcd&0x0f) > 9) {
		if (adv) bcd += 6;
		else     bcd &= 0xf0;
	}//if
	return bcd;
}

#define	dimmer adow
#define	cfgbit aday
//__________________________________________________
void seg2port(uint8_t idx, uint8_t adv) {

	if (idx==3 && !adv && time[sec]&0x01) {
		//________ show day of week once a while when showing date
		seg2port(time[dow]+SPOS_WDAY, 0x10);
		return;
	}//if

	uint8_t *dp = time + (show[idx]&0x0f);
	uint8_t range_high = range[idx];

	switch (adv&0x0f) {
		case 1:	// 1st and 2nd digit advance in pair
			dp = time + (show[idx]>>4);
			*dp = bcd_chk(*dp+1, 1);
			if ((*dp&0x7f) >= range_high) 
				*dp = (*dp&0x80) | (range_high & 0x01);
			break;
		case 2:	// 3rd digit advance
			if (*dp >= 0x50) *dp -= 0x50;
			else 			 *dp += 0x10;
			break;
		case 3:	// 4th digit advance
			*dp = bcd_chk(*dp+1, 0);
			break;
		default:
			break;
	}//switch

	uint8_t dot = 0x00;
	uint8_t bcd = (time[show[idx]>>4]) & 0x7f;

	if ((range_high==0x24) && (time[cfgbit]&0x01)) {
		// adjust for 12hr display
		if (bcd >= 0x12) {
			//________ pm indicator
			dot = 1<<3;
			bcd -= 0x12;
			if (bcd & 0x08) bcd -= 0x06;
		}//if
		if (!bcd) bcd = 0x12;
	}//if

	uint16_t bcd4;

	if (adv&0x10)
		bcd4 = menu_desc[idx];
	else
		bcd4 = (bcd<<8) | (time[show[idx]&0x0f] & 0x7f);

	if (idx==3) dot |= 1<<0;	// date
	if (idx==4) {
		dot |= 1<<2;			// alarm
		if (!(time[ahr]&0x80))
			dot |= 1<<0;		// alarm on
	}//if

	uint8_t i = 0x08;
	uint8_t *pp = output + 11;

	while (i) {
		bcd = bcd4 & ((adv&0x10) ? 0x1f : 0x0f);
		if (i==1) {
			if (!bcd) {
				bcd = 0x0a;
			}//if
			else {
				if (adv&0x10) bcd = POS_i;	// pretend w and m
			}//else
		}//if
		*pp-- = digit2ports[bcd][0];
		*pp = digit2ports[bcd][1];
		if (dot&i) *pp |= SEG_d_P1;
		pp--;
		*pp-- = digit2ports[bcd][2];
		bcd4 >>= (adv&0x10) ? 5 : 4;
		i >>= 1;
	}//while
}

#define I2C_READ	0x80

volatile uint8_t ticks=0;
uint8_t stays=1;
//uint8_t setup_pos[]  = { 0x00, 0x01, 0x04, 0x01, 0x01, 0x02, }; 
//uint8_t setup_addn[] = { 0x00, 0x01, 0x00, 0x05, 0x03, 0x00, }; 
uint8_t setup_addx[] = { 0x00, 0x11, 0x90, 0x15, 0x13, 0x20, }; 
uint8_t digit_map1[] = { DIGIT_0_P1, DIGIT_1_P1, DIGIT_2_P1, DIGIT_3_P1, };
uint8_t digit_map2[] = { DIGIT_0_P2, DIGIT_1_P2, DIGIT_2_P2, DIGIT_3_P2, };
//______________________________________________________________________
void main(void) {

	uint8_t pos=0; 
	WDTCTL = WDTPW + WDTHOLD + WDTNMI + WDTNMIES;	// stop WDT, enable NMI hi/lo

	BCSCTL1 = CALBC1_1MHZ;			// Set DCO to 1MHz
	DCOCTL  = CALDCO_1MHZ;

	BCSCTL2 |= DIVM_1|DIVS_1;       // MCLK/SMCLK divide by 2
	CCR0  = (1000000/16/2);			// 16 ticks/s and div2
	CCTL0 = CCIE;					// CCR0 interrupt enabled
	TACTL = TASSEL__SMCLK + MC__UP;	// SMCLK, upmode

	_BIS_SR(GIE);					// enable interrupt

	P1SEL = 0;
	P2SEL = 0;


	uint8_t mode=1;
	uint8_t state=0;
	uint8_t setup=0;
	while (1) { 

		if (!setup && stacked) {
			while (stacked) {
				stacked--;
				time[sec] = bcd_chk(time[sec]+1, 1);
				if (time[sec] >= 0x60) // sync w/ rtc every minute
					state |= ST_READ;
			}//while
			if ((mode != 1) || !time[sec]) state |= ST_REFRESH;
		}//if

		uint8_t adv = 0;
		uint8_t txt = 0;
		//if (state & ST_BUTTON) state &= ~ST_BUTTON;
		//_____________________________________ check input
		if (state & ST_BUTTON) {				// needs attention
			if (state & ST_BUZZ) {	// stop alarm, notify rtc
				mode = 1;
				time[ctl2] = 0x00;
				state &= ~(ST_BUZZ|ST_BUTTON);
				state |= ST_REFRESH|ST_UPDATE;
				continue;
			}//if

			if (setup) {
				//___________ in digit setup, need to advance digit or position
				if (state & ST_HOLD) {
					switch (setup) {
						case 9: 	// dimmer done
						case 4: 	// toggle done
							if (mode == 3) {	// in calender menu, now do year
								setup = 1;
								mode = 5;
							}//if
							else {
								setup = 0;
								state |= ST_UPDATE;
							}//else
							break;
						default:
							setup++;
							if (setup == 4)	{
								if (mode == 5) {
									setup = 0;
									state |= ST_UPDATE;
									mode = 3;
								}//if
								else {
									// digit setup done, advance to toggle
									state |= ST_PRESSED;
								}//else
							}//if
						case 0:
							break;
					}//switch
				}//if
				if (state & ST_PRESSED) {
					switch (setup) {
						case 9:
							time[dimmer]++;
							time[dimmer] &= 0x03;
							break;
						case 4:				// toggle text options
							switch (mode) {	// register to use
								case 1:	// hhmm
									if (!(state&ST_HOLD))
										time[cfgbit] ^= 0x01;
									txt = time[cfgbit] & 0x01;
									break;
								case 3:	// mmdd
									if (!(state&ST_HOLD)) {
										time[dow]++;
										if (time[dow]>6) time[dow] = 0;
									}//if
									txt = time[dow];
									break;
								case 4:	// alarm
									if (!(state&ST_HOLD))
										time[ahr] ^= 0x80;
									txt = time[ahr] >> 7;
									break;
							}//swich
							txt += (setup_addx[mode]&0x0f);	// add offset to description
							break;
						default:
							adv = setup;	// signal digit advance at current setup position
							break;
					}//switch
				}//if
			}//if
			else {
				//___________ see if long or short pressed
				if (state & ST_PRESSED) {
					//_______ advance display mode
					mode++; 
					if (mode == 5) mode = 0;
					stays = 0;
				}//if
				else {
					//_______ long pressed, enter menu
					if (mode) {
						setup = setup_addx[mode]>>4;
					}//if
				}//else
			}//else
		}//if

		if (state&(ST_REFRESH|ST_BUTTON) && !stays) {
			state &= ~(ST_REFRESH|ST_BUTTON);
			if (txt) {
				//_____________ need to show menu description
				seg2port(txt, 0x10);
			}//if
			else {
				//_________ show whatever mode dictates
				seg2port(mode, (setup<<5)|adv);
				adv = 0;
			}//else
		}//if


		//
		if (state & ST_BUZZ) {
			if (ticks & 0x04) {
				P2DIR  = 0x00;
				P1DIR  = BUZZ_PINP|BUZZ_PINN;
				P1OUT ^= BUZZ_PINP; 	// toggle buzzer
			}//if
		}//if
		//
		//
		if (stays & 0x0f) { stays--; continue; }

		//stays >>= *time>>8;
		/*
		if (mode == 3) {
			uint8_t dim = (ticks&0x1f)>>1;
			if (ticks&0x20) dim ^= 0x0f;
			stays >>= dim;
		}//if
		else {
			stays >>= (time[dimmer]&0x03);
		}//else
		*/
		stays >>= (time[dimmer]&0x03);
		// try pulsing brightness
		/*
		uint8_t dim = (ticks&0x0f)>>2;
		if (ticks&0x10) dim ^= 0x03;
		stays >>= dim;
		*/
		P2DIR = 0;
		P1DIR = 0;
		P2OUT = 0;
		P1OUT = 0;
		//
		if (state&(ST_UPDATE|ST_READ)) {
			_BIC_SR(GIE);					// disable interrupt
			if (state&ST_UPDATE) {
				time[sec]    &= ~0x80;		// sec
				time[dimmer] |= 0x80;		// always disable dow, day alarm
				time[cfgbit] |= 0x80;		// as they are used as config storage
				if (time[ahr] & 0x80)		// sync min and hour alarm enablement
					time[amin] |= 0x80;
				else
					time[amin] &= ~0x80;
				i2c_snd_rcv(15, 0, (uint8_t*) &time[ctl1]);
			}//if
			i2c_snd_rcv(I2C_READ|14, 1, (uint8_t*) &time[ctl2]);
			if (time[ctl2]&0x08) {
				state |= ST_BUZZ;	// alarm happened
				mode = 1;
			}//if
			state &= ~(ST_UPDATE|ST_READ);
			if (time[sec]>=0x60) {
				// we shouldn't be reading this unless rtc gone bad (backup powersheetji failed)
				// so we will loop again and write to clear the "bad" flag
				state |= ST_UPDATE;
			}//if
			state |= ST_REFRESH;
			_BIS_SR(GIE);					// enable interrupt
		}//if

		//
		if (stays) { stays--; continue; }

		//___________ check button
		//            button must be position at P1.2 as it's tied to RESET pin
		//            we need it be pressed after power's been applied (boot)
		P1REN = 1<<2;		// pull-up for button read
		P1DIR = 0;
		uint16_t wait=0;
		do {
			if (wait == 0x0020) {
				state |= ST_PRESSED;
			}//if
			else {
				if (wait++ > 0x4000) {
					state &= ~ST_PRESSED;
					state |= ST_HOLD;
					//if (wait&0x0f) P1DIR ^= 0x30;
					P1DIR ^= 0x30;
				}//if
			}//else
			wait++;
		} while (P1IN & (1<<2));
		P1REN = 0;

		if (state & ST_BUTTON) continue;
		if (!mode) {
			time[ctl2] = 0x02;			// ask rtc to wake me up for alarm
			i2c_snd_rcv(2, 0, (uint8_t*) &time[ctl1]);
			P2DIR = 0; P2OUT = 0;

			P1REN = BIT7;	// need pull-up for rtc interrupt pin
			P1OUT = BIT7;
			P1DIR = 0;

			P1IES &= ~BIT2;	// low-high trigger
			P1IES |= BIT7;	// high-low trigger
			P1IFG &= ~(BIT2|BIT7);	// clear
			P1IE   = (BIT2|BIT7);	// pin interrupt enable

			_BIS_SR(LPM4_bits|GIE);	// u are tired, go sleep

			if (P1IFG & BIT2)		// from keypress
				while (P1IN&0x02); 	// make sure key is not depressed
			else
				state |= ST_BUZZ;	// alarm wake-up
			P1IFG &= ~(BIT2|BIT7);	// clear interrupt flag
			mode = 1;
			time[ctl2] &= ~0x02;	// suppress interrupt at wake up
			i2c_snd_rcv(2, 0, (uint8_t*) &time[ctl1]);
			P1REN = 0;
			P2DIR = 0; P1DIR = 0;
			P2OUT = 0; P1OUT = 0;
			state |= ST_READ;
		}//if

		// flasher during individual digit setup
		//___________ load segments
		uint8_t *ioptr = output + (pos*3);
		if (!setup || !(ticks & 0x04) || (pos != setup && (pos|setup) != 0x01)) {
			P2OUT = *ioptr & ~digit_map2[pos];
			P2DIR = *ioptr++ | digit_map2[pos];
			P1OUT = *ioptr & ~digit_map1[pos];
			P1DIR = *ioptr++ | digit_map1[pos];
			stays = *ioptr;
		}//if

		pos++;
		pos &= 0x03;
	}//while

}
//______________________________________________________________________
//interrupt(PORT1_VECTOR) PORT1_ISR(void) {
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void) {
	P1IE &= ~(BIT2|BIT7);	// disable pin interrupt
	_BIC_SR_IRQ(LPM4_bits);	// wake up, got keypressed
}
//______________________________________________________________________
//interrupt(TIMERA0_VECTOR) Timer_A(void) {
#pragma vector=TIMERA0_VECTOR
__interrupt void TIMERA0_ISR(void) {
	ticks++;
	if (!(ticks&0x0f)) {	// around 1 sec
		stacked++;
	}//if
}

