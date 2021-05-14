#ifndef _SSD_NVME_H_
#define _SSD_NVME_H_

enum {
    NVME_REG_CAP = 0x0000,     /* Controller Capabilities */
    NVME_REG_VS = 0x0008,      /* Version */
    NVME_REG_INTMS = 0x000c,   /* Interrupt Mask Set */
    NVME_REG_INTMC = 0x0010,   /* Interrupt Mask Clear */
    NVME_REG_CC = 0x0014,      /* Controller Configuration */
    NVME_REG_CSTS = 0x001c,    /* Controller Status */
    NVME_REG_NSSR = 0x0020,    /* NVM Subsystem Reset */
    NVME_REG_AQA = 0x0024,     /* Admin Queue Attributes */
    NVME_REG_ASQ = 0x0028,     /* Admin SQ Base Address */
    NVME_REG_ACQ = 0x0030,     /* Admin CQ Base Address */
    NVME_REG_CMBLOC = 0x0038,  /* Controller Memory Buffer Location */
    NVME_REG_CMBSZ = 0x003c,   /* Controller Memory Buffer Size */
    NVME_REG_BPINFO = 0x0040,  /* Boot Partition Information */
    NVME_REG_BPRSEL = 0x0044,  /* Boot Partition Read Select */
    NVME_REG_BPMBL = 0x0048,   /* Boot Partition Memory Buffer
                                * Location
                                */
    NVME_REG_PMRCAP = 0x0e00,  /* Persistent Memory Capabilities */
    NVME_REG_PMRCTL = 0x0e04,  /* Persistent Memory Region Control */
    NVME_REG_PMRSTS = 0x0e08,  /* Persistent Memory Region Status */
    NVME_REG_PMREBS = 0x0e0c,  /* Persistent Memory Region Elasticity
                                * Buffer Size
                                */
    NVME_REG_PMRSWTP = 0x0e10, /* Persistent Memory Region Sustained
                                * Write Throughput
                                */
    NVME_REG_DBS = 0x1000,     /* SQ 0 Tail Doorbell */
};

#endif
