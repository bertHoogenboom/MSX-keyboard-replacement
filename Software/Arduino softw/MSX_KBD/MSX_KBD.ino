#include <hidboot.h>
#include <usbhub.h>
#include "scancode.h"


// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

#define FLAGS_BITMASK 0xe0 // ATMEGA324
#define DEBUG = 1
#define RUN = 1

class KbdRptParser : public KeyboardReportParser
{
  public:
    KbdRptParser(void);
    void copytable(void);
  private:
    void Empty_KBDBuffer(void);
    
  protected:
    void OnControlKeysChanged(uint8_t before, uint8_t after);
    void OnKeyDown	(uint8_t mod, uint8_t key);
    void OnKeyUp	(uint8_t mod, uint8_t key);
//     void OnKeyPressed(uint8_t key);
    void decodekey(uint8_t scancode, uint8_t up_dn);
    void Write_KBDBuffer(uint8_t data);
//    void copytable(void);
    void Showtable(void);
};
uint8_t KBDbuffer[16];
//uint8_t shift = 0, ctrl = 0, alt = 0, gui = 0;
uint8_t is_up;
uint8_t tablenbr;
uint8_t leds;
uint8_t flags, oldflags;

KbdRptParser::KbdRptParser(void) //constructor
{
  Empty_KBDBuffer();
  PORTA = 0xFF; // set all pins 1
  DDRA = 0xFF;  // All pins of port D set to output
  PORTA = 0xFF; // set all pins 1
  PORTC |= 0x1F;// set all output pins 1
  DDRC = 0x1F;  // pins 00 to 4 of port C set as output
  PORTC |= 0x1F;// set all output pins 1

}

void KbdRptParser::decodekey(uint8_t scancode, uint8_t up_dn)
{
  uint8_t code;
  code = 0;
  is_up = up_dn;
  if (scancode < 0x80)
  {
    code = pgm_read_byte(&(ScanCode[tablenbr][scancode]));
    if (code != 0xff)
    {
      Write_KBDBuffer(code);
//      NEWTABLE = 1;
//      key_seq_busy = false;
    }
  }
}

void KbdRptParser::Empty_KBDBuffer(void)
{
  uint8_t i;
  for (i = 0; i < 15; i++)
  {
    KBDbuffer[i] = 0xff;
  }
}

uint8_t nibble_to_byte[8]= {0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f}; 
void KbdRptParser::Write_KBDBuffer(uint8_t data)
{
uint8_t x,y;
  x = ((data >> 4) & 0x0f);
  y = (data & 0xf);
  y = nibble_to_byte[y];
  if (is_up)
    KBDbuffer[x] |= ~y;
  else
    KBDbuffer[x] &= y;
  is_up = 0;
}

void KbdRptParser::copytable(void)
{ 
  uint8_t i,j;
  for (i = 0; i < 12; i++)  // transfer KBbuffer to CPLD KBbuffer
  {
    PORTA = KBDbuffer[i]; // set data ATMEGA324
    PORTC = 0xff & (i | 0xf0);    // set bufferadress + loadpuls
    PORTC &= 0xEF; //send write puls
    for (j = 0; j < 10; j++){;} 
    PORTC |= 0x10; 
  }
  PORTA = 0xff;
}

void KbdRptParser::Showtable(void)
{
  copytable();

#ifdef DEBUG  //showtable
  static uint8_t x, y;
  //Serial.print(0x0c); //clear screen
  Serial.print('\n');
  for (y = 0; y < 8; y++)
  {
    for (x = 0; x < 12; x++)
    {
      Serial.print(' ');
      Serial.print(char((((KBDbuffer[x]) >> y) & 0x01) | '0'));
    }
    Serial.print('\n');
  }
#endif
}

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)
{
  Serial.print("DN ");
/*  uint8_t c = OemToAscii(mod, key);

  if (c)
    OnKeyPressed(c);
 */
  decodekey(key,0);
  Showtable();
}

void KbdRptParser::OnControlKeysChanged(uint8_t before, uint8_t after) {

  MODIFIERKEYS beforeMod;
  *((uint8_t*)&beforeMod) = before;

  MODIFIERKEYS afterMod;
  *((uint8_t*)&afterMod) = after;

  tablenbr = (after >> 4) | (after & 0x0f);
  tablenbr = ((tablenbr & 0x08) >> 1) | (tablenbr & 0x07);  //GUI key is the same as ALT key
  tablenbr &= 0x0f; 
//  Serial.print("Table number =");
//  Serial.println(tablenbr, HEX);

  
  if ((beforeMod.bmLeftCtrl != afterMod.bmLeftCtrl) | (beforeMod.bmRightCtrl != afterMod.bmRightCtrl)){
    if ((tablenbr & 0x01) != 0)
      is_up = 0;
    else
      is_up = 1;
    Write_KBDBuffer(0x61);
  }
  if ((beforeMod.bmLeftShift != afterMod.bmLeftShift) | (beforeMod.bmRightShift != afterMod.bmRightShift)) {
    if ((tablenbr & 0x02) != 0)
      is_up = 0;
    else
      is_up = 1;
    Write_KBDBuffer(0x60);
  }
  if ((beforeMod.bmLeftAlt != afterMod.bmLeftAlt) | (beforeMod.bmRightAlt != afterMod.bmRightAlt)) {
    if ((tablenbr & 0x04) != 0)
      is_up = 0;
    else
      is_up = 1;
    Write_KBDBuffer(0x64);
  }
  if ((beforeMod.bmLeftGUI != afterMod.bmLeftGUI) | (beforeMod.bmRightGUI != afterMod.bmRightGUI)) {
    if ((tablenbr & 0x08) != 0)
      is_up = 0;
    else
      is_up = 1;
    Write_KBDBuffer(0x62);
  }
  Showtable();
}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)
{
//  Serial.print("UP ");
  is_up = 1;
  decodekey(key,1);
  Showtable();
}
/*
void KbdRptParser::OnKeyPressed(uint8_t key)
{
  Serial.print("ASCII: ");
  Serial.print((char)key);
};
*/
USB     Usb;
//USBHub     Hub(&Usb);
HIDBoot<USB_HID_PROTOCOL_KEYBOARD>    HidKeyboard(&Usb);

KbdRptParser Prs;

void setup()
{
  Serial.begin( 115200 );
#if !defined(__MIPSEL__)
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  Serial.println("Start");

  if (Usb.Init() == -1)
    Serial.println("OSC did not start.");

  delay( 200 );

  HidKeyboard.SetReportParser(0, &Prs);
  
}

void loop()
{
  Usb.Task();
  Prs.copytable();
}
