/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <Arduino.h>

void setup(void) {
  printf("Hello, Arduino world!\r\n");
  pinMode(2, OUTPUT);
}

class TestClass {
 public:
  TestClass() {
    printf("constructed\n");
  }
  ~TestClass() {
    printf("destroyed %d\n", a_);
  }
  int a_ = 0;
};

void loop() {
  static boolean value = 0;
  digitalWrite(2, value);
  printf("%s\r\n", (value == 0 ? "Tick" : "Tock"));
  delay(500);
  value = (value ? 0 : 1);
  TestClass *foo = new TestClass();
  foo->a_ = 1;
  delete (foo);
}
