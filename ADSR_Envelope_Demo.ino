
/* ADSR Envelope Synth Demo
   Teensy 3.6 with I2S Audio Shield
   Arduino IDE 1.8.10

   Uses button debounce library  https://github.com/thomasfredericks/Bounce2
   Uses SSD1306 display driver by Adafruit: https://github.com/adafruit/Adafruit_SSD1306
   Uses Adafruit GFX library: https://github.com/adafruit/Adafruit-GFX-Library

   Gadget Reboot
*/

#include <Bounce2.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET 4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

AudioSynthWaveform       waveform1;
AudioSynthWaveform       waveform2;
AudioSynthWaveform       waveform3;
AudioEffectEnvelope      adsrEnvelope1;
AudioEffectEnvelope      adsrEnvelope2;
AudioEffectEnvelope      adsrEnvelope3;
AudioMixer4              mixer1;
AudioOutputI2S           i2s1;
AudioConnection          patchCord1(waveform1, adsrEnvelope1);
AudioConnection          patchCord2(waveform2, adsrEnvelope2);
AudioConnection          patchCord3(waveform3, adsrEnvelope3);
AudioConnection          patchCord4(waveform1, 0, mixer1, 0);
AudioConnection          patchCord5(adsrEnvelope1, 0, mixer1, 1);
AudioConnection          patchCord6(adsrEnvelope2, 0, mixer1, 2);
AudioConnection          patchCord7(adsrEnvelope3, 0, mixer1, 3);
AudioConnection          patchCord8(mixer1, 0, i2s1, 0);
AudioConnection          patchCord9(mixer1, 0, i2s1, 1);
AudioControlSGTL5000     audioShield;

// pot inputs
#define attackPot  A17
#define decayPot   A16
#define sustainPot A15
#define releasePot A14

// button inputs
#define button1    29  // waveform pitch 1
#define button2    30  // waveform pitch 2
#define button3    31  // waveform pitch 3
#define button4    32  // waveform pitch 1 sound test without applying ADSR envelope

// create debounce instances for buttons
Bounce buttonPlayWaveform1       = Bounce();
Bounce buttonPlayWaveform2       = Bounce();
Bounce buttonPlayWaveform3       = Bounce();
Bounce buttonPlayTestWaveform    = Bounce();

// store pot readings into ADSR parameter settings
int   attackParam;
int   decayParam;
float sustainParam;
int   releaseParam;

// limit maximum timing for parameters set by pots
#define attackMax  2000
#define decayMax   2000
#define releaseMax 2000

// waveform frequencies for tones based on musical note frequencies
#define waveformFreq1 97.99886  note "G"
#define waveformFreq2 130.8128  note "C"
#define waveformFreq3 174.6141  note "F"

void setup() {

  Serial.begin(9600);
  Serial.println("ADSR Demo");

  // assign input pins to debounce instances with a certain debounce time
  buttonPlayWaveform1.attach(button1, INPUT_PULLUP);
  buttonPlayWaveform1.interval(10);
  buttonPlayWaveform2.attach(button2, INPUT_PULLUP);
  buttonPlayWaveform2.interval(10);
  buttonPlayWaveform3.attach(button3, INPUT_PULLUP);
  buttonPlayWaveform3.interval(10);
  buttonPlayTestWaveform.attach(button4, INPUT_PULLUP);
  buttonPlayTestWaveform.interval(10);

  // initialize OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();

  // configure audio settings
  AudioMemoryUsageMaxReset();
  AudioMemory(5);
  audioShield.enable();
  audioShield.volume(0.5);

  // set volume of audio paths into mixer
  mixer1.gain(0, 0);
  mixer1.gain(1, 0.2);
  mixer1.gain(2, 0.2);
  mixer1.gain(3, 0.2);

  // configure three audio sources for playback
  waveform1.begin(0.3, waveformFreq1, WAVEFORM_SAWTOOTH);
  waveform2.begin(0.3, waveformFreq2, WAVEFORM_SAWTOOTH);
  waveform3.begin(0.3, waveformFreq3, WAVEFORM_SAWTOOTH);

  // set default ADSR envelope parameters to be applied to all audio sources
  attackParam = 50;
  decayParam = 50;
  sustainParam = 0.5;
  releaseParam = 250;

  // assign envelope parameters
  adsrEnvelope1.delay(0);
  adsrEnvelope1.attack(attackParam);    // max 11880 mS
  adsrEnvelope1.hold(0);                // max 11880 mS
  adsrEnvelope1.decay(decayParam);      // max 11880 mS
  adsrEnvelope1.sustain(sustainParam);  // gain level from 0 to 1.0
  adsrEnvelope1.release(releaseParam);  // max 11880 mS

  adsrEnvelope2.delay(0);
  adsrEnvelope2.attack(attackParam);    // max 11880 mS
  adsrEnvelope2.hold(0);                // max 11880 mS
  adsrEnvelope2.decay(decayParam);      // max 11880 mS
  adsrEnvelope2.sustain(sustainParam);  // gain level from 0 to 1.0
  adsrEnvelope2.release(releaseParam);  // max 11880 mS

  adsrEnvelope3.delay(0);
  adsrEnvelope3.attack(attackParam);    // max 11880 mS
  adsrEnvelope3.hold(0);                // max 11880 mS
  adsrEnvelope3.decay(decayParam);      // max 11880 mS
  adsrEnvelope3.sustain(sustainParam);  // gain level from 0 to 1.0
  adsrEnvelope3.release(releaseParam);  // max 11880 mS
 
  AudioInterrupts();

}

void loop() {

  // update button debounce status
  buttonPlayWaveform1.update();
  buttonPlayWaveform2.update();
  buttonPlayWaveform3.update();
  buttonPlayTestWaveform.update();

  // play notes based on button press/release
  if (buttonPlayWaveform1.fell()) {
    adsrEnvelope1.noteOn();
  }
  if (buttonPlayWaveform1.rose()) {
    adsrEnvelope1.noteOff();
  }
  if (buttonPlayWaveform2.fell()) {
    adsrEnvelope2.noteOn();
  }
  if (buttonPlayWaveform2.rose()) {
    adsrEnvelope2.noteOff();
  }
  if (buttonPlayWaveform3.fell()) {
    adsrEnvelope3.noteOn();
  }
  if (buttonPlayWaveform3.rose()) {
    adsrEnvelope3.noteOff();
  }

  // play sample note without ADSR envelope
  if (buttonPlayTestWaveform.read() == false) {
    mixer1.gain(0, 0.2);
  }
  else {
    mixer1.gain(0, 0);
  }

  // read in pots and adjust ADSR parameters
  attackParam  = map(analogRead(attackPot), 0, 1023, 0, attackMax);
  decayParam   = map(analogRead(decayPot), 0, 1023, 0, decayMax);
  sustainParam = mapf(analogRead(sustainPot), 0, 1023, 0, 1.0);
  releaseParam = map(analogRead(releasePot), 0, 1023, 0, releaseMax);

  // optional debug serial output
  Serial.print("Attack: ");
  Serial.print(attackParam);
  Serial.print("   Decay: ");
  Serial.print(decayParam);
  Serial.print("   Sustain: ");
  Serial.print(sustainParam);
  Serial.print("   Release: ");
  Serial.println(releaseParam);

  // draw the current ADSR envelope on OLED
  displayADSRPattern();

  // apply ADSR parameters to sound generators
  adsrEnvelope1.attack(attackParam);    // max 11880 mS
  adsrEnvelope1.decay(decayParam);      // max 11880 mS
  adsrEnvelope1.sustain(sustainParam);  // gain level from 0 to 1.0
  adsrEnvelope1.release(releaseParam);  // max 11880 mS

  adsrEnvelope2.attack(attackParam);    // max 11880 mS
  adsrEnvelope2.decay(decayParam);      // max 11880 mS
  adsrEnvelope2.sustain(sustainParam);  // gain level from 0 to 1.0
  adsrEnvelope2.release(releaseParam);  // max 11880 mS

  adsrEnvelope3.attack(attackParam);    // max 11880 mS
  adsrEnvelope3.decay(decayParam);      // max 11880 mS
  adsrEnvelope3.sustain(sustainParam);  // gain level from 0 to 1.0
  adsrEnvelope3.release(releaseParam);  // max 11880 mS

  // determine how much actual memory is needed for the audio processes
  Serial.print("Max audio memory blocks used: ");
  Serial.println(AudioMemoryUsageMax());
  Serial.println();

}

// floating point map function taken from https://forum.arduino.cc/index.php?topic=361046.0
double mapf(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// draw the current ADSR envelope on OLED
void displayADSRPattern() {

  byte x0, y0, x1, y1;     // start/end coordinates for drawing lines on OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("ADSR");

  // draw attack line
  x0 = 0;
  y0 = 63;
  x1 = map(attackParam, 0, attackMax, 0, ((SCREEN_WIDTH / 4) - 1));
  y1 = 20;
  display.drawLine(x0, y0,  x1,  y1, WHITE);

  // draw decay line
  x0 = x1;  // start line from previous line's final x,y location
  y0 = y1;
  x1 = x0 + map(decayParam, 0, decayMax, 0, ((SCREEN_WIDTH / 4) - 1));
  y1 = mapf(sustainParam, 0, 1.0, 63, 20);
  display.drawLine(x0, y0,  x1,  y1, WHITE);

  // draw sustain line
  x0 = x1;  // start line from previous line's final x,y location
  y0 = y1;
  x1 = x0 + ((SCREEN_WIDTH / 4) - 1);
  y1 = y0;
  display.drawLine(x0, y0,  x1,  y1, WHITE);

  // draw release line
  x0 = x1;  // start line from previous line's final x,y location
  y0 = y1;
  x1 = x0 + map(releaseParam, 0, releaseMax, 0, ((SCREEN_WIDTH / 4) - 1));
  y1 = 63;
  display.drawLine(x0, y0,  x1,  y1, WHITE);

  display.display();
}
