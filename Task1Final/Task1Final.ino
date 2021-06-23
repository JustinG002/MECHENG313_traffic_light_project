#include <avr/io.h> //for tinker cad
//#include <avr/iom328p.h> //for VS code
#include <avr/interrupt.h>

//global variables
uint8_t trafficLightMode = 1;

//timer 1 CTC interrupt
/*
  function: This will update the trafficLightMode variable everytime
  it's called (1s), and turn on and off the appr
      0->1->2->0 Loop
*/

ISR(TIMER1_COMPA_vect) {

  //update the mode by 1 if the count is 0/1
  if (trafficLightMode < 2) {

    trafficLightMode++;

  }

  //else if the mode is 2 reset back to 0
  else {

    trafficLightMode = 0;

  }

  //mode=0 Green on Y&R off, mode=1 Yellow on G&R off, mode=2 Red on Y/G off
  if (trafficLightMode == 0) {
    
    PORTB |= (1 << PORTB1),  PORTB &= ~(1 << PORTB2) & ~(1 << PORTB3);
    
  } else if (trafficLightMode == 1) {
    
    PORTB |= (1 << PORTB2),  PORTB &= ~(1 << PORTB1) & ~(1 << PORTB3);
    
  } else if (trafficLightMode == 2) {
    
    PORTB |= (1 << PORTB3),  PORTB &= ~(1 << PORTB1) & ~(1 << PORTB2);
    
  }

}

//main function
/*
  setting up all the masks and interupts

  Superloop to update the trafficlight
*/
int main(void) {

  cli(); //disable all interrupts during the setup

  //Setup Pins
  DDRB |= (1 << DDB1) | (1 << DDB2) | (1 << DDB3); //Output pins: LEDs, B1(pin D9) green | B2(pin D10) yellow | B3(pin D11) red

  //Setup CTC CompareMatch with OCR1A Interrupt on timer 1
  TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);   //enable CTC mode on timer/counter 1 | set prescaler to 1024
  TIMSK1 |= (1 << OCIE1A);                              //enable interrupt on compare match OCR1A
  OCR1A = 15625;                                        //Setting top of CTC mode to 1second
  TCNT1 = 0;                                            //inticiate timer 1 to 0

  sei(); //enable all interrupt

  //superloop
  while (1) {

  }
}
