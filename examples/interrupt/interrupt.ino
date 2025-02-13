// In this example we demonstrate how ActionScheduler can be used on interrupt routine
// The general idea is to push some function out from ISR context to normal execution context
// e.g. Debounce a button, we definitely can't delay in ISR handler, but we can schedule a delayed check
#include <ActionScheduler.h>

const byte ledPin = 13;
const byte buttonPin = 2;
const uint32_t DEBOUNCE_DELAY = 50; //ms
static byte state = LOW; 
ActionScheduler actionScheduler;

ActionReturn_t deBounce(void* arg){
  if(digitalRead(buttonPin) == LOW){
    digitalWrite(ledPin, state);
    state = !state;
  }

  return ACTION_ONESHOT;
}

void isrHandler() {
  actionScheduler.schedule(DEBOUNCE_DELAY, deBounce, NULL);
}

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), isrHandler, FALLING);
}

void loop() {
  static uint32_t lastTime = 0;
  uint32_t nowTime = millis();
  actionScheduler.proceed(nowTime - lastTime);
  lastTime = nowTime;
}