

; -------------------------------------------------
;
; -------------------------------------------------
; $Author:: Paul L Calinawan                      $
; $Date::   Sept 15,1996                          $
; $Revision::                                     $
; $Version::                                      $
; -------------------------------------------------
; -------------------------------------------------
; This is the original D208C (Version 1.0)
; Eight (8) point expander board for the 2212
; monobus :   MC68HC705J1A memory layout.
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
;
;Rev 001 changes:
;    1.  Leave LED to ON STEADY when comm fails.
;    2.  Made PA3 and PA4 output ports
;    3.  SyncSerial resynchronizes at CommState == 4
;    4.  OtherState CMP to determine if we got the last
;        byte was corrected to CommState == 4
;    5.  Used table to sequence point scanning
;    6.  Added a Transmit Comm State Machine
;
;Rev 003 changes:
;    1.  Added masking to PrevPtState when comparing
;           (InstPtState == PrevPtState)
;    2.  Added more comments
;    3.  Fixed Mux Addressing to match the point scan
;	 4.  Reformat the way pt states are stored 
;			based on the (monobus protocol)
;	 5.  Added a timer to wait for system to stabilize
;			before point scanning (1 sec). On Reset.
;
;Rev 005 changes:
;	 1.  Changed LED port to use high current sink port A7
;	 2.  Enabled Port Pull Downs where necessary ie..
;		    Address Switch, Not Connected Ports
;	 3.  On power up the LED blinks 3 times to indicate
;			a properly running board module
;    4.  Address switch now uses the port pull down 
;           circuit instead of the resistor pull up
;    5.  Fixed Comm Resynchronization
;	 6.  Added monobus conversion table to convert comparator
;       	data to monobus data



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; PORT DEFINITIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PORT_A          EQU $00       	;Port A
PORT_B          EQU $01       	;Port B

DDRA            EQU $04         ;Port A Direction Register
DDRB            EQU $05         ;Port B Direction Register

PANEL_OUT       EQU 1           ;MonoBus output port_B
PANEL_IN        EQU 0           ;MonoBus input port_B

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; REGISTER DEFINITIONS
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TCSR            EQU $08         ;Timer Control and Status Register
TCR             EQU $09         ;Timer Counter Register

RTIFR           EQU 2           ; ***705*** Bit 2 of Timer Control Register

COP_SERVICE     EQU $07F0       ;COP Service Reqister   ;;;;;;;;;; <-- J1
COP             EQU 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


					
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; SYSTEM VARIABLES
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



		ORG $00C0


T8_2msTicks         DS  1

SystemFlags         DS  1           ;a byte for various SystemFlags

ALIEN_PACKET        EQU 3           ;Alien Packet detected, Bit Flag
DO_XMIT_WAIT        EQU 4           ;State to wait 8.2 msec before sending reply
BYTE_RECV_FULL      EQU 5           ;Receive Register is full, Bit flag
SW_RTIF             EQU 6           ;software RTIF                  
DROP_IRQ            EQU 7           ;sometimes need to drop IRQs


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   Communication Variables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

RecvCommState       DS 1                        ;Receive Comm state machine
XmitCommState       DS 1                        ;Transmit Comm state machine

TransmitLen         DS 1                        ;Transmit Byte Counter
NORMAL_LENGTH       EQU $05                     ;0 - 5
ACK_LENGTH          EQU $01
ACK_REPLY           EQU $55                     ;Ack character for addressed poll


RECV_PLEN           EQU $04                     ;Receive packet length, Comm State 0-3, 
												;4 Bytes of data expected

RECV_PLEN_OVER      EQU $06                     ;This indicates 2 byte over


SReg1               DS 1                        ;temporary

tempx               DS 1                        ;temporary for IRQ

ByteReceived        DS 1                        ;IRQ interrupt data storage

CheckSum            DS 1                        ;data check sum

CommBuff            DS 6                        ;communications buffer
ByteToSend          DS 1

NoCommTimer         DS 1                        ;no comm timeout variable


COMM_TIMEOUT        EQU $3C                     ;Comm timeout: 0x3C * 50msec = 3 sec
												;HeartBeat stops if No Comm

BROADCAST_ADDR      EQU $7f

RECEIVE_CKSM        EQU $ff                     ;Master (Panel) to Slave (Device)
SEND_CKSM           EQU $7f                     ;Slave to Master
SLAVE_REPLY         EQU $80                     ;Sets the last bit of the Address Byte
												;of a Slave Reply Packet
CMD6_REPLY          EQU $06
CMD6_REPLY_LEN      EQU $04

TempPtReply			DS 1
PointReply_1_4      DS 1                        ;Contains the point data to be
PointReply_5_8      DS 1                        ;sent over the comm bus


COMMAND_0           EQU $00                     ;Reset Slave Device
COMMAND_2           EQU $02                     ;Req Curr Pt Stat
COMMAND_3           EQU $03                     ;Req Latched Pt Stat
COMMAND_4           EQU $04                     ;Req Pt Stat Change

XMIT_IDLE           EQU $FE                     ;Xmit Comm State definitions
XMIT_WAIT           EQU $FF                     ;wait 8 msec
XMIT_SENDING        EQU $00                     ;Send bytes 0-5

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; MAIN TASKS VARIABLES
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;


InstPtState         DS 2                    ; Point States as Instantaneous Reading
ScandPtState        DS 2                    ; Point States as Debounced Reading
TempPtState         DS 2                    ; Used to store temporary point state readings

LatchedPtState      DS 2                    ; Contains Points that are not NORMAL after debounce       
LastSntPtState      DS 2                    ; Used to determine How To Reply to a poll
PrevPtState         DS 2                    ; Used to determine Point state changes                    
					   
;
PointStat           DS 1                    ; Keeps the temp value of the current point



POINT_IN_LINES      EQU $18                 ; port A3 and A4

											; These are the 2 bit information as 
											; seen from the comparator circuit.
POINT_NORMAL        EQU $03                 ; Point State Definitions (N/O/S)
POINT_SHORT         EQU $02                 ; These will be converted into 
POINT_OPEN          EQU $01                 ; the monobus protocol specification

ALL_PTS_NORMAL      EQU $00

MaskShift           DS 1                    ; Number of times a mask is shifted
PointStateMask      DS 2                    ; Used to place point state values
											; in their correct location
POINT_STATE_MASK    EQU $03                 ; 0000 0011

;

MuxAddrLines        DS 1                    ; value 0 - 7
PointNumber         DS 1                    ; Curr Point 0 - 7
MUX_ADDR_LINES      EQU $07                 ; port A0, A1 and A2

HeartBeatTimer      DS 1                    ; Beat every 800msec if Comm ok
HEARTBEAT           EQU $80                 ; Heart Beat Mask, Port A7
HEART_BEAT_TIME     EQU $10                 ; 0x10 * 50msec = 800msec

MAX_POINT           EQU $07


BBRD_ADDR           EQU $01                 ; Base Board Address

DIP_SWITCH_MASK     EQU $1C                 ; Dip Address Switch Mask,
											; Port B2,B3 and B4

BoardAddress        DS 1                    ; Debounced address
PrevBoardAddr       DS 1                    ; address matched readings


PDRA				EQU	$0010				; Pull Down Register PortA
PDRB				EQU $0011				; PortB

StartUpTimer		DS  1                   ; 0x14 * 50msec = 1 sec
STARTUPTIMEOUT		EQU	$14					


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; INITIALIZATION ROUTINE
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

		ORG $0300               ;;;;;;;;;; <-- J1

StartUp

		LDA     #%10000000		;Clear LED on PowerUp
		STA     PORT_A

		LDA     #%10000111      ;port A0-A2 output      - Mux Control
								;port A3-A4 input       - Point State Input
		STA     DDRA            ;                       - A3 = U18 pin 1
								;                       - A4 = U18 pin 7
								;						- A5, A6 - NC
								;						- A7 = HeartBeat LED
								;	NOTE : LED output is inverted

		CLR     PORT_B
		
		LDA     #%00000010      ;port B0 is input       - MonoBus In
								;port B1 is output      - MonoBus Out
								;port B2-B4 is input    - Address Switch
								;port B5 		        - NC
										  
		STA     DDRB

								;SW Pull Down Control
								;1 = disabled, 0 = enabled
		LDA		#%10011111		;EnAble pull downs for PortA 
		STA		PDRA			
		LDA		#%11000011		;EnAble pull downs for PortB
		STA		PDRB

		LDA     #%00010000      ;RTIE = true, 8.2ms RTI rate
		STA     TCSR

;
; RAM (0xC0 to 0xFF) Clearing Routine
; All Points to NORMAL
; Timers= 0
;
		LDX     #$C0
ClearNextByte
		CLR     ,x
		INCX
		BNE     ClearNextByte

;
; Initialize System Variables
;
		LDA     #XMIT_IDLE
		STA     XmitCommState

		LDA     #HEART_BEAT_TIME
		STA     HeartBeatTimer

		LDA     #ALL_PTS_NORMAL     ;All Points Starts as NORMAL
		STA     ScandPtState+1      ;LoByte
		STA     ScandPtState        ;HiByte
		STA     LatchedPtState+1
		STA     LatchedPtState
		STA     PrevPtState+1
		STA     PrevPtState
		STA     LastSntPtState+1
		STA     LastSntPtState

		LDA		#STARTUPTIMEOUT
		STA		StartUpTimer

		INC     T8_2msTicks         ;0 to 1, start timer immediately


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; MAIN
;
;
MainLoop

		LDA     #$80            ; ***705*** enable IRQ
		STA     $0A             ; ***705***
		CLI                     ;enable interrupts

        RSP                     ;reset the stack pointer

;
; Update system ticks
;
		BRCLR   SW_RTIF,SystemFlags,Not8pt2msYet    ;Not 8.2 yet ?


        JSR     Do8pt2msTasks                       ;Its 8.2 now

												
        DEC     T8_2msTicks                         ;down to 0
        BNE     Not50msYet                          ;if 0, branch doneUpdateTicks
                                                    ;else time for 50 ms tasks

        JSR     Do50msecTasks                       ;Its 50 msec now

Not8pt2msYet
Not50msYet

		JSR     XmitCommMachine                 ;Process Transmit State
                                                ;Machine every main loop
;
;   do nothing but clear WDog
;   and back to MainLoop
;
;   Clear Watchdog reset routine
;

EndMain

		SEI                     ;disable interrupt before resetting watchdog
        CLRA
        STA     COP_SERVICE     ;reset watchdog
        JMP     MainLoop        ;start main loop again
;
;
;
;
;
;end main loop
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
;
trap0    BRA     trap0
;
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; 
Do8pt2msTasks

        BCLR    SW_RTIF,SystemFlags                 

		JSR     SyncSerial              ;Re-sync on 8.2ms Mono-bus
										;idle time (2400)

		BRCLR   DO_XMIT_WAIT,SystemFlags,WaitNotOver   ;Check Xmit Wait state

		BCLR    DO_XMIT_WAIT,SystemFlags
		LDA     #XMIT_SENDING
		STA     XmitCommState

		BRA     ReturnD82T
										
WaitNotOver

		LDA     XmitCommState
		CMP     #XMIT_WAIT
		BNE     ReturnD82T

		BSET    DO_XMIT_WAIT,SystemFlags

ReturnD82T

        RTS



;
;
trap1    BRA     trap1
;
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Point Scan Rate = 400 msec
;
Do50msecTasks
														;Yes, 50 msec is up
			LDA     #$6                                 ;re init down counter
			STA     T8_2msTicks                         ;8.2ms x 6 = 49.2 msec

			LDA		StartUpTimer
			BEQ		SystemGo

			DEC		StartUpTimer
			BRA		SystemWait
SystemGo

			JSR     ReadPointStat
			JSR     SelectNextPt
			JSR     UpdatePointStat
			JSR     UpdateBoardAddr
			JSR     CheckNoComm
			JSR     UpdateLed

SystemWait               
            RTS     
;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;
;
trap2    BRA     trap2
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : ReadPointStat
; Read the State of the current point.
;
; InstPtState contains the 2 bit info
; of the instantaneous state (N/O/S)
; of the current point.

ReadPointStat
            LDA     #POINT_STATE_MASK       ;PointStateMask = POINT_STATE_MASK
			STA     PointStateMask+1
			CLR     PointStateMask

			CLR     TempPtState+1           ;Init = 0
			CLR     TempPtState

			LDA     PORT_A
			AND     #POINT_IN_LINES
			ASRA                            ;Adjust point data
			ASRA
			ASRA			
			STA     PointStat               ;Keep Point Stat Value
											;for later use
			TAX
			LDA     MonoBusConvert,x

			STA     TempPtState+1           ;Store Into Low Byte

			LDX     MaskShift               ;TempPtState <<= MaskShift;
			BEQ     NoShiftTPS              ;No Shift ?
ShiftRPS
			LSL     TempPtState+1           ;LoByte
			ROL     TempPtState             ;HiByte
			LSL     PointStateMask+1
			ROL     PointStateMask
			DECX
			BNE     ShiftRPS

NoShiftTPS                                  ;HiByte
			LDA     TempPtState+1           ;InstPtState = TempPtState
			STA     InstPtState+1
			LDA     TempPtState             ;LoByte
			STA     InstPtState

			LDA     PointNumber            ;MaskShift = MuxAddressValue*2
			LSLA                            ;x2
			STA     MaskShift
ReturnRPS
			RTS
;
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
MonoBusConvert      ;MonoBus     	Comparator
					;Equivalent	 	Data:

			FCB     $03         ;    00		MISSING
			FCB     $01         ;    01     OPEN
			FCB     $02         ;    02     SHORT
			FCB     $00         ;    03     NORMAL

;
;
trap3    BRA     trap3
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : UpdatePointStat
; Updates the value of ScandPtState
; for the current point. A change
; is only accepted after a debounce
; is successful
;
											;For the current point:
UpdatePointStat                             ;InstPtState == (PrevPtState & PointStateMask)?

			LDA     PrevPtState+1           ;LoByte
			AND     PointStateMask+1
			CMP     InstPtState+1
			BNE     NoUPS
			LDA     PrevPtState             ;HiByte
			AND     PointStateMask
			CMP     InstPtState
			BNE     NoUPS
UpdatePtScand                               ;ScandPtState = InstState
											;LoByte
											;Clear 2 bit info
			LDA     PointStateMask+1        ;ScandPtState_1_8 &= ~PointStateMask;
			COMA
			AND     ScandPtState+1
			STA     ScandPtState+1
											;Update 2 bit info    
			LDA     InstPtState+1           ;ScandPtState_1_8 |= InstPtState;
			ORA     ScandPtState+1
			STA     ScandPtState+1

											;HIByte
											;Clear 2 bit info
			LDA     PointStateMask          ;ScandPtState_1_8 &= ~PointStateMask;
			COMA
			AND     ScandPtState
			STA     ScandPtState
											;Update 2 bit info
			LDA     InstPtState             ;ScandPtState_1_8 |= InstPtState;
			ORA     ScandPtState
			STA     ScandPtState

CheckPtNormal
			LDA     PointStat               ;If PointStat == NORMAL (comparator)
			CMP     #POINT_NORMAL           ;Check If Point is FAULTED
			BNE     UpdateLatchedPt         ;Update Latch Pt info
			BRA     SkipUPL

UpdateLatchedPt
											;LoByte
			LDA     PointStateMask+1        ;LatchedPtState_1_8 &= ~PointStateMask;
			COMA
			AND     LatchedPtState+1
			STA     LatchedPtState+1
			LDA     InstPtState+1           ;LatchedPtState_1_8 |= InstPtState;
			ORA     LatchedPtState+1
			STA     LatchedPtState+1
											;HiByte
			LDA     PointStateMask          ;LatchedPtState_1_8 &= ~PointStateMask;
			COMA
			AND     LatchedPtState
			STA     LatchedPtState
			LDA     InstPtState             ;LatchedPtState_1_8 |= InstPtState;
			ORA     LatchedPtState
			STA     LatchedPtState

SkipUPL
NoUPS                                       ;PrevPtState = InstPtState
											;LoByte
			LDA     PointStateMask+1        ;PrevPtState_1_8 &= ~PointStateMask;
			COMA
			AND     PrevPtState+1
			STA     PrevPtState+1
			LDA     InstPtState+1           ;PrevPtState_1_8 |= InstPtState;
			ORA     PrevPtState+1
			STA     PrevPtState+1
											;HiByte
			LDA     PointStateMask          ;PrevPtState_1_8 &= ~PointStateMask;
			COMA
			AND     PrevPtState
			STA     PrevPtState
			LDA     InstPtState             ;PrevPtState_1_8 |= InstPtState;
			ORA     PrevPtState
			STA     PrevPtState
ReturnUPS
			RTS
;
;


;
;
trap4    BRA     trap4
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : SelectNextPt
; Controls the Mux Address Lines to
; prepare for the next point reading
;
SelectNextPt
			LDA     PointNumber
			CMP     #MAX_POINT
			BHS     SetMux000

			INC     PointNumber            ;PointNumber++
			BRA     WriteMuxAddr
SetMux000
			CLR     PointNumber            ;PointNumber=0

WriteMuxAddr
			LDX     PointNumber
			LDA     MuxAddrTable,X
			STA     MuxAddrLines

			LDA     #MUX_ADDR_LINES         ;Save other ports state
			COMA
			AND     PORT_A
            ORA     MuxAddrLines
			STA     PORT_A                  ;Write Mux Addr to port
ReturnSNP
            RTS
;
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
MuxAddrTable      ;MUX ADDR     POINT NO:

			FCB     $07         ;    1
			FCB     $00         ;    2
			FCB     $01         ;    3
			FCB     $02         ;    4
			FCB     $03         ;    5
			FCB     $04         ;    6
			FCB     $05         ;    7
			FCB     $06         ;    8

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
;
trap5    BRA     trap5
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : UpdateBoardAddress
; Read and Debounce Board Address
; Setting of the DIP Switch
;
;
UpdateBoardAddr
            LDA     PORT_B                      ;refresh board address
                                                ;with current reading
            AND     #DIP_SWITCH_MASK            ;mask address data

            ASRA                                ;adjust data
            ASRA                                ;
            ADD     #BBRD_ADDR                  ;

            CMP     PrevBoardAddr
			BNE     NoUpdateBrdAddr
            STA     BoardAddress                ; is ==, so update
            BRA     ReturnUBA
NoUpdateBrdAddr
            STA     PrevBoardAddr               ;prev = curr
ReturnUBA
			RTS
;
;


;
;
trap6    BRA     trap6
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : CheckNoComm
; Counts up every 50msec; The variable
; is reset by the comm routine whenever
; a valid packet is received
;
CheckNoComm
            INC     NoCommTimer                 ;track # of 50 msec chnks no comm
            LDA     NoCommTimer
            CMP     #COMM_TIMEOUT

            BLS     NoCommTrbl                  ;do comm trouble if same

CommTimerOver

            DEC     NoCommTimer                 ;when higher dec NoCommTimer --
			BRA     ReturnCNC                   ;to keep overflow

NoCommTrbl            

ReturnCNC
            RTS
;
;


;
;
trap7    BRA     trap7
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : UpdateLed
; Run heartbeat if comm is ok
;
;
UpdateLed
            INC     HeartBeatTimer
            LDA     HeartBeatTimer
			CMP     #HEART_BEAT_TIME            ;Is it time to beat?
            BLS     ReturnUL                    ;Not Yet

			LDA     NoCommTimer                 ;Comm Ok?
            CMP     #COMM_TIMEOUT
            BHS     SteadyLED

BeatLed
			CLR     HeartBeatTimer              ;Reset Timer
			LDA     PORT_A                      ;read heart beat port
			EOR     #HEARTBEAT                  ;toggle
			STA     PORT_A                      ;write to port
			BRA     ReturnUL
SteadyLED
			LDA     #HEARTBEAT                  ;LED state is inverted
			COMA
			AND     PORT_A                      ;save other port states
			STA     PORT_A                      ;write to port
ReturnUL
			RTS
;
;


;
;
trap8    BRA     trap8
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : SyncSerial
; Routine to re-synchronize serial communication
;
; STATE:
;  0     1     2     3      4     5    6     7
; ADR - LEN - CMD - CKSM
;
;

SyncSerial
		BRSET   BYTE_RECV_FULL,SystemFlags,doneSyncSerial

		LDA     RecvCommState               ;clear buffer if RecvCommState < 6
		CMP     #RECV_PLEN_OVER             ;over packet length?
											;
		BCC     doneSyncSerial

		BCLR    ALIEN_PACKET,SystemFlags    ;YES, Resync Comm
		CLR     RecvCommState

doneSyncSerial                              ;NO
		BCLR    BYTE_RECV_FULL,SystemFlags
		RTS
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;
;
trap9    BRA     trap9
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;This is the Real-Time
;Interrupt routine
;
;

RealTimeInt BSET    RTIFR,TCSR                  ;***705*** Clear RTIF by writing 1 to RTIFR
			BSET    SW_RTIF,SystemFlags         ;Set copy of flag
            RTI

;
; END TIMER ISR
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;
;
trap10    BRA     trap10
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;   This is the SERIAL INPUT
;   data interrupt routine
;

IRQInt                                              ;10 (interrupt to get here)
            BRCLR   DROP_IRQ,SystemFlags,DoIntrr    ;5  ;drop irq if set
            BCLR    DROP_IRQ,SystemFlags            ;clear flag
            RTI
                                                    ;skip irq
DoIntrr
            BRCLR   PANEL_IN,PORT_B,GotData         ;5  ;go gotdata if input is 0-
            RTI                                     ;otherwise skip irq


GotData     LDA     #$08                     ;2  ;8 data bits
			STA     tempx                    ;4  ;init loop counter
            CLR     ByteReceived             ;5  ;zero out communication reg

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

GetDataBits
            JSR     Delay1BitTime                   ;804 (2400-BAUD)
            LDA     ByteReceived                    ;3
            LSRA                                    ;3  ;shift received data left
            BRCLR   PANEL_IN,PORT_B,BuildDataByte   ;5
            ORA     #$80                            ;2  (average = 2/2 = 1)

BuildDataByte
            STA     ByteReceived               ;4
            DEC     tempx                      ;3
            BNE     GetDataBits                ;3   ;get more bits

            ;;;;;;; Total cycles (2400 BAUD) = 804+3+3+5+1+4+3+3 = 826

												;done receiving all bits at this point
			BSET    BYTE_RECV_FULL,SystemFlags  ;set receive register full flag
            TST     ByteReceived                ;is ByteReceived < 128, Not A Valid Address Poll
            BMI     nope2400                    ;skip if last bit received is a one
            BEQ     nope2400                    ;skip if byte received is a 0
            BSET    DROP_IRQ,SystemFlags        ;otherwise drop the next IRQ interrupt

nope2400    BRSET   ALIEN_PACKET,SystemFlags,SkipByte   ;data error detected
            TST     RecvCommState                       ;if RecvCommState is 0 (1st byte)
            BNE     OtherStates                         ;branch if not 0

												;state 0 is get address byte
            LDA     ByteReceived                ;did we receive the right HEADER
            CMP     BoardAddress
            BEQ     StoreHeader                 ;message is for us
            CMP     #BROADCAST_ADDR
            BEQ     StoreHeader                 ;message is for us / AllDevice

			BSET    ALIEN_PACKET,SystemFlags    ;alien msg, Not for us,
                                                ;get ready to skip
SkipByte
			BRA     ISRreturn

;
;
;
StoreHeader
            STA     CheckSum            ;otherwise put header into checksum
            INC     RecvCommState       ;advance state to get length code
			BRA     ISRreturn           ;done with state specific stuff

OtherStates 
            LDA     RecvCommState       ;if RecvCommState < 4
			CMP     #RECV_PLEN
            BLO     GetMoreBytes
			BRA     ISRreturn           ;Wrong Data
GetMoreBytes
			LDX     RecvCommState       ;put data into comm buffer-
            LDA     ByteReceived        ;at position pointed to by RecvCommState
            STA     CommBuff,x          ;CommBuff,0 contains NOTHING
                                        ;1=LEN, 2=CMD, 3=CKSM

            LDA     CheckSum            ;add data to the check sum
            ADD     ByteReceived
            STA     CheckSum

            INC     RecvCommState       ;set SM to get next byte
            LDA     RecvCommState

            CMP     #RECV_PLEN          ;if this was the last byte
            BLO     ISRreturn           ;NO, get more data

                                        ;Got Last Byte
            LDA     CheckSum            ;YES, Compute CheckSum
			CMP     #RECEIVE_CKSM       ;if check sum is OK

            BEQ     CheckSumOK

WrongCkeckSum

            INC     RecvCommState       ;Force State to be 1 over
            BRA     ISRreturn           ;otherwise done

CheckSumOK

            CLR     RecvCommState       ;Get RecvCommState Machine
                                        ;Ready for the next poll

            CLR     NoCommTimer         ;communicating OK      
            
            LDA     #XMIT_WAIT          ;XmitCommState = XMIT_WAIT
			STA     XmitCommState

            JSR     SetUpXmitBuff

ISRreturn

            RTI
;
;
; End SERIAL INPUT ISR
;******************************************************************************
;******************************************************************************


;
;
trap11    BRA     trap11
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
SetUpXmitBuff
CaseCommand
			LDA     CommBuff+2          ;COMMAND BYTE

            CMP     #COMMAND_0          ;Reset Cmd?
            BNE     CmdCase2            ;No
            BRA     ForceWDog           ;Yes, Force a watchdog
CmdCase2
            CMP     #COMMAND_2          ;Req Pt Stat Cmd?
            BNE     CmdCase3            ;No
            JSR     BuildCmd2Reply      ;Yes
            BRA     ReturnCR

CmdCase3
            CMP     #COMMAND_3          ;Req Latched Pt Stat Cmd?
            BNE     CmdCase4            ;No
            JSR     BuildCmd3Reply      ;Yes
            BRA     ReturnCR

CmdCase4
            CMP     #COMMAND_4          ;Req Changed Pt Stat Cmd?
			BNE     ReturnCR            ;No, Unknown Command
            JSR     BuildCmd4Reply      ;Yes
            BRA     ReturnCR

ForceWDog   BRA     ForceWDog           ;Yes, Force a watchdog
										;Within 8.2 msec
ReturnCR
            RTS

;
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;
;
trap12    BRA     trap12
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : BuildCmd2Reply
; Request Curr Point Status !
;   1. If Latched Point Status is the
;       same as Last Sent Point Status
;       SEND : Scanned Pt Stat
;              (Current Pt Stat)
; else
;   2.  SEND : Latched Point Status
;
;
BuildCmd2Reply
									;LastSntPtState == LatchedPtState?
									;Send LatchedPt State as it is.
		LDA     LatchedPtState+1    ;LoByte
		CMP     LastSntPtState+1
		BNE     JSR_SendLPS

		LDA     LatchedPtState      ;HiByte
		CMP     LastSntPtState
		BNE     JSR_SendLPS

SendSPS
		LDA     ScandPtState+1      ;Update LatchedPt State
		STA     LatchedPtState+1    ;LatchedPtState = ScandPtState
		LDA     ScandPtState
		STA     LatchedPtState

JSR_SendLPS

		JSR     SendLatchedPS    

ReturnBC2R

		RTS
;
;
;


;
;
trap13    BRA     trap13
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : BuildCmd3Reply
; Request Latched Point status
;   1.  SEND : Latched Point Status
;
;
BuildCmd3Reply

		JSR     SendLatchedPS

		RTS
;
;
;


;
;
trap14    BRA     trap14
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : BuildCmd3Reply
; Request Changed Point Status
;   1. Send ACK if NO CHANGE between
;       LastSntPtState and
;       LatchedPtState
;
; else
;   2. Send Latched Point Status
;
;
BuildCmd4Reply
									;LastSntPtState == LatchedPtState?
		LDA     LatchedPtState+1    ;LoByte
		CMP     LastSntPtState+1
		BNE     JSR2_SendLPS

		LDA     LatchedPtState      ;HiByte
		CMP     LastSntPtState
		BNE     JSR2_SendLPS

CheckLPS_SPS                     	;is equal

		LDA     ScandPtState+1      ;Update LatchedPt State
		STA     LatchedPtState+1    ;LatchedPtState = ScandPtState
		LDA     ScandPtState
		STA     LatchedPtState
									;LastSntPtState == LatchedPtState?
		LDA     LatchedPtState+1    ;LoByte
		CMP     LastSntPtState+1
		BNE     JSR2_SendLPS

		LDA     LatchedPtState      ;HiByte
		CMP     LastSntPtState
		BNE     JSR2_SendLPS
									;is equal
		JSR     SendAck             ;No Change
		BRA     ReturnBC4R

JSR2_SendLPS

		JSR     SendLatchedPS 

ReturnBC4R

		RTS
;
;
;


;
;
trap15    BRA     trap15
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : SendLatchedPS
; Sends LatchedPtState Point Data
;

SendLatchedPS

		LDA     #NORMAL_LENGTH      ; Set Transmit Length normal to send
		STA     TransmitLen         ; a 5-byte packet with point data

		;Format the pt data based based on the monobus protocol

		CLR		TempPtReply
									;LoByte
		LDA     LatchedPtState+1    ;Pt1
		AND		#$03
		LSLA
		LSLA
		LSLA
		LSLA
		LSLA
		LSLA
		ORA		TempPtReply
		STA		TempPtReply
		LDA     LatchedPtState+1    ;Pt2
		AND		#$0C
		LSLA
		LSLA
		ORA		TempPtReply
		STA		TempPtReply
		LDA     LatchedPtState+1    ;Pt3
		AND		#$30
		LSRA
		LSRA
		ORA		TempPtReply
		STA		TempPtReply
		LDA     LatchedPtState+1    ;Pt4
		AND		#$C0
		LSRA
		LSRA
		LSRA
		LSRA
		LSRA
		LSRA
		ORA		TempPtReply
		STA     PointReply_1_4


		CLR		TempPtReply
									;HiByte
		LDA     LatchedPtState      ;Pt5
		AND		#$03
		LSLA
		LSLA
		LSLA
		LSLA
		LSLA
		LSLA
		ORA		TempPtReply
		STA		TempPtReply
		LDA     LatchedPtState      ;Pt6
		AND		#$0C
		LSLA
		LSLA
		ORA		TempPtReply
		STA		TempPtReply
		LDA     LatchedPtState      ;Pt7
		AND		#$30
		LSRA
		LSRA
		ORA		TempPtReply
		STA		TempPtReply
		LDA     LatchedPtState      ;Pt8
		AND		#$C0
		LSRA
		LSRA
		LSRA
		LSRA
		LSRA
		LSRA
		ORA		TempPtReply
		STA     PointReply_5_8

		RTS
;
;
;


;
;
trap16    BRA     trap16
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : SendAck
; Sends A 1 Byte ACK (0x55) to the
; communications bus

SendAck
        LDA     #ACK_LENGTH     ; Set Transmit Length to 1 to send
        STA     TransmitLen     ; a 1 byte ACK 

		RTS
;
;


;
;
trap17    BRA     trap17
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : TransmitComm
; Sends A Packet of Data
; to the communications bus

XmitCommMachine

        LDA     XmitCommState
        CMP     #XMIT_IDLE
        BEQ     DoCommIDLE
        BRA     CaseCommWAIT

DoCommIDLE                              ;The Receive IRQ routine brings
		RTS                             ;us out of IDLE

CaseCommWAIT
        CMP     #XMIT_WAIT              ;The 8.2msec service routine 
        BEQ     DoCommWAIT
		BRA     CaseCommSEND            ;brings us out of WAIT STATE

DoCommWAIT
        RTS

CaseCommSEND

        LDA     TransmitLen             ;Check if we want to send 
		CMP     #ACK_LENGTH             ;a Full Packet
        BNE     SendFullPacket

        LDA     #ACK_REPLY              ;just an ACK byte
        STA     ByteToSend
        JSR     SendByte
		JMP     CaseGoIDLE  

SendFullPacket
		LDA     XmitCommState
		CMP     #$0
		BNE     CaseSendLen

		CLR     CheckSum

		LDA     BoardAddress            ;Send Address Header
		ORA     #SLAVE_REPLY
		STA     ByteToSend
		ADD     CheckSum
		STA     CheckSum
		JSR     SendByte
		INC     XmitCommState
		RTS

CaseSendLen
		CMP     #$1
		BNE     CaseSendCmd

		LDA     #CMD6_REPLY_LEN         ;Send Length Byte
		STA     ByteToSend
		ADD     CheckSum
		STA     CheckSum
		JSR     SendByte
		INC     XmitCommState
		RTS

CaseSendCmd
		CMP     #$2
		BNE     CaseSendP14

		LDA     #CMD6_REPLY             ;Send Command Byte
		STA     ByteToSend
		ADD     CheckSum
		STA     CheckSum
		JSR     SendByte
		INC     XmitCommState
		RTS

CaseSendP14
		CMP     #$3
		BNE     CaseSendP58

		LDA     PointReply_1_4          ;Send Data for Pt 1-4
		STA     ByteToSend
		ADD     CheckSum
		STA     CheckSum
		JSR     SendByte
		INC     XmitCommState
		RTS

CaseSendP58
		CMP     #$4
		BNE     CaseSendCHK
				
		LDA     PointReply_5_8          ;Send Data for Pt 5-8
		STA     ByteToSend
		ADD     CheckSum
		STA     CheckSum
		JSR     SendByte
		INC     XmitCommState
		RTS

CaseSendCHK
		CMP     #$5
		BNE     UpdateLastPtSent

		LDA     #SEND_CKSM              ;chksum = 0x7f - currChksumTotal
		SUB     CheckSum
		STA     ByteToSend
		JSR     SendByte
		INC     XmitCommState
		RTS

UpdateLastPtSent

		LDA     LatchedPtState+1        ;Update Last Point Sent
		STA     LastSntPtState+1        ;LoByte
		LDA     LatchedPtState
		STA     LastSntPtState          ;HiByte

CaseGoIDLE
		
		LDA     #XMIT_IDLE
		STA     XmitCommState

ReturnXCM

		RTS
;
;


;
;
trap18    BRA     trap18
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Function : SendByte
; This routine sends out a byte
; ByteToSend = byte to send, LSB first
;
;
SendByte
		SEI                             ;disable interrupts
        LDA     #$0A                    ;init counter: 1start+8data+1stop
        STA     SReg1
        BSET    PANEL_OUT,PORT_B        ;send start bit (inverted)

BitSendLoop                             ;send the 8 bits
        JSR     Delay1BitTime           ;804 (2400-BAUD)
        DEC     SReg1                   ;5
		BEQ     EndSendByte             ;3
        LSR     ByteToSend              ;5     ;ByteToSend = byte to send, LSB first
        BSET    7,ByteToSend            ;5     ;optimization for sending stop bit
        BCC     space                   ;3
mark    BCLR    PANEL_OUT,PORT_B        ;5     ;send mark (inverted)
		BRA     BitSendLoop             ;3
space   BSET    PANEL_OUT,PORT_B        ;5     ;send space (inverted)
		BRA     BitSendLoop             ;3

        ;;;;;;; Total cycles per bit (2400 BAUD) = 804+5+3+5+5+3+5+3 = 833

EndSendByte
		RTS
;
;


;
;
trap19    BRA     trap19
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This loop delays for a bit time,
; assuming 2MHz CPU clock.
; Delay loop used can delay 12+9(A)
; cycles, A = accumulator.
; Bit-time(2400 BAUD) = 833 cycles,
;
Delay1BitTime                               ;6      (JSR to get here)
        LDA     #$56                        ;2      ; 2400 BAUD -> 86

		NOP                                 ;2
        NOP                                 ;2

        JSR     DelayLoop                   ;12+9(A)
        RTS                                 ;6
;
;;;;;;; Total cycles (2400 BAUD) = 6+2+4(12+9(86))+6 = 804
;
;


;
;
trap20    BRA     trap20
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Delay Loop
;
; A delay loop, assume A is set as the loop counter
;

DelayLoop                       ; 6  (JSR to get here)
		BRA     NextI           ; 3
NextI   DECA                    ; 3
		BNE     DelayLoop       ; 3
		RTS                     ; 6
;
;;;;;;; Total cycles: 6+(3+3+3)(A)+6 = 12+9(A)
;



;
;
trap21    BRA     trap21
;
;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Reply Wait
; Wait Some time before responding
;
ReplyWait
            LDX     #$14                ;8 msec
replyWaitLoop
            JSR     Delay1BitTime       ;400 microsec * 20 = 8 msec
            DECX
            BNE     ReplyWaitLoop

            RTS
;
;
;



;
;
trap    BRA     trap
;
;



;*************************************************************************
;705J1 Mask Option Register Setting (On = 1, Off = 0)
;====================================================================
;        BITS                                             Setting
;====================================================================
;Bit 7  -  SOSCD   Short Oscillator Delay Enable             0
;Bit 6  -  EMPSEC  EPROM Security Enable                     0
;Bit 5  -  OSCRES  Oscillator Parallel Resistor              0
;Bit 4  -  SWAIT   STOP Instruction Mode                     0
;Bit 3  -  SWPDI   Port Pulldowns Inhibit                    0
;Bit 2  -  PIRQ    Port A IRQ Interrupt Function             0
;Bit 3  -  LEVEL   IRQ Edge Sensitivity                      0
;Bit 0  -  COPEN   COP Watchdog Enable                       1

				ORG $07F1
				FCB 1



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ISR Vectors
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

		ORG     $07F8

		FDB     RealTimeInt     ;7F8,7F9  Timer Interrupt ISR
		FDB     IRQInt          ;7FA,7FB  IRQ ISR
		FDB     trap            ;7FC,7FD  SWI interrupt ISR (not used)
		FDB     StartUp         ;7FE,7FF  Reset Vector

;
;
;
; End of file
;
