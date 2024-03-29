/*
  isp.c - part of USBasp

  Autor..........: Thomas Fischl <tfischl@gmx.de> 
  Description....: Provides functions for communication/programming
                   over ISP interface
  Licence........: Free under certain conditions. See Documentation.
  Creation Date..: 2005-02-23
  Last change....: 2005-04-20
*/

#include <avr/io.h>
#include "isp.h"
#include "clock.h"

#define spiHWdisable() SPCR = 0

void spiHWenable() {
  	/* enable SPI, master, 375kHz SCK */ 
  	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1);
   	SPSR = (1 << SPI2X);
}

void ispSetSCKOption(uchar option) {
  if (option == 0) {
    /* use software spi */
    ispTransmit = ispTransmit_sw;
  } else {
    /* use hardware spi */
	ispTransmit = ispTransmit_hw;
  }
}
  
void ispDelay() {
  uint8_t starttime = TIMERVALUE;
  while ((uint8_t) (TIMERVALUE - starttime) < 12) { }
}

void ispDelay_5x() {
  uint8_t starttime = TIMERVALUE;
  while ((uint8_t) (TIMERVALUE - starttime) < 2) { }
}

void ispConnect() {

  /* all ISP pins are inputs before */
  /* now set output pins */
  ISP_DDR |= (1 << ISP_RST) | (1 << ISP_SCK) | (1 << ISP_MOSI);

	if(chip==ATM){
			/* reset device */
 		ISP_OUT &= ~(1 << ISP_RST);   /* RST low */
  	ISP_OUT &= ~(1 << ISP_SCK);   /* SCK low */

  		/* positive reset pulse > 2 SCK (target) */
  	ispDelay();
  	ISP_OUT |= (1 << ISP_RST);    /* RST high */
  	ispDelay();                
  	ISP_OUT &= ~(1 << ISP_RST);   /* RST low */
  	ispDelay();
  	if(ispTransmit==ispTransmit_hw){
    	spiHWenable();
  	} 
   }else{
  	  /* reset device */
  	ISP_OUT |= (1 << ISP_RST);   /* RST high */
  	ISP_OUT &= ~(1 << ISP_SCK);   /* SCK low */

  		/* positive reset pulse > 2 SCK (target) */
  	ispDelay();
  	ISP_OUT &= ~(1 << ISP_RST);    /* RST low */
 		ispDelay();                
  	ISP_OUT |= (1 << ISP_RST);   /* RST high */
  	ispDelay();
	}
}

void ispDisconnect() {
  
  /* set all ISP pins inputs */
  ISP_DDR &= ~((1 << ISP_RST) | (1 << ISP_SCK) | (1 << ISP_MOSI));
  /* switch pullups off */
  ISP_OUT &= ~((1 << ISP_RST) | (1 << ISP_SCK) | (1 << ISP_MOSI));

  /* disable hardware SPI */
  spiHWdisable();
}

uchar ispTransmit_sw(uchar send_byte) {

  uchar rec_byte = 0;
  uchar i;
  for (i = 0; i < 8; i++) {

    /* set MSB to MOSI-pin */
    if ((send_byte & 0x80) != 0) {
      ISP_OUT |= (1 << ISP_MOSI);  /* MOSI high */
    } else {
      ISP_OUT &= ~(1 << ISP_MOSI); /* MOSI low */
    }
    /* shift to next bit */
    send_byte  = send_byte << 1;

    /* receive data */
    rec_byte = rec_byte << 1;
    if ((ISP_IN & (1 << ISP_MISO)) != 0) {
      rec_byte++;
    }

    /* pulse SCK */
    ISP_OUT |= (1 << ISP_SCK);     /* SCK high */
    ispDelay();
    ISP_OUT &= ~(1 << ISP_SCK);    /* SCK low */
    ispDelay();
  }
    
  return rec_byte;
}

uchar ispTransmit_5x(uchar send_byte) {

  uchar rec_byte = 0;
  uchar i;
  for (i = 0; i < 8; i++) {

    /* set MSB to MOSI-pin */
    if ((send_byte & 0x80) != 0) {
      ISP_OUT |= (1 << ISP_MOSI);  /* MOSI high */
    } else {
      ISP_OUT &= ~(1 << ISP_MOSI); /* MOSI low */
    }
    /* shift to next bit */
    send_byte  = send_byte << 1;

    /* receive data */
    rec_byte = rec_byte << 1;
    if ((ISP_IN & (1 << ISP_MISO)) != 0) {
      rec_byte++;
    }

    /* pulse SCK */
    ISP_OUT |= (1 << ISP_SCK);     /* SCK high */
    ispDelay_5x();
    ISP_OUT &= ~(1 << ISP_SCK);    /* SCK low */
    ispDelay_5x();
  }
    
  return rec_byte;
}

uchar ispTransmit_hw(uchar send_byte) {
  SPDR = send_byte;  
  while (!(SPSR & (1 << SPIF)));
  return SPDR;
}

uchar ispEnterProgrammingMode() {
  uchar check;
  uchar count=16;
  chip=ATM;
  ispConnect();
  while(count--){
    ispTransmit(0xAC);
    ispTransmit(0x53);
    check=ispTransmit(0);
    ispTransmit(0);    
    if(check==0x53){
      return 0;
    }
    spiHWdisable();    
    /* pulse SCK */
    ISP_OUT|=(1<<ISP_SCK);     /* SCK high */
    ispDelay();
    ISP_OUT&= ~(1<<ISP_SCK);    /* SCK low */
    ispDelay();
    if(ispTransmit==ispTransmit_hw){
      spiHWenable();
    }  
  }
  count=16;
  chip=S5x;
  if(ispTransmit==ispTransmit_hw){
  	spiHWdisable();
  	ispTransmit=ispTransmit_5x;
  } 
  ispConnect();
  while(count--){
    ispTransmit(0xAC);
    ispTransmit(0x53);
    ispTransmit(0);
    check=ispTransmit(0);    
    if(check==0x69){
      return 0;
    }    
    /* pulse SCK */
    ISP_OUT|=(1<<ISP_SCK);     /* SCK high */
    ispDelay();
    ISP_OUT&= ~(1<<ISP_SCK);    /* SCK low */
    ispDelay();  
  }  
  return 1;  /* error: device dosn't answer */
}

uchar ispReadFlash(unsigned int address) {
	if(chip==ATM){
 	 ispTransmit(0x20|((address & 1)<<3));
  	ispTransmit(address>>9);
  	ispTransmit(address>>1);
  }else{
  	ispTransmit(0x20);
  	ispTransmit(address>>8);
  	ispTransmit(address);
  }
  return ispTransmit(0);
}


uchar ispWriteFlash(unsigned int address, uchar data, uchar pollmode) {
	if(chip==ATM){
  	ispTransmit(0x40|((address&1)<<3));
  	ispTransmit(address>>9);
  	ispTransmit(address>>1);
  	ispTransmit(data);
  	if(pollmode==0)return 0;
  	if(data == 0x7F){
    	clockWait(15);
    	return 0;
 	  }else{
    	uchar retries=30;
    	uint8_t starttime=TIMERVALUE;
    	while(retries!=0){
    	  if(ispReadFlash(address)!=0x7F)return 0;    
     	  if((uint8_t)(TIMERVALUE-starttime)>CLOCK_T_320us){
					starttime=TIMERVALUE;
					retries--;
    	  }
    	}
    }
   }else{  
    ispTransmit(0x40);
  	ispTransmit(address >> 8);
  	ispTransmit(address);
  	ispTransmit(data);
  	return 0;
   }
   return 1; /* error */
}


uchar ispFlushPage(unsigned int address, uchar pollvalue) {
  ispTransmit(0x4C);
  ispTransmit(address >> 9);
  ispTransmit(address >> 1);
  ispTransmit(0);


  if (pollvalue == 0xFF) {
    clockWait(15);
    return 0;
  } else {

    /* polling flash */
    uchar retries = 30;
    uint8_t starttime = TIMERVALUE;

    while (retries != 0) {
      if (ispReadFlash(address) != 0xFF) {
	return 0;
      };

      if ((uint8_t) (TIMERVALUE - starttime) > CLOCK_T_320us) {
	starttime = TIMERVALUE;
	retries --;
      }

    }

    return 1; /* error */
  }
    
}


uchar ispReadEEPROM(unsigned int address) {
  ispTransmit(0xA0);
  ispTransmit(address >> 8);
  ispTransmit(address);
  return ispTransmit(0);
}


uchar ispWriteEEPROM(unsigned int address, uchar data) {

  ispTransmit(0xC0);
  ispTransmit(address >> 8);
  ispTransmit(address);
  ispTransmit(data);

  clockWait(30); // wait 9,6 ms 

  return 0;
  /* 
  if (data == 0xFF) {
    clockWait(30); // wait 9,6 ms 
    return 0;
  } else {

    // polling eeprom 
    uchar retries = 30; // about 9,6 ms 
    uint8_t starttime = TIMERVALUE;

    while (retries != 0) {
      if (ispReadEEPROM(address) != 0xFF) {
	return 0;
      };

      if ((uint8_t) (TIMERVALUE - starttime) > CLOCK_T_320us) {
	starttime = TIMERVALUE;
	retries --;
      }

    }
    return 1; // error 
  }
  */

}
