#ifndef DSID_H
#define DSID_H

#include "stddef.h"

int set_dsid(int pid, uint32 dsid);
int set_dsid_param(uint32 dsid, uint32 freq, uint32 size, uint32 inc, uint32 mask);
uint32 get_l2_traffic(uint32 dsid);

#endif