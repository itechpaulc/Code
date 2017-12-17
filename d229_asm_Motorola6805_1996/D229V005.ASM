
; -------------------------------------------------
;
; -------------------------------------------------
; $Author:: Paul L Calinawan                      $
; $Date::                                         $
; $Revision::                                     $
; $Version::                                      $
; -------------------------------------------------
; -------------------------------------------------
; This is the original D229 (Version 1.0)
; relay board with MC68HC705J1A memory layout.
; -------------------------------------------------
; Changes Made:
;    1.  Add a definition, RTIFR, for Bit 2 of Timer Status and Control
;        Register ($08).  RTIFR is the write-only Real-Time Interrupt Flag
;        Reset bit.
;    2.  Clear the RTIF (Real-Time Interrupt Flag) by writing an one to the
;        RTIFR instead of the RTIF itself.
;    3.  Always enable IRQ in the beginning of main loop (safty feature).
;    4.  Set the 705J1 Mask Option Register to 0x09 to inhibit the 705J1's
;        Port Pulldown feature and enable the COP Watchdog.
;    5.  Add "trap" loops around functions (safety feature).
; -------------------------------------------------
; 1/9/96    Added HeartBeat on PortB 5
;           CommTimeOut is now 30 seconds
;           Added ADDRESS DIP Switch handling
; -------------------------------------------------


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; PORT DEFINITIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PORT_A          EQU $00         ;Port A
PORT_B          EQU $01         ;Port B

DDRA            EQU $04         ;Port A Direction Register
DDRB            EQU $05         ;Port B Direction Register

PANEL_OUT       EQU 1           ;MonoBus output port_B
PANEL_IN        EQU 0           ;MonoBus input port_B

TCSR            EQU $08         ;Timer Control and Status Register
TCR             EQU $09         ;Timer Counter Register

RTIFR           EQU 2           ; ***705*** Bit 2 of Timer Control Register

COP_SERVICE     EQU $07F0       ;COP Service Reqister   ;;;;;;;;;; <-- J1
COP             EQU 0


RECV_PLEN       EQU $05         ;Receive packet length

REPLY_ACK       EQU $55         ;Ack character for addressed poll


STEADY_DATA     EQU $00         ;RELAY STATE data definition
PULSE_DATA      EQU $01
OFF_DATA        EQU $02         ;03 is undefined


PULSE_CAD       EQU $0F         ; 0000 1111 each bit is latched
                                ; every 150 msec to acheive the pulse


SHIFT_1_5       EQU $00         ;used to shift RELAY data mask
SHIFT_2_6       EQU $02
SHIFT_3_7       EQU $04
SHIFT_4_8       EQU $06


HEARTBEAT       EQU $20         ; Heart Beat Mask, Port B 5


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; VARIABLES
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



        ORG $00C0


T8pt2msTicks    DS  1

Flags           DS  1           ;a byte for various flags
mask            DS  1

DROP_IRQ        EQU 7           ;sometimes need to drop IRQs
SW_RTIF         EQU 6           ;software RTIF

RECV_FULL       EQU 5           ;Receive register full flag

ALIEN_PACKET    EQU 3


NoCommTimer         DS  1       ;no comm timeout variable
COMM_TIMEOUT        EQU $96     ;comm timeout: 150 * 150 ms = 25 sec
                                ;clear port A outputs on comm timeout

CState              DS 1                        ;Comm state machine

SReg1               DS 1                        ;temporary

tempx               DS 1                        ;temporary for IRQ
CommReg             DS 1                        ;IRQ interrupt data storage

chk                 DS 1                        ;data check sum

CBuff               DS 5                        ;communications buffer
                                                ;ADR - LEN - DL - DH - CHK
                                                ;00  - 01  - 02 - 03 - 04

CadenceShift        DS 1
CadenceMask         DS 1

PortMask            DS 1

lowRLY              DS 1
highRLY             DS 1
cpylowRLY           DS 1
cpyhighRLY          DS 1
tmplowRLY           DS 1
tmphighRLY          DS 1

currRLYnum          DS 1                        ; 1 to 8
currPortNum         DS 1                        ; 0 to 7


selectRLYshift      DS 1



BBRD_ADDR           EQU $0A                     ;Base Board Address

DIPSWITCH           EQU $1C                     ; Dip Address Switch Mask,
                                                ; Port B2,B3 and B4

BoardAddress        DS 1                        ; calculated and updated every
                                                ; main loop so calculation is done
                                                ; outside of the comm interrupt


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; INITIALIZATION ROUTINE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ORG $0300               ;;;;;;;;;; <-- J1

StartUp
        CLR     PORT_A
        CLR     PORT_B          ;STROBE and PANEL_OUT bits are cleared

        LDA     #$FF            ;port A are all outputs
        STA     DDRA

        LDA     #%00100010      ;port B0 is input - MonoBus In
                                ;port B1 is output - MonoBus Out
                                ;port B2-B4 is input - Address Switch
                                ;port B5 is output - heartBeat LED
        STA     DDRB

        LDA     #%00010000      ;RTIE = true, 8.2ms RTI rate
        STA     TCSR

;
; RAM (0xC0 to 0xFF) Clearing Routine
;
        LDX     #$C0
ClearNextByte
        CLR     ,x
        INCX
        BNE     ClearNextByte
;

;
;Relay data initialized to off states
;
        LDA     #$AA

        STA     cpylowRLY
        STA     cpyhighRLY

        STA     lowRLY
        STA     highRLY

        INC     T8pt2msTicks    ;start timer immediately

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MAIN
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

MainLoop

        LDA     #$80            ; ***705*** enable IRQ
        STA     $0A             ; ***705***
        CLI                     ;enable interrupts

;
; Update system ticks
;
        BRCLR   SW_RTIF,Flags,doneUpdateTicks   ;Not 8.2 yet ?
        BCLR    SW_RTIF,Flags                   ;Its 8.2 now

        JSR     SyncSerial                      ;Re-sync on 8.2ms Mono-bus
                                                ;idle time (2400)

        DEC     T8pt2msTicks                    ;255 to 00
        BNE     doneUpdateTicks                 ;if Z == 00 branch doneUpdateTicks
                                                ;else time for 150 ms tasks

        LDA     #$13                            ;re init down counter

        STA     T8pt2msTicks                    ;8.2ms x 13h = 150 ms
        BRA     do150msTasks

doneUpdateTicks

        LDA     PORT_B                          ;refresh board address
        
        COMA                                    ;switch data invert
                                                ;ON = 0, OFF = 1
        
        AND     #DIPSWITCH                      ;mask address data
            
        ASRA                                    ;adjust data
        ASRA                                    ;
        ADD     #BBRD_ADDR                      ;
        STA     BoardAddress                    ;update ram

                                                ;do nothing but clear WDog
        JMP     endMain                         ;and back to MainLoop


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
trap1 BRA trap1
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
do150msTasks

            INC     NoCommTimer                 ;track # of 150 ms chnks no comm
            LDA     NoCommTimer
            CMP     #COMM_TIMEOUT
            BLO     chkCadence                  ;A < #COMM ? skip comm trouble code

            BEQ     commtrbl                    ;do comm trouble if same
            DEC     NoCommTimer                 ;when higher dec NoCommTimer --

commtrbl    CLR     PORT_A                      ;clear all outputs on port A
            JMP     endMain

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;call comm state machine

chkCadence

            LDA     CadenceShift
            CMP     #$08                        ;shift overflow ?
            BEQ     getComDat                   ;finish a cadence before
            BRA     doRelays                    ;latching to new data

getComDat
            JSR     cpyComDt


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;call relay state machines

doRelays

            JSR     doUpdtRLY


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Run heartbeat if comm is ok

            LDA     NoCommTimer
            CMP     #$05                        ;stop beat if 1 sec of no comm
            BLS     BeatHeart

            LDA     #HEARTBEAT                  ;clear heart beat port
            COMA                                ;invert
            AND     PORT_B                      ;
            STA     PORT_B                      ;write to port

            BRA     endMain

BeatHeart                                       ;toggle heart beat
            LDA     PORT_B                      ;clear heart beat port
            EOR     #HEARTBEAT                  ;toggle
            STA     PORT_B                      ;write to port


endMain

            JMP     clearWDog

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Clear Watchdog reset routine
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
trap BRA trap
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


clearWDog

        SEI                     ;disable interrupt before resetting watchdog
        CLRA
        STA     COP_SERVICE     ;reset watchdog
        RSP                     ;reset the stack pointer
        JMP     MainLoop        ;start main loop again


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
trap3 BRA trap3
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; end clear watchdog
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;end main loop
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;******************
trap4 BRA trap4
;******************

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Function returns RLY data through the reg A

getData
            LDA     cpylowRLY                   ;local copies
            STA     tmplowRLY
            LDA     cpyhighRLY
            STA     tmphighRLY

            LDA     currRLYNum
            CMP     #$04                        ;if (currRLYNum  < 4)
            BLS     case1                       ;switch statement
            SUB     #$04                        ;currRLYNum -= 4

case1       CMP     #$01                        ;if data != 1
            BNE     case2                       ;
            LDA     #SHIFT_1_5                  ;case 1
            STA     selectRLYshift
            BRA     caseDone

case2       CMP     #$02                        ;if data != 2
            BNE     case3
            LDA     #SHIFT_2_6                  ;case 2
            STA     selectRLYshift
            BRA     caseDone

case3       CMP     #$03                        ;if data != 3
            BNE     case4
            LDA     #SHIFT_3_7                  ;case 3
            STA     selectRLYshift
            BRA     caseDone

case4       CMP     #$04                        ;if data != 4
            BNE     caseDone
            LDA     #SHIFT_4_8
            STA     selectRLYshift

caseDone    LDA     currRLYNum
            CMP     #$04                        ;if (currRLYNum  < 4)
            BLS     doLoRLY

doHiRLY     LDA     selectRLYshift
chckHshf    CMP     #$00                        ;shift?
            BNE     shftHmsk                    ;if cadenceShift != 0 shift
            LDA     tmpHighRLY
            AND     #$03
            BRA     returnA
shftHmsk    LSR     tmpHighRLY                  ; >>
            DECA
            BRA     chckHshf

doLoRLY     LDA     selectRLYshift
chckLshf    CMP     #$00                        ;shift?
            BNE     shftLmsk                    ;if cadenceShift != 0 shift
            LDA     tmpLowRLY
            AND     #$03
            BRA     returnA
shftLmsk    LSR     tmpLowRLY                   ; >>
            DECA
            BRA     chckLshf

returnA
            RTS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; end get data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;******************
trap5 BRA trap5
;******************




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Subroutines for controlling the port outputs for the relays

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; PORT_A = PORT_A | PortMask

;******************
trap6 BRA trap6
;******************

setPort
            LDA     PORT_A
            ORA     PortMask
            STA     PORT_A

            RTS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; PORT_A = PORT_A & ~PortMask

;******************
trap7 BRA trap7
;******************

clearPort
            LDA     PortMask        ;invert mask
            EOR     #$FF
            STA     PortMask

            LDA     PORT_A
            AND     PortMask        ;&
            STA     PORT_A

            RTS


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; Based on Current Cadence, set or clear

;******************
trap8 BRA trap8
;******************

playPulse

            LDA     CadenceMask
            AND     #PULSE_CAD

            BNE     doClear         ;if ( CadenceMask & PULSE_CAD) == 0
                                    ;clearPort else
            JSR     setPort         ;setPort
            RTS

doClear     JSR     clearPort
            RTS




;******************************************************************************
;This is the communications Sub routine
;that updates the copy of relay data
;******************************************************************************

;******************
trap9 BRA trap9
;******************

cpyComDt
            LDA     CadenceShift
            CMP     #$08                    ;if (CadenceShift == 8),
                                            ;done with cadence
            BNE     CadNotDn

            LDA     lowRLY                  ;dont want to change data
            STA     cpylowRLY               ;in the middle of cadence
                                            ;get copy of relay data 1-4
            LDA     highRLY                 ;get copy of relay data 5-8
            STA     cpyhighRLY

            CLR     CadenceShift
CadNotDn

            RTS

;******************************************************************************
; End copy comm data
;******************************************************************************




;******************************************************************************
;This is the RLY UPDATE sub routine
;******************************************************************************

;******************
trap10 BRA trap10
;******************

doUpdtRLY

            LDA     #$01
            STA     currRLYnum                  ;curr RLY = 1
            CLR     currPortNum                 ;curr port = 0

            LDA     #$80                        ;start with 10000000
            STA     CadenceMask

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; CadenceMask >>= CadenceShift

            LDA     CadenceShift
checkZer    CMP     #$00                        ;shift?
            BNE     shiftCad                    ;if cadenceShift != 0 shift
            BRA     doneShft
shiftCad    LSR     CadenceMask                 ; >>
            DECA
            BRA     checkZer
doneShft

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; for(;currPortNum <= 7; currPortNum++, currRLYnum++)

portLoop    LDA     currPortNum
            CMP     #$07                        ;while currPortNum <= 7
            BLS     doRLYs
            BRA     donePort

doRLYs
            LDA     #$01
            STA     PortMask                    ;PortMask = 1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;; PortMask <<=  PortMaskShift

            LDA     currPortnum
chckZpms    CMP     #$00                        ;shift?
            BNE     shftPmsk                    ;if cadenceShift != 0 shift
            BRA     donePmsk
shftPmsk    LSL     PortMask                    ; <<
            DECA
            BRA     chckZpms
donePmsk

            JSR     getData                     ;latch RLY data. Return
                                                ; with data on reg A

            CMP     #OFF_DATA                   ;if data != OFF
            BNE     checkOn                     ;check for ON
            JSR     clearPort
            BRA     endFor

checkOn     CMP     #STEADY_DATA                ;if data != STEADY
            BNE     chckPlse                    ;check for PULSE
            JSR     setPort
            BRA     endFor

chckPlse    CMP     #PULSE_DATA                 ;if data != PULSE
            BNE     chckUndf                    ;check for UNDEF
            JSR     playPulse
            BRA     endFor
chckUndf                                        ;UNDEF, do nothing

endFor
            INC     currPortnum                 ;currPortnum++
            INC     currRLYNum                  ;currRLYNum++
            BRA     portLoop

donePort
            INC     CadenceShift

            RTS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;End for RLY UPDATE sub routine
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;******************
trap11 BRA trap11
;******************



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Routine to re-synchronize serial communication
;

SyncSerial
        BRSET   RECV_FULL,Flags,doneSyncSerial
        LDA     CState          ;clear buffer if CState < 7
        CMP     #$07            ;2 bytes over packet length
        BCC     doneSyncSerial
        BCLR    ALIEN_PACKET,flags
        CLR     CState
doneSyncSerial
        BCLR    RECV_FULL,Flags
        RTS


;******************
trap12 BRA trap12
;******************

;******************************************************************************
;This is the Real-Time Interrupt routine
;******************************************************************************

RealTimeInt BSET    RTIFR,TCSR                   ;***705*** Clear RTIF by writing 1 to RTIFR
            BSET    SW_RTIF,Flags                ;Set copy of flag
            RTI
;******************************************************************************
; END TIMER ISR
;******************************************************************************




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
trap13 BRA trap13
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;******************************************************************************
;******************************************************************************
;This is the SERIAL INPUT data interrupt routine
;******************************************************************************

IRQInt                                       ;10 (interrupt to get here)
            BRCLR   DROP_IRQ,Flags,doint     ;5  ;drop irq if set
            BCLR    DROP_IRQ,Flags               ;clear flag
            RTI


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
trap14 BRA trap14
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                                             ;skip irq
doint       BRCLR   PANEL_IN,PORT_B,gotdata  ;5  ;go gotdata if input is 0-
            RTI                                  ;otherwise skip irq

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
trap15 BRA trap15
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


gotdata     LDA     #$08                     ;2  ;8 data bits
            STA     tempx                    ;4  ;init loop counter
            CLR     CommReg                  ;5  ;zero out communication reg

            ;;;;;;; Total cycles: 10+5+5+2+4+5 = 31


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; The following loop determine WHEN within a bit time the sample is made.
;;; Bit-time(2400 BAUD) = 833 cycles,
;;;
;;;                                          ;31 (from above)
            LDA     #$11                     ;2  ;2400 BAUD -> 17
Looper      DECA                             ;3
            BNE     Looper                   ;3

            ;;;;;;; Total cycles (2400 BAUD) = 31+2+(3+3)17 = 135
            ;;;;;;;
            ;;;;;;; D202 Rev 0.12 original code = 134


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

getbits     JSR     Delay1BitTime              ;804 (2400-BAUD)
            LDA     CommReg                    ;3
            LSRA                               ;3  ;shift received data left
            BRCLR   PANEL_IN,PORT_B,updateByte ;5
            ORA     #$80                       ;2  (average = 2/2 = 1)
updateByte  STA     CommReg                    ;4
            DEC     tempx                      ;3
            BNE     getbits                    ;3   ;get more bits

            ;;;;;;; Total cycles (2400 BAUD) = 804+3+3+5+1+4+3+3 = 826

                                        ;done receiving all bits at this point
            BSET    RECV_FULL,Flags     ;set receive register full flag
            TST     CommReg             ;is CommReg < 128?
            BMI     nope2400            ;skip if last bit received is a one
            BEQ     nope2400            ;skip if byte received is a 0
            BSET    DROP_IRQ,Flags      ;otherwise drop the next IRQ interrupt

nope2400    BRSET   ALIEN_PACKET,flags,skipByte ;data error detected
            TST     CState              ;if CState is 0 (1st byte)
            BNE     otherstate          ;branch if not 0

                                        ;state 0 is get address byte
            LDA     CommReg             ;did we receive the right HEADER
            CMP     BoardAddress

            BEQ     storeHeader         ;message is for us
            BSET    ALIEN_PACKET,flags  ;alien msg, get ready to skip
skipByte
            RTI

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
trap17 BRA trap17
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


storeHeader
            STA     chk                 ;otherwise put header into checksum
            INC     CState              ;advance state to get length code
            RTI                         ;done with state specific stuff


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
trap18 BRA trap18
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


otherstate  LDA     CState              ;if CState < 5
            CMP     #RECV_PLEN
            BCC     ISRreturn           ;done with state specific stuff
            LDX     CState              ;put data into comm buffer-
            LDA     CommReg             ;at position pointed to by CState
            STA     CBuff,x
            LDA     chk                 ;add data to the check sum
            ADD     CommReg
            STA     chk
            INC     CState              ;SM to get next byte
            LDA     CState
            CMP     #RECV_PLEN          ;if this was the last byte
            BNE     ISRreturn           ;otherwise done

            LDA     chk                 ;
            CMP     #$FF                ;if check sum is 0xff


            BEQ     chkok

            BRA     ISRreturn           ;otherwise done

chkok       LDA     CBuff+2
            STA     lowRLY

            LDA     CBuff+3
            STA     highRLY

            LDX     #$14                ;8 msec
waitrply    JSR     Delay1BitTime       ;400 microsec * 20 = 8 msec
            DECX
            BNE     waitrply

            LDA     NoCommTimer
            CMP     #COMM_TIMEOUT
            BLO     commok

commReset   BRA     commReset           ;wait for watchdog reset

commok      CLR     NoCommTimer         ;communicating OK
            LDA     #REPLY_ACK
            STA     mask
            JSR     SendByte

ISRreturn

            RTI

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
trap19 BRA trap19
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;******************************************************************************
; End SERIAL INPUT ISR
;******************************************************************************
;******************************************************************************




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This routine sends out a byte
; mask = byte to send, LSB first
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

SendByte
        SEI                             ;disable interrupts
        LDA     #$0A                    ;init counter: 1start+8data+1stop
        STA     SReg1
        BSET    PANEL_OUT,PORT_B        ;send start bit (inverted)

sendloop                                ;send the 8 bits
        JSR     Delay1BitTime           ;804 (2400-BAUD)
        DEC     SReg1                   ;5
        BEQ     endSendByte             ;3
        LSR     mask                    ;5     ;mask = byte to send, LSB first
        BSET    7,mask                  ;5     ;optimization for sending stop bit
        BCC     space                   ;3
mark    BCLR    PANEL_OUT,PORT_B        ;5     ;send mark (inverted)
        BRA     sendloop                ;3
space   BSET    PANEL_OUT,PORT_B        ;5     ;send space (inverted)
        BRA     sendloop                ;3

        ;;;;;;; Total cycles per bit (2400 BAUD) = 804+5+3+5+5+3+5+3 = 833

endSendByte
        CLI                             ;enable interrupts
        RTS


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
trap20 BRA trap20
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This loop delays for a bit time, assuming 2MHz CPU clock.
; Delay loop used can delay 12+9(A) cycles, A = accumulator.
; Bit-time(2400 BAUD) = 833 cycles,
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

Delay1BitTime                               ;6      (JSR to get here)
        LDA     #$56                        ;2      ; 2400 BAUD -> 86

        NOP                                 ;2
        NOP                                 ;2

goLoop  JSR     DelayLoop                   ;12+9(A)
        RTS                                 ;6

        ;;;;;;; Total cycles (2400 BAUD) = 6+2+4(12+9(86))+6 = 804
        ;;;;;;; D202 Rev 0.12 original code = 805


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
trap21 BRA trap21
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Delay Loop
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; A delay loop, assume A is set as the loop counter
;

DelayLoop                       ; 6  (JSR to get here)
        BRA     next            ; 3
next    DECA                    ; 3
        BNE     DelayLoop       ; 3
        RTS                     ; 6

        ;;;;;;; Total cycles: 6+(3+3+3)(A)+6 = 12+9(A)



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
trap23 BRA trap23
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;*************************************************************************
;705J1 Mask Option Register Setting (On = 1, Off = 0)
;====================================================================
;        BITS                                             Setting
;====================================================================
;Bit 7  -  SOSCD   Short Oscillator Delay Enable             0
;Bit 6  -  EMPSEC  EPROM Security Enable                     0
;Bit 5  -  OSCRES  Oscillator Parallel Resistor              0
;Bit 4  -  SWAIT   STOP Instruction Mode                     0
;Bit 3  -  SWPDI   Port Pulldown Inhibit                     1
;Bit 2  -  PIRQ    Port A IRQ Interrupt Function             0
;Bit 3  -  LEVEL   IRQ Edge Sensitivity                      0
;Bit 0  -  COPEN   COP Watchdog Enable                       1

        ORG $07F1
        FCB 9


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ISR Vectors
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        ORG     $07F8

        FDB     RealTimeInt     ;7F8,7F9  Timer Interrupt ISR
        FDB     IRQInt          ;7FA,7FB  IRQ ISR
        FDB     trap            ;7FC,7FD  SWI interrupt ISR (not used)
        FDB     StartUp         ;7FE,7FF  Reset Vector


; End of file
