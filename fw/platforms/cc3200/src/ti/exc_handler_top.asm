;
; Copyright (c) 2014-2016 Cesanta Software Limited
; All rights reserved
;
; Exception handler prologue functions that determine which stack pointer
; should be used to find application's stack frame.
; See Section 2.3.7 of ARM DUI 0553A.

  .thumb

  .def hard_fault_handler_top
  .def bus_fault_handler_top
  .def mem_fault_handler_top
  .def usage_fault_handler_top

  .ref hard_fault_handler_bottom
  .ref bus_fault_handler_bottom
  .ref mem_fault_handler_bottom
  .ref usage_fault_handler_bottom

  .thumbfunc hard_fault_handler_top
  .thumbfunc mem_fault_handler_top
  .thumbfunc bus_fault_handler_top
  .thumbfunc usage_fault_handler_top

hard_fault_handler_top:
  ldr r1, hard_fault_handler_bottom_p
  b fault_handler_common

bus_fault_handler_top:
  ldr r1, bus_fault_handler_bottom_p
  b fault_handler_common

mem_fault_handler_top:
  ldr r1, mem_fault_handler_bottom_p
  b fault_handler_common

usage_fault_handler_top:
  ldr r1, usage_fault_handler_bottom_p

fault_handler_common:
  tst lr, #4
  ite eq
  mrseq r0, msp
  mrsne r0, psp
  bx r1

hard_fault_handler_bottom_p:  .word hard_fault_handler_bottom
bus_fault_handler_bottom_p:   .word bus_fault_handler_bottom
mem_fault_handler_bottom_p:   .word mem_fault_handler_bottom
usage_fault_handler_bottom_p: .word usage_fault_handler_bottom
