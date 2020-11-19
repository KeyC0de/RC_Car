/*** RC Car, using MSP430G2553 USCI module in UART mode, bluetooth module HC-06. by Nick J. Lazaridis ***/

#include <msp430g2553.h>
	
#define EN1 BIT6
#define EN2 BIT0
#define RXD BIT1 // P1.1 is the UART receive pin
#define TXD BIT2 // P1.2 is the UART transmit pin

void dutyCycle(unsigned int i, unsigned int j);

volatile unsigned char rxData = 0;	// variable that will be used to help with the UART configuration
	
void main(void){
  WDTCTL = WDTPW + WDTHOLD;
  
  /*** Clock System Configuration ***/
  DCOCTL = 0;
  BCSCTL1 = CALBC1_1MHZ;
  DCOCTL = CALDCO_1MHZ; // Make sure that Submain clock is selected with DCO Clock source running at 1Mhz
  // With these settings a clock cycle needs 1/1,000,000Hz=1us to be completed.
  
  /*** Pin Configuration ***/
  P1OUT &= ~(EN1 | EN2 | BIT1 | BIT2 | BIT3 | BIT5 | BIT7 | BIT4);
  P1DIR |= EN1 | EN2 | RXD | TXD | BIT3 | BIT5 | BIT7 | BIT4;
  P1SEL |= EN1;      // Port P1.6. is set to be an output port for TimerA Compare circuit, in order to generate PWM pulses with duty cycle defined by the formula: TACCR1/TACCR0.
  P1SEL |= (RXD | TXD); 
  P1SEL2 |= (RXD | TXD); // Sets third secondary function for pins P1.1 and P1.2, which is used to receive and trasmit data to the UART module, from the bluetooth adapter
  
  /*** Configure USCI(-A) module for UART mode ***/
  UCA0CTL1 |= UCSWRST | UCSSEL_2; // Before we configure the USCI module for UART mode it's a good idea to disable it. Also we choose SMCLK as the clock source for the USCI-A module
  UCA0BR0 = 104;		  // I want baud rate 9600(bps). So Baud Rate Control Register 0 is set with 104, which means SMCLK Frequency / UCA0BR0 = 1,000,000/104 = 9615bps
                                  // Since 9615 is a fluctuation of less than 1% from 9600 all is fine
  UCA0BR1 = 0;			  // Clears Baud Rate Control Register 1 (the prescaler register)
  UCA0MCTL |= UCBRS_1;		  // (or UCBRS0) Configure UART Modulation. 1 start bit, 8 data bits, no address bit, no parity bits used,  1 stop bit
  UCA0CTL1 &= ~UCSWRST;		  // Initialize USCI state machine
  IE2 |= UCA0RXIE;		  // Enable USCI_A0 RX(only) interrupt, which lies within the IE2 SFR register. The flag UCA0RXIFG is set when data is received into the UCA0RXBUF.
                                  // This happens in the USCI ISR and the flag is reset automatically.
  
  /*** Timer A Configuration ***/
  TACTL |= TASSEL_2 | MC_1 | ID_3; // select TimerA source SMCLK, set mode to up-counting and divide Timer A input clock by 8
  // Submain clock is sourced by DCOClock with a default frequency of 1MHz as specified above. Therefore the Frequency of Timer A is 1,000,000/8 = 125,000Hz
  // a (Timer A) clock cycle needs 1/125,000Hz = 8us = 0.000008s to be completed.
  TACCTL1 = OUTMOD_7 | CCIE; // select timer compare mode 
  
  volatile unsigned int k,i;
  
  __bis_SR_register(LPM0_bits + GIE);	// Enter LPM0 mode (CPU OFF), with interrupts enabled

  /*** Main Program, depending on the ASCII character received the MCU responds appropriately ***/
  while(1){
    switch (rxData){
    case '1':                   // ASCII:1, Hex:31 -> Forward Full speed
      P1SEL |= EN1;
      P1OUT |= EN1; // Enable Motor 1 (controlling the Back wheels)
      P1OUT |= BIT5;
      P1OUT &= ~BIT3; // 1-0 FORWARD
      dutyCycle(100,100); // Set Forward Motor Speed = Duty Cycle [= TACCR1/TACCR0] = MAX
      // t0 = 100*0.000008s = 0.0008s = 0.8ms : time it takes for the count to the value (100 given Timer A frequency of 15,630Hz). The same is applied to calculate the time elapsed according to the number given
      break;
    case '2' :                  // ASCII:2, Hex:32 -> Forward 75% speed
      P1SEL |= EN1;
      P1OUT |= EN1;
      P1OUT |= BIT5;
      P1OUT &= ~BIT3;
      dutyCycle(75,100);
      break;
    case '3' :                 // ASCII:3, Hex:33 -> Forward 50% speed
      P1SEL |= EN1;
      P1OUT |= EN1;
      P1OUT |= BIT5;
      P1OUT &= ~BIT3;
      dutyCycle(50,100);
      break;
    case '4' :                  // ASCII:4, Hex:34 -> Forward 25% speed
      P1SEL |= EN1;
      P1OUT |= EN1;
      P1OUT |= BIT5;
      P1OUT &= ~BIT3;
      dutyCycle(25,100);
      break;
    case '5' :                  // ASCII:5, Hex:35 -> Backwards 25% speed
      P1SEL |= EN1;
      P1OUT |= EN1;
      P1OUT &= ~BIT5;
      P1OUT |= BIT3; // 0-1 BACKWARD
      dutyCycle(25,100);
      break;
    case '6' :                  // ASCII:6, Hex:36 -> Backwards 50% speed
      P1SEL |= EN1;
      P1OUT |= EN1;
      P1OUT &= ~BIT5;
      P1OUT |= BIT3;
      dutyCycle(50,100);
      break;
    case '7' :                  // ASCII:7, Hex:37 -> Backwards 75% speed
      P1SEL |= EN1;
      P1OUT |= EN1;
      P1OUT &= ~BIT5;
      P1OUT |= BIT3;
      dutyCycle(75,100);
      break;
    case '8' :                  // ASCII:7, Hex:38 -> Backwards MAX speed
      P1SEL |= EN1;
      P1OUT |= EN1;
      P1OUT &= ~BIT5;
      P1OUT |= BIT3;
      dutyCycle(100,100);
      break;
    case 'r' :                  // ASCII:r, Hex:72 -> Turns Right
      P1OUT |= EN2;  // Enable Motor 2 (controlling the Front wheels)
      P1OUT |= BIT7;
      P1OUT &= ~BIT4; // 1-0 RIGHT
      break;
    case 'l' :                  // ASCII:l, Hex:6C -> Turns Left
      P1OUT |= EN2;  // Enable Motor 2 (controlling the Front wheels)
      P1OUT &= ~BIT7;
      P1OUT |= BIT4; // 0-1 LEFT
      break;
    case 'S' :                  // ASCII:S, Hex:53 -> Active Stop (everything)
      P1SEL &= ~EN1;
      P1OUT &= ~(EN1 | EN2); // Turn off both Motor Actuators
      break;
    case 's' :                  // ASCII:s, Hex:73 -> Front Wheel( Motor2) Straight
      P1OUT &= ~EN2; // Disables Motor2
    case 'E' :                  // ASCII:E , Hex:45 -> Execute Car Exercise Final Question
      P1SEL |= EN1;
      P1OUT |= EN1 | EN2;  // Enable MOTOR 1 and MOTOR 2
 
      P1OUT |= BIT5;
      P1OUT &= ~BIT3; // 1-0 -> FORWARD - Motor 1
      dutyCycle(25,100); // Set Forward Motor Speed 1/4 of max
    
      for(k=0;k<36000;k++);
      for(k=0;k<36000;k++); // ~1 second has passed
      P1OUT |= BIT7;
      P1OUT &= ~BIT4; // 1-0 -> Turns RIGHT - Motor 2
      for(k=0;k<6000;k++); // small delay to realise and see the front wheel turn
      P1OUT |= BIT4; // 1-1 Active Stop (front) MOTOR 2
      for(k=0;k<30000;k++);
      for(k=0;k<36000;k++); // ~2 seconds have passed
  
      dutyCycle(100,100); // Set Forward Motor Speed at maximum
  
      for(k=0;k<36000;k++); // ~2 and a half seconds have passed
      P1OUT &= ~BIT7; // 0-1 -> TURN LEFT - Motor 2
      for(k=0;k<6000;k++); // small delay to realise and see the front wheel turn // just enough to note the turning of the front wheel. Putting a smaller delay the turning of the wheel would not be noticeable.
      P1OUT &= ~BIT4; // 0-0 Active Stop (front) MOTOR 2
      for(k=0;k<30000;k++); // ~3 seconds have passed
  
      P1SEL &= ~EN1;
      P1OUT &= ~(EN1 | EN2); // Turn off Led's
    case 'e' :                  // ASCII:e, Hex:65 -> Execute Car Exercise First Question
      P1SEL &= ~EN1; // I use this here to disable Duty Cycle in case it exists
      P1OUT |= EN1; // Enable MOTOR1
 
      P1OUT |= BIT5;
      P1OUT &= ~BIT3; // 1-0 -> FORWARD - Motor 1
      for(i=0;i<36000;i++);
      for(i=0;i<36000;i++);
      P1OUT |= BIT3; // 1-1 -> Active Stop - Motor 1
      for(i=0;i<36000;i++);  
      for(i=0;i<36000;i++);	// Motor 1
      P1OUT &= ~BIT5; // 0-1 -> BACKWARD - Motor 1
      for(i=0;i<36000;i++);
      for(i=0;i<36000;i++);
      P1OUT &= ~BIT3; // 0-0 -> Active Stop - Motor 1

      P1OUT &= ~EN1;
      
    default:
      break;
    }
       
    __bis_SR_register(LPM0_bits);	// Enter LPM0 mode again (with interrupts enabled)
  }
  
}

 /*** Timer A PWM-Configuration Subroutine ***/
void dutyCycle(volatile unsigned int i, volatile unsigned int j){
  TACCR0 = j; // Set TACCRO register -> count to j 
  TACCR1 = i; // Timer A Interrupt is called when counter reaches TACCR1 i value
}
 
/*** Timer A ISR ***/
#pragma vector=TIMER0_A1_VECTOR
__interrupt void Timer_A(void){
  TACCTL1 &= ~CCIFG;
}

/*** USCI-A Module, UART mode ISR ***/
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCIA0RX_ISR(void){
  rxData = UCA0RXBUF; // Stores char received through wireless bluetooth communication, in rxData variable
  __bic_SR_register_on_exit(LPM0_bits); // Wakes CPU in order to serve incoming order
}
