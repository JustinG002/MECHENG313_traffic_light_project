//#include <Arduino.h>
#include <avr/io.h> //for tinker cad
//#include <avr/iom328p.h> //for VS code
//#include <stdio.h>
//#include <stdlib.h>
#include <avr/interrupt.h>
//#include <HardwareSerial.h>

//global Variables
volatile uint16_t flashingFrequency = 0;                                                                            //flashing frequency of configuration LED (white LED)
volatile uint8_t trafficLightControlSignal = 0;                                                                     //the state of the traffic light (0 for green, 1 for yellow, 2 for red, 3 for OFF)
volatile int configurationLightLastState = 0, configurationLightControlSignal = 1, configurationLightCounter = 0;   //for configuration light (white LED)
volatile uint8_t modeSelection = 0;  //the running state of the system 0 for traffic light mode(alternating between RGB), 1 for configuration mode(Red LED ON, and flashing of white LED depend on ADC value)

//Timer 1 CTC interrupt
//function: controll the light change period of the traffic lights (RGB LED) and conguration light (white LED)
ISR(TIMER1_COMPA_vect) {

  cli();//disable all interrupts

  //traffic light mode: modeSelection = 0
  //for values of trafficLightControlSignal, 0 for green, 1 for yellow, 2 for red, 3 for OFF
  if (modeSelection == 0) {

    configurationLightControlSignal = 1; //turn on configuration LED (white)

    if (trafficLightControlSignal != 2) { //if it's not red(2) yet

      trafficLightControlSignal++;

    }
    else {

      trafficLightControlSignal = 0; //reset the signal if it's red(2)

    }

  }

  // In configuration mode
  // for value of configurationLightControlSignal, 0 for OFF and 1 for ON
  if (modeSelection == 1) {

    trafficLightControlSignal = 2; //Put Red Light on

    //Flashing White LED
    //flashign period is depend on the ADC value last for 1 second, then it would be OFF for 2 seconds before the next flash
    if (configurationLightLastState == 0) { //OFF period: lastState = 0

      configurationLightControlSignal = 0;//set white LED signal to OFF

      if (configurationLightCounter > 0) {

        configurationLightCounter = configurationLightCounter - 1;

      }

      //update lastState variable to switch to white LED flashing period
      if (configurationLightCounter == 0) {

        configurationLightLastState = 1;

      }

    }

    else if (configurationLightLastState == 1) { //Flashing period: lastState = 1


      //this is used for flashing of WHITE LED
      if (configurationLightControlSignal == 1) {
        configurationLightControlSignal = 0;
      } else {
        configurationLightControlSignal = 1;
      }

      //Determining off time for the flashing
      configurationLightCounter += 2;
      if (configurationLightCounter == (flashingFrequency * 4)) {
        configurationLightLastState = 0;
      }
    }
  }

  sei(); //enable all interrupts

}

//INT0 IO driveen interrupt(pin 2)
//function: switch the system between configuration mode and traffic light mode by updating modeSelection and initiciate/disable ADC conversion based on modeSelection      
// for value of modeSelection: 1 configuration mode and 0 traffic light mode
ISR(INT0_vect) {

  cli(); //disable all interrupts

  if (modeSelection == 0) { //switch to configuration mode

    modeSelection = 1; //mode selction -> enter configuration mode
    PORTB &= ~(1 << PORTB1) & ~(1 << PORTB2); //turn off any traffic light LEDs other than RED that are still ON

    //enable and start ADC conversion on button press
    ADCSRA |= (1 << ADEN);  //enable ADC
    ADCSRA |= (1 << ADSC);  //start ADC conversion
    TIMSK0 |= (1 << TOIE0); //enable timer 0 overflow interrupt for ADC conversion

    configurationLightLastState = 1; configurationLightCounter = 0; //Flashing Configuration(White) LED settings

    OCR1A = 15625 / flashingFrequency;  //update the frequency of the Configuration(white) LED flashing, default to 1hz until the first ADC conversion
    TCNT1 = 0;                          //reset timer 1 to 0

  }
  else { //switch back to Traffic Light mode

    modeSelection = 0; //mode selction -> enter traffic light mode

    ADCSRA &= ~(1 << ADEN); //disable ADC
    PORTB &= ~(1 << PORTB0); //turn off white LED.
    TIMSK0 &= ~(1 << TOIE0); //disable timer 0 overflow interrupt

    OCR1A = 15625 * flashingFrequency; //update the switching period for traffic light LEDs based on the lastest ADC read out
    TCNT1 = 0; //reset timer 1

  }

  sei(); //enable interrupts

}

//Timer0 overflow interrupt
//function: this is used to control the frequency of ADC conversion, it's about every 0.02s
ISR(TIMER0_OVF_vect) {

  //Convert ADC when in Configuration Mode.
  if (modeSelection == 1) {
    ADCSRA |= (1 << ADSC); //start ADC conversion
  }

}



ISR(ADC_vect) {

  cli();//disable all interrupts

  int newFlashingFrequency = ((ADC / 256) + 1); //update the flashing frequency of the LEDs


  if (newFlashingFrequency != flashingFrequency) { //only reset timer 1, and change TOP of timer 1 if the flashing frequency has been changed

    flashingFrequency = newFlashingFrequency;   //update flashingFrequency to the new state

    if (modeSelection == 1) { //while in Configuration Mode

      OCR1A = 15625 / (flashingFrequency * 2);  //update white LED flashing frequency
      configurationLightCounter = 0;            //configuration mode couter

    }

    TCNT1 = 0; //reset timer/counter 1
  }

  sei(); //enable interrupt
}


int main(void) {

  cli();//disable all interrupts

  //set up, check all components are connect to the right ports
  DDRD &= ~(1 << DDD2);                                           //Input pin: interrupt button, button connect to pin 2 on Arduino uno
  DDRC &= ~(1 << DDC0);                                           //Input pin: ADC, ADC middle pin connect to A0 on Arduino Uno
  DDRB |= (1 << DDB1) | (1 << DDB2) | (1 << DDB3) | (1 << DDB0);  //Output pin: LEDs, B1(pin 9) green | B2(pin 10) yellow | B3(pin 11) red | B0(pin 8) WHite on Audrino Uno


  //interrupt(INT0) button D2 port, pin2
  EICRA |= (1 << ISC01) | (1 << ISC00);   //interrupt on rising edge
  EIMSK |= (1 << INT0);                   //enable int0 interupt, pin 2

  //ADC will be connect to A0 on the Audrin Uno, which is PC0
  //ADC set up, the ports are defaulted to PC0 ADC port
  ADMUX = 0x0;
  ADMUX |= (1 << REFS0);                                  //reference voltage AVcc (5V)
  ADCSRA |= (1 << ADEN) | (1 << ADIE);                    //enable ADC | ADC interrupt enable
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);   //set a prescaler for ADC, as when ADC is too fast it does not update

  //set up ctc interrupt on timer 1
  //function: the flashing frequency of white LED, and switch period of traffic light LED
  TCCR1A = 0, TCCR1B = 0, TIMSK1 = 0;
  TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);   //enable CTC mode on timer/counter 1 | set prescaler to 1024
  TIMSK1 |= (1 << OCIE1A);                              //interrupt enable on compare match to OCR1A
  TCNT1 = 0;                                            //initiate timer/counter to 0
  OCR1A = 15625;                                        //top of CTC match 1s, calc: (1s * prescaler[1024] / 16e || default to 1 sec)

  //set up overflow interrupt on timer 0
  TCCR0A = 0; //normal operation
  TCCR0B |= (1 << CS02) | (1 << CS00);    //set 1024 prescaler on timer 0
  TIMSK0 |= (1 << TOIE0);                 //enable over flow interrupt mask
  TCNT0 = 0;                              //initiate timer 0 to 0;

  sei();//enable interrupts

  while (1) {

    //traffic light logic
    //0 for green, 1 for yellow, 2 for red, 3 for OFF
    if (trafficLightControlSignal == 0) {
      PORTB |= (1 << PORTB1),  PORTB &= ~(1 << PORTB2) & ~(1 << PORTB3);
    } else if (trafficLightControlSignal == 1) {
      PORTB |= (1 << PORTB2),  PORTB &= ~(1 << PORTB1) & ~(1 << PORTB3);
    } else if (trafficLightControlSignal == 2) {
      PORTB |= (1 << PORTB3),  PORTB &= ~(1 << PORTB1) & ~(1 << PORTB2);
    }

    //need to check this, when INT0 disabled and enabled
    //diable external interrupt 0, and only enable if traffic light LED is red OR it's in configuration mode
    if (trafficLightControlSignal == 2) {
      EIMSK |= (1 << INT0); //enable external interrupt int0
    } else if (modeSelection == 1) {
      EIMSK |= (1 << INT0); //enable external interrupt int0
    } else {
      EIMSK &= ~(1 << INT0); //disable configration mode change by disable external interrupt on pin___
    }


    //configuration light logic
    //0 for OFF, 1 for ON
    if (configurationLightControlSignal == 0) {
      PORTB &= ~(1 << PORTB0);  //Turn configuration Light(white LED) on
    }
    else if (configurationLightControlSignal == 1) {
      PORTB |= (1 << PORTB0);   //Turn configuration light(white LED) off
    }
  }
}
