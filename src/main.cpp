#include <Arduino.h>
#include <SPI.h>
#include "RF24.h"
#include "DHT.h"
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>

RF24 radio(PIN_A2, PIN_A3); // PIN radio
const byte address[6] = "sond1"; // Adresse transmission

volatile byte counterWD = 0;  // Nombre répétition watchdog

DHT dht(PIN_A1, DHT22); // DHT PIN


ISR(WDT_vect)
{
  wdt_disable();        // Désactive le watchdog
  counterWD ++;          // Incrémente le nb de répétition               
} 

void resetWatchDog () 
{
  wdt_reset();
  MCUSR = 0;
  WDTCSR |= (1<<WDCE) | (1<<WDE);  // Timing pour modification du wdt, voir doc
  WDTCSR = (1<<WDIE) | (1<<WDP3) | (1<<WDP0); // set WDIE (Interrupt only, no Reset) and 8 second TimeOut                                             
  wdt_reset ();     // Reset watchdog avec les paramètres voulus
} 

void sleep_now(){
   set_sleep_mode (SLEEP_MODE_PWR_DOWN);    //Régle en mode de sommeil profond                     
   noInterrupts ();                        // Désactive les interruptions par précaution
   resetWatchDog ();                       // Reset le watchdog
   sleep_enable ();                        // Autorise le système à entrer en sommeil
   interrupts ();                          // Active les interruptions
   sleep_cpu ();                           // Envoie le système en sommeil 

   // Réveil aprés l'interruption du watchdog
   sleep_disable ();                       // Réveil du système
  
}

void setup() {
  radio.begin();                           // Démarre la communication sans fils
  radio.openWritingPipe(address);          // Règle l'adresse où sera transmis les données
  radio.setPayloadSize(16);                // Règle la taille de transmission
  radio.setPALevel(RF24_PA_MAX);           // Règle la distance de transmission
  ADCSRA = 0;                              // Désactive ADC
  ACSR |= (1<<ACD);                        // Désactive le comparateur analogique
  CLKPR = (1<<CLKPCE);                     // Active le prescaler
  CLKPR = (1<<CLKPS0);                     // Divise l'horloge par 2
  radio.powerDown();
  resetWatchDog ();
}


void loop() {
  if (counterWD == 7)            // if the WDog has fired 7 times......
  {
    dht.begin();
    float h = dht.readHumidity(); // on lit l'humidité
    float t = dht.readTemperature(); //on lit la temperature

    byte dataArray[14] = {
      ((uint8_t*)&h)[0],
      ((uint8_t*)&h)[1],
      ((uint8_t*)&h)[2],
      ((uint8_t*)&h)[3],
      ((uint8_t*)&t)[0],
      ((uint8_t*)&t)[1],
      ((uint8_t*)&t)[2],
      ((uint8_t*)&t)[3],
      address[0],
      address[1],
      address[2],
      address[3],
      address[4],
      address[5],
   };

    radio.powerUp();
    radio.write(dataArray, sizeof(dataArray)); 
    radio.powerDown();
    counterWD = 0;
  }

  sleep_now();

}