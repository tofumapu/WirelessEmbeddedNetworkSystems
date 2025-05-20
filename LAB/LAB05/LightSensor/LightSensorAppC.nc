#define NEW_PRINTF_SEMANTICS
#include "printf.h"

configuration LightSensorAppC { }

implementation {
  // Khai báo các component
  components LightSensor as App;
  components MainC;
  components LedsC;
  components new TimerMilliC() as Timer;
  components new HamamatsuS10871TsrC() as LightSource;

  // Kết nối interface giữa các component
  App.Boot -> MainC.Boot;
  App.Leds -> LedsC;
  App.Timer -> Timer;
  App.Read -> LightSource;

  // Hỗ trợ printf ra cổng Serial
  components PrintfC;
  components SerialStartC;
}
