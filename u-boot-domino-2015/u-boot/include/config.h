#define CONFIG_BOOTDELAY 2
#define CONFIG_AUTOBOOT_KEYED 2
#define CONFIG_MAX_UBOOT_SIZE_KB 256
#define CONFIG_DELAY_TO_AUTORUN_HTTPD        3
#define CONFIG_DELAY_TO_AUTORUN_CONSOLE      5
#define CONFIG_DELAY_TO_AUTORUN_NETCONSOLE   7
#define CONFIG_MAX_BUTTON_PRESSING          10
#define CFG_CONSOLE_INFO_QUIET
#define CONFIG_AR7240                        1
#define CONFIG_MACH_HORNET                   1
#define CONFIG_HORNET_1_1_WAR                1
#define CONFIG_FOR_DOMINO       1
#undef COMPRESSED_UBOOT
#define GPIO_WLAN_LED_BIT                    0
#define GPIO_WLAN_LED_ON                     1
#define GPIO_RST_BUTTON_BIT                 11
#define DEFAULT_FLASH_SIZE_IN_MB            16
#define BOARD_CUSTOM_STRING                  "AP121 (AR9331) U-Boot for DOMINO v1"

/* Automatically generated - do not edit */
#include <configs/ap121.h>
