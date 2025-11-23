#ifndef TUSB_CONFIG_CUSTOM_H
#define TUSB_CONFIG_CUSTOM_H

// 自定义TinyUSB配置
// 这些配置会覆盖默认的TinyUSB配置

// 增加Vendor类的缓冲区大小
#ifndef CFG_TUD_VENDOR_RX_BUFSIZE
#define CFG_TUD_VENDOR_RX_BUFSIZE    256  // 增加到256字节
#endif

#ifndef CFG_TUD_VENDOR_TX_BUFSIZE
#define CFG_TUD_VENDOR_TX_BUFSIZE    256  // 增加到256字节
#endif

#ifndef CFG_TUD_VENDOR_EPSIZE
#define CFG_TUD_VENDOR_EPSIZE        64   // 端点大小
#endif

// 增加FIFO大小
#ifndef CFG_TUD_VENDOR_FIFO_SIZE
#define CFG_TUD_VENDOR_FIFO_SIZE     256  // FIFO大小
#endif

// 启用调试（只在未定义时定义）
#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG               1
#endif

#ifndef CFG_TUSB_DEBUG_LEVEL
#define CFG_TUSB_DEBUG_LEVEL         1
#endif

// 优化任务配置
#ifndef CFG_TUD_TASK_PRIORITY
#define CFG_TUD_TASK_PRIORITY        5
#endif

#ifndef CFG_TUD_TASK_STACK_SIZE
#define CFG_TUD_TASK_STACK_SIZE      4096
#endif

// 启用DMA模式
#ifndef CFG_TUD_MODE
#define CFG_TUD_MODE                 TUD_MODE_DMA
#endif

#endif // TUSB_CONFIG_CUSTOM_H 