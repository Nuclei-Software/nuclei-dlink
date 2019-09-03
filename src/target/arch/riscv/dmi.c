#include <stdlib.h>

#include "dmi.h"
#include "dtm-jtag.h"
#include "rvl-assert.h"

typedef struct rvl_dmi_s
{
    struct pt pt;
    uint32_t i;
    uint32_t data;
    uint32_t op;
}rvl_dmi_t;

static rvl_dmi_t rvl_dmi_i;
#define self rvl_dmi_i


#define RISCV_DMI_OP_NOP        0
#define RISCV_DMI_OP_READ       1
#define RISCV_DMI_OP_WRITE      2

#define RISCV_DMI_RESULT_DONE   0
#define RISCV_DMI_RESULT_FAIL   2
#define RISCV_DMI_RESULT_BUSY   3

#ifndef RVL_DMI_RETRIES
#define RVL_DMI_RETRIES         6
#endif

void rvl_dmi_init(void)
{
    PT_INIT(&self.pt);

    rvl_dtm_init();
}


PT_THREAD(rvl_dmi_nop(void))
{
    PT_BEGIN(&self.pt);

    self.data = 0;
    self.op = RISCV_DMI_OP_NOP;

    PT_WAIT_THREAD(&self.pt, rvl_dtm_dmi(0, &self.data, &self.op));

    PT_END(&self.pt);
}


PT_THREAD(rvl_dmi_read(uint32_t addr, uint32_t* data, uint32_t *result))
{
    PT_BEGIN(&self.pt);

    self.data = 0;
    self.op = RISCV_DMI_OP_READ;

    PT_WAIT_THREAD(&self.pt, rvl_dtm_dmi(addr, &self.data, &self.op));

    for(self.i = 0; self.i < RVL_DMI_RETRIES; self.i++) {
        self.data = 0;
        self.op = RISCV_DMI_OP_NOP;

        PT_WAIT_THREAD(&self.pt, rvl_dtm_dmi(0, &self.data, &self.op));

        if(self.op == RISCV_DMI_RESULT_BUSY) {
            PT_WAIT_THREAD(&self.pt, rvl_dtm_run(32));
        } else {
            break;
        }
    }

    if(self.op != RISCV_DMI_RESULT_DONE)
    {
        PT_WAIT_THREAD(&self.pt, rvl_dtm_dtmcs_dmireset());
    } else {
        *data = self.data;
    }

    if(result) {
        *result = self.op;
    }

    PT_END(&self.pt);
}


PT_THREAD(rvl_dmi_write(uint32_t addr, uint32_t data, uint32_t *result))
{
    PT_BEGIN(&self.pt);

    self.data = data;
    self.op = RISCV_DMI_OP_WRITE;

    PT_WAIT_THREAD(&self.pt, rvl_dtm_dmi(addr, &self.data, &self.op));

    self.data = 0;
    self.op = RISCV_DMI_OP_NOP;

    PT_WAIT_THREAD(&self.pt, rvl_dtm_dmi(0, &self.data, &self.op));

    if(self.op != RISCV_DMI_RESULT_DONE)
    {
        PT_WAIT_THREAD(&self.pt, rvl_dtm_dtmcs_dmireset());
    }

    if(result) {
        *result = self.op;
    }

    PT_END(&self.pt);
}

PT_THREAD(rvl_dmi_read_vector(uint32_t start_addr, uint32_t* buffer, uint32_t len, uint32_t *result))
{
    PT_BEGIN(&self.pt);

    for(self.i = 0; self.i < len; self.i++) {
        self.data = 0;
        self.op = RISCV_DMI_OP_READ;
        PT_WAIT_THREAD(&self.pt, rvl_dtm_dmi(start_addr + self.i, &self.data, &self.op));
        if(self.i > 0) {
            buffer[self.i - 1] = self.data;
        }
    }

    self.data = 0;
    self.op = RISCV_DMI_OP_NOP;

    PT_WAIT_THREAD(&self.pt, rvl_dtm_dmi(0, &self.data, &self.op));
    buffer[self.i - 1] = self.data;

    if(self.op != RISCV_DMI_RESULT_DONE)
    {
        PT_WAIT_THREAD(&self.pt, rvl_dtm_dtmcs_dmireset());
    }

    *result = self.op;

    PT_END(&self.pt);
}

PT_THREAD(rvl_dmi_write_vector(uint32_t start_addr, const uint32_t* buffer, uint32_t len, uint32_t *result))
{
    PT_BEGIN(&self.pt);

    for(self.i = 0; self.i < len; self.i++) {
        self.data = buffer[self.i];
        self.op = RISCV_DMI_OP_WRITE;
        PT_WAIT_THREAD(&self.pt, rvl_dtm_dmi(start_addr + self.i, &self.data, &self.op));
    }

    self.data = 0;
    self.op = RISCV_DMI_OP_NOP;

    PT_WAIT_THREAD(&self.pt, rvl_dtm_dmi(0, &self.data, &self.op));

    if(self.op != RISCV_DMI_RESULT_DONE)
    {
        PT_WAIT_THREAD(&self.pt, rvl_dtm_dtmcs_dmireset());
    }

    *result = self.op;

    PT_END(&self.pt);
}
