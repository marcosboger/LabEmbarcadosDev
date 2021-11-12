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

int pinAdc = 34;
int pinGpio = 13;
//int pinWater = 14;
int pinDht = 26;
float humidity = 0;
float temperature = 0;
struct mgos_dht *dht = NULL;


static void timer_cb(void *dht) {
  int voltage = mgos_adc_read_voltage(pinAdc);
  LOG(LL_INFO, ("Sensor Voltage Read: %d", voltage));
  humidity = mgos_dht_get_humidity(dht);
  temperature = mgos_dht_get_temp(dht);
  LOG(LL_INFO, ("Temperature: %lf", temperature));
  LOG(LL_INFO, ("Humidity: %lf", humidity));
}

enum mgos_app_init_result mgos_app_init(void) {
  bool adc = mgos_adc_enable(pinAdc);
  bool gpio = mgos_gpio_setup_output(pinGpio, 1);
  dht = mgos_dht_create(pinDht, DHT11);
  if(adc && gpio){
    mgos_set_timer(1000 /* ms */, MGOS_TIMER_REPEAT, timer_cb, dht);
  }
  else 
    LOG(LL_INFO,
      ("Error in Setting everything up!"));
  return MGOS_APP_INIT_SUCCESS;
}
