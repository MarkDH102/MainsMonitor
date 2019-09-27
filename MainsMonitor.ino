// ============================================================
// TITLE:  MainsMonitor.c
// AUTHOR: M Hollingworth
// DATE:   24/09/2019
//
// This application monitors the incoming 5V (from a mains adapter)
// on A0. If it dips below 4.75V then save the current time stamp
// to EEPROM so we at least know when the power has dropped
// The clock module can be a DS1338 (3.3V or 5V) or a DS1307 (5V)
// Daylight saving is performed according to UK rules
// ============================================================

#include <Wire.h>
#include <EEPROM.h>

unsigned long     _ulngCloopTime;
char              _bytCommandString[16];
unsigned char     _ubytCommandIndex;
bool              _blnDSTdone;
bool              _blnPowerLost;
unsigned long     _ulngStableUpTime;
unsigned long     _ulngLEDblipTime;

#define EEPROM_RESET_COUNT_ADDR       0
#define EEPROM_RESET_INFO_BLOCK_START 2
#define EEPROM_RESET_INFO_BLOCK_SIZE  7
#define POWER_RESET                   0x99
#define POWER_UP                      0xAA
#define POWER_DOWN                    0x55

#define LED                           9
#define BUTTON                        10

bool checkButton(void)
{
  static uint32_t suintKeyPressTimer;

  // See if the PRG button is pressed
  if (digitalRead(BUTTON) == 0)
  {
    if (suintKeyPressTimer == 0)
      suintKeyPressTimer = millis();
      
    if (millis() - suintKeyPressTimer > 2000)
    {
      // Button has been pressed for 2 seconds so it has REALLY been pressed!
      suintKeyPressTimer = 0;
      return true;
    }
  }
  else
    suintKeyPressTimer = 0;

  return false;
}

void incResetCount(void)
{
  word wrdTemp1;
  EEPROM.get(EEPROM_RESET_COUNT_ADDR, wrdTemp1);
  wrdTemp1++;
  EEPROM.put(EEPROM_RESET_COUNT_ADDR, wrdTemp1);
}

void writeRecord(byte bytReason)
{
  word wrdAddr;
  
  markReadDate();
  
  EEPROM.get(EEPROM_RESET_COUNT_ADDR, wrdAddr);
  wrdAddr = (wrdAddr * EEPROM_RESET_INFO_BLOCK_SIZE) + EEPROM_RESET_INFO_BLOCK_START;

  EEPROM.put(wrdAddr, bcdToDec(getSecond()));
  EEPROM.put(wrdAddr + 1, bcdToDec(getMinute()));
  EEPROM.put(wrdAddr + 2, bcdToDec(getHour()));
  EEPROM.put(wrdAddr + 3, getDate());
  EEPROM.put(wrdAddr + 4, bcdToDec(getMon()));
  EEPROM.put(wrdAddr + 5, bcdToDec(getYears()));
  EEPROM.put(wrdAddr + 6, bytReason);

  incResetCount();
}

void showTheResetInfo(void)
{
  word          wrdTemp;
  word          wrdC;
  word          wrdAddr;
  byte          bytReason;
  byte          bytS;
  byte          bytM;
  byte          bytH;
  byte          bytD;
  byte          bytMM;
  byte          bytY;
  
  EEPROM.get(EEPROM_RESET_COUNT_ADDR, wrdTemp);
  Serial.print("Reset count = ");
  Serial.println(wrdTemp);

  wrdAddr = EEPROM_RESET_INFO_BLOCK_START;
  for (wrdC = 0; wrdC < wrdTemp; wrdC++)
  {
    EEPROM.get(wrdAddr, bytS);
    EEPROM.get(wrdAddr + 1, bytM);
    EEPROM.get(wrdAddr + 2, bytH);
    EEPROM.get(wrdAddr + 3, bytD);
    EEPROM.get(wrdAddr + 4, bytMM);
    EEPROM.get(wrdAddr + 5, bytY);
    EEPROM.get(wrdAddr + 6, bytReason);
    switch (bytReason)
    {
      case POWER_UP:
        Serial.print("Power UP : ");
      break;
      case POWER_DOWN:
        Serial.print("Power DOWN : ");
      break;
      case POWER_RESET:
        Serial.print("Power RESET : ");
      break;
      default:
        Serial.print("Unknown :");
    }
    if (bytD < 10) Serial.print("0");
    Serial.print((int)bytD);
    Serial.print("/");
    if (bytMM < 10) Serial.print("0");
    Serial.print((int)bytMM);
    Serial.print("/");
    if (bytY < 10) Serial.print("0");
    Serial.print((int)bytY);
    Serial.print(" ");
    if (bytH < 10) Serial.print("0");
    Serial.print((int)bytH);
    Serial.print(":");
    if (bytM < 10) Serial.print("0");
    Serial.print((int)bytM);
    Serial.print(":");
    if (bytS < 10) Serial.print("0");
    Serial.println((int)bytS);
    
    wrdAddr += EEPROM_RESET_INFO_BLOCK_SIZE;
  }
}

void setup()
{
  char          bytDate[32];
  word          wrdTemp;
  word          wrdAddr;
  
  Serial.begin(9600);
  Wire.begin();

  // Get some up to date local RTC values
  markReadDate();

  // Only do this if we want to reinitialise everything
  //wrdTemp = 0;
  //EEPROM.put(EEPROM_RESET_COUNT_ADDR, wrdTemp);

  // If the EEPROM is uninitialised then reset the count
  if (wrdTemp == 0xFFFF)
    wrdTemp = 0;
    
  writeRecord(POWER_RESET);
       
  _ubytCommandIndex = 0;
  _bytCommandString[0] = 0x00;   
  _ulngCloopTime = millis();
   
  // Use this to set the time/date manually - specifically, set the Day Of Week (1 = Sunday, 7 = Saturday, there is no 0)
  //markSetDate();
  //markReadDate();
      
  _blnDSTdone = false;

  readDate(bytDate);
  Serial.println(bytDate);
  Serial.print("Day of week = ");   
  Serial.println(getDayOfWeek());

  _blnPowerLost = false;
  _ulngStableUpTime = 0;

  _ulngLEDblipTime = millis();

  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);
  pinMode(BUTTON, INPUT_PULLUP);
}

void loop() 
{
  unsigned long currentTime;
  char          bytI;
  word          wrdTemp;
  byte          bytHour;
  word          wrdA0;

  if (checkButton() == true)
  {
    wrdTemp = 0;
    EEPROM.put(EEPROM_RESET_COUNT_ADDR, wrdTemp);
    Serial.println("Reset counter done");

    for (bytI = 0; bytI < 4; bytI++)
    {
      digitalWrite(LED, 1);
      delay(500);
      digitalWrite(LED, 0);
      delay(500);
    }    
  }
  
  // Just show that we are alive
  if (_blnPowerLost == false)
  {
    if (millis() - _ulngLEDblipTime > 120000)
    {
      _ulngLEDblipTime = millis();
      digitalWrite(LED, 1);
      delay(50);
      digitalWrite(LED, 0);    
    }
  }
    
  wrdA0 = analogRead(A0);

  if (wrdA0 < 900)
  {
    // Reset our up time so we know that power hasn't been fully restored...
    _ulngStableUpTime = millis();    
  }
  
  if (wrdA0 < 900 && _blnPowerLost == false)
  {
    _blnPowerLost = true;
    Serial.println("Power lost");
    writeRecord(POWER_DOWN);

    for (bytI = 0; bytI < 2; bytI++)
    {
      digitalWrite(LED, 1);
      delay(250);
      digitalWrite(LED, 0);
      delay(250);
    }
  }
  
  if (wrdA0 > 1000 && _blnPowerLost == true)
  {
    if (millis() - _ulngStableUpTime > 5000)
    {
      _blnPowerLost = false;
      Serial.println("Power back up");    
      writeRecord(POWER_UP);
      for (bytI = 0; bytI < 4; bytI++)
      {
        digitalWrite(LED, 1);
        delay(250);
        digitalWrite(LED, 0);
        delay(250);
      }
    }
  }
    
  // We need to be able to set the date/time 
  // we do this from a serial input with the format (see rtc_util)
  // date DD/MM/YY HH:MM:SS
  if (Serial.available() > 0) 
  {
    char inByte = Serial.read();
    _bytCommandString[_ubytCommandIndex] = inByte;
    _ubytCommandIndex++;      
    if (inByte == 13) 
    {      
      if(_bytCommandString[0] == 'd' && _bytCommandString[1] == 'a' && _bytCommandString[2] == 't' && _bytCommandString[3] == 'e' && _ubytCommandIndex > 21)
      {
        _ubytCommandIndex = 0;
        Serial.println(_bytCommandString);
        //remove "date " from beginning of string
        for (bytI = 0; bytI < 17; bytI++) 
          _bytCommandString[bytI] = _bytCommandString[bytI + 5];
  
        setDateTime(_bytCommandString);
      }
      if (_bytCommandString[0] == 'i' && _bytCommandString[1] == 'n' && _bytCommandString[2] == 'f' && _bytCommandString[3] == 'o')
      {
        _ubytCommandIndex = 0;
        showTheResetInfo();
      }
      if (_bytCommandString[0] == 'r' && _bytCommandString[1] == 's' && _bytCommandString[2] == 't')
      {
        _ubytCommandIndex = 0;
        wrdTemp = 0;
        EEPROM.put(EEPROM_RESET_COUNT_ADDR, wrdTemp);
        Serial.println("Reset counter done");
      }
    }
    if(_ubytCommandIndex >= 59)
      _ubytCommandIndex = 0;
  }
   
  currentTime = millis();
   
  bytHour = getHour();
  
  // This is the main loop which reads the clock every 60 seconds
  if((currentTime >= (_ulngCloopTime + 60000)))
  {
    _ulngCloopTime = currentTime;          
     
    markReadDate();
 
    // Check for DST
    // October
    // Month is in BCD - no point converting...
    if (getMon() == 0x10)
    {
      //Serial.println("OCT");
      // Sunday
      if (getDayOfWeek() == 1)
      {
        //Serial.println("SUN");
        // Last Sunday in the month
        if (getDate() + 7 > 31)
        {
          //Serial.println("LAST");
          if (bytHour == 2 && _blnDSTdone == false)
          {
            //Serial.println("NEW");
            // Set the hour back by one
            _blnDSTdone = true;
            setHour(1);
          }
        }
      }
      else
      {
        _blnDSTdone = false;
      }
    }
    else
    {
      _blnDSTdone = false;
    }
      
    // March
    // Month is in BCD - no point converting...
    if (getMon() == 0x03)
    {
      // Sunday
      if (getDayOfWeek() == 1)
      {
        // Last Sunday in the month
        if (getDate() + 7 > 31)
        {
          if (bytHour == 1 && _blnDSTdone == false)
          {
            // Set clock forward 1 hour
            _blnDSTdone = true;
            setHour(2);
          }
        }
      }
      else
      {
        _blnDSTdone = false;
      }       
    }
    else
    {
      _blnDSTdone = false;
    }     
  }  
}
