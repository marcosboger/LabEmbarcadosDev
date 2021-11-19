/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mgos.h"
#include "mgos_adc.h"
#include "mgos_gpio.h"
#include "mgos_dht.h"
#include "mgos_blynk.h"
#include "mgos_wifi.h"
#include "esp_sleep.h"

#include "http_get.c"

#define BLYNK_AUTH_TOKEN "nma24wK-jF4Q-FIF5E0hqGh3SzhM2oUN";
#define NETWORK_NAME "Luke s House";
#define NETWORK_PASSWORD "123abc456";
#define API_KEY "ZKVAX1H28KORXNXB";

//int pinAdc = 34;
int pinAdc = 34;
int pinGpio = 13;
int pinWater = 14;
int pinDht = 26;
float humidity = 0;
float temperature = 0;
int voltage = 0;
int s_read_humidity_virtual_pin = 0;
int s_read_temperature_virtual_pin = 1;
int s_write_water_virtual_pin = 2;
int watering;
struct mgos_config_wifi_sta cfg;
uint64_t wakeup_time_sec = 3600; 
struct mgos_dht *dht = NULL;

RTC_DATA_ATTR static int boot_count = 0;
int messages_sent = 0;

static void timer_cb(void *dht) {
  humidity = mgos_dht_get_humidity(dht);
  temperature = mgos_dht_get_temp(dht);
  voltage = mgos_adc_read_voltage(pinAdc);
  http_get_task(1, temperature, 2, humidity, 3, voltage); 
  if(boot_count % 12 == 0){
    boot_count = 0;
    mgos_gpio_toggle(pinWater);
    LOG(LL_INFO, ("Watering!"));
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    mgos_gpio_toggle(pinWater);
    LOG(LL_INFO, ("Stopped Watering!"));
  }
  if(messages_sent > 4){
    LOG(LL_INFO, ("Entering Deep Sleep!"));
    esp_deep_sleep_start();
  }
  messages_sent++;
}


void default_blynk_handler(struct mg_connection *c, const char *cmd,
                                  int pin, int val, int id, void *user_data) {
  if (strcmp(cmd, "vr") == 0) {
    if (pin == s_read_humidity_virtual_pin) {
      if(!watering){
        humidity = mgos_dht_get_humidity(dht);
        blynk_virtual_write(c, s_read_humidity_virtual_pin, humidity, id);
        LOG(LL_INFO, ("Humidity: %lf", humidity));
      }
    }
    if (pin == s_read_temperature_virtual_pin) {
      if(!watering){
        temperature = mgos_dht_get_temp(dht);
        blynk_virtual_write(c, s_read_temperature_virtual_pin, temperature, id);
        LOG(LL_INFO, ("Temperature: %lf", temperature));
      }
    }
  }
  (void) user_data;
}

enum mgos_app_init_result mgos_app_init(void) {
  boot_count++;
  messages_sent = 0;
  const char *blynkAuth = BLYNK_AUTH_TOKEN;
  mgos_sys_config_set_blynk_auth(blynkAuth);
  cfg.enable = 1;
  cfg.ssid = NETWORK_NAME;
  cfg.pass = NETWORK_PASSWORD;
  bool wifi = mgos_wifi_setup_sta(&cfg);
  blynk_set_handler(default_blynk_handler, NULL);
  bool adc = mgos_adc_enable(pinAdc);
  bool gpio = mgos_gpio_setup_output(pinGpio, 1);
  bool water = mgos_gpio_setup_output(pinWater, 0);
  watering = 0;
  dht = mgos_dht_create(pinDht, DHT11);
  if(gpio && water && adc && wifi){
    esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);
    mgos_set_timer(30000 /* ms */, MGOS_TIMER_REPEAT, timer_cb, dht);
  }
  else 
    LOG(LL_INFO,
      ("Error in Setting everything up!"));
  return MGOS_APP_INIT_SUCCESS;
}
