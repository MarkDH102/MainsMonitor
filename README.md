# MainsMonitor
ATMEGA328p power cut monitor

This project monitors mains and detects when the power is removed.

It has 2* 2.5V 10F supercapacitors to keep power supplied to the board while it writes a record to the internal EEPROM and also to keep running for medium term cuts.

It has a battery backed RTC for timestamping.

There are 3 record types :

*POWER RESET - When the ATMEGA code executes a reset. I.e. the supercapcitors have completely run out.
*POWER LOST - When power is lost
*POWER UP - When power is restored (this will come 5s after good power is restored - to make sure it is good!).

You can connect the unit to a standard Arduino UNO (minus it's chip) via the programming header.

The serial port runs at 9600,8,N,1.

There are 3 commands : 

*info<CR> - This returns all stored records as readable ASCII text.
*rst<CR> - This resets the record count to zero.
*datetime DD/MM/YY HH:MM:SS<CR> - Sets the time.
  
  
