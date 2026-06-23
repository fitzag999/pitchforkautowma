#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUSB_RHPORT0_MODE       OPT_MODE_DEVICE
#define CFG_TUSB_RHPORT1_MODE       OPT_MODE_HOST
#define CFG_TUSB_OS                 OPT_OS_PICO

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUD_ENABLED             1
#define CFG_TUD_MAX_SPEED           OPT_MODE_DEFAULT_SPEED

#define CFG_TUD_VID                 0x2E8A
#define CFG_TUD_PID                 0x0005

#define CFG_TUD_HID                 (1)
#define CFG_TUD_HID_BUFSIZE         64
#define CFG_TUD_HID_EP_BUFSIZE      64

//--------------------------------------------------------------------
// HOST CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUH_ENABLED             1
#define CFG_TUH_MAX_SPEED           OPT_MODE_DEFAULT_SPEED

#define CFG_TUH_HID                 (1)
#define CFG_TUH_HID_EP_BUFSIZE      64
#define CFG_TUH_HID_DEVICE_COUNT    1   // one guitar

#ifdef __cplusplus
 }
#endif

#endif
