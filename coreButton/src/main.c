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

#include "http_get.c"


#define BLYNK_AUTH_TOKEN "nma24wK-jF4Q-FIF5E0hqGh3SzhM2oUN";
#define NETWORK_NAME "Luke s House";
#define NETWORK_PASSWORD "123abc456";
#define API_KEY "ZKVAX1H28KORXNXB";

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
int s_write_qty_virtual_pin = 3;
int watering;
int watering_per_day = 0;
int cycles_gone = 0; 
float sleep_time_minutes = 30;
struct mgos_config_wifi_sta cfg;
struct mgos_dht *dht;



static void timer_cb(void *dht) {
  voltage = mgos_adc_read_voltage(pinAdc);
  //LOG(LL_INFO, ("Sensor voltage read: %d", voltage));
  if(!watering){
    humidity = mgos_dht_get_humidity(dht);
    temperature = mgos_dht_get_temp(dht);
    voltage = mgos_adc_read_voltage(pinAdc);
  } 
}

static void water_cb(void *dht) {
  voltage = mgos_adc_read_voltage(pinAdc);
  humidity = mgos_dht_get_humidity(dht);
  temperature = mgos_dht_get_temp(dht);
  http_get_task(1, temperature, 2, humidity, 3, voltage);
  cycles_gone++;
  LOG(LL_INFO, ("cycles_gone: %d", cycles_gone));
  LOG(LL_INFO, ("watering_per_day: %d", watering_per_day));
  int hour_water;
  if(watering_per_day)
    hour_water = (24*2)/watering_per_day;
  else  
    return;
  if(cycles_gone >= hour_water){
    cycles_gone = 0;
    mgos_gpio_toggle(pinWater);
    LOG(LL_INFO, ("Watering!"));
    vTaskDelay(4000 / portTICK_PERIOD_MS);
    mgos_gpio_toggle(pinWater);
    LOG(LL_INFO, ("Stopped Watering!"));
  }
}

void default_blynk_handler(struct mg_connection *c, const char *cmd,
                                  int pin, int val, int id, void *user_data) {
  humidity = mgos_dht_get_humidity(dht);
  temperature = mgos_dht_get_temp(dht);
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
    if (pin == s_write_qty_virtual_pin) {
      watering_per_day = val;
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
  bool adc = mgos_adc_enable(pinAdc);
  bool gpio = mgos_gpio_setup_output(pinGpio, 1);
  bool water = mgos_gpio_setup_output(pinWater, 0);
  watering = 0;
  dht = mgos_dht_create(pinDht, DHT11);
  if(gpio && water && wifi && adc){
    mgos_set_timer(sleep_time_minutes * 60 * 1000 /* ms */, MGOS_TIMER_REPEAT, water_cb, dht);
  }
  else 
    LOG(LL_INFO,
      ("Error in Setting everything up!"));
  return MGOS_APP_INIT_SUCCESS;
}
