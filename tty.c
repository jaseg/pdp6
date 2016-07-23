#include "pdp6.h"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <pthread.h>    
#include <poll.h>

/*
 * This device is not accurately modeled after the schematics.
 */

#define IO_TTY (0120>>2)

void tty_recalc_req(Tty *tty) {
    Emu *emu = tty->emu;

    u8 req = 0;
    if (tty->tto_flag || tty->tti_flag)
           req = tty->pia;

    if(req != emu->ioreq[IO_TTY]){
        emu->ioreq[IO_TTY] = req;
        apr_recalc_req(emu->apr);
    }
}

void *tty_thread_handler(void *arg) {
    Tty *tty = (Tty *)arg;
    int sock, cli;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        tty->error_code = TTY_ERR_SOCKET;
        return NULL;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; /* FIXME make this configurable */
    serv_addr.sin_port = htons(tty->port);
    if (bind(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        tty->error_code = TTY_ERR_BIND;
        return NULL;
    }

    listen(sock, 5);

    tty->error_code = TTY_OK;
    while ((cli = accept(sock, NULL, NULL))) {
        debug_print(tty->emu, "TTY attached\n");
        tty->fd = cli;

        size_t n;
        char c;
        while ((n = read(tty->fd, &c, 1)) > 0) {
            tty->tti = c|0200;
            tty->tti_flag = 1;
            tty_recalc_req(tty);
        }
        tty->fd = -1;
        close(cli);
    }

    if (cli < 0)
        tty->error_code = TTY_ERR_ACCEPT;
    return NULL;
}

static void tty_wake(void *arg) {
    Tty *tty = (Tty *)arg;
    Emu *emu = tty->emu;

    if (iob_reset(emu)) {
        tty->pia = 0;
        tty->tto_busy = 0;
        tty->tto_flag = 0;
        tty->tto = 0;
        tty->tti_busy = 0;
        tty->tti_flag = 0;
        tty->tti = 0;
        emu->ioreq[IO_TTY] = 0;
    }

    if (iob_status(emu)) {
        if(tty->tti_busy)
            emu->iobus0 |= F29;
        if(tty->tti_flag)
            emu->iobus0 |= F30;
        if(tty->tto_busy)
            emu->iobus0 |= F31;
        if(tty->tto_flag)
            emu->iobus0 |= F32;
        emu->iobus0 |= tty->pia & 7;
    }

    if (iob_datai(emu))
        emu->apr->ar = tty->tti;

    if (iob_cono_clear(emu))
        tty->pia = 0;

    if (iob_cono_set(emu)){
        if (emu->iobus0 & F25) tty->tti_busy = 0;
        if (emu->iobus0 & F26) tty->tti_flag = 0;
        if (emu->iobus0 & F27) tty->tto_busy = 0;
        if (emu->iobus0 & F28) tty->tto_flag = 0;
        if (emu->iobus0 & F29) tty->tti_busy = 1;
        if (emu->iobus0 & F30) tty->tti_flag = 1;
        if (emu->iobus0 & F31) tty->tto_busy = 1;
        if (emu->iobus0 & F32) tty->tto_flag = 1;
        tty->pia |= emu->iobus0 & 7;
    }

    if (iob_datao_clear(emu))
        tty->tto = 0;

    if (iob_datao_set(emu)) {
        tty->tto = emu->iobus0 & 0377;
        tty->tto_busy = 1;
        if ((tty->tto & 0200) && (tty->fd >= 0)){
            tty->tto &= ~0200;
            write(tty->fd, &tty->tto, 1);
        }
        tty->tto_busy = 0;
        tty->tto_flag = 1;
    }

    tty_recalc_req(tty);
}

Tty *tty_init(Emu *emu, int port) {
    if (!port)
        port = 6666;

    Tty *tty = calloc(sizeof(tty), 1);
    if (!tty)
        return NULL;

    tty->emu = emu;
    tty->fd = -1;
    tty->port = port;
    tty->error_code = TTY_NOT_STARTED;

    if (pthread_create(&tty->thr, NULL, tty_thread_handler, tty))
        return free(tty), NULL;

    emu->ioreq[IO_TTY] = 0;
    emu->iobusmap[IO_TTY].func = tty_wake;
    emu->iobusmap[IO_TTY].arg = tty;

    return tty;
}

