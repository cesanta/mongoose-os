#include <stm32_sdk_hal.h>
#include "common/queue.h"
#include "mgos_i2c.h"

enum i2c_it_status { I2C_IN_PROGRESS, I2C_COMPLETED, I2C_ERROR };

struct mgos_i2c {
  I2C_HandleTypeDef *hi2c;
  uint16_t addr;
  enum i2c_it_status status;
  SLIST_ENTRY(mgos_i2c) conn_list;
};

SLIST_HEAD(s_i2c_connections, mgos_i2c)
s_i2c_connections = SLIST_HEAD_INITIALIZER(s_i2c_connections);

static struct mgos_i2c *get_conn_by_handle(I2C_HandleTypeDef *hi2c) {
  struct mgos_i2c *c, *conn_tmp;
  SLIST_FOREACH_SAFE(c, &s_i2c_connections, conn_list, conn_tmp) {
    if (c->hi2c == hi2c) {
      return c;
    }
  }

  return NULL;
}

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  struct mgos_i2c *c = get_conn_by_handle(hi2c);
  if (c != NULL) {
    c->status = I2C_COMPLETED;
  }
}

void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  struct mgos_i2c *c = get_conn_by_handle(hi2c);
  if (c != NULL) {
    c->status = I2C_COMPLETED;
  }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
  struct mgos_i2c *c = get_conn_by_handle(hi2c);
  if (c != NULL) {
    c->status = I2C_ERROR;
  }
}

static void reset_io_state(struct mgos_i2c *c) {
  c->status = I2C_IN_PROGRESS;
}

static int wait_for_io_completion(struct mgos_i2c *c) {
  while (c->status == I2C_IN_PROGRESS) {
    HAL_Delay(1);
  }

  return c->status == I2C_COMPLETED;
}

struct mgos_i2c *mgos_i2c_create(const struct sys_config_i2c *cfg) {
  if (cfg->enable) {
    struct mgos_i2c *i2c = calloc(1, sizeof(*i2c));
    i2c->hi2c = &I2C_DEFAULT;
    SLIST_INSERT_HEAD(&s_i2c_connections, i2c, conn_list);
    return i2c;
  } else {
    return NULL;
  }
}

void mgos_i2c_stop(struct mgos_i2c *conn) {
  reset_io_state(conn);
  HAL_StatusTypeDef status = HAL_I2C_Master_Sequential_Transmit_IT(
      conn->hi2c, conn->addr, NULL, 0, I2C_LAST_FRAME);
  if (status == HAL_OK) {
    wait_for_io_completion(conn);
  }
}

bool mgos_i2c_write(struct mgos_i2c *conn, uint16_t addr, const void *data,
                    size_t len, bool stop) {
  conn->addr = (addr << 1);
  reset_io_state(conn);
  HAL_StatusTypeDef status = HAL_I2C_Master_Sequential_Transmit_IT(
      conn->hi2c, conn->addr, (void *) data, len,
      stop ? I2C_FIRST_AND_LAST_FRAME : I2C_FIRST_AND_NEXT_FRAME);
  if (status != HAL_OK) {
    return false;
  }
  if (wait_for_io_completion(conn) != 1) {
    return false;
  }
  (void) stop;
  return true;
}

bool mgos_i2c_read(struct mgos_i2c *conn, uint16_t addr, void *data, size_t len,
                   bool stop) {
  conn->addr = (addr << 1);
  reset_io_state(conn);
  HAL_StatusTypeDef status = HAL_I2C_Master_Sequential_Receive_IT(
      conn->hi2c, conn->addr, data, len,
      stop ? I2C_FIRST_AND_LAST_FRAME : I2C_FIRST_AND_NEXT_FRAME);
  if (status != HAL_OK || wait_for_io_completion(conn) != 1) {
    return false;
  }
  return true;
}

void mgos_i2c_close(struct mgos_i2c *conn) {
  struct mgos_i2c *c, *conn_tmp;
  SLIST_FOREACH_SAFE(c, &s_i2c_connections, conn_list, conn_tmp) {
    if (c == conn) {
      SLIST_REMOVE(&s_i2c_connections, c, mgos_i2c, conn_list);
      free(conn);
      break;
    }
  }
}
