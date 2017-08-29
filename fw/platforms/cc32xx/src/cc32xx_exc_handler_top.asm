;
; Copyright (c) 2014-2016 Cesanta Software Limited
; All rights reserved
;
; Exception handler prologue functions that determine which stack pointer
; should be used to find application's stack frame.
; See Section 2.3.7 of ARM DUI 0553A.

  .thumb

  .def cc32xx_hard_fault_handler_top
  .def cc32xx_bus_fault_handler_top
  .def cc32xx_mem_fault_handler_top
  .def cc32xx_usage_fault_handler_top

  .ref cc32xx_hard_fault_handler_bottom
  .ref cc32xx_bus_fault_handler_bottom
  .ref cc32xx_mem_fault_handler_bottom
  .ref cc32xx_usage_fault_handler_bottom

  .thumbfunc cc32xx_hard_fault_handler_top
  .thumbfunc cc32xx_mem_fault_handler_top
  .thumbfunc cc32xx_bus_fault_handler_top
  .thumbfunc cc32xx_usage_fault_handler_top

cc32xx_hard_fault_handler_top:
  ldr r1, cc32xx_hard_fault_handler_bottom_p
  b cc32xx_fault_handler_common

cc32xx_bus_fault_handler_top:
  ldr r1, cc32xx_bus_fault_handler_bottom_p
  b cc32xx_fault_handler_common

cc32xx_mem_fault_handler_top:
  ldr r1, cc32xx_mem_fault_handler_bottom_p
  b cc32xx_fault_handler_common

cc32xx_usage_fault_handler_top:
  ldr r1, cc32xx_usage_fault_handler_bottom_p

cc32xx_fault_handler_common:
  tst lr, #4
  ite eq
  mrseq r0, msp
  mrsne r0, psp
  bx r1

cc32xx_hard_fault_handler_bottom_p:  .word cc32xx_hard_fault_handler_bottom
cc32xx_bus_fault_handler_bottom_p:   .word cc32xx_bus_fault_handler_bottom
cc32xx_mem_fault_handler_bottom_p:   .word cc32xx_mem_fault_handler_bottom
cc32xx_usage_fault_handler_bottom_p: .word cc32xx_usage_fault_handler_bottom
