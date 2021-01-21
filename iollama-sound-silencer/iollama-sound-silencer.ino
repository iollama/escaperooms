/*************************************************************************
  Sound Silencer box by IO Llama
  to learn more: http://iollama.com/

  ABOUT
  -----
  This box makes loud human sounds stop. It does that by finding the exact
  phase of the soundwave, and they inverting it and sending it back at the
  source. Just kidding. This box detects loud continuous sounds (think neighbors
  having a party at midnight). When a loud continuous sound is detected,
  the box will start outputting an annoying sound of its own.
  Hopefully that will make the noisy party go away.

  I was having trouble displaying anything on the TML1636 anbd making a 
  measure in the same time, so mp3 and user messages 
  got moved over to milis()


  USES / LIBRARIES:
  ----------------
  ioxhop/OPEN-SMART-RedMP3: https://github.com/ioxhop/OPEN-SMART-RedMP3
  rjbatista/tm1638-library: https://github.com/rjbatista/tm1638-library

  PREPERATION
  YOU would need a micro SD card with some annoying music!
  It is optional to have a hi-powered hi-fi system


  You can see how the code works here:
  https://youtu.be/qY7LQZtbCrQ

  PARTS LIST:
  ----------
  UART Serial MP3 Music Player Module - https://amzn.to/2HIERde
  Arduino NANO compatible board: https://amzn.to/3myqerZ
  Arduino IO shield: https://amzn.to/3oNOeJQ
  Led and Key: https://amzn.to/3ogmnkn
  LM386 Sound Sensor: https://amzn.to/36BUnBE
*************************************************************************/


#include <TM1638.h>
#include <SoftwareSerial.h>
#include "RedMP3.h"


/********************** sound detector ***********************/
/*************************************************************/
#define PIN_SOUND_DIN 2
#define PIN_SOUND_AIN A0
#define NOISE_MED 512
#define SAMPLE_DISTANCE 1000 //3000
#define SAMPLES_TO_AVARAGE 5 //5
#define TIMES_TO_SAMPLE 3 //3 MAX 8

bool bNoise[TIMES_TO_SAMPLE] = {0};
unsigned long prevSample = 0;
int           sensitivity = 4; // (higher means less sensitive) we initialize with a not so sensitive device 

/************************* LED & KEY *************************/
/*************************************************************/
#define PIN_LNK_DATA 8
#define PIN_LNK_CLK 9
#define PIN_LNK_STRB 10
#define SCROLL_TIMER 500
#define LED_ID(x) 0x01<<((x)-1)

int leds = 0xFF; // mask to display LEDS
byte keys = 0; // key value
unsigned long msgTimeStamp = 0;
unsigned long msgTimeToShow = 0;


/************************* MP3***** *************************/
/*************************************************************/
#define MP3_NUM_OF_SONGS 4
#define MP3_TIME_TO_PLAY 3000 //20000
MP3 mp3(A5, A4);
int           song = 01;
unsigned long mp3StartMillis = 0;


// define a LED and key module
TM1638 module(PIN_LNK_DATA, PIN_LNK_CLK, PIN_LNK_STRB);
unsigned int volume   = 0; // starting our with voluim of 20/100 - trust me it's not as quite it you may think
bool         playing  = false;


/*************************************************************************/
/* setup()                                                               */
/* initialize pins and serial                                            */
/* user massage                                                          */ 
/* RETURNS: NULL                                                         */
/*************************************************************************/
void setup() {

  pinMode(PIN_SOUND_DIN, INPUT);
  pinMode(PIN_SOUND_AIN, INPUT);
  Serial.begin(9600);


  module.clearDisplay(); // clear the display
  displayMessage(3000, "INIT SEQ");
}

/*************************************************************************/
/* loop()                                                                */
/* main loop                                                             */
/* RETURNS: NULL                                                         */
/*************************************************************************/
void loop() {

  /* if the neighbors are making noise, let them have it*/
  if (isNoise())
  {
    delay(1000);
    mp3Play();
  }

  /* move the RED MP3 player to milis*/
  mp3Loop();

  /* do we havce any input? if so act accordingly*/
  switch (keys) {

    case LED_ID(1): // primitive help 
      showHelp();
      break;

    case LED_ID(2): // cycle between volium levels
      mp3volumeStep(true);
      break;

    case LED_ID(3): // cycle between songs 
      mp3SongStep();
      break;

    case LED_ID(4): // credit massage
      for (int thisPin = 0; thisPin < 8; thisPin++) {
        // turn the pin on:
        module.setDisplayToString("IOLLAMA", 0, thisPin);
        delay(SCROLL_TIMER);
        module.setDisplayToString("        ", 0, thisPin);
      }
      break;

    case LED_ID(5): // manula start/stop palying
      mp3PlayToggle();
      break;

    case LED_ID(6): //cycle through noise detection sensitivity 
      sensitivityStep();
      break;
   
    case LED_ID(8): // for debug
      module.clearDisplay();
      break;
      
    case LED_ID(7): //reserved
    default:
      break;
  }

  // get a key press
  getKeyPressed();

  // move user message to millis
  refreshMessage();
}

/*************************************************************************/
/* getKeyPressed()                                                       */
/* did anyone press something? store it in a global variable             */
/* RETURNS: NULL                                                         */
/*************************************************************************/
void getKeyPressed() {
  keys = module.getButtons();
  //Serial.println(keys);
  // light the LED
  module.setLEDs(keys & leds);
  if (0 != keys)
    delay(200);
}


/*************************************************************************/
/* mp3PlayToggle()                                                       */
/* start / stop playing                                                  */
/* RETURNS: NULL                                                         */
/*************************************************************************/
void mp3PlayToggle() {
  if (false == playing)
  {
    mp3Play();
  } else {
    mp3Stop();
  }
}

/*************************************************************************/
/* mp3Play()                                                             */
/* start playing                                                         */
/* RETURNS: NULL                                                         */
/*************************************************************************/
void mp3Play() {
  mp3StartMillis = millis();
  String msg = "PLAYING  ";
  mp3.playWithFileName(01, song);
  playing = true;
  displayMessage(500, msg);
}

/*************************************************************************/
/* mp3Stop()                                                             */
/* stop playing                                                          */
/* RETURNS: NULL                                                         */
/*************************************************************************/
void mp3Stop() {
  String msg = "STOPPING   ";
  mp3.stopPlay();
  playing = false;
  displayMessage(500, msg);
}

/*************************************************************************/
/* mp3volumeStep()                                                       */
/* cycle between volume levels                                           */
/* RETURNS: NULL                                                         */
/*************************************************************************/
void mp3volumeStep(bool bMsg) {
  
  volume = (volume + 1)%5;

  int volToSet = 20*(1+volume);
  mp3.setVolume(volToSet);
  if (true == bMsg)
  {
    String msg = String("VOL= ");
    msg += String(volToSet) + "  ";
    displayMessage(500, msg);
  }
}

/*************************************************************************/
/* mp3SongStep()                                                         */
/* cycle btween songs. Sadlym, we need ot pre set the numnber of songs   */
/* manually in the code via the  MP3_NUM_OF_SONGS parameter              */
/* RETURNS: NULL                                                         */
/*************************************************************************/
void mp3SongStep() {
  song = (song + 1);
  if (MP3_NUM_OF_SONGS < song) song = 01;

  String msg = String("Sng=");
  msg += String(song) + "  ";
  displayMessage(500, msg);

  delay(200);
}

/*************************************************************************/
/* sensitivityStep()                                                     */
/* cycle between sensitivity levels. this is the "distance" from quite 
 * that we will consider noice. 10 (whihc is the lowest) will consdier 
 * almost anything as noice. 50 will need quite a strong signal
 * RETURNS: NULL                                                         */
/*************************************************************************/
void sensitivityStep() {

  sensitivity = (sensitivity+1)%5;
  String msg = String("Snvt=");
  msg += String(10*(sensitivity+1)) + "  ";
  displayMessage(500, msg);

  delay(200);
}

/*************************************************************************/
/* sampleNoise()                                                         
 * we take a few noise measurements and avarage them.
 * timesToSample - how many times to sample
 * RETURNS: avarage noise over timesToSample                              
/*************************************************************************/
unsigned int sampleNoise(int timesToSample)
{
  int totalNoise = 0;
  int avgNoise = 0;

  if (0 != msgTimeToShow)
    return 0;
  for (int i = 0; i < timesToSample; i++)
  {
    int noise = analogRead(PIN_SOUND_AIN);
    totalNoise += noise;
  }
  avgNoise = abs( (totalNoise / timesToSample) - NOISE_MED);
  //Serial.println(avgNoise);
  return avgNoise;
}

/*************************************************************************/
/* isNoise()                                                         
 * we take a few noise measurements and avarage them.
 * timesToSample - how many times to sample
 * RETURNS: are the neighbors having too much fun?                              
/*************************************************************************/
bool isNoise()
{
  unsigned long curSample = millis();
  int noise;
  bool combinedSamples = true;
  if (true == playing) // no point in measireing noise if we are the ones making it
    return 0;

  if (curSample - prevSample > SAMPLE_DISTANCE) // only measure once ever SAMPLE_DISTANCE miliseconds
  {
    String msg = "";
    String ledDisplay = "";
    int i;
    int noiseLevel = sampleNoise(SAMPLES_TO_AVARAGE); // read ambiant
    
    msg = "curent = " + String(noiseLevel);
    for (i = 0; i < (TIMES_TO_SAMPLE - 1) ; i++)
    {
      bNoise[i] = bNoise[i + 1];
    }


    bNoise[i] = (noiseLevel > 10*(sensitivity+1) ); // is it noisy right now?
    for (i = 0;  i < TIMES_TO_SAMPLE ; i++)
    {
      combinedSamples = combinedSamples && bNoise[i]; // we consider the situation to be noisy only if there are a few consecutive noisy samples
      msg = msg + "; bNoise[" + String(i) + "]= " + String(bNoise[i]);
      ledDisplay += String(bNoise[i]);
    }

    prevSample = curSample;
    Serial.println(msg);
    displayMessage(200, ledDisplay);

    return combinedSamples;
  }
  return false;
}

/*************************************************************************/
/* displayMessage()                                                         
 * Show some info to the user via the LED & keyt module
 * timeToShow - how many milis to show the message. there is no queue so a 
 *              new message will run over an old message
 * msg - what to say
 * RETURNS: NULL                             
/*************************************************************************/
void displayMessage(unsigned long timeToShow, String msg)
{
  module.clearDisplay();
  module.setDisplayToString(msg);
  msgTimeStamp = millis();
  msgTimeToShow = timeToShow;
  Serial.println(msg);
}

/*************************************************************************/
/* displayMessage()                                                         
 *  kill the usert message after a while
 * RETURNS: NULL                             
/*************************************************************************/
void  refreshMessage()
{
  unsigned long curMillis = millis();
  if (0 < msgTimeToShow && curMillis - msgTimeStamp > msgTimeToShow)
  {
    module.clearDisplay();
    msgTimeToShow = 0;
  }
}

/*************************************************************************/
/* displayMessage()                                                         
 *  move mp3 handling to millis so its not blocking
 * RETURNS: NULL                             
/*************************************************************************/
void mp3Loop()
{
  unsigned long curMillis = millis();
  if (playing && curMillis - mp3StartMillis > MP3_TIME_TO_PLAY)
  {
    mp3Stop();
  }
  if (playing)
  {
    String msg = "P=" + String((MP3_TIME_TO_PLAY-(curMillis - mp3StartMillis)));
    displayMessage(300, msg);
  }
}

/*************************************************************************/
/* showHelp()                                                         
 * basic help. really, its as basic as it can get
 * RETURNS: NULL                             
/*************************************************************************/
void showHelp()
{
  displayMessage(2000, "HVSCPS__");
}
