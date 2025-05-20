#include "Timer.h"
#include "printf.h"

// Định nghĩa một module tên là LightTempSensor
module LightTempSensor {
  uses {
    interface Boot;
    interface Leds;
    interface Timer<TMilli>;
    interface Read<uint16_t> as LRead; // Giao diện đọc cảm biến ánh sáng
    interface Read<uint16_t> as TRead; // Giao diện đọc cảm biến nhiệt độ
  }
}
implementation {

  // Biến lưu giá trị ánh sáng và nhiệt độ
  uint16_t lux;
  uint16_t Temp;

  // Thời gian đọc cảm biến định kỳ (1 giây)
  #define SAMPLING_FREQUENCY 1000

  // Khi hệ thống khởi động (booted), bắt đầu Timer lặp định kỳ
  event void Boot.booted() {
    call Timer.startPeriodic(SAMPLING_FREQUENCY);
  }

  // Khi Timer "bắn" sau mỗi chu kỳ → gửi yêu cầu đọc cả hai cảm biến
  event void Timer.fired() {
    call LRead.read();
    call TRead.read();
  }

  // Khi hoàn tất đọc cảm biến ánh sáng
  event void LRead.readDone(error_t result, uint16_t data) {
    if (result == SUCCESS) {
      call Leds.led0On();
      // Công thức gần đúng cho cảm biến Hamamatsu S1087-01 (TelosB)
      lux = (data * 2.5 * 6250.0) / 4096.0;

      // In dữ liệu ra Serial qua printf
      printf("-----------------------------\n");
      printf("Light raw: %u\n", data);     // Dữ liệu ADC gốc
      printf("Lux: %d \r\n", lux);              // Độ sáng (lux)
    }
  }

  // Khi hoàn tất đọc cảm biến nhiệt độ
  event void TRead.readDone(error_t result, uint16_t data) {
    if (result == SUCCESS) {
      call Leds.led1On(); // Bật LED 1 để báo đã đọc xong nhiệt độ

      // Temp (°C) = -39.6 + 0.01 × dữ liệu ADC
      Temp = (-39.60 + 0.01 * data);

      // In dữ liệu ra Serial
      printf("Temp raw: %u\n", data);       // Dữ liệu ADC thô
      printf("Temp: %d \r\n", Temp);             // Nhiệt độ (°C)
    }
  }

}
