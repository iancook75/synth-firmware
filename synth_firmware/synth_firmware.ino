// midi_read.ino

#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator
#include <StackArray.h>

Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);

byte incomingByte=0;
byte notebyte=0;
byte velocitybyte=0;
byte statusbuffer=0;
boolean arp_triggernext=false;
boolean firstbyte;

byte NOTE_ON = 144;
byte NOTE_OFF = 128;
byte PITCH_WHEEL = 224; // wrong
byte CONTROLLER = 208; // wrong

StackArray <int> stack;

void setup() {
  Serial.begin(57600);
  Serial.println("MIDI CONSOLE READER START");
  
  Serial1.begin(31250); // MIDI BAUD RATE

  startMozzi(); // uses the default control rate of 64, defined in mozzi_config.h
  aSin.setFreq(0); // set the frequency
}

int updateAudio(){
  // this would make more sense with a higher resolution signal
  // but still benefits from using HIFI to avoid the 16kHz pwm noise
  return aSin.next()<<6; // 8 bits scaled up to 14 bits
}

void loop(){
  audioHook();
  midiPoll();
}

void midiPoll(){
  if (Serial1.available() > 0) {
    do {
      // read the incoming byte:
      incomingByte = Serial1.read();
      if (incomingByte>247) {
        // this is where MIDI clock stuff is done
        switch (incomingByte){
        }
      }
      else if (incomingByte>240) {
        statusbuffer = 0;
        //sysex stuff done here
      }
      else if (incomingByte>127) {
        statusbuffer = incomingByte;
        firstbyte = true;
        notebyte = 0;
        velocitybyte = 0;
      }
      else if (statusbuffer!=0) {
        if (firstbyte == true) {
          // must be first byte
          notebyte = incomingByte;
          firstbyte = false;
        }
        else {
          // so must be second byte then
          velocitybyte = incomingByte;
          //process the message here
          if (statusbuffer == NOTE_ON && velocitybyte != 0) {
            //MIDI note on subroutine
            log_midi();
            play_note();
          }
          else if (statusbuffer == NOTE_OFF || (statusbuffer == NOTE_ON && velocitybyte == 0)) {
            //MIDI note off subroutine
            log_midi();
            stop_note();
          }
          else if (statusbuffer == PITCH_WHEEL){
             //pitch bend wheel
          }
          else if (statusbuffer == CONTROLLER){
            if (notebyte==1) {
              //MIDI_modwheel_level = velocitybyte;
            }
          }
          //now clear them for next note
          notebyte = 0;
          velocitybyte = 0;
          firstbyte = true;        
        }
      }
    } while (Serial1.available() > 0);
  }
}

void log_midi() {
  Serial.print("Note : ");
  Serial.print(notebyte);
  Serial.print(" Velocity : ");
  Serial.print(velocitybyte);
  Serial.print("\n");
}

void play_note() {
  stack.push(int(notebyte));
  int note = convert_to_frequency(notebyte);
  
  aSin.setFreq(note);
}

void updateControl(){}

void stop_note() {
  int(notebyte) = stack.pop();
  if (stack.isEmpty()) {
    aSin.setFreq(0);
  } else {
    aSin.setFreq(convert_to_frequency(stack.peek()));
  }
}

int convert_to_frequency(int nb) {
  double num = double(nb - 69) / 12;
  double frequency = double(pow(2,num)) * 440;
  return int(frequency);
}
