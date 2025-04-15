#ifndef X__LBMAID_EMU_H__X
#define X__LBMAID_EMU_H__X

#ifndef _WIN32
#include <stdint.h>
typedef  uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef float FLOAT;
#define FALSE 0x00
#define TRUE  0x01
#endif

typedef enum {
        ADD_AI,
        ADV,
        ADD_AM,
        ADD_AB,
        AND_AI,
        AND_AM,
        AND_AB,
        CALL_SHORT,
        CALL_NEAR,
        CALL_FAR,
        CMP_AB,
        CMP_AI,
        CMP_AM,
        CMPS_BYTE,
        CMPS_WORD,
        CMPS_DWORD,
        CURPOS,
        FADD,
        FSUB,
        GETCHAR,
        HLT,
        JC,
        JNC,
        JZ,
        JNZ,
        JO,
        JS,
        JMP_SHORT,
        JMP_NEAR,
        JMP_FAR,
        LODS_BYTE,
        LODS_WORD,
        LODS_DWORD,
        LOOP,
        LOOPZ,
        LOOPNZ,
        LOD_AM,
        LOD_CM,
        LOD_RM,
        LDI_AI,
        LDI_RI,
        MOV_AR,
        MOV_RR,
        MOV_SPBP,
        MOV_BPSP,
        MOVS_BYTE,
        MOVS_WORD,
        MOVS_DWORD,
        NOT,
        OR_AI,
        OR_AR,
        POP_A,
        POP_M,
        POP_BP,
        POPF,
        PUSH_A,
        PUSH_I,
        PUSH_M,
        PUSH_BP,
        REP__REPZ,
        REPNZ,
        RET,
        SCAS_BYTE,
        SCAS_WORD,
        SCAS_DWORD,
        STO_MA,
        STO_MI,
        STOS_BYTE,
        STOS_WORD,
        STOS_DWORD,
        SUB_AI,
        SUV,
        SUB_AM,
        SUB_AB,
        TEST_AB,
        TEST_AI,
        TEST_AM,
        ZERO,
        XOR_AB,
        PUTCHAR,
        PUSHF
} MAID_INSTRUCTIONS;

typedef enum {
        MAID_REGISTER_ACC,
        MAID_REGISTER_B,
        MAID_REGISTER_C,
        MAID_REGISTER_XI,
        MAID_REGISTER_YI,
        MAID_REGISTER_SP,
        MAID_REGISTER_BP,
        MAID_REGISTER_FLG,
        MAID_REGISTER_IP,
        MAID_REGISTER__MAX
} MAID_REGISTER;

#define MAID_FLAG_ZERO 1
#define MAID_FLAG_CARRY 2
#define MAID_FLAG_OVERFLOW 4
#define MAID_FLAG_SIGN 8

typedef struct {
        DWORD accumulator, b, c, xi, yi, sp, bp, flags, ip;
        BYTE *memory;
} MAID_STATE;

void maid_get_to_work(const char *input, DWORD length);

#endif
