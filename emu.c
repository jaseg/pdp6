#include "pdp6.h"

Emu *emu_init(size_t memsize) {
	Emu *emu = calloc(sizeof(Emu), 1);
	if (!emu)
		return NULL;

	if (!(emu->apr = apr_init(emu)))
		return free(emu), NULL;
	emu->apr->emu = emu;

	if (!(emu->mem = mem_init(memsize, NULL, NULL)))
		return free(emu->apr), free(emu), NULL;

	return emu;
}

void emu_destroy(Emu *emu) {
	free(emu->apr);
	free(emu);
}

bool iob_reset(Emu *emu) {
    return emu->iobus1 & IOBUS_IOB_RESET;
}

bool iob_datao_clear(Emu *emu) {
    return emu->iobus1 & IOBUS_DATAO_CLEAR;
}

bool iob_datao_set(Emu *emu) {
    return emu->iobus1 & IOBUS_DATAO_SET;
}

bool iob_cono_clear(Emu *emu) {
    return emu->iobus1 & IOBUS_CONO_CLEAR;
}

bool iob_cono_set(Emu *emu) {
    return emu->iobus1 & IOBUS_CONO_SET;
}

bool iob_status(Emu *emu) {
    return emu->iobus1 & IOBUS_IOB_STATUS;
}

bool iob_datai(Emu *emu) {
    return emu->iobus1 & IOBUS_IOB_DATAI;
}

