/*************************************************************************
* Escape Room Riddle box by IO Llama
* to learn more: http://iollama.com/
* 
* ABOUT
* -----
* This escape room box presents a riddle that participants have to solve. 
* Once the riddle is solved, the box opens and they get a prize (or a key) 
* It uses an arduino controller to control a lock and a speaker.
* the input is presented with RFID tags, that can be attacehd to letters,
* numbers or pictures.
*
*
* USES / LIBRARIES:
* ----------------
* ioxhop/OPEN-SMART-RedMP3: https://github.com/ioxhop/OPEN-SMART-RedMP3
* miguelbalboa/rfid: https://github.com/miguelbalboa/rfid
* adafruit/Adafruit_NeoMatrix: https://github.com/adafruit/Adafruit_NeoMatrix
* adafruit/Adafruit_NeoPixel: https://github.com/adafruit/Adafruit_NeoPixel
* Matrix code: http://arduinolearning.com/code/neomatrix-scrolling-text-example-on-an-arduino.php
*
* PREPERATION
* YOU would need a micro SD card with three sounds files: {Good}, {Bad} and {Well Done}! 
* 
*
* You can see how the code works here:
* https://youtu.be/Flvw812iim8
* 
* PARTS LIST:
* ----------
* UART Serial MP3 Music Player Module - https://amzn.to/2HIERde
* Arduino NANO compatible board: https://amzn.to/3myqerZ
* Arduino IO shield: https://amzn.to/3oNOeJQ
* Lock: https://amzn.to/3ox0pdI
* Relay: https://amzn.to/3jyShFW
* RF reader: https://amzn.to/2HKM78S
* LED matrix: https://amzn.to/3jBOBmN
* Battery holder: https://amzn.to/3mwE9Po
*
*************************************************************************/


/***********************************************************************/
/****************************EDIT HERE**********************************/
/* Write the RFID tags that make the solution in the right order.
 *  the format for eahc tags is as follows: (the last line does not have
 *  a "\"
 *  {"XX XX XX XX", COLOR}, \
 *  you can get the tags IDs by putting thme next to the tag reader
 *  and observing the console
 */
/***********************************************************************/
#define SOLUTION_TAGS \
               {"90 CB 76 BE", GREEN},  \
               {"40 EC 73 BE", YELLOW}, \
               {"70 EC 57 BE", GREEN},  

/***********************************************************************/
/****************************END EDIT***********************************/
/***********************************************************************/


#include <SoftwareSerial.h>

/* ****************** LOCK ********************* */
/* ********************************************* */
#define LOCK_PIN 4

/* ****************** MP3 ********************** */
/* ********************************************* */
#include "RedMP3.h"
MP3 mp3(A5, A4);   
#define SOUND_DIR_NAME  04
#define AUDIO_SUCCESS 01
#define AUDIO_FAILURE 02
#define AUDIO_COMPLETE 03

/* ****************** NFC ********************** */
/* ********************************************* */
#include <SPI.h>
#include <MFRC522.h>
 
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
String content= "";

/* ******************MATRIX********************* */
/* ********************************************* */
// https://learn.adafruit.com/adafruit-gfx-graphics-library?view=all#characters-and-text

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif
#define MAX_TEXT_WRAP 16

/* ***************COLOR PRESETS **************** */
/* ********************************************* */
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
uint16_t colors[] = {BLUE,  RED,  GREEN,  CYAN,  MAGENTA,  YELLOW,  WHITE}; 



/* ***************BUILD A MATRIX *************** */
/* ********************************************* */
#define MATRIX_PIN 8
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, MATRIX_PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB            + NEO_KHZ800);

int mat_x    = matrix.width();
int pass = 0;

unsigned long matrixTimeStamp = 0;


/* ***************riddle things **************** */
/* ********************************************* */
typedef struct ansName {
   const char* nfcTag;
   uint16_t    color;
}AnsName;

#define SOLUTION_SIZE ((int)(sizeof(solution)/sizeof(AnsName)))

int curStep = 0;


/* find the tag IDs by putting the tags next to the box and looking at the cosole */
AnsName solution[] = 
{
  SOLUTION_TAGS
};


/*************************************************************************/
/* doMatrix()                                                            */
/* refreshing the color matrix depending on the proigress of the solution*/
/* RETURNS: NULL                                                         */
/*************************************************************************/
void doMatrix()
{
  unsigned long curTimeStamp = millis();

  if (curTimeStamp < matrixTimeStamp + 100)
    return;
  matrixTimeStamp = curTimeStamp;

  matrix.fillScreen(BLACK);
  matrix.setCursor(mat_x, 0);

  String toprint;
  uint16_t color;
  
  if (0 < (SOLUTION_SIZE - curStep))
  {
     toprint = SOLUTION_SIZE - curStep;
     color = solution[curStep].color;
  }  else
  {
    toprint = F("TADA!");
    color = random(WHITE);
  }
  matrix.print(toprint);
  
  if(--mat_x < -(MAX_TEXT_WRAP)) {
    mat_x = matrix.width();
    if(++pass >= 3) pass = 0;
    matrix.setTextColor(color);
  }
  matrix.show();   
  return;
}

/*************************************************************************/
/* lockOpen()                                                            */
/* openning the lock                                                     */
/* RETURNS: NULL                                                         */
/*************************************************************************/
void lockOpen()
{
  digitalWrite(LOCK_PIN, LOW);
  delay(100);
  digitalWrite(LOCK_PIN, HIGH);
}


/*************************************************************************/
/* readNFC()                                                             */
/* Lets look at the NFC reader and see if something is there             */
/* RETURNS:                                                              */
/*   FALSE - no ID found                                                 */
/*   TRUE -  new ID found                                                */
/*************************************************************************/
bool readNFC()
{
  content= "";
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return false;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return false;
  }
  //Show UID on serial monitor
  Serial.print("UID tag :");

  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  content.toUpperCase();
  return true;
}

/*************************************************************************/
/* readAndSet()                                                          */
/* Lets see if we got a card that advances the answer                    */
/* RETURNS:                                                              */
/*   FALSE - meh                                                         */
/*   TRUE -  we're going somewhere                                       */
/*************************************************************************/
bool readAndSet()
{
  int audioToPlay;
  if (false == readNFC())
    return false;

  if (content.substring(1) == solution[curStep].nfcTag)
  {
    curStep++;
    audioToPlay = AUDIO_SUCCESS;
  }
  else
  {
    curStep = 0;
    audioToPlay = AUDIO_FAILURE;    
  }
  Serial.println((String)"Message: Curent Letter = :" + curStep + "\n"); 
  mp3.playWithFileName(SOUND_DIR_NAME,audioToPlay);
  return true;
}


void setup() 
{

  /*starting the serial */
  Serial.begin(9600);   // Initiate a serial communication

  /* lets get a random seed */
  randomSeed(analogRead(0));;
  
  Serial.println((String)"Message: size of solution = :" + SOLUTION_SIZE + "\n"); 
      
  /* NFC */
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  Serial.println("Approximate your card to the reader...");
  Serial.println();

  /* MATRIX */
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(40);
  matrix.setTextColor(solution[curStep].color);;
  matrix.fillScreen(BLACK);
  matrix.show();
  
  /* MP3 */
  mp3.setVolume(40);

  /* setting up the relay */
  pinMode(LOCK_PIN, OUTPUT);
  digitalWrite(LOCK_PIN, HIGH);
}


void loop() 
{
  /*NFC */
  bool bNFCread = false;
  bNFCread = readAndSet();
  doMatrix();
  
  if (SOLUTION_SIZE == curStep)
  {
    mp3.playWithFileName(SOUND_DIR_NAME,AUDIO_COMPLETE);
    delay(500);
    lockOpen();
    curStep++;
  }
  if (bNFCread) delay(500);

  delay(75);
} 
