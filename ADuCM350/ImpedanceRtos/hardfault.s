// Based on: http://blog.frankvh.com/2011/12/07/cortex-m3-m4-hard-fault-handler/

        PUBLIC HardFault_Handler

        RSEG CODE:CODE:NOROOT(2)
        THUMB

        EXTERN hard_fault_handler_c

HardFault_Handler
        TST LR, #4
        ITE EQ
        MRSEQ R0, MSP
        MRSNE R0, PSP
        B hard_fault_handler_c
  
        END
