#include <Arduino.h>
#include <stdio.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Arduino_APDS9960.h"

#define LED_TIMER LEDC_TIMER_0
#define LED_SPEEDMODE LEDC_LOW_SPEED_MODE
#define LED_PIN GPIO_NUM_23
#define LED_CHANNEL LEDC_CHANNEL_0
#define LED_PWMFREQ LEDC_TIMER_12_BIT
#define MAX_DEC_VAL 200

static uint8_t led_state = 0;
static uint8_t led_brightness = 0;

typedef struct TaskData
{
  uint16_t duration;
  bool destroyTask = false;
} TaskData_t;

TaskData_t taskDataSync = {};

void initPWM()
{

  // configure the timer for the PWM.
  ledc_timer_config_t pwmConfig = {};
  pwmConfig.duty_resolution = LED_PWMFREQ;
  pwmConfig.timer_num = LED_TIMER;
  pwmConfig.speed_mode = LED_SPEEDMODE;
  pwmConfig.freq_hz = 10000;

  // apply the configuration
  ESP_ERROR_CHECK(ledc_timer_config(&pwmConfig));

  // configure the channel to output on a certain pin.

  ledc_channel_config_t channelConfig = {};
  channelConfig.speed_mode = LED_SPEEDMODE;
  channelConfig.timer_sel = LED_TIMER;
  channelConfig.intr_type = LEDC_INTR_DISABLE;
  channelConfig.duty = 0; // start with a duty cycle of 0 %
  channelConfig.gpio_num = LED_PIN;
  channelConfig.hpoint = 0;

  ledc_channel_config(&channelConfig);
}

uint16_t duty2dec(uint8_t duty)
{
  return MAX_DEC_VAL * pow(duty / 100.0f, 2);
}

void IRAM_ATTR timerUpdaterTask(void *params)
{
  for (;;)
  {
    ledc_update_duty(LED_SPEEDMODE, LED_CHANNEL);
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void IRAM_ATTR rampUpTask(void *params)
{
  TaskData_t *taskData = (TaskData_t *)params;

  for (uint16_t i = 0; i < 100; i++)
  {
    ledc_set_duty(LED_SPEEDMODE, LED_CHANNEL, duty2dec(i));
    // Serial.println(i / 100.0f);
    vTaskDelay(((uint32_t)(taskData->duration / 100)) / portTICK_PERIOD_MS);
  }

  if (taskData->destroyTask)
    vTaskDelete(NULL);
}

void IRAM_ATTR rampDownTask(void *params)
{
  TaskData_t *taskData = (TaskData_t *)params;

  for (uint16_t i = 100; i > 0; i--)
  {
    ledc_set_duty(LED_SPEEDMODE, LED_CHANNEL, duty2dec(i));
    // Serial.println((uint32_t)(pow(i / 100, 2) * 4095));
    vTaskDelay(((uint32_t)(taskData->duration / 100)) / portTICK_PERIOD_MS);
  }

  if (taskData->destroyTask)
    vTaskDelete(NULL);
}

void rampUp(uint16_t duration)
{
  TaskData_t taskData = {};
  taskData.duration = duration;
  taskData.destroyTask = true;

  uint32_t startDuty = (uint32_t)((ledc_get_duty(LED_SPEEDMODE, LED_CHANNEL) / 4096) * 100);
  Serial.println(startDuty);

  xTaskCreate(
      rampUpTask,
      "rampUpTask",
      10000,
      (void *)&taskData,
      0,
      NULL);
}

void rampDown(uint16_t duration)
{
  TaskData_t taskData = {};
  taskData.duration = duration;
  taskData.destroyTask = true;

  xTaskCreate(
      rampDownTask,
      "rampDownTask",
      10000,
      (void *)&taskData,
      1,
      NULL);
}

void setup()
{
  initPWM();

  Serial.begin(115200);
  Serial.println("Starting...");

  if (!APDS.begin())
  {
    Serial.println("Error initializing APDS-9960 sensor!");
  }

  xTaskCreate(
      timerUpdaterTask, // Function to implement the task
      "timerUpdater",   // Name of the task
      1000,             // Stack size in words
      NULL,             // Task input parameter
      1,                // Priority of the task
      NULL              // Task handle.
  );                    // Core where the task should run
}

void loop()
{
  // delay or suffer wdt timeouts.
  // vTaskDelay(2000 / portTICK_PERIOD_MS);

  if (APDS.gestureAvailable())
  {
    // a gesture was detected, read and print to Serial Monitor
    int gesture = APDS.readGesture();

    switch (gesture)
    {
    case GESTURE_UP:
      Serial.println("LED Strip ON.");
      if (led_state != 1)
      {
        led_state = 1;
        led_brightness = 100;
        taskDataSync.duration = 1000;
        rampUpTask((void *)&taskDataSync);
      }

      break;

    case GESTURE_DOWN:
      Serial.println("LED Strip OFF.");
      if (led_state != 0)
      {
        led_state = 0;
        led_brightness = 0;
        taskDataSync.duration = 1000;
        rampDownTask((void *)&taskDataSync);
      }
      break;

    case GESTURE_RIGHT:
      Serial.println("Brightness Down.");
      // if the led strip is actually on
      if (led_state == 1)
      {
        // decrease brightness by 25%
        led_brightness -= 25;
        // limit
        if (led_brightness <= 0)
        {
          led_brightness = 0;
          led_state = 0;
        }
      }

      ledc_set_duty(LED_SPEEDMODE, LED_CHANNEL, duty2dec(led_brightness));
      break;

    case GESTURE_LEFT:
      Serial.println("Brightness Up.");
      // if the led strip is actually on
      if (led_state == 1)
      {
        // increase brightness by 25%
        led_brightness += 25;

        // limit
        if (led_brightness >= 100)
        {
          led_brightness = 100;
        }
      }

      ledc_set_duty(LED_SPEEDMODE, LED_CHANNEL, duty2dec(led_brightness));
      break;

    default:
      // ignore
      break;
    }
  }
}