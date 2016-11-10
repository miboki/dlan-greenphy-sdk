/*
 * clickboardconfig.c
 *
 *  Created on: 21.09.2015
 *      Author: Sebastian Sura
 */

#include "types.h"

int m1old, m2old;
extern config_t *gpconfig;

int setupclickboard(int port, int board) {

    switch (board) {
    case 1:
        init_temp_sensor(port);
        break;
    case 2:
        init_relay(port);
        break;
    case 3:
        init_dali(port);
        break;
    case 4:
        init_color_sensor(port);
        break;
    case 5:
        init_uv_sensor(port);
        break;
    default:
        break;
    }
    return 1;
}

int deinit(int board) {
    switch (board) {
    case 1:
        deinit_temp_sensor();
        break;
    case 2:
        deinit_relay();
        break;
    case 3:
        deinit_dali();
        break;
    case 4:
        deinit_color_sensor();
        break;
    case 5:
        deinit_uv_sensor();
        break;
    default:
        break;
    }
}

void checkconfig(char *c) {

    /*Save last clickboardconfig for deinit*/
    m1old = gpconfig->M1;
    m2old = gpconfig->M2;

    if (strstr(c, "M1=temp")) {
        gpconfig->M1 = 1;
    }
    if (strstr(c, "M1=relay")) {
        gpconfig->M1 = 2;
    }
    if (strstr(c, "M1=dali")) {
        gpconfig->M1 = 3;
    }
    if (strstr(c, "M1=color")) {
        gpconfig->M1 = 4;
    }
    if (strstr(c, "M1=uv")) {
        gpconfig->M1 = 5;
    }
    if (strstr(c, "M1=none")) {
        gpconfig->M1 = 0;
    }
    if (strstr(c, "M2=temp")) {
        gpconfig->M2 = 1;
    }
    if (strstr(c, "M2=relay")) {
        gpconfig->M2 = 2;
    }
    if (strstr(c, "M2=dali")) {
        gpconfig->M2 = 3;
    }
    if (strstr(c, "M2=color")) {
        gpconfig->M2 = 4;
    }
    if (strstr(c, "M2=uv")) {
        //m2 = 5;
        /*UVClick not supported on M2*/
        gpconfig->M2 = m2old;
    }
    if (strstr(c, "M2=none")) {
        gpconfig->M2 = 0;
    }

    if (gpconfig->M1 == 0 && gpconfig->M2 == 0) {
        deinit(m1old);
        deinit(m2old);
        writeflash();
        return;
    }

    /*Initialize Clickboards*/
    if (gpconfig->M1 != gpconfig->M2) {
        if (gpconfig->M1 != m1old) {
            deinit(m1old);
            setupclickboard(1, gpconfig->M1);
        } else {
            /*restore last config*/
            gpconfig->M1 = m1old;
        }
        if (gpconfig->M2 != m2old) {
            deinit(m2old);
            setupclickboard(2, gpconfig->M2);
        } else {
            /*restore last config*/
            gpconfig->M2 = m2old;
        }
    } else {
        /*restore last config*/
        gpconfig->M1 = m1old;
        gpconfig->M2 = m2old;
        return;
    }

    /*Save config in Flash*/
    writeflash();
}

