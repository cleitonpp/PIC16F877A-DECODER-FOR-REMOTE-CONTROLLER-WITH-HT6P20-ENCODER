#ifdef __PCB__
#error  Pulsin is not supported for the PCB compiler.
#endif
#ifdef __PCD__
#error  Pulsin is not supported for the PCD compiler.
#endif

#ifdef __PCM__
#define interrupt_enabled(x)  !!(*make8(x,1) & make8(x,0))
#endif
#ifdef __PCH__
#define interrupt_enabled(x)  !!(*(make8(x,1) | 0xF00) & make8(x,0))
#endif

#define BYTE_PTR(x) &(int8 *)(x)

// ASM definitions
#define W  0
#define F  1

// Status Register Bits
#define Z  2
#define C  0

#ifdef __PCM__
// Register addresses (16F)
#byte INDF   = 0x00
#byte STATUS = 0x03
#byte FSR    = 0x04
#endif

#ifdef __PCH__
// Register addresses (18F)
#byte INDF0  = 0xFEF
#byte STATUS = 0xFD8
#byte FSR0H  = 0xFEA
#byte FSR0L  = 0xFE9
#endif

//---------------------------------------
// #define PULSIN_MAX  60000

#ifdef PULSIN_MAX
  #if(PULSIN_MAX > 65535)
      #error PULSIN MAX is too High. Maximum value is 65535. 
  #endif
#endif

//---------------------------------------

#ifdef __PCM__
int16 pulsin(int8 ccs_pin, int8 state)
{
int8 io_port;
#endif

#ifdef __PCH__
int16 pulsin(int16 ccs_pin, int8 state)
{
int16 io_port;
#endif

int8 const bitmask_table[] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};
int8 gie_enabled = FALSE;

int8 bitmask;
int8 flip;
int16 count;

// These variables are used in the ASM code
// and must be located in the same RAM bank.
#locate io_port = 0x30  // Can be 8 or 16 bits
#locate count   = 0x32  // 16 bits
#locate state   = 0x34
#locate bitmask = 0x35
#locate flip    = 0x36


if(interrupt_enabled(GLOBAL))
  {
   disable_interrupts(GLOBAL);
   gie_enabled = TRUE;
  }

// Get the I/O Port address and the bitmask from
// the CCS pin number value.
io_port = ccs_pin >> 3;

bitmask = bitmask_table[ccs_pin & 7];

// Set TRIS = input.
#ifdef __PCM__
*(io_port | 0x80) |= bitmask;  // For 16F PICs
#endif
#ifdef __PCH__
*(io_port + 0x12) |= bitmask; // For 18F PICs
#endif


#asm   
PULSINT:
#ifdef __PCM__
         movf    io_port, W      ; Get port
         movwf   FSR
#endif

#ifdef __PCH__
         movf    io_port, W      ; Get port lsb
         movwf   FSR0L
         movf    BYTE_PTR(io_port) +1, W   ; Get port msb
         movwf   FSR0H
#endif
         movf    bitmask,w

         clrf    flip            ; Preset state tracking
         btfss   state, 0        ; State tracking preset for flip
         movwf   flip            ; Set state to bit mask
   
         call    statecnt        ; Wait for idle state
         btfsc   STATUS, Z       ; If timeout then its over
         goto    done

         call    statecnt        ; Wait for starting edge
         btfsc   STATUS, Z       ; If timeout then its over
         goto    done
   
         call    statecnt        ; Do the count
         goto    done

//--------------------------------------------------
statecnt:
         movf    bitmask, W      ; Flip state each time through here
         xorwf   flip, F
#ifdef PULSIN_MAX
         movlw   ((-(PULSIN_MAX)) + 1)       ; Start counter -PULSIN_MAX + 1
         movwf   count
         movlw   (((-(PULSIN_MAX)) + 1) >> 8)
         movwf   BYTE_PTR(count) + 1
#else
         movlw    1
         movwf    count
         clrf     BYTE_PTR(count) + 1
#endif

statecntloop:                    ; Takes 10 usec
#ifdef __PCM__
         movf    INDF, W         ; 1 Read Port with INDF
#endif
#ifdef __PCH__
         movf    INDF0, W        ; 1 Read Port with INDF0
#endif
         andwf   bitmask, W      ; 1 Mask it with bitmask
         xorwf   flip, W         ; 1 Mix with desired state
         btfss   STATUS, Z       ; 2 / 1
#ifdef PULSIN_MAX
         goto    statecntexit    ; 0 / 2 No longer in state
#else
         return                  ; 0 / 2 No longer in state
#endif
         incf    count, F        ; 1 Increment counter
         btfsc   STATUS, Z       ; 1 / 2
         incfsz  BYTE_PTR(count) + 1, F   ; 1 / 2
         goto    statecntloop    ; 2 / 0
         return                  ; 0 / 2 Counter overflow
#ifdef PULSIN_MAX
statecntexit:
         movlw   PULSIN_MAX      ; Normalize count
         addwf   count, F
         movlw   PULSIN_MAX >> 8
         btfsc   STATUS, C
         addlw   1
         addwf   BYTE_PTR(count) + 1, F
         bcf     STATUS, Z       ; Indicate no overflow
         return
#endif
done:

#endasm

if(gie_enabled)
  {
   enable_interrupts(GLOBAL);
  }

return(count);
} 