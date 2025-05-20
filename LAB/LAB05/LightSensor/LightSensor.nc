#include "Timer.h"
#include "printf.h"
module LightSensor
{
  uses {
    interface Boot;
    interface Leds;
    interface Timer<TMilli>;
    interface Read<uint16_t>;
  }
}
implementation
{
  // Tần số lấy mẫu cảm biến ánh sáng (ms)
  #define SAMPLING_FREQUENCY 1000
  
  event void Boot.booted() {
    // Khởi động Timer: đọc cảm biến ánh sáng mỗi 1000ms (1 giây)
    call Timer.startPeriodic(SAMPLING_FREQUENCY);
    printf("System booted. Starting light sampling...\n");
  }

  event void Timer.fired() {
    // Khi timer hết, bắt đầu đọc cảm biến
    call Read.read();
  }

  event void Read.readDone(error_t result, uint16_t data) 
  {
    if (result == SUCCESS){
    uint16_t lux = (data * 2.5 * 6250.0) / 4096.0;
    printf("Anh sang raw: %u\n", data);
    printf("Lux: %d \r\n", lux);
    
    call Leds.led0Off(); 
    call Leds.led1Off();
    call Leds.led2Off(); 

    
    // Biểu diễn mức sáng bằng LED
    if (lux > 1000.0) {
      call Leds.led0On(); // Đèn đỏ: quá sáng
    } else if (lux >= 100.0) {
        call Leds.led1On(); // Đèn vàng: sáng vừa
    } else {
        call Leds.led2On(); // Đèn xanh: tối
    }
      
    }
  }
}
