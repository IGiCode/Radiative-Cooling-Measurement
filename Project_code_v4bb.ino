 /* EDITS 
 *
 * v3: program now writes data to sdcar(nf), added second sample sensor.
 */

#include <Wire.h>
#include <max6675.h>  // for temperature sensors
// ^^Make sure the above libraries are installed before runnning this program.

//Max6675 Temp Sensor 3(c), Sample2 Temperature Sensor(not heated)  
int ktc3_SO = 5;
int ktc3_CS = 6;
int ktc3_CLK = 7;
MAX6675 ktc3(ktc3_CLK, ktc3_CS, ktc3_SO);

//double temp_samp2_c_real;
int pin_interrupt_sl = 2;

void setup() {
  Serial.begin(57600);
  while (!Serial) {
    // wait for serial port to connect. Needed for native USB port only
  }

  attachInterrupt(digitalPinToInterrupt(pin_interrupt_sl), toggle, RISING);
}

void loop() {
  delay(500); // don't do anything
}

void toggle() {
  //temp_samp2_c_real = ktc3.readCelsius();
  //Serial.println(temp_samp2_c_real);
  Serial.println(ktc3.readCelsius());
  Serial.flush();
}
