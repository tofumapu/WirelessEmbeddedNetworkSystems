#define NEW_PRINTF_SEMANTICS
#include "printf.h"

// Cấu hình của ứng dụng LightTempSensor
configuration LightTempSensorAppC { 
} 

implementation {  
  // Khai báo và tạo các components cần thiết
  components MainC; 
  components LightTempSensor as App;
  components LedsC;
  components new TimerMilliC();  

  // Cảm biến ánh sáng
  components new HamamatsuS10871TsrC() as LightSource;

  // Cảm biến nhiệt độ
  components new SensirionSht11C() as TempSource;

  // Kết nối các interface của App với các thành phần tương ứng
  App.Boot -> MainC.Boot;
  App.Leds -> LedsC;
  App.Timer -> TimerMilliC;

  App.LRead -> LightSource;
  App.TRead -> TempSource.Temperature; 

  // Cho phép sử dụng printf và khởi tạo cổng Serial để xem log
  components PrintfC;    	
  components SerialStartC;
}
