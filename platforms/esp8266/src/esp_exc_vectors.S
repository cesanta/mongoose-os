/*
 * Handling of 8-bit loads from flash.
 *
 * Pinched from esp-open-rtos, with minor modifications.
 *
 * https://github.com/SuperHouse/esp-open-rtos/blob/12d0da0/core/exception_vectors.S
 *
 * Original copyright statement below:
 *
 * Original vector contents Copyright (C) 2014-2015 Espressif Systems
 * Additions Copyright (C) Superhouse Automation Pty Ltd and Angus Gratton
 * BSD Licensed as described in the file LICENSE
 */

#define EXCCAUSE_LOAD_STORE_ERROR 3
#define EXCCAUSE_LEVEL1_INTERRUPT 4

        .section .bss

        .balign 16
.LoadStoreErrorHandlerStack:
        .word   0       # a0
        .word   0       # (unused)
        .word   0       # a2
        .word   0       # a3
        .word   0       # a4

/***************************** Exception Vectors *****************************/

        .section .OurVectors.text, "x"

/* Note: Exception vectors must be aligned on a 256-byte (0x100) boundary or
 * they will not function properly.  (This is taken care of in the linker
 * script by ensuring .vecbase.text is aligned properly, and putting VecBase
 * right at the beginning of .vecbase.text) */
        .org    0
VecBase:
        .global VecBase
        /* IMPORTANT: exception vector literals will go here, but we
         * can't have more than 4 otherwise we push DebugExceptionVector past
         * offset 0x10 relative to VecBase. There should be ways to avoid this,
         * and also keep the VecBase offsets easy to read, but this works for
         * now. */
        .literal_position

        .org    VecBase + 0x10
_OurDebugExceptionVector:
        .global _OurDebugExceptionVector
        .type   _OurDebugExceptionVector, @function
        j       _DebugExceptionVector

        .org    VecBase + 0x20
_OurNMIExceptionVector:
        .global _OurNMIExceptionVector
        .type   _OurNMIExceptionVector, @function
        j       _OurNMIExceptionHandler

        .org    VecBase + 0x30
_OurKernelExceptionVector:
        .global _OurKernelExceptionVector
        .type   _OurKernelExceptionVector, @function
        j       _KernelExceptionVector

        .org    VecBase + 0x50
_OurUserExceptionVector:
        .global _OurUserExceptionVector
        .type   _OurUserExceptionVector, @function
        wsr     a1, excsave1
        rsr     a1, exccause
        beqi    a1, EXCCAUSE_LOAD_STORE_ERROR, LoadStoreErrorHandler
        rsr     a1, excsave1
        wsr     a0, excsave1
        j       _UserExceptionVector

        .org    VecBase + 0x70
_OurDoubleExceptionVector:
        .global _OurDoubleExceptionVector
        .type   _OurDoubleExceptionVector, @function
        j       _DoubleExceptionVector

/* Reset vector at offset 0x80 is unused, as vecbase gets reset to mask ROM
 * vectors on chip reset. */

/*************************** LoadStoreError Handler **************************/

        .section .OurVectors.text, "x"

/* Xtensa "Load/Store Exception" handler:
 * Completes L8/L16 load instructions from Instruction address space, for which
 * the architecture only supports 32-bit reads.
 *
 * Called from UserExceptionVector if EXCCAUSE is LoadStoreErrorCause
 *
 * (Fast path (no branches) is for L8UI)
 */
        .literal_position

        .balign 4
LoadStoreErrorHandler:
        .type   LoadStoreErrorHandler, @function

        /* Registers are saved in the address corresponding to their register
         * number times 4.  This allows a quick and easy mapping later on when
         * needing to store the value to a particular register number. */
        movi    sp, .LoadStoreErrorHandlerStack
        s32i    a0, sp, 0
        s32i    a2, sp, 0x08
        s32i    a3, sp, 0x0c
        s32i    a4, sp, 0x10
        rsr     a0, sar         # Save SAR in a0 to restore later

        /* Examine the opcode which generated the exception */
        /* Note: Instructions are in this order to avoid pipeline stalls. */
        rsr     a2, epc1
        movi    a3, ~3
        ssa8l   a2              # sar is now correct shift for aligned read
        and     a2, a2, a3      # a2 now 4-byte aligned address of instruction
        l32i    a4, a2, 0
        l32i    a2, a2, 4
        movi    a3, 0x00700F    # opcode mask for l8ui/l16si/l16ui
        src     a2, a2, a4      # a2 now instruction that failed
        and     a3, a2, a3      # a3 is masked instruction
        bnei    a3, 0x000002, .LSE_check_l16

        /* Note: At this point, opcode could technically be one of two things:
         *   xx0xx2 (L8UI)
         *   xx8xx2 (Reserved (invalid) opcode)
         * It is assumed that we'll never get to this point from an illegal
         * opcode, so we don't bother to check for that case and presume this
         * is always an L8UI. */

        movi    a4, ~3
        rsr     a3, excvaddr    # read faulting address
        and     a4, a3, a4      # a4 now word aligned read address

        l32i    a4, a4, 0       # perform the actual read
        ssa8l   a3              # sar is now shift to extract a3's byte
        srl     a3, a4          # shift right correct distance
        extui   a4, a3, 0, 8    # mask off bits we need for an l8

.LSE_post_fetch:
        /* We jump back here after either the L8UI or the L16*I routines do the
         * necessary work to read the value from memory.
         * At this point, a2 holds the faulting instruction and a4 holds the
         * correctly read value.

         * Restore original SAR value (saved in a0) and update EPC so we'll
         * return back to the instruction following the one we just emulated */

        /* Note: Instructions are in this order to avoid pipeline stalls */
        rsr     a3, epc1
        wsr     a0, sar
        addi    a3, a3, 0x3
        wsr     a3, epc1

        /* Stupid opcode tricks: The jumptable we use later on needs 16 bytes
         * per entry (so we can avoid a second jump by just doing a RFE inside
         * each entry).  Unfortunately, however, Xtensa doesn't have an addx16
         * operation to make that easy for us.  Luckily, all of the faulting
         * opcodes we're processing are guaranteed to have bit 3 be zero, which
         * means if we just shift the register bits of the opcode down by 3
         * instead of 4, we will get the register number multiplied by 2.  This
         * combined with an addx8 will give us an effective addx16 without
         * needing any extra shift operations. */
        extui   a2, a2, 3, 5    # a2 is now destination register 0-15 times 2

        bgei    a2, 10, .LSE_assign_reg     # a5..a15 use jumptable
        beqi    a2, 2, .LSE_assign_a1       # a1 uses a special routine

        /* We're storing into a0 or a2..a4, which are all saved in our "stack"
         * area.  Calculate the correct address and stick the value in there,
         * then just do our normal restore and RFE (no jumps required, which
         * actually makes a0..a4 substantially faster). */
        addx2   a2, a2, sp
        s32i    a4, a2, 0

        /* Restore all regs and return */
        l32i    a0, sp, 0
        l32i    a2, sp, 0x08
        l32i    a3, sp, 0x0c
        l32i    a4, sp, 0x10
        rsr     a1, excsave1    # restore a1 saved by UserExceptionVector
        rfe

.LSE_assign_reg:
        /* At this point, a2 contains the register number times 2, a4 is the
         * read value. */

        /* Calculate the jumptable address, and restore all regs except a2 and
         * a4 so we have less to do after jumping. */
        /* Note: Instructions are in this order to avoid pipeline stalls. */
        movi    a3, .LSE_jumptable_base
        l32i    a0, sp, 0
        addx8   a2, a2, a3      # a2 is now the address to jump to
        l32i    a3, sp, 0x0c

        jx      a2

        .balign 4
.LSE_check_l16:
        /* At this point, a2 contains the opcode, a3 is masked opcode */
        movi    a4, 0x001002    # l16si or l16ui opcode after masking
        bne     a3, a4, .LSE_wrong_opcode

        /* Note: At this point, the opcode could be one of two things:
         *   xx1xx2 (L16UI)
         *   xx9xx2 (L16SI)
         * Both of these we can handle. */

        movi    a4, ~3
        rsr     a3, excvaddr    # read faulting address
        and     a4, a3, a4      # a4 now word aligned read address

        l32i    a4, a4, 0       # perform the actual read
        ssa8l   a3              # sar is now shift to extract a3's bytes
        srl     a3, a4          # shift right correct distance
        extui   a4, a3, 0, 16   # mask off bits we need for an l16

        bbci    a2, 15, .LSE_post_fetch  # Not a signed op
        bbci    a4, 15, .LSE_post_fetch  # Value does not need sign-extension

        movi    a3, 0xFFFF0000
        or      a4, a3, a4      # set 32-bit sign bits
        j       .LSE_post_fetch

.LSE_wrong_opcode:
        /* If we got here it's not an opcode we can try to fix, so bomb out.
         * Restore registers so any dump the fatal exception routine produces
         * will have correct values */
        wsr     a0, sar
        l32i    a0, sp, 0
        /*l32i    a2, sp, 0x08*/
        l32i    a3, sp, 0x0c
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        mov     a2, a1
        movi    a3, 0
#ifdef RTOS_SDK
        movi    a0, system_restart_in_nmi
#else
        movi    a0, system_restart_local_sdk
#endif
        callx0  a0

        .balign 4
.LSE_assign_a1:
        /* a1 is saved in excsave1, so just update that with the value, */
        wsr     a4, excsave1
        /* Then restore all regs and return */
        l32i    a0, sp, 0
        l32i    a2, sp, 0x08
        l32i    a3, sp, 0x0c
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        rfe

        .balign 4
.LSE_jumptable:
        /* The first 5 entries (80 bytes) of this table are unused (registers
         * a0..a4 are handled separately above).  Rather than have a whole bunch
         * of wasted space, we just pretend that the table starts 80 bytes
         * earlier in memory. */
        .set    .LSE_jumptable_base, .LSE_jumptable - (16 * 5)

        .org    .LSE_jumptable_base + (16 * 5)
        mov     a5, a4
        l32i    a2, sp, 0x08
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 6)
        mov     a6, a4
        l32i    a2, sp, 0x08
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 7)
        mov     a7, a4
        l32i    a2, sp, 0x08
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 8)
        mov     a8, a4
        l32i    a2, sp, 0x08
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 9)
        mov     a9, a4
        l32i    a2, sp, 0x08
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 10)
        mov     a10, a4
        l32i    a2, sp, 0x08
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 11)
        mov     a11, a4
        l32i    a2, sp, 0x08
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 12)
        mov     a12, a4
        l32i    a2, sp, 0x08
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 13)
        mov     a13, a4
        l32i    a2, sp, 0x08
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 14)
        mov     a14, a4
        l32i    a2, sp, 0x08
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        rfe

        .org    .LSE_jumptable_base + (16 * 15)
        mov     a15, a4
        l32i    a2, sp, 0x08
        l32i    a4, sp, 0x10
        rsr     a1, excsave1
        rfe

/*
 * We store NMI registers here before handing off to SDK handler.
 * These are then used by our soft WDT exception interceptor (ShowCritical).
 */
_OurNMIExceptionHandler:
        .global _OurNMIExceptionHandler
        .type   _OurNMIExceptionHandler, @function
        wsr.excsave3 a0
        movi    a0, g_exc_regs
        // 0 ... 60 - a0 ... a15
        s32i    a1, a0, 4
        s32i    a2, a0, 8
        s32i    a3, a0, 12
        s32i    a4, a0, 16
        s32i    a5, a0, 20
        s32i    a6, a0, 24
        s32i    a7, a0, 28
        s32i    a8, a0, 32
        s32i    a9, a0, 36
        s32i    a10, a0, 40
        s32i    a11, a0, 44
        s32i    a12, a0, 48
        s32i    a13, a0, 52
        s32i    a14, a0, 56
        s32i    a15, a0, 60
        // 64 - pc
        rsr.sar a1
        s32i    a1, a0, 68
        rsr.litbase a1
        s32i    a1, a0, 72
        // 76 - sr176, not used
        // 80 - sr208, not used
        rsr.ps  a1
        s32i    a1, a0, 84
        // Put a0 where it belongs
        rsync
        rsr.excsave3 a1
        s32i    a1, a0, 0   // a0
        s32i    a1, a0, 64  // pc
        // Restore a1
        l32i    a1, a0, 4
        // Restore a0 (_NMIExceptionVector will stash it to excsave3 again)
        rsr.excsave3 a0
        j       _NMIExceptionVector
