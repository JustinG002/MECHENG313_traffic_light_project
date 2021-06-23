//#include <Arduino.h>
#include <avr/io.h> //for tinker cad
//#include <avr/iom328p.h> //for VS code
//#include <stdio.h>
//#include <stdlib.h>
#include <avr/interrupt.h>
//#include <HardwareSerial.h>
//#include <util/delay.h>

//Global Variables

volatile uint8_t trafficLightCameraTrigged = 0;   //record the number of time the traffic light camera has been triggerd
volatile uint8_t trafficLightControlSignal = 0;   //the state of the traffic light (0 for green, 1 for yellow, 2 for red, 3 for OFF)
volatile uint8_t ledFlash = 0;                    //used to control the pulse of white LED after each traffic light camera trigger
volatile uint8_t timer0OVFCounter = 0;            //record the number of time timer 0 has been overfloed. Used to control flashing of white LED

//INT0 IO driven interrupt
//function: update PWM output on OC1A up on traffic light camera has been triggerd

ISR(INT0_vect) {

  //camera only get triggered when traffic light is red (eg. trafficLightControlSignal is 2)
  if (trafficLightControlSignal == 2) {

    trafficLightCameraTrigged++;
    ledFlash = 3;


    //update PWM output, only update id OCR1A is smaller than 62500(1sec) less than 100 Cars
    if (OCR1A < 62500) {
      OCR1A = OCR1A + 625;

    }

    //initciate flashing of white LED
    PORTD |= (1 << PORTD4);   //turn on white LED
    TCNT0 = 0;                //reset timer 0
    timer0OVFCounter = 0;     //reset overflow counter
    OCR0A = 255;              //set OCR0A for timer 0

  }

}


//Timer 0 CTC on OCR0A interrupt
//Function: For White LED flashing, Period .25s, 50% duty cycle

ISR(TIMER0_COMPA_vect) {

  cli(); //Disable all interrupt

  //amounts of time it hits COMPA because 8bit isnt enough to count for .125s
  timer0OVFCounter = timer0OVFCounter + 1;

  //set OCR0A(top value) to 161 for the next CTC compare so it is the .125s
  if (timer0OVFCounter == 7) {

    OCR0A = 161;

  }

  //every 8 timer 0 overflow, it has been 0.125s
  else if (timer0OVFCounter >= 8) {

    OCR0A = 255; //reset to max
    timer0OVFCounter = 0; //reset overflow counter

    //every loop -1 to ledFlash
    if (ledFlash != 0) {

      ledFlash = ledFlash - 1;

    }

    //control yhe flashing of white LED
    //if Ledflash is an even number turn off white LED
    if ((ledFlash % 2) == 0) {

      PORTD &= ~(1 << PORTD4); //turn off the white LED

    }
    //else turn on LED
    else {

      PORTD |= (1 << PORTD4); //turn on the white LED

    }


  }

  sei();//enable all interrpts
}

//Timer 1 overflow
/*
  Traffic signal on 1s cycle mode
*/
ISR(TIMER1_OVF_vect) {

  cli(); //disable all interrpts

  //update the mode by 1 if the count is 0/1
  if (trafficLightControlSignal < 2) {
    

    trafficLightControlSignal++;

  }
  //goes from 2 -> 0
  else {

    trafficLightControlSignal = 0;

  }

  //signal=0 Green on Y&R off, signal=1 Yellow on G&R off, signal=2 Red on Y/G off
  if (trafficLightControlSignal == 0) {

    PORTD |= (1 << PORTD5),  PORTD &= ~(1 << PORTD6) & ~(1 << PORTD7);

  }
  else if (trafficLightControlSignal == 1) {

    PORTD |= (1 << PORTD6),  PORTD &= ~(1 << PORTD5) & ~(1 << PORTD7);

  }
  else {

    PORTD |= (1 << PORTD7),  PORTD &= ~(1 << PORTD5) & ~(1 << PORTD6);

  }

  sei(); //enable all interrpts
}



int main(void) {

  cli(); //enable all interrpts
  //pin setup
  //make sure all pins are plug to the right location
  DDRB |= (1 << DDB1);                                //output pin: PWM Wave, OC1A pin 9
  DDRD &= ~(1 << DDD2);                               //Input Pins: button (2) for INT0 IO dirven interrupt
  DDRD |= (1 << DDD5) | (1 << DDD6) | (1 << DDD7) ;   //LED pins Red(7) | Yellow(6) | Green(5)
  DDRD |= (1 << DDD4);                                //Output pins: White LED (4)

  //external interrupt INT0 pin2, trigger on rising edge
  EICRA |= (1 << ISC01) | (1 << ISC00);   //set trigger on rising edge
  EIMSK |= (1 << INT0);                   //enable INT0 interupt mask

  //setup Timer 1 Interupt Masks
  TCCR1A |= (1 << COM1A1);                // Set OC1A on Compare Match, clear OC1A at BOTTOM
  TIMSK1 |= (1 << TOIE1);                 //enable overflow interrupt
  TCCR1A |= (1 << WGM11);                 //set fast PWM mode with ICR1 as TOP
  TCCR1B |= (1 << WGM12) | (1 << WGM13);  //set fast PWM mode with ICR1 as TOP
  TCCR1B |= (1 << CS12);                  //set 256 prescaler on timer 1

  //setting up compA on Timer 1
  ICR1 = 62500;   // set TOP to 1 second
  OCR1A = 0;      // COMPA at 0 originally
  TCNT1 = 0;      //inticaite timer 1 to 0

  //setup Timer 0 CTC interrupt
  TCCR0A |= (1 << WGM01);               //choose ctc mode
  TCCR0B |= (1 << CS02) | (1 << CS00);  //set 1024 prescaler
  TIMSK0 |= (1 << OCIE0A);              //enable comp A interrupt interrupt
  TCNT0 = 0;                            //inicitare timer 0 to 0
  OCR0A = 255;                          //set match for Timer 0 to 255


  sei(); //enable all interrpts

  //super loop
  while (1) {

  }

}
