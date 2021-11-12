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

#define BLYNK_AUTH_TOKEN "nma24wK-jF4Q-FIF5E0hqGh3SzhM2oUN";
#define NETWORK_NAME "Luke s House";
#define NETWORK_PASSWORD "123abc456";

//int pinAdc = 34;
int pinGpio = 13;
int pinWater = 14;
int pinDht = 26;
float humidity = 0;
float temperature = 0;
int s_read_humidity_virtual_pin = 0;
int s_read_temperature_virtual_pin = 1;
int s_write_water_virtual_pin = 2;
int watering;
struct mgos_config_wifi_sta cfg;
//int wakeup_time_sec = 10; 



static void timer_cb(void *dht) {
  //int voltage = mgos_adc_read_voltage(pinAdc);
  //LOG(LL_INFO, ("Sensor voltage read: %d", voltage));
  if(!watering){
    humidity = mgos_dht_get_humidity(dht);
    temperature = mgos_dht_get_temp(dht);
    LOG(LL_INFO, ("Temperature: %lf", temperature));
    LOG(LL_INFO, ("Humidity: %lf", humidity));
  } 
}

void default_blynk_handler(struct mg_connection *c, const char *cmd,
                                  int pin, int val, int id, void *user_data) {
  if (strcmp(cmd, "vr") == 0) {
    if (pin == s_read_humidity_virtual_pin) {
      if(!watering)
        blynk_virtual_write(c, s_read_humidity_virtual_pin, humidity, id);
    }
    if (pin == s_read_temperature_virtual_pin) {
      if(!watering)
        blynk_virtual_write(c, s_read_temperature_virtual_pin, temperature, id);
    }
  } 
  if (strcmp(cmd, "vw") == 0) {
    if (pin == s_write_water_virtual_pin) {
      mgos_gpio_toggle(pinWater);
      watering = !watering;
    }
  }
  (void) user_data;
}

enum mgos_app_init_result mgos_app_init(void) {
  const char *blynkAuth = BLYNK_AUTH_TOKEN;
  mgos_sys_config_set_blynk_auth(blynkAuth);
  cfg.enable = 1;
  cfg.ssid = NETWORK_NAME;
  cfg.pass = NETWORK_PASSWORD;
  bool wifi = mgos_wifi_setup_sta(&cfg);
  blynk_set_handler(default_blynk_handler, NULL);
  //bool adc = mgos_adc_enable(pinAdc);
  bool gpio = mgos_gpio_setup_output(pinGpio, 1);
  bool water = mgos_gpio_setup_output(pinWater, 0);
  watering = 0;
  struct mgos_dht *dht = mgos_dht_create(pinDht, DHT11);
  //esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);
  //esp_deep_sleep_start();
  if(gpio && water && wifi)
    mgos_set_timer(500 /* ms */, MGOS_TIMER_REPEAT, timer_cb, dht);
  else 
    LOG(LL_INFO,
      ("Error in Setting everything up!"));
  return MGOS_APP_INIT_SUCCESS;
}
