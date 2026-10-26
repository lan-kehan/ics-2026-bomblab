#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8
#define SECRET_PREFIX "I can't live without claude codeOPEN"

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_01 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    count = 1;
    // step0 32bits(challenge + high_32_bits(challenge) * 8) ^ 0xb2c
    uint32_t step0 = (uint32_t)(challenge + ((challenge >> 32) * 8)) ^ 0xb2c;
    // step1 (high32(challenge) - 32(challenge >> 16)) * 5 + 0x59cc, then cut to 32 bits
    uint32_t step1 = (uint32_t)((uint64_t)((uint32_t)(challenge >> 32) - (uint32_t)(challenge >> 16)) * 5 + 0x59cc);
    // step2 (rdi + rdi * 2 + esi) ^ 0x24bb
    uint32_t step2 = (step1 * 3 + (uint32_t)(challenge)) ^ 0x24bb;
    values[count-1] = step2 ^ step0;
    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    uint32_t x;
    (void)challenge;
    // TODO: 根据 phase_02 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    count = 6;

    values[0] = 3 * ((uint32_t)(challenge >> 8) & 0xff);
    int i = 1;
    for (; i < 6; ++i) {
        x =  ((((uint32_t) (challenge >> (i + 10))) & 0x3f) + i) ^ values[i-1];
        x = x + (x << 2);
        values[i] = x;
    }
    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_03 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    count = 2;
    values[0] = (uint32_t) challenge & 0x7;
    uint32_t b = (challenge >> 3) ^ (challenge >> 32);
    switch (values[0]) {
        case 0: values[1] = 0x12dd1 + 5 * b; break;
        case 1: values[1] = (b >> 2) + (b ^ 0x6c59); break;
        case 2: values[1] = (b >> 1) + (b ^ 0x4177); break;
        case 3: values[1] = (b * 2) ^ 0x34ee; break;
        case 4: values[1] = 5 * b + 0xe015; break;
        case 5: values[1] = b * 2 ^ 0x6504; break;
        case 6: values[1] =  0x77ab + 5 * b; break;
        case 7: values[1] = 0xee9d + 5 * b; break;
        default: values[1] = 0; break;
    }
    // a = 0 - 6 corresponds to p3 case 3 7 0 1 5 4 2 6

    return write_answer(out, cap, values, count);
}

uint32_t walk_phase_4(uint32_t s, uint32_t di, uint32_t dx, uint32_t c) {
    uint32_t a = di;
    uint32_t newdi = ((dx - s) >> 1) + s;
    if (a == newdi) {
        return (0x20 + newdi) ^c;
    } else if (a < newdi) {
        return (walk_phase_4(s, a, newdi - 1, c) * 3) ^ (newdi + c) ^ 0x3;
    } else {
        return (walk_phase_4(newdi + 1, a, dx, c) * 3) ^ (newdi + c) ^ 0xf;
    }
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_04 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    count = 1;
    uint32_t s = 0;
    uint32_t di = challenge & 0x7f;
    uint32_t dx = 0x7f;
    uint32_t c = (challenge >> 32) ^ (challenge >> 7);

    values[0] = walk_phase_4(s, di, dx, c);
    return write_answer(out, cap, values, count);
}

// ❯ gdb -batch -ex "x/8gx 0x405b60" ./bomb          
// 0x405b60 <p5_heads>:    0x0000000000405c10      0x0000000000405c00
// 0x405b70 <p5_heads+16>: 0x0000000000405bd0      0x0000000000405bc0
// 0x405b80 <p5_heads+32>: 0x0000000000405be0      0x0000000000405c30
// 0x405b90 <p5_heads+48>: 0x0000000000405c20      0x0000000000405ba0

//  ~/ics-2026-bomblab  main +1 !2 ?8                                                                                                                                                                                          root@ics
// ❯ gdb -batch -ex "x/48wx 0x405ba0" ./bomb
// 0x405ba0 <p5_nodes>:    0x00405bf0      0x00000000      0x0000087b      0x0000d973
// 0x405bb0 <p5_nodes+16>: 0x00000000      0x00000000      0x00000a86      0x00005722
// 0x405bc0 <p5_nodes+32>: 0x00405be0      0x00000000      0x00000b64      0x0000fed3
// 0x405bd0 <p5_nodes+48>: 0x00405bc0      0x00000000      0x00000684      0x0000268d
// 0x405be0 <p5_nodes+64>: 0x00405c30      0x00000000      0x0000034f      0x00000505
// 0x405bf0 <p5_nodes+80>: 0x00405c50      0x00000000      0x00000011      0x0000af26
// 0x405c00 <p5_nodes+96>: 0x00405bd0      0x00000000      0x00000474      0x0000085f
// 0x405c10 <p5_nodes+112>:        0x00405c00      0x00000000      0x00000255      0x0000113d
// 0x405c20 <p5_nodes+128>:        0x00405ba0      0x00000000      0x00000549      0x000032e4
// 0x405c30 <p5_nodes+144>:        0x00405c20      0x00000000      0x00000763      0x00005cf0
// 0x405c40 <p5_nodes+160>:        0x00405bb0      0x00000000      0x0000016e      0x0000d1e4
// 0x405c50 <p5_nodes+176>:        0x00405c40      0x00000000      0x0000093a      0x00003c66

struct node{
    int next;
    uint32_t v1;
    uint32_t v2;
};

static const struct node nodes[] = {
    {5, 0x0000087b, 0x0000d973},
    {-1, 0x00000a86, 0x00005722},
    {4, 0x00000b64, 0x0000fed3},
    {2, 0x00000684, 0x0000268d},
    {9, 0x0000034f, 0x00000505},
    {11, 0x00000011, 0x0000af26},
    {3, 0x00000474, 0x0000085f},
    {6, 0x00000255, 0x0000113d},
    {0, 0x00000549, 0x000032e4},
    {8, 0x00000763, 0x00005cf0},
    {1, 0x0000016e, 0x0000d1e4},
    {10, 0x0000093a, 0x00003c66},
};

static const int heads[]= {
    7, 6, 3, 2, 4, 9, 8, 0,
};

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_05 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    count = 2;
    uint32_t a = challenge & 0x7;
    uint32_t times = (challenge >> 3) & 0x3;
    uint32_t i = 1;
    int head = heads[a];
    uint32_t s = nodes[head].v1;
    a = nodes[head].v2 ^ ((challenge >> 6) ^ (challenge >> 32)) ^ 0xd322;
    a += s;
    values[1] = s;
    if (times > 0) {
        for (; i <= times; ++i) {
            head = nodes[head].next;
            s = nodes[head].v1;
            a ^= nodes[head].v2;
            a += s;
            values[1] = s;
        }
    }
    values[0] = a;
    return write_answer(out, cap, values, count);
}

typedef uint32_t (*p6_op)(uint32_t);

static uint32_t p6_op_0(uint32_t a) { return (a ^ (a >> 7)) + 0x5a06; }
static uint32_t p6_op_1(uint32_t a) { return (a * 5) ^ (0xe3fd + (a << 7)); }
static uint32_t p6_op_2(uint32_t a) { return (a + 0x31a9) ^ (a >> 2); }
static uint32_t p6_op_3(uint32_t a) { return  ((a << 5) | (a >> 27)) ^ 0x8661; }
static uint32_t p6_op_4(uint32_t a) { return ((a >> 2) | (a << 30)); }
static uint32_t p6_op_5(uint32_t a) { return (a << 5) + (a ^ 0x6a58); }

static const p6_op p6ops[] = {
    p6_op_2, p6_op_1,
    p6_op_4, p6_op_1,
    p6_op_5, p6_op_1,
    p6_op_3, p6_op_0,
};

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 0;
    (void)challenge;
    // TODO: 根据 phase_06 的汇编，用 challenge 算出口令中的各个整数，依次存入 values，并把 count 改为整数个数。
    count = 1;
    uint32_t a = (challenge >> 6) ^ (challenge >> 32);
    uint32_t b = challenge & 0x38;
    uint32_t c = challenge & 0x7;
    // 0x405e00 <p6_ops>:      0x402510 <p6_op_2>      0x4024f0 <p6_op_1>
    // 0x405e10 <p6_ops+16>:   0x402530 <p6_op_4>      0x4024f0 <p6_op_1>
    // 0x405e20 <p6_ops+32>:   0x402540 <p6_op_5>      0x4024f0 <p6_op_1>
    // 0x405e30 <p6_ops+48>:   0x402520 <p6_op_3>      0x4024d0 <p6_op_0>
    uint32_t first = p6ops[c](a);
    first ^= 0xcb1f;
    uint32_t second = p6ops[b >> 3](first);
    values[0] = second + 0xa499;
    return write_answer(out, cap, values, count);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    (void)challenge;
    (void)context;
    (void)out;
    (void)cap;
    // TODO（选做）: 根据 secret_gate、secret_copy 和 secret_phase 的汇编构造完整的第七行，写入 out 后返回 0。
    // 第七行不一定只有整数，可以直接用 snprintf 写入；还没完成时保持返回 1，测试会显示 not attempted。
    uint32_t a = context & 3;
    uint32_t tmp1 = a * 0x53b8;
    uint32_t tmp2 = challenge ^ (challenge >> 32);
    uint32_t tmp3 = context ^ (context >> 32);
    uint32_t tmp4 = tmp1 ^ (context >> 16);
    uint32_t mixinput = tmp2 ^ tmp3;
    uint32_t mixoutput =  ((mixinput >> 3)  ^ 0xe24e) + (mixinput ^ 0x53b8);
    uint32_t b = tmp4 ^ mixoutput;
    uint32_t c = b ^ (tmp3 + a + 0xe24e + (tmp2 >> 3));
    int n = snprintf(out, cap, SECRET_PREFIX "%" PRIu32 " %" PRIu32 " %" PRIu32, a, b, c);
    if (n < 0 || (size_t)n >= cap) return 1;
    return 0;
}
