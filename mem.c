#include "pdp6.h"
#include <ctype.h>

int mem_read(const char *fname, word *mem, size_t size) {
    FILE *f;
    char buf[100], *s;
    size_t a;
    word w;
    if (!(f = fopen(fname, "r")))
        return -1;
    a = 0;
    while ((s = fgets(buf, 100, f))){
        while (*s){
            if (*s == ';') {
                break;
            } else if ('0' <= *s && *s <= '7') {
                w = strtol(s, &s, 8);
                if (*s == ':') {
                    a = w;
                    s++;
                } else {
                    mem[a++] = w;
                    if (a == size) {
                        fclose(f);
                        return -1;
                    }
                }
            } else {
                s++;
            }
        }
    }
    fclose(f);
    return a;
}

Mem *mem_init(size_t memsize, const char *memfile, const char *regfile) {
    if (!memsize)
        memsize = 65536;

    Mem *mem = calloc(sizeof(Mem) + memsize, 1);
    if (!mem)
        return NULL;

    mem->size = memsize;

    if (memfile)
        mem_read(memfile, mem->memory, memsize);

    if (regfile)
        mem_read(regfile, mem->fmem, 16);

    return mem;
}

void mem_dump(const Mem *mem, const char *filename) {
    FILE *f;

    if(!(f = fopen(filename, "w")))
        return;

    for (size_t a = 0; a < sizeof(mem->fmem); a++)
        fprintf(f, "%02lo: %012lo\n", a, mem->fmem[a]);

    for (size_t a = 0; a < mem->size; a++) {
        if (mem->memory[a]){
            fprintf(f, "%06lo: ", a);
            fprintf(f, "%012lo\n", mem->memory[a]);
        }
    }

    fclose(f);
}

/* 
 * When a cycle is requested we acknowledge the address
 * by pulsing the processor through the bus.
 * A read is completed immediately and signalled by a second pulse.
 * A write is completed on a second call.
 *
 * TODO: implement this properly... according to the manual
 */
int mem_wake(Mem *mem) {
    hword a;
    if (mem->membus0 & MEMBUS_RQ_CYC) {
        a = mem->membus0>>4 & 037777;
        if (mem->membus0 & MEMBUS_MA21_1)
            a |= 0040000;
        if (mem->membus0 & MEMBUS_MA20_1)
            a |= 0100000;
        if (mem->membus0 & MEMBUS_MA19_1)
            a |= 0200000;
        if (mem->membus0 & MEMBUS_MA18_1)
            a |= 0400000;
        if (a >= mem->size || ((mem->membus0 & MEMBUS_MA_FMC_SEL1) && a >= 16))
            return 1;

        mem->membus0 |= MEMBUS_MAI_ADDR_ACK;
        mem->hold = (mem->membus0 & MEMBUS_MA_FMC_SEL1) ? &mem->fmem[a] : &mem->memory[a];

        if (mem->membus0 & MEMBUS_RD_RQ){
            mem->membus1 = (*mem->hold) & FW;
            mem->membus0 |= MEMBUS_MAI_RD_RS;
            if (!(mem->membus0 & MEMBUS_WR_RQ))
                mem->hold = NULL;
        }
    }

    if ((mem->membus0 & MEMBUS_WR_RS) && mem->hold) {
        *mem->hold = mem->membus1 & FW;
        mem->hold = NULL;
        mem->membus0 &= ~MEMBUS_WR_RS;
    }
    return 0;
}

