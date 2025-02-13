// You can treat the ActionScheduler queue as an event queue, the arg as event, and Callback as event handler
// The idea of event is to decouple code library as much as possible, so you can easily send event from one library
// to trigger an action from another library, without doing much thing in the callback
#include <ActionScheduler.h>

typedef enum{
  EVENT_APP_START,
  EVENT_SOME_ISR_TRIGGERED
}event_t;

const byte buttonPin = 2;
ActionScheduler actionScheduler;

// In the architecture of an event system, the event handler is the main place where you do inter-library actions
ActionReturn_t eventHandler(void* arg){
  event_t evt = (event_t)(uint32_t)arg;  // To suppress the compiler complain
  switch(evt)
  {
    case EVENT_APP_START:
      Serial.println("Event App Start");
      // Do some thing when App started, e.g. play music, display a fancy splash screen
      // ...
    break;
    case EVENT_SOME_ISR_TRIGGERED:
      Serial.println("Event Some ISR Triggered");
      // Do a bunch of things when some ISR triggered
      // ...
    break;
    default:
      Serial.println("Unknown Event");
    break;
  }
  return ACTION_ONESHOT;
}

void sendEvent(event_t evt){
  // A schedule to 0 delay is to push event to the ActionScheduler queue and mark it to be processed ASAP
  actionScheduler.schedule(0, eventHandler, (void*)evt);
}

void isrHandler() {
  sendEvent(EVENT_SOME_ISR_TRIGGERED);
}

void setup() {
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(buttonPin), isrHandler, FALLING);
  sendEvent(EVENT_APP_START);
}

void loop() {
  static uint32_t lastTime = 0;
  uint32_t nowTime = millis();
  actionScheduler.proceed(nowTime - lastTime);
  lastTime = nowTime;
}