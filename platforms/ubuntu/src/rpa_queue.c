/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// APR queue implementation with no apr-util dependencies.
// Source: https://github.com/chrismerck/rpa_queue
//
// clang-format off

#include "rpa_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

// uncomment to print debug messages
// #define QUEUE_DEBUG

struct rpa_queue_t {
  void **data;
  volatile uint32_t nelts; /**< # elements */
  uint32_t in;  /**< next empty location */
  uint32_t out;   /**< next filled location */
  uint32_t bounds;/**< max size of queue */
  uint32_t full_waiters;
  uint32_t empty_waiters;
  pthread_mutex_t *one_big_mutex;
  pthread_cond_t *not_empty;
  pthread_cond_t *not_full;
  int terminated;
};

#ifdef QUEUE_DEBUG
static void Q_DBG(const char*msg, rpa_queue_t *q) {
  fprintf(stderr, "#%d in %d out %d\t%s\n",
          q->nelts, q->in, q->out,
          msg
          );
}
#else
#define Q_DBG(x,y)
#endif

/**
 * Detects when the rpa_queue_t is full. This utility function is expected
 * to be called from within critical sections, and is not threadsafe.
 */
#define rpa_queue_full(queue) ((queue)->nelts == (queue)->bounds)

/**
 * Detects when the rpa_queue_t is empty. This utility function is expected
 * to be called from within critical sections, and is not threadsafe.
 */
#define rpa_queue_empty(queue) ((queue)->nelts == 0)

static void set_timeout(struct timespec * abstime, int wait_ms)
{
  clock_gettime(CLOCK_REALTIME, abstime);
  /* add seconds */
  abstime->tv_sec += (wait_ms / 1000);
  /* add and carry microseconds */
  long ms = abstime->tv_nsec / 1000000L;
  ms += wait_ms % 1000;
  while (ms > 1000) {
    ms -= 1000;
    abstime->tv_sec += 1;
  }
  abstime->tv_nsec = ms * 1000000L;
}

/**
 * Callback routine that is called to destroy this
 * rpa_queue_t when its pool is destroyed.
 */
void rpa_queue_destroy(rpa_queue_t * queue)
{
  /* Ignore errors here, we can't do anything about them anyway. */
  pthread_cond_destroy(queue->not_empty);
  pthread_cond_destroy(queue->not_full);
  pthread_mutex_destroy(queue->one_big_mutex);
}

/**
 * Initialize the rpa_queue_t.
 */
bool rpa_queue_create(rpa_queue_t **q, uint32_t queue_capacity)
{
  rpa_queue_t *queue;
  queue = malloc(sizeof(rpa_queue_t));
  if (!queue) {
    return false;
  }
  *q = queue;
  memset(queue, 0, sizeof(rpa_queue_t));

  if (!(queue->one_big_mutex = malloc(sizeof(pthread_mutex_t)))) return false;
  if (!(queue->not_empty = malloc(sizeof(pthread_cond_t)))) return false;
  if (!(queue->not_full = malloc(sizeof(pthread_cond_t)))) return false;

  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  int rv = pthread_mutex_init(queue->one_big_mutex, &attr);
  if (rv != 0) {
    Q_DBG("pthread_mutex_init failed", queue);
    goto error;
  }

  rv = pthread_cond_init(queue->not_empty, NULL);
  if (rv != 0) {
    Q_DBG("pthread_cond_init not_empty failed", queue);
    goto error;
  }

  rv = pthread_cond_init(queue->not_full, NULL);
  if (rv != 0) {
    Q_DBG("pthread_cond_init not_full failed", queue);
    goto error;
  }

  /* Set all the data in the queue to NULL */
  queue->data = malloc(queue_capacity * sizeof(void*));
  queue->bounds = queue_capacity;
  queue->nelts = 0;
  queue->in = 0;
  queue->out = 0;
  queue->terminated = 0;
  queue->full_waiters = 0;
  queue->empty_waiters = 0;

  return true;

error:
  free(queue);
  return false;
}

/**
 * Push new data onto the queue. Blocks if the queue is full. Once
 * the push operation has completed, it signals other threads waiting
 * in rpa_queue_pop() that they may continue consuming sockets.
 */
bool rpa_queue_push(rpa_queue_t *queue, void *data)
{
  return rpa_queue_timedpush(queue, data, RPA_WAIT_FOREVER);
}

bool rpa_queue_timedpush(rpa_queue_t *queue, void *data, int wait_ms)
{
  bool rv;

  if (wait_ms == RPA_WAIT_NONE) return rpa_queue_trypush(queue, data);

  if (queue->terminated) {
    return false; /* no more elements ever again */
  }

  rv = pthread_mutex_lock(queue->one_big_mutex);
  if (rv != 0) {
    Q_DBG("failed to lock mutex", queue);
    return false;
  }

  if (rpa_queue_full(queue)) {
    if (!queue->terminated) {
      queue->full_waiters++;
      if (wait_ms == RPA_WAIT_FOREVER) {
        rv = pthread_cond_wait(queue->not_full, queue->one_big_mutex);
      } else {
        struct timespec abstime;
        set_timeout(&abstime, wait_ms);
        rv = pthread_cond_timedwait(queue->not_full, queue->one_big_mutex,
          &abstime);
      }
      queue->full_waiters--;
      if (rv != 0) {
        pthread_mutex_unlock(queue->one_big_mutex);
        return false;
      }
    }
    /* If we wake up and it's still empty, then we were interrupted */
    if (rpa_queue_full(queue)) {
      Q_DBG("queue full (intr)", queue);
      rv = pthread_mutex_unlock(queue->one_big_mutex);
      if (rv != 0) {
        return false;
      }
      if (queue->terminated) {
        return false; /* no more elements ever again */
      } else {
        return false; //EINTR;
      }
    }
  }

  queue->data[queue->in] = data;
  queue->in++;
  if (queue->in >= queue->bounds) {
    queue->in -= queue->bounds;
  }
  queue->nelts++;

  if (queue->empty_waiters) {
    Q_DBG("sig !empty", queue);
    rv = pthread_cond_signal(queue->not_empty);
    if (rv != 0) {
      pthread_mutex_unlock(queue->one_big_mutex);
      return false;
    }
  }

  pthread_mutex_unlock(queue->one_big_mutex);
  return true;
}

/**
 * Push new data onto the queue. If the queue is full, return RPA_EAGAIN. If
 * the push operation completes successfully, it signals other threads
 * waiting in rpa_queue_pop() that they may continue consuming sockets.
 */
bool rpa_queue_trypush(rpa_queue_t *queue, void *data)
{
  bool rv;

  if (queue->terminated) {
    return false; /* no more elements ever again */
  }

  rv = pthread_mutex_lock(queue->one_big_mutex);
  if (rv != 0) {
    return false;
  }

  if (rpa_queue_full(queue)) {
    rv = pthread_mutex_unlock(queue->one_big_mutex);
    return false; //EAGAIN;
  }

  queue->data[queue->in] = data;
  queue->in++;
  if (queue->in >= queue->bounds) {
    queue->in -= queue->bounds;
  }
  queue->nelts++;

  if (queue->empty_waiters) {
    Q_DBG("sig !empty", queue);
    rv = pthread_cond_signal(queue->not_empty);
    if (rv != 0) {
      pthread_mutex_unlock(queue->one_big_mutex);
      return false;
    }
  }

  pthread_mutex_unlock(queue->one_big_mutex);
  return true;
}

/**
 * not thread safe
 */
uint32_t rpa_queue_size(rpa_queue_t *queue) {
  return queue->nelts;
}

/**
 * Retrieves the next item from the queue. If there are no
 * items available, it will block until one becomes available.
 * Once retrieved, the item is placed into the address specified by
 * 'data'.
 */
bool rpa_queue_pop(rpa_queue_t *queue, void **data)
{
  return rpa_queue_timedpop(queue, data, RPA_WAIT_FOREVER);
}

bool rpa_queue_timedpop(rpa_queue_t *queue, void **data, int wait_ms)
{
  bool rv;

  if (wait_ms == RPA_WAIT_NONE) return rpa_queue_trypop(queue, data);

  if (queue->terminated) {
    return false; /* no more elements ever again */
  }

  rv = pthread_mutex_lock(queue->one_big_mutex);
  if (rv != 0) {
    return false;
  }

  /* Keep waiting until we wake up and find that the queue is not empty. */
  if (rpa_queue_empty(queue)) {
    if (!queue->terminated) {
      queue->empty_waiters++;
      if (wait_ms == RPA_WAIT_FOREVER) {
        rv = pthread_cond_wait(queue->not_empty, queue->one_big_mutex);
      } else {
        struct timespec abstime;
        set_timeout(&abstime, wait_ms);
        rv = pthread_cond_timedwait(queue->not_empty, queue->one_big_mutex,
          &abstime);
      }
      queue->empty_waiters--;
      if (rv != 0) {
        pthread_mutex_unlock(queue->one_big_mutex);
        return false;
      }
    }
    /* If we wake up and it's still empty, then we were interrupted */
    if (rpa_queue_empty(queue)) {
      Q_DBG("queue empty (intr)", queue);
      rv = pthread_mutex_unlock(queue->one_big_mutex);
      if (rv != 0) {
        return false;
      }
      if (queue->terminated) {
        return false; /* no more elements ever again */
      } else {
        return false; //EINTR;
      }
    }
  }

  *data = queue->data[queue->out];
  queue->nelts--;

  queue->out++;
  if (queue->out >= queue->bounds) {
    queue->out -= queue->bounds;
  }
  if (queue->full_waiters) {
    Q_DBG("signal !full", queue);
    rv = pthread_cond_signal(queue->not_full);
    if (rv != 0) {
      pthread_mutex_unlock(queue->one_big_mutex);
      return false;
    }
  }

  pthread_mutex_unlock(queue->one_big_mutex);
  return true;
}

/**
 * Retrieves the next item from the queue. If there are no
 * items available, return RPA_EAGAIN.  Once retrieved,
 * the item is placed into the address specified by 'data'.
 */
bool rpa_queue_trypop(rpa_queue_t *queue, void **data)
{
  bool rv;

  if (queue->terminated) {
    return false; /* no more elements ever again */
  }

  rv = pthread_mutex_lock(queue->one_big_mutex);
  if (rv != 0) {
    return false;
  }

  if (rpa_queue_empty(queue)) {
    rv = pthread_mutex_unlock(queue->one_big_mutex);
    return false; //EAGAIN;
  }

  *data = queue->data[queue->out];
  queue->nelts--;

  queue->out++;
  if (queue->out >= queue->bounds) {
    queue->out -= queue->bounds;
  }
  if (queue->full_waiters) {
    Q_DBG("signal !full", queue);
    rv = pthread_cond_signal(queue->not_full);
    if (rv != 0) {
      pthread_mutex_unlock(queue->one_big_mutex);
      return false;
    }
  }

  pthread_mutex_unlock(queue->one_big_mutex);
  return true;
}

bool rpa_queue_interrupt_all(rpa_queue_t *queue)
{
  bool rv;
  Q_DBG("intr all", queue);
  if ((rv = pthread_mutex_lock(queue->one_big_mutex)) != 0) {
    return false;
  }
  pthread_cond_broadcast(queue->not_empty);
  pthread_cond_broadcast(queue->not_full);

  if ((rv = pthread_mutex_unlock(queue->one_big_mutex)) != 0) {
    return false;
  }

  return true;
}

bool rpa_queue_term(rpa_queue_t *queue)
{
  bool rv;

  if ((rv = pthread_mutex_lock(queue->one_big_mutex)) != 0) {
    return false;
  }

  /* we must hold one_big_mutex when setting this... otherwise,
   * we could end up setting it and waking everybody up just after a
   * would-be popper checks it but right before they block
   */
  queue->terminated = 1;
  if ((rv = pthread_mutex_unlock(queue->one_big_mutex)) != 0) {
    return false;
  }
  return rpa_queue_interrupt_all(queue);
}
