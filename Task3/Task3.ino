#include <avr/io.h> //for tinker cad
//#include <avr/iom328p.h> //for VS code
#include <avr/interrupt.h>

//global variable
volatile uint32_t enterTime[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};   //store the LB1 brached time of up to 10 vehicles
volatile float elapsedTime = 0;                                  //time different between LB1 and LB2 breach
volatile uint8_t vehicleEnterCounter = 0;                           //record the vacant index in enterTime to store the enter time of next LB1 breached
volatile uint8_t vehicleExitCounter = 0;                            // record the index in enterTime of the the enter time of the next LB2 breached vehicle
volatile uint8_t vehicleEnterCounterOverflowFlag = 0;               //The flag will be raised when vehicleEnterCounter overflow, and will be cleared when vehicleExitCounter overflow
volatile uint8_t timer0OverflowCounter = 0;                         //record number of time timer0 has been overflowed for calculation of the enterTime of each vehicle, as well as time elapsed since LB1 breach
volatile uint8_t LB1BlinkCounter = 0, LB2BlinkCounter = 0;          //The flashing of LEDs on light beam breach
volatile uint16_t speedKMH = 0;                                     //the speed in Km/H of the last vehicle breached LB2

//simulate LB1
//INT0, IO dirven interrupt on pin 2
//function: record the enter time of each veicle into enterTime arry
//  and trigger red LED ON to indicated LB1 has been breached
ISR(INT0_vect) {

  cli(); //diable all interrupt

  enterTime[vehicleEnterCounter] = TCNT0 + timer0OverflowCounter * 256; //record entrer time upon LB1 breach

  //update vehicleEnterCounterOverflowFlag as vehicleEnterCounter overflow
  if (vehicleEnterCounter == 9) {
    vehicleEnterCounterOverflowFlag = 1;
  }

  vehicleEnterCounter = (++vehicleEnterCounter) % 10; //update vacant index in enterTime for next LB1 breach

  PORTB |= (1 << PORTB3); //turn ON red LED
  LB1BlinkCounter = 6; //LED stay on for 100ms

  sei(); //enable all interrupts

}

//Simulate LB2
//INT1 IO dirven interrupt on pin 3
//function: calculate the speed of the vehicle, trigger green LED on LB2 breach, and update PWM duty cycle based on speed of the vehicle
ISR(INT1_vect) {



  cli();//diable all interrupt

  //filter false trigger of LB2
  //if LB1 has been breached as many times as LB2, it will be determined as a flase breached
  if ((vehicleEnterCounterOverflowFlag * 10 + vehicleEnterCounter) >= (vehicleExitCounter + 1)) {

    //calculate the average speed of the veicle been LB1 and LB2 breach
    uint32_t currentTime = TCNT0 + timer0OverflowCounter * 256;
    elapsedTime = (currentTime - enterTime[vehicleExitCounter]) * 1024.0 / 16000000; //calculate the elsaped time between LB1 and LB2 breach
    speedKMH = ((20 / elapsedTime) * 3.6);                                           //calculate speed of the vehicle in Km/h

    //clear vehicleEnterCounterOverflowFlag upon vehicleExitCounter overflow
    if (vehicleExitCounter == 9) {
      vehicleEnterCounterOverflowFlag = 0;
    }
    
    vehicleExitCounter = (++vehicleExitCounter) % 10; //update index of the vehicle enter time of next LB2 breach

    
    //update the duty cycle of the PWM out on OC1A (pin 9)
    if (speedKMH > 100) {
      speedKMH = 100;
    }
    OCR1A = (speedKMH / 100.0) * 62500;
    TCNT1 = 0; //reset timer/counter1;


    PORTD |= (1 << PORTD6);   //turn ON green LED
    LB2BlinkCounter = 6;      //LED stay on for 100ms
   
    //Serial.print(speedKMH);
    //Serial.print("\n");
    
    sei(); //enable all interrupts
  }
}

//Timer 0 CTC interrupt
//Function: record number of times Timer 0 overflowed, and turn off green adn red LED after it's been on for 100ms
ISR(TIMER0_COMPA_vect) {

  //update overflow counter for timer 0
  timer0OverflowCounter++;

  //turn on green LED when counter is cleared to 0
  if (LB2BlinkCounter != 0) {
    LB2BlinkCounter--;
  } else {
    PORTD &= ~(1 << PORTD6); //turn off green LED
  }

  if (LB1BlinkCounter != 0) {
    LB1BlinkCounter--;
  } else {
    PORTB &= ~(1 << PORTB3); //turn off green LED
  }

}


int main(void) {
  cli();
  //setup Pins
  DDRB |= (1 << DDB1) | (1 << DDB3); //Output pin: PWM out put(pin 9) | RED LED - flash on LB1 breach(pin 11)
  DDRD |= (1 << DDD6); //Output pin: GREEN LED - flash on LB2 breach (pin 6)
  DDRD &= ~(1 << DDD2) & ~(1 << DDD3); //Input Pin: button for IO driven interuppt. INT0 is for LB1 (pin 2) | INT1 is for LB2 (pin 3)

  //INT0 (pin 2) IO driven interrupt, trigger on rising edge, correspond to LB1
  EICRA |= (1 << ISC01) | (1 << ISC00); //trigger on rising edge
  EIMSK |= (1 << INT1);                 //enable INT1 interrupt mask

  //INT1 (pin 3) IO driven interrput, trigger on rising edge, correspond to LB2
  EICRA |= (1 << ISC11) | (1 << ISC10); //trigger on rising edge
  EIMSK |= (1 << INT0);                 //enable INT0 interrupt mask

  //timer/counter 0 CTC to flash green and red LED, and used to calculate Light beam brached time of a vehicle
  TCCR0A |= (1 << WGM01);               //enable CTC mode
  TCCR0B |= (1 << CS02) | (1 << CS00);  //1024 prescaler
  TIMSK0 |= (1 << OCIE0A);              //enable interrupt on compare match OCR0A
  TCNT0 = 0;                            //initilise timer/counter0 to 0
  OCR0A = 255;                          //set TOP to 255

  //timer/counter 1 fast PWM mode for output using on OC1A (pin 9)
  TCCR1A |= (1 << COM1A1);                  //fast PWM, clear OC1A on compare made with OCR1A, set OC1A at bottom
  TCCR1A |= (1 << WGM11);                   //set fast PWM mode with ICR1 as TOP
  TCCR1B |= (1 << WGM12) | (1 << WGM13);    //set fast PWM mode with ICR1 as TOP
  TCCR1B |= (1 << CS12);                    //set 256 prescaler
  OCR1A = 0;                                //default to compare match value to 0
  ICR1 = 62500;                             //set TOP to 62500, 1 sec
  TCNT1 = 0;                                //initilise timer/counter 1 to 0

  
  sei();

  Serial.begin(9600);
  Serial.print("hello world \n");

  while (1) {

    //reset timer0OverflowCounter if there are no vehicle between LB1 and LB2
    if (vehicleExitCounter == (vehicleEnterCounterOverflowFlag * 10 + vehicleEnterCounter)) {
      timer0OverflowCounter = 0;
    }

  }
}
