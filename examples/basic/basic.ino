//Optionally config the max nodes
#define ACTION_SCHEDULER_MAX_NODES 64
#include <ActionScheduler.h>

ActionScheduler actionScheduler;

ActionReturn_t printTask(void* arg){
  Serial.println("Hello ActionScheduler!");
  return ACTION_RELOAD;
}

ActionReturn_t printTaskOneShot(void* arg){
  Serial.println("This is oneshot!");
  return ACTION_ONESHOT;
}

void setup() {
  Serial.begin(115200);
  actionScheduler.schedule(1000, printTask, NULL);
  actionScheduler.schedule(2000, printTaskOneShot, NULL);
}

void loop() {
  static uint32_t lastTime = 0;
  uint32_t nowTime = millis();
  actionScheduler.proceed(nowTime - lastTime);
  lastTime = nowTime;
}