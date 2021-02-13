/*
 * Copyright (c) 2020 Deomid "rojer" Ryabkov
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// "Virtual" pins, created as needed and retain output values.
// TODO: Proper input support, including inpterrupts.
//       At the moment, all inputs are 0.

#include "mgos_gpio_hal.h"

#include <map>
#include <memory>

#include "ubuntu.h"

extern "C" {

struct GPIOPinCtx {
  int pin;
  enum mgos_gpio_mode mode;
  enum mgos_gpio_pull_type pull;
  bool in_value;
  bool out_value;

  GPIOPinCtx(int pin, enum mgos_gpio_mode mode)
      : pin(pin),
        mode(mode),
        pull(MGOS_GPIO_PULL_NONE),
        in_value(false),
        out_value(false) {
  }
};

static std::map<int, std::unique_ptr<GPIOPinCtx>> *pins_ = nullptr;

static GPIOPinCtx *GetPinCtx(int pin) {
  if (pins_ == nullptr) {
    pins_ = new std::map<int, std::unique_ptr<GPIOPinCtx>>();
  }
  auto it = pins_->find(pin);
  if (it == pins_->end()) return nullptr;
  return it->second.get();
}

static GPIOPinCtx *GetOrCreatePinCtx(int pin, enum mgos_gpio_mode mode) {
  char buf[8];
  GPIOPinCtx *ctx = GetPinCtx(pin);
  if (ctx == nullptr) {
    std::unique_ptr<GPIOPinCtx> new_ctx(new GPIOPinCtx(pin, mode));
    LOG(LL_INFO, ("GPIO: New pin %s, mode %d (%s)", mgos_gpio_str(pin, buf),
                  mode, (mode == MGOS_GPIO_MODE_INPUT ? "input" : "output")));
    ctx = new_ctx.get();
    pins_->emplace(pin, std::move(new_ctx));
  }
  return ctx;
}

const char *mgos_gpio_str(int pin_def, char buf[8]) {
  snprintf(buf, 8, "%d", pin_def);
  buf[7] = '\0';
  return buf;
}

bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  GetOrCreatePinCtx(pin, mode);
  return true;
}

bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  GPIOPinCtx *ctx = GetPinCtx(pin);
  if (ctx == nullptr) {
    char buf[8];
    LOG(LL_INFO, ("mgos_gpio_set_pull(%s, %d): invalid pin",
                  mgos_gpio_str(pin, buf), pull));
    return false;
  }
  ctx->pull = pull;
  return true;
}

bool mgos_gpio_read(int pin) {
  GPIOPinCtx *ctx = GetPinCtx(pin);
  if (ctx == nullptr) {
    char buf[8];
    LOG(LL_INFO, ("mgos_gpio_read(%s): invalid pin", mgos_gpio_str(pin, buf)));
    return false;
  }
  return ctx->in_value;
}

bool mgos_gpio_read_out(int pin) {
  GPIOPinCtx *ctx = GetPinCtx(pin);
  if (ctx == nullptr) {
    char buf[8];
    LOG(LL_INFO,
        ("mgos_gpio_read_out(%s): invalid pin", mgos_gpio_str(pin, buf)));
    return false;
  }
  return ctx->out_value;
}

void mgos_gpio_write(int pin, bool level) {
  GPIOPinCtx *ctx = GetPinCtx(pin);
  if (ctx == nullptr) {
    char buf[8];
    LOG(LL_INFO, ("mgos_gpio_write(%s, %d): invalid pin",
                  mgos_gpio_str(pin, buf), level));
    return;
  }
  ctx->out_value = level;
}

bool mgos_gpio_hal_enable_int(int pin) {
  char buf[8];
  LOG(LL_INFO, ("mgos_gpio_hal_enable_int(%s): not implemented",
                mgos_gpio_str(pin, buf)));
  return true;
}

bool mgos_gpio_hal_disable_int(int pin) {
  char buf[8];
  LOG(LL_INFO, ("mgos_gpio_hal_disable_int(%s): not implemented",
                mgos_gpio_str(pin, buf)));
  return true;
}

bool mgos_gpio_hal_set_int_mode(int pin, enum mgos_gpio_int_mode mode) {
  char buf[8];
  LOG(LL_INFO, ("mgos_gpio_hal_set_int_mode(%s, %d): not implemented",
                mgos_gpio_str(pin, buf), mode));
  return true;
}

bool mgos_gpio_setup_output(int pin, bool level) {
  GPIOPinCtx *ctx = GetOrCreatePinCtx(pin, MGOS_GPIO_MODE_OUTPUT);
  ctx->out_value = level;
  return true;
}

enum mgos_init_result mgos_gpio_hal_init(void) {
  return MGOS_INIT_OK;
}

}  // extern "C"
