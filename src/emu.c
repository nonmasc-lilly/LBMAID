#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "emu.h"

static DWORD decode_memory(const MAID_STATE *state, BYTE *memloc, DWORD *inc) {
        DWORD dummy_inc;
        if(inc == NULL) inc = &dummy_inc;
        switch(*memloc) {
        case 0x00: *inc = 5; return *(DWORD*)(memloc + 1);
        case 0x01: *inc = 1; return state->b;
        case 0x02: *inc = 1; return state->xi;
        case 0x03: *inc = 1; return state->yi;
        case 0x04: *inc = 1; return state->bp;
        case 0x05: *inc = 5; return state->b  + *(DWORD*)(memloc + 1);
        case 0x06: *inc = 5; return state->xi + *(DWORD*)(memloc + 1);
        case 0x07: *inc = 5; return state->bp + *(DWORD*)(memloc + 1);
        case 0x08: *inc = 1; return state->b << 1;
        case 0x09: *inc = 1; return state->b << 2;
        default: *inc = 1; return 0;
        }
}
static DWORD *get_register(MAID_STATE *state, MAID_REGISTER a_register) {
        switch(a_register) {
        case MAID_REGISTER_ACC: return &state->accumulator;
        case MAID_REGISTER_B:   return &state->b;
        case MAID_REGISTER_C:   return &state->c;
        case MAID_REGISTER_XI:  return &state->xi;
        case MAID_REGISTER_YI:  return &state->yi;
        case MAID_REGISTER_SP:  return &state->sp;
        case MAID_REGISTER_BP:  return &state->bp;
        case MAID_REGISTER_FLG: return &state->flags;
        case MAID_REGISTER_IP:  return &state->ip;
        }
}
static void s_putchar(BYTE c) {
        putchar(*(char*)&c);
        fflush(stdout);
}
static void s_getchar(MAID_STATE *state) {
        state->accumulator = getchar();
}
static void s_curpos(WORD x, WORD y) {
        printf("\033[%u;%uH", y+1, x+1);
}

void maid_get_to_work(const char *input, DWORD length) {
        MAID_STATE state;
        DWORD increment, tmp, memtmp, *r1, *r2;
        WORD wtmp;
        FLOAT ftmp, ftmpb;
        BYTE rep_flag = 0, btmp, halted = FALSE;

        memset(&state, 0, sizeof(state));
        state.memory = calloc(1, *(DWORD*)input > length - sizeof(DWORD) ? *(DWORD*)input : length - sizeof(DWORD));
        memcpy(state.memory, input+sizeof(DWORD), length - sizeof(DWORD));

        while(!halted) {
                increment = 0;
                switch(state.memory[state.ip]) {
                case ADD_AI:
                        tmp = state.accumulator;
                        state.accumulator += *(DWORD*)(state.memory + state.ip + 1);
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) |
                                ((tmp > state.accumulator) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF > state.accumulator & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 4;
                        break;
                case ADV:
                        tmp = state.c;
                        state.c += *(DWORD*)(state.memory + state.ip + 1);
                        state.flags = (!state.c * MAID_FLAG_ZERO) |
                                ((tmp > state.c) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF > state.c & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((state.c >> 31) * MAID_FLAG_SIGN);
                        increment = 4;
                        break;
                case ADD_AM:
                        memtmp = decode_memory(&state, state.memory+state.ip+1, &increment);
                        tmp = state.accumulator;
                        state.accumulator += *(DWORD*)(state.memory + memtmp);
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) |
                                ((tmp > state.accumulator) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF > state.accumulator & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        break;
                case ADD_AB:
                        tmp = state.accumulator;
                        state.accumulator += state.b;
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) |
                                ((tmp > state.accumulator) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF > state.accumulator & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 0;
                        break;
                case AND_AI:
                        state.accumulator &= *(DWORD*)(state.memory + state.ip + 1);
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) | ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 4;
                        break;
                case AND_AM:
                        memtmp = decode_memory(&state, state.memory+state.ip+1, &increment);
                        state.accumulator &= *(DWORD*)(state.memory + memtmp);
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) | ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        break;
                case AND_AB:
                        state.accumulator &= state.b;
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) | ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 0;
                        break;
                case CALL_SHORT:
                        state.sp -= 4;
                        *(DWORD*)(state.memory + state.sp) = state.ip;
                        state.ip += state.memory[state.ip + 1];
                        increment = 0;
                        continue;
                case CALL_NEAR:
                        state.sp -= 4;
                        *(DWORD*)(state.memory + state.sp) = state.ip;
                        state.ip += *(WORD*)(state.memory + state.ip + 1);
                        increment = 0;
                        continue;
                case CALL_FAR:
                        state.sp -= 4;
                        *(DWORD*)(state.memory + state.sp) = state.ip;
                        state.ip = *(DWORD*)(state.memory + state.ip + 1);
                        increment = 0;
                        continue;
                case CMP_AB:
                        tmp = state.accumulator - state.b;
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) |
                                ((tmp < state.accumulator) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF < state.accumulator & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 0;
                        break;
                case CMP_AI:
                        tmp = state.accumulator - *(DWORD*)(state.memory + state.ip + 1);
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) |
                                ((tmp < state.accumulator) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF < state.accumulator & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 4;
                        break;
                case CMP_AM:
                        memtmp = decode_memory(&state, state.memory + state.ip + 1, &increment);
                        tmp = state.accumulator - *(DWORD*)(state.memory + memtmp);
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) |
                                ((tmp < state.accumulator) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF < state.accumulator & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        break;
                case CMPS_BYTE:
                        tmp = state.memory[state.yi] - state.memory[state.xi];
                        state.flags = (!state.memory[state.yi] * MAID_FLAG_ZERO) |
                                ((tmp < state.memory[state.yi]) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEF < state.memory[state.yi] & 0xEF) * MAID_FLAG_OVERFLOW) |
                                ((state.memory[state.yi] >> 31) * MAID_FLAG_SIGN);
                        state.xi++;
                        state.yi++;
                        if(rep_flag) for(state.c--; state.c && rep_flag == 1 ? state.flags & MAID_FLAG_ZERO : ~state.flags & MAID_FLAG_ZERO;) {
                                tmp = state.memory[state.yi] - state.memory[state.xi];
                                state.flags = (!state.memory[state.yi] * MAID_FLAG_ZERO) |
                                        ((tmp < state.memory[state.yi]) * MAID_FLAG_CARRY) |
                                        ((tmp & 0xEF < state.memory[state.yi] & 0xEF) * MAID_FLAG_OVERFLOW) |
                                        ((state.memory[state.yi] >> 31) * MAID_FLAG_SIGN);
                                state.xi++;
                                state.yi++;
                                state.c--;
                        }
                        increment = 0;
                        break;
                case CMPS_WORD:
                        tmp = *(WORD*)(state.memory + state.yi) - *(WORD*)(state.memory + state.xi);
                        state.flags = (!*(WORD*)(state.memory + state.yi) * MAID_FLAG_ZERO) |
                                ((tmp < *(WORD*)(state.memory + state.yi)) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFF < *(WORD*)(state.memory + state.yi) & 0xEFFF) * MAID_FLAG_OVERFLOW) |
                                ((*(WORD*)(state.memory + state.yi) >> 31) * MAID_FLAG_SIGN);
                        state.xi+=2;
                        state.yi+=2;
                        if(rep_flag) for(state.c--; state.c && rep_flag == 1 ? state.flags & MAID_FLAG_ZERO : ~state.flags & MAID_FLAG_ZERO;) {
                                tmp = *(WORD*)(state.memory + state.yi) - *(WORD*)(state.memory + state.xi);
                                state.flags = (!*(WORD*)(state.memory + state.yi) * MAID_FLAG_ZERO) |
                                        ((tmp < *(WORD*)(state.memory + state.yi)) * MAID_FLAG_CARRY) |
                                        ((tmp & 0xEFFF < *(WORD*)(state.memory + state.yi) & 0xEFFF) * MAID_FLAG_OVERFLOW) |
                                        ((*(WORD*)(state.memory + state.yi) >> 31) * MAID_FLAG_SIGN);
                                state.xi+=2;
                                state.yi+=2;
                                state.c--;
                        }
                        increment = 0;
                        break;
                case CMPS_DWORD:
                        tmp = *(WORD*)(state.memory + state.yi) - *(WORD*)(state.memory + state.xi);
                        state.flags = (!*(WORD*)(state.memory + state.yi) * MAID_FLAG_ZERO) |
                                ((tmp < *(WORD*)(state.memory + state.yi)) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF < *(WORD*)(state.memory + state.yi) & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((*(WORD*)(state.memory + state.yi) >> 31) * MAID_FLAG_SIGN);
                        state.xi+=4;
                        state.yi+=4;
                        if(rep_flag) for(state.c--; state.c && rep_flag == 1 ? state.flags & MAID_FLAG_ZERO : ~state.flags & MAID_FLAG_ZERO;) {
                                tmp = *(WORD*)(state.memory + state.yi) - *(WORD*)(state.memory + state.xi);
                                state.flags = (!*(WORD*)(state.memory + state.yi) * MAID_FLAG_ZERO) |
                                        ((tmp < *(WORD*)(state.memory + state.yi)) * MAID_FLAG_CARRY) |
                                        ((tmp & 0xEFFFFFFF < *(WORD*)(state.memory + state.yi) & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                        ((*(WORD*)(state.memory + state.yi) >> 31) * MAID_FLAG_SIGN);
                                state.xi+=4;
                                state.yi+=4;
                                state.c--;
                        }
                        increment = 0;
                        break;
                case CURPOS:
                        s_curpos(state.accumulator & 0xFFFF, state.accumulator >> 16);
                        break;
                case FADD:
                        r1 = get_register(&state, state.memory[state.ip + 1] & 0xF);
                        r2 = get_register(&state, state.memory[state.ip + 1] >> 4);
                        ftmp = *(FLOAT*)r1;
                        ftmpb = ftmp + *(FLOAT*)r2;
                        *r1 = *(DWORD*)&ftmpb;
                        state.flags = (ftmpb == 0) * MAID_FLAG_ZERO |
                                (ftmpb < 0) * MAID_FLAG_SIGN;
                        increment = 1;
                        break;
                case FSUB:
                        r1 = get_register(&state, state.memory[state.ip + 1] & 0xF);
                        r2 = get_register(&state, state.memory[state.ip + 1] >> 4);
                        ftmp = *(FLOAT*)r1;
                        ftmpb = ftmp - *(FLOAT*)r2;
                        *r1 = *(DWORD*)&ftmpb;
                        state.flags = (ftmpb == 0) * MAID_FLAG_ZERO |
                                (ftmpb < 0) * MAID_FLAG_SIGN;
                        increment = 1;
                        break;
                case GETCHAR:
                        s_getchar(&state);
                        break;
                case HLT: halted = TRUE; break;
                case JC:
                        if(state.flags & MAID_FLAG_CARRY) {
                                state.ip += state.memory[state.ip + 1];
                                continue;
                        }
                        increment = 1;
                        break;
                case JNC:
                        if(!(state.flags & MAID_FLAG_CARRY)) {
                                state.ip += state.memory[state.ip + 1];
                                continue;
                        }
                        increment = 1;
                        break;
                case JZ:
                        if(state.flags & MAID_FLAG_ZERO) {
                                state.ip += state.memory[state.ip + 1];
                                continue;
                        }
                        increment = 1;
                        break;
                case JNZ:
                        if(!(state.flags & MAID_FLAG_ZERO)) {
                                state.ip += state.memory[state.ip + 1];
                                continue;
                        }
                        increment = 1;
                        break;
                case JO:
                        if(state.flags & MAID_FLAG_OVERFLOW) {
                                state.ip += state.memory[state.ip + 1];
                                continue;
                        }
                        increment = 1;
                        break;
                case JS:
                        if(state.flags & MAID_FLAG_SIGN) {
                                state.ip += state.memory[state.ip + 1];
                                continue;
                        }
                        increment = 1;
                        break;
                case JMP_SHORT:
                        state.ip += state.memory[state.ip + 1];
                        increment = 1;
                        break;
                case JMP_NEAR:
                        state.ip += *(WORD*)(state.memory + state.ip + 1);
                        increment = 2;
                        break;
                case JMP_FAR:
                        state.ip += *(DWORD*)(state.memory + state.ip + 1);
                        increment = 4;
                        break;
                case LODS_BYTE:
                        state.accumulator = state.memory[state.xi];
                        state.xi++;
                        increment = 0;
                        break;
                case LODS_WORD:
                        state.accumulator = *(WORD*)(state.memory + state.xi);
                        state.xi += 2;
                        increment = 0;
                        break;
                case LODS_DWORD:
                        state.accumulator = *(DWORD*)(state.memory + state.xi);
                        state.xi += 4;
                        increment = 0;
                        break;
                case LOOP:
                        if(state.c != 0) {
                                state.ip += state.memory[state.ip + 1];
                                state.c -= 1;
                                continue;
                        }
                        increment = 1;
                        break;
                case LOOPZ:
                        if(state.c != 0 && state.flags & MAID_FLAG_ZERO) {
                                state.ip += state.memory[state.ip + 1];
                                state.c -= 1;
                                continue;
                        }
                        increment = 1;
                        break;
                case LOOPNZ:
                        if(state.c != 0 && !(state.flags & MAID_FLAG_ZERO)) {
                                state.ip += state.memory[state.ip + 1];
                                state.c -= 1;
                                continue;
                        }
                        increment = 1;
                        break;
                case LOD_AM:
                        memtmp = decode_memory(&state, state.memory+state.ip+1, &increment);
                        state.accumulator = *(DWORD*)(state.memory + memtmp);
                        break;
                case LOD_CM:
                        memtmp = decode_memory(&state, state.memory+state.ip+1, &increment);
                        state.c = *(DWORD*)(state.memory + memtmp);
                        break;
                case LOD_RM:
                        r1 = get_register(&state, state.memory[state.ip + 1] & 0xF);
                        memtmp = decode_memory(&state, state.memory+state.ip+2, &increment);
                        *r1 = *(DWORD*)(state.memory + memtmp);
                        increment += 1;
                        break;
                case LDI_AI:
                        state.accumulator = *(DWORD*)(state.memory + 1);
                        increment = 4;
                        break;
                case LDI_RI:
                        r1 = get_register(&state, state.memory[state.ip + 1] & 0xF);
                        *r1 = *(DWORD*)(state.memory + state.ip + 2);
                        increment = 5;
                        break;
                case MOV_AR:
                        r1 = get_register(&state, state.memory[state.ip + 1] & 0xF);
                        state.accumulator = *r1;
                        increment = 1;
                        break;
                case MOV_RR:
                        r1 = get_register(&state, state.memory[state.ip + 1] & 0xF);
                        r2 = get_register(&state, state.memory[state.ip + 1] >> 4);
                        *r1 = *r2;
                        increment = 1;
                        break;
                case MOV_SPBP:
                        state.sp = state.bp;
                        increment = 0;
                        break;
                case MOV_BPSP:
                        state.bp = state.sp;
                        increment = 0;
                        break;
                case MOVS_BYTE:
                        state.memory[state.yi] = state.memory[state.xi];
                        state.yi++;
                        state.xi++;
                        if(rep_flag == 1) for(state.c--; state.c; state.c--) {
                                state.memory[state.yi] = state.memory[state.xi];
                                state.xi++;
                                state.yi++;
                        }
                        increment = 0;
                        break;
                case MOVS_WORD:
                        *(WORD*)(state.memory + state.yi) = *(WORD*)(state.memory + state.xi);
                        state.yi += 2;
                        state.xi += 2;
                        if(rep_flag == 1) for(state.c--; state.c; state.c--) {
                                *(WORD*)(state.memory + state.yi) = *(WORD*)(state.memory + state.xi);
                                state.xi+=2;
                                state.yi+=2;
                        }
                        increment = 0;
                        break;
                case MOVS_DWORD:
                        *(DWORD*)(state.memory + state.yi) = *(DWORD*)(state.memory + state.xi);
                        state.yi += 4;
                        state.xi += 4;
                        if(rep_flag == 1) for(state.c--; state.c; state.c--) {
                                *(DWORD*)(state.memory + state.yi) = *(DWORD*)(state.memory + state.xi);
                                state.xi+=4;
                                state.yi+=4;
                        }
                        increment = 0;
                        break;
                case NOT:
                        state.accumulator = ~state.accumulator;
                        increment = 0;
                        break;
                case OR_AI:
                        state.accumulator |= *(DWORD*)(state.memory + state.ip + 1);
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) | ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 4;
                        break;
                case OR_AR:
                        r1 = get_register(&state, state.memory[state.ip + 1] & 0xF);
                        state.accumulator |= *r1;
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) | ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 1;
                        break;
                case POP_A:
                        state.accumulator = *(DWORD*)(state.memory + state.sp);
                        state.sp += 4;
                        increment = 0;
                        break;
                case POP_M:
                        memtmp = decode_memory(&state, state.memory + state.ip + 1, &increment);
                        *(DWORD*)(state.memory + memtmp) = *(DWORD*)(state.memory + state.sp);
                        state.sp += 4;
                        break;
                case POP_BP:
                        state.bp = *(DWORD*)(state.memory + state.sp);
                        state.sp += 4;
                        increment = 0;
                        break;
                case POPF:
                        state.flags = *(DWORD*)(state.memory + state.sp);
                        state.sp += 4;
                        increment = 0;
                        break;
                case PUSH_A:
                        state.sp -= 4;
                        *(DWORD*)(state.memory + state.sp) = state.accumulator;
                        increment = 0;
                        break;
                case PUSH_I:
                        state.sp -= 4;
                        *(DWORD*)(state.memory + state.sp) = *(DWORD*)(state.memory + state.ip + 1);
                        increment = 4;
                        break;
                case PUSH_M:
                        memtmp = decode_memory(&state, state.memory + state.ip + 1, &increment);
                        state.sp -= 4;
                        *(DWORD*)(state.memory + state.sp) = *(DWORD*)(state.memory + memtmp);
                        break;
                case PUSH_BP:
                        state.sp -= 4;
                        *(DWORD*)(state.memory + state.sp) = state.bp;
                        break;
                case PUSHF:
                        state.sp -= 4;
                        *(DWORD*)(state.memory + state.sp) = state.flags;
                        break;
                case REP__REPZ:
                        rep_flag = 1;
                        state.ip += 1;
                        continue;
                case REPNZ:
                        rep_flag = 2;
                        state.ip += 1;
                        continue;
                case RET:
                        state.ip = *(DWORD*)(state.memory + state.sp);
                        state.sp += state.memory[state.ip + 1] + 4;
                        continue;
                case SCAS_BYTE:
                        tmp = state.memory[state.yi] - state.accumulator & 0xFF;
                        state.flags = (!state.memory[state.yi] * MAID_FLAG_ZERO) |
                                ((tmp < state.memory[state.yi]) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEF < state.memory[state.yi] & 0xEF) * MAID_FLAG_OVERFLOW) |
                                ((state.memory[state.yi] >> 31) * MAID_FLAG_SIGN);
                        state.yi++;
                        if(rep_flag) for(state.c--; state.c && rep_flag == 1 ? state.flags & MAID_FLAG_ZERO : ~state.flags & MAID_FLAG_ZERO;) {
                                tmp = state.memory[state.yi] - state.accumulator;
                                state.flags = (!state.memory[state.yi] * MAID_FLAG_ZERO) |
                                        ((tmp < state.memory[state.yi]) * MAID_FLAG_CARRY) |
                                        ((tmp & 0xEF < state.memory[state.yi] & 0xEF) * MAID_FLAG_OVERFLOW) |
                                        ((state.memory[state.yi] >> 31) * MAID_FLAG_SIGN);
                                state.yi++;
                                state.c--;
                        }
                        increment = 0;
                        break;
                case SCAS_WORD:
                        tmp = *(WORD*)(state.memory + state.yi) - state.accumulator & 0xFF;
                        state.flags = (!*(WORD*)(state.memory + state.yi) * MAID_FLAG_ZERO) |
                                ((tmp < *(WORD*)(state.memory + state.yi)) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFF < *(WORD*)(state.memory + state.yi) & 0xEFFF) * MAID_FLAG_OVERFLOW) |
                                ((*(WORD*)(state.memory + state.yi) >> 31) * MAID_FLAG_SIGN);
                        state.yi+=2;
                        if(rep_flag) for(state.c--; state.c && rep_flag == 1 ? state.flags & MAID_FLAG_ZERO : ~state.flags & MAID_FLAG_ZERO;) {
                                tmp = *(WORD*)(state.memory + state.yi) - state.accumulator;
                                state.flags = (!*(WORD*)(state.memory + state.yi) * MAID_FLAG_ZERO) |
                                        ((tmp < *(WORD*)(state.memory + state.yi)) * MAID_FLAG_CARRY) |
                                        ((tmp & 0xEFFF < *(WORD*)(state.memory + state.yi) & 0xEFFF) * MAID_FLAG_OVERFLOW) |
                                        ((*(WORD*)(state.memory + state.yi) >> 31) * MAID_FLAG_SIGN);
                                state.yi+=2;
                                state.c--;
                        }
                        increment = 0;
                        break;
                case SCAS_DWORD:
                        tmp = *(DWORD*)(state.memory + state.yi) - state.accumulator & 0xFF;
                        state.flags = (!*(DWORD*)(state.memory + state.yi) * MAID_FLAG_ZERO) |
                                ((tmp < *(DWORD*)(state.memory + state.yi)) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF < *(DWORD*)(state.memory + state.yi) & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((*(DWORD*)(state.memory + state.yi) >> 31) * MAID_FLAG_SIGN);
                        state.yi+=4;
                        if(rep_flag) for(state.c--; state.c && rep_flag == 1 ? state.flags & MAID_FLAG_ZERO : ~state.flags & MAID_FLAG_ZERO;) {
                                tmp = *(DWORD*)(state.memory + state.yi) - state.accumulator;
                                state.flags = (!*(DWORD*)(state.memory + state.yi) * MAID_FLAG_ZERO) |
                                        ((tmp < *(DWORD*)(state.memory + state.yi)) * MAID_FLAG_CARRY) |
                                        ((tmp & 0xEFFFFFFF < *(DWORD*)(state.memory + state.yi) & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                        ((*(DWORD*)(state.memory + state.yi) >> 31) * MAID_FLAG_SIGN);
                                state.yi+=4;
                                state.c--;
                        }
                        increment = 0;
                        break;
                case STO_MA:
                        memtmp = decode_memory(&state, state.memory + state.ip + 1, &increment);
                        *(DWORD*)(state.memory + memtmp) = state.accumulator;
                        break;
                case STO_MI:
                        memtmp = decode_memory(&state, state.memory + state.ip + 5, &increment);
                        *(DWORD*)(state.memory + memtmp) = *(DWORD*)(state.memory + state.ip + 1);
                        increment += 4;
                        break;
                case STOS_BYTE:
                        state.memory[state.yi] = state.accumulator;
                        state.yi++;
                        if(rep_flag == 1) for(state.c--; state.c; state.c--) {
                                state.memory[state.yi] = state.accumulator;
                                state.yi++;
                        }
                        increment = 0;
                        break;
                case STOS_WORD:
                        *(WORD*)(state.memory + state.yi) = state.accumulator;
                        state.yi+=2;
                        if(rep_flag == 1) for(state.c--; state.c; state.c--) {
                                *(WORD*)(state.memory + state.yi) = state.accumulator;
                                state.yi+=2;
                        }
                        increment = 0;
                        break;
                case STOS_DWORD:
                        *(DWORD*)(state.memory + state.yi) = state.accumulator;
                        state.yi+=4;
                        if(rep_flag == 1) for(state.c--; state.c; state.c--) {
                                *(DWORD*)(state.memory + state.yi) = state.accumulator;
                                state.yi+=4;
                        }
                        increment = 0;
                        break;
                case SUB_AI:
                        tmp = state.accumulator;
                        state.accumulator -= *(DWORD*)(state.memory + state.ip + 1);
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) |
                                ((tmp < state.accumulator) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF < state.accumulator & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 4;
                        break;
                case SUV:
                        tmp = state.c;
                        state.c -= *(DWORD*)(state.memory + state.ip + 1);
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) |
                                ((tmp < state.accumulator) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF < state.accumulator & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 4;
                        break;
                case SUB_AM:
                        memtmp = decode_memory(&state, state.memory+state.ip+1, &increment);
                        tmp = state.accumulator;
                        state.accumulator -= *(DWORD*)(state.memory + memtmp);
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) |
                                ((tmp < state.accumulator) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF < state.accumulator & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        break;
                case SUB_AB:
                        tmp = state.accumulator;
                        state.accumulator -= state.b;
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) |
                                ((tmp < state.accumulator) * MAID_FLAG_CARRY) |
                                ((tmp & 0xEFFFFFFF < state.accumulator & 0xEFFFFFFF) * MAID_FLAG_OVERFLOW) |
                                ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 0;
                        break;
                case TEST_AB:
                        tmp = state.accumulator & state.b;
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) | ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 0;
                        break;
                case TEST_AI:
                        tmp = state.accumulator & *(DWORD*)(state.memory + state.ip + 1);
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) | ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 4;
                        break;
                case TEST_AM:
                        memtmp = decode_memory(&state, state.memory + state.ip + 1, &increment);
                        tmp = state.accumulator & *(DWORD*)(state.memory + memtmp);
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) | ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        break;
                case ZERO:
                        r1 = get_register(&state, state.memory[state.ip + 1] & 0xF);
                        *r1 = 0;
                        increment = 1;
                        break;
                case XOR_AB:
                        state.accumulator ^= state.b;
                        state.flags = (!state.accumulator * MAID_FLAG_ZERO) | ((state.accumulator >> 31) * MAID_FLAG_SIGN);
                        increment = 0;
                        break;
                case PUTCHAR:
                        s_putchar(state.accumulator & 0xFF);
                        break;
                }

                rep_flag = 0;
                state.ip += increment + 1;
        }
}
