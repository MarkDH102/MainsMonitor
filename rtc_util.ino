/*
** Hobbytronics rtc_util
** DS1307/ DS1338 RTC utility functions to Read date/time as string
** and to set date/time from a string
*/

#define RTC_ADDRESS 0x68

uint8_t _second;
uint8_t _minute;
uint8_t _hour;
uint8_t _dayOfWeek;
uint8_t _date;
uint8_t _month;
uint8_t _year;

byte bcdToDec(byte val)  
{
  // Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}

byte decToBcd(byte val)
{
  // Convert normal decimal numbers to binary coded decimal
  return ( (val/10*16) + (val%10) );
}

void resetSecond(void)
{
  _second = 0;
}
byte getSecond()
{
  return _second;
}
byte getMinute()
{
  return _minute;
}
byte getHour()
{
  return _hour;
}
byte getDayOfWeek()
{
  return _dayOfWeek;
}
byte getDate()
{
  return bcdToDec(_date);
}
byte getMon()
{
  return _month;
}
byte getYears()
{
  return _year;
}

void markReadDate(void)
{
  // Reset the register pointer
  Wire.beginTransmission(RTC_ADDRESS);

  byte zero = 0x00;
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(RTC_ADDRESS, 7);

  _second = Wire.read();
  _minute = Wire.read();
  _hour = Wire.read() & 0b111111; //h (24hr)
  _dayOfWeek = Wire.read();
  _date = Wire.read();
  _month = Wire.read();
  _year = Wire.read();
}

void readDate(char datetime[]){
  // Read the Date and Time from RTC and set the char array passed in
  
  // Reset the register pointer
  Wire.beginTransmission(RTC_ADDRESS);

  byte zero = 0x00;
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(RTC_ADDRESS, 7);

  int second = bcdToDec(Wire.read());
  int minute = bcdToDec(Wire.read());
  int hour = bcdToDec(Wire.read() & 0b111111); //24 hour time
  int weekDay = bcdToDec(Wire.read()); //1-7 -> sunday - Saturday
  int monthDay = bcdToDec(Wire.read());
  int month = bcdToDec(Wire.read());
  int year = bcdToDec(Wire.read());

  //print the date EG  DD/MM/YY HH:MM:SS 
  sprintf(datetime,"%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d", monthDay, month, year, hour, minute, second);

}

void markSetDate(void)
{
  Wire.beginTransmission(RTC_ADDRESS);

  // Stop oscillator
  Wire.write(0x00);
  // The minute, second, _dayOfWeek, _date, _month and _year values are up to date in BCD read straight from the clock
  Wire.write(_second);
  Wire.write(_minute);
  Wire.write(_hour);    
  Wire.write(_dayOfWeek);
  Wire.write(_date);
  Wire.write(_month);
  Wire.write(_year);
  // Start oscillator
  Wire.write(0x00);

  Wire.endTransmission();  
}


void setHour(uint8_t hour)
{
  Wire.beginTransmission(RTC_ADDRESS);

  // Stop oscillator
  Wire.write(0x00);
  // The minute, second, _dayOfWeek, _date, _month and _year values are up to date in BCD read straight from the clock
  Wire.write(_second);
  Wire.write(_minute);
  Wire.write(decToBcd(hour));    
  // Sunday is a 1 - we only call this from the DST routine which always happens on a Sunday
  Wire.write(1);
  Wire.write(_date);
  Wire.write(_month);
  Wire.write(_year);
  // Start oscillator
  Wire.write(0x00);

  Wire.endTransmission();  
}

void setDateTime(char datetime[]){
  
  // Takes a char string of the format
  // DD/MM/YY HH:MM:SS
  // and uses this to set the date/time of the RTC

  byte second;
  byte minute;
  byte hour;
  // Default to Sunday
  byte weekDay = 1;
  byte monthDay;
  byte month;
  byte year;

  monthDay = datetime[0]-48;
  monthDay = monthDay*10 + datetime[1]-48;   
     
  month = datetime[3]-48;
  month = month*10 + datetime[4]-48; 
     
  year = datetime[6]-48;
  year = year*10 + datetime[7]-48;     
      
  hour = datetime[9]-48;
  hour = hour*10 + datetime[10]-48;   
    
  minute = datetime[12]-48;
  minute = minute*10 + datetime[13]-48; 
    
  second = datetime[15]-48;
  second = second*10 + datetime[16]-48;     
  
  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write(0x00); //stop Oscillator

  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(weekDay));
  Wire.write(decToBcd(monthDay));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));

  Wire.write(0x00); //start 

  Wire.endTransmission();  

}
