/*
* Purpose: CFC Read/Write modulers of USBD(MSDC)
*
* Author: wschung
*
* Date: 2008/09/09
*
* Copyright Generalplus Corp. ALL RIGHTS RESERVED.
*
* Version : 1.00
* History :
* Note : There is no any waiting for CFC DMA..
*/

#include "storages.h"

//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#if (defined CFC_EN) && (CFC_EN == 1)                             //
//================================================================//


INT32S ucfc_read_cmd_phase(INT32U lba, INT32U seccount);
INT32S ucfc_read_dma_phase(INT32U *buf, INT8U ifwait);
INT32S ucfc_read_cmdend_phase(void);
INT32S ucfc_write_cmd_phase(INT32U lba, INT32U seccount);
INT32S ucfc_write_dma_phase(INT32U *buf, INT8U ifwait);
INT32S ucfc_write_cmdend_phase(void);


INT32S ucfc_read_cmd_phase(INT32U lba, INT32U seccount)
{
    INT32S	ret;
    ret = cfc_multiread_cmds(lba, seccount);
    return ret;
}

INT32S ucfc_read_dma_phase(INT32U *buf, INT8U ifwait)
{
    INT32S	ret;
    ret = cfc_read_dma(buf, ifwait);
    return ret;
}
INT32S ucfc_read_cmdend_phase(void)
{
    return 0;
}
INT32S ucfc_write_cmd_phase(INT32U lba, INT32U seccount)
{
    INT32S	ret;
    ret = cfc_multiwrite_cmds(lba, seccount);
    return ret;
}
INT32S ucfc_write_dma_phase(INT32U *buf, INT8U ifwait)
{
    INT32S	ret;
    ret = cfc_write_dma(buf, ifwait);
    return ret;
}
INT32S ucfc_write_cmdend_phase(void)
{
    return 0;
}

struct Usb_Storage_Access USBD_CFC_ACCESS =
{
    ucfc_read_cmd_phase,
    ucfc_read_dma_phase,
    ucfc_read_cmdend_phase,
    ucfc_write_cmd_phase,
    ucfc_write_dma_phase,
    ucfc_write_cmdend_phase,
};
//=== This is for code configuration DON'T REMOVE or MODIFY it ===//
#endif  //(defined CFC_EN) && (CFC_EN == 1)                       //
//================================================================//