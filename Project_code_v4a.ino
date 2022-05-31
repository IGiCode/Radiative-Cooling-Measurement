 /* EDITS 
 * remember to remove > temp_amb_c = 30;
 * remember to remove > pid_out = 0; and uncomment the original line
 * 
 * v3: program now writes data to sdcard, added second sample sensor.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <max6675.h>  // for temperature sensors
#include <SPI.h>  // for sd card, uses pins D10,D11,D12&D13
#include <SD.h> // for sd card
#include "RTClib.h" // for rtc
// ^^Make sure the above libraries are installed before runnning this program.

LiquidCrystal_I2C lcd(0x27,16,2); // set the LCD address to 0x27 for a 16 chars and 2 line display
RTC_DS1307 rtc;

const int chipSelect = 10; // pin for sd card writing, depends on sdcard shield/module
String dline = "";
String fname = "datalog"; //name of file to be saved. date will be appended automatically.

const int heaterpin = 3; // pwm output for heater control
int pin_interrupt_ma = 2;

//Max6675 Temp Sensor 1(a), Sample1 Temperature Sensor(heated)
int ktc1_SO = 4;
int ktc1_CS = 5;
int ktc1_CLK = 6;
MAX6675 ktc1(ktc1_CLK, ktc1_CS, ktc1_SO);

//Max6675 Temp Sensor 2(d), Ambient Temperature Sensor
int ktc2_SO = 7;
int ktc2_CS = 8;
int ktc2_CLK = 9;
MAX6675 ktc2(ktc2_CLK, ktc2_CS, ktc2_SO);

//Max6675 Temp Sensor 3(c), Sample2 Temperature Sensor(not heated)  
//int ktc3_SO = 2;
//int ktc3_CS = 2;
//int ktc3_CLK = 2;
//MAX6675 ktc3(ktc3_CLK, ktc3_CS, ktc3_SO);

double calb_state = 0;      // set to 1(for real sensor values,useful when calibratiing) or 0(for adjusted values, after calibration)
double uplim_ktc1 = 102.25; // ktc1 sensor reading(in Celsius) at boiling point of water, 100C(only adjust these set of values when calibrating the sensors)
double lolim_ktc1 = 1.00;   // ktc1 sensor reading(in Celsius) at freezing point of water, 0C ( ^^ )
double uplim_ktc2 = 103.50; // ktc2 sensor reading(in Celsius) at boiling point of water, 100C( ^^ )
double lolim_ktc2 = 2.25;   // ktc2 sensor reading(in Celsius) at freezing point of water, 0C ( ^^ )
double uplim_ktc3 = 102.25; // ktc3 sensor reading(in Celsius) at boiling point of water, 100C( ^^ )
double lolim_ktc3 = 1.00;   // ktc3 sensor reading(in Celsius) at freezing point of water, 0C ( ^^ )

double hpad_resistance = 24.8;                                   // heating pad resistance in ohms
double hpad_volt;                                                // voltage supplied to heating pad in volts
double hpad_volt_sup = 5;                                        // heating pad max voltage supply in volts
double heatingpower = (hpad_volt*hpad_volt)/hpad_resistance;     // in watts  = ((voltage supplied)^2/pad resistance)
unsigned long timetracker = -1;                                  // some variables
int htime = -1;
int htime_tot = -1;
double energy_in = 0;
double energy_tot = 0;
double temp_samp1_c_real;
double temp_samp1_c;
double temp_samp2_c_real;
double temp_samp2_c;
double temp_amb_c_real;
double temp_amb_c;

// variables for pid control
double set_temp;
double current_temp;
double pid_error;
double pid_lasterror;
double pid_intgrl;
double pid_derv;
double pid_kp = 50; //p:300, i:1000, d:300 //p:100, i:300, d:100
double pid_ki = 180;    //1.5
double pid_kd = 30;    //5
double pid_out;
int pwm_max = 255;
int pwm_min = 0;
double pid_ctrl(double set_tempp,double current_tempp);

void setup() {
  lcd.init();                                                    // initialize the lcd
  lcd.backlight();                                               // turn on the lcd backlight 

  pinMode(heaterpin, OUTPUT);                                    // initialize heater pin as an output, (For Heating Element)
  pinMode(pin_interrupt_ma, OUTPUT);
  analogWrite(heaterpin, 0);                                     // reset heater pin to zero
  
  Serial.begin(57600);
  while (!Serial) {
    // wait for serial port to connect. Needed for native USB port only
  }
  
  Serial.print("Initializing SD card... ");
  if (!SD.begin(chipSelect)) {  // see if the card is present and can be initialized:
    Serial.println("failed");
    while (1);  // don't do anything more:
  }
  Serial.println("success");
  
  if (! rtc.begin()) {
  Serial.println("Couldn't find RTC");
  Serial.flush();
  while (1); // don't do anything more
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //uncomment and upload to adjust RTC
  
  //Serial.println("Date_Time,Sample1_Temp(°C),Sample2_Temp(°C),Ambient_Temp(°C),elapsed_time(s),energy_tot,energy_in, hpower"); // serial plotter labels
}

void loop() {
  unsigned long loopTime = millis();
  DateTime now = rtc.now();
  double hpower;
  timetracker = timetracker + 1;

  digitalWrite(pin_interrupt_ma, HIGH);
  while(Serial.available()<1);
  temp_samp2_c_real = Serial.readStringUntil('\n').toDouble();
  temp_samp1_c_real = ktc1.readCelsius();                        // gets temperature reading from sample1 sensor
  temp_amb_c_real = ktc2.readCelsius();                          // gets temperature reading from ambient sensor
  digitalWrite(pin_interrupt_ma, LOW);

  if(calb_state == 1){
    temp_samp1_c = temp_samp1_c_real;
    temp_samp2_c = temp_samp2_c_real;
    temp_amb_c = temp_amb_c_real;
  } else{
    temp_samp1_c = 100*(temp_samp1_c_real - lolim_ktc1)/(uplim_ktc1 - lolim_ktc1); // Calibration correction
    temp_samp2_c = 100*(temp_samp2_c_real - lolim_ktc3)/(uplim_ktc3 - lolim_ktc3); // Calibration correction
    temp_amb_c = 100*(temp_amb_c_real - lolim_ktc2)/(uplim_ktc2 - lolim_ktc2);     // Calibration correction
  }

  //temp_amb_c = 25;

  pid_out = pid_ctrl(temp_amb_c, temp_samp1_c);
  //pid_out = 0;
  analogWrite(heaterpin, pid_out);                               // sets desired voltage output
  hpad_volt = (pid_out/pwm_max) * hpad_volt_sup;    // 0.8825*((pid_out/pwm_max) * hpad_volt_sup) - 0.0082;
  heatingpower = (hpad_volt*hpad_volt)/hpad_resistance;
  
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("S1:");                             //lcd.print("S1:");
  lcd.print(temp_samp1_c,1); //rounded to 1dp     //lcd.print(temp_samp1_c,1);
  lcd.print(" S2:");                              //lcd.print(" S2:");
  lcd.print(temp_samp2_c,1); //rounded to 1dp     //lcd.print(temp_samp2_c,1);
    
  if (pid_out > 0) {    // if Sample Temperature is LOWER THAN Ambient Temp
    
    lcd.setCursor(0, 0);
    lcd.print("AmbT: ");
    lcd.print(temp_amb_c,1); //rounded to 1dp     //lcd.print(temp_amb_c,1);
    lcd.write(char(223));
    lcd.print("C");
    lcd.setCursor(14, 0);
    lcd.print("H");

    hpower = heatingpower;
    htime = htime + 1;
    htime_tot = htime_tot + 1;
    energy_in = energy_in + (hpower*1);       // energy_in = energy_in + (hpower*1); energy_in = hpower*htime;
    energy_tot = energy_tot + energy_in;  // energy_tot = energy_tot + energy_in; energy_tot = hpower*htime_tot;
  } 
  else {
    
    lcd.setCursor(0, 0);
    lcd.print("AmbT: ");
    lcd.print(temp_amb_c,1); //rounded to 1dp     //lcd.print(temp_amb_c,1);
    lcd.write(char(223));
    lcd.print("C");

    hpower = heatingpower;
    htime = 0;
    htime_tot = htime_tot;
    energy_in = hpower*htime;   // energy_in = energy_in + (hpower*1); energy_in = hpower*htime;
    energy_tot = energy_tot + energy_in;    // energy_tot = energy_tot + energy_in; energy_tot = energy_tot;
  }

  dline = now.timestamp(DateTime::TIMESTAMP_DATE)+' '+now.timestamp(DateTime::TIMESTAMP_TIME)+','
              +String(temp_samp1_c)+','+String(temp_samp2_c)+','+String(temp_amb_c)+','+String(timetracker)
              +','+String(energy_tot)+','+String(energy_in)+','+String(hpower);
  
  Serial.println(dline);

  File dataFile = SD.open(fname + ".txt", FILE_WRITE);
  if (dataFile) { // if the file is available, writes to it:
    dataFile.println(dline);
    dataFile.close();
    lcd.setCursor(15, 0);
    lcd.print("s");
  }
  else {  // if the file isn't open, pops up an error:
    Serial.println("error opening file");
    lcd.setCursor(15, 0);
    lcd.print("n");
  }

  delay(1000UL + loopTime - millis());                           //delays for an appropriate time for the serial plotter's x-axis to be in seconds
}

double pid_ctrl(double set_tempp,double current_tempp){
  set_temp = set_tempp;
  current_temp = current_tempp;

  pid_error = set_temp - current_temp;
  pid_intgrl = pid_intgrl + pid_error;
  pid_derv = pid_error - pid_lasterror;
  
  pid_out = (pid_kp * pid_error) + (pid_ki * pid_intgrl) + (pid_kd * pid_derv);

  if (pid_out > pwm_max){
    pid_out = pwm_max;
    pid_intgrl = pid_intgrl - pid_error;
  }
  else if (pid_out < pwm_min){
    pid_out = pwm_min;
    pid_intgrl = pid_intgrl - pid_error;
  }
  
  pid_lasterror = pid_error;

  return pid_out;
}
