#ifndef __MTK_DEBUGFS_H__
#define __MTK_DEBUGFS_H__

/**
 * MUSB_HsetPortMode.
 * An HSET port mode.
 */
typedef enum
{
    /** No special port mode */
    MUSB_HSET_PORT_NONE,
    /** TEST_SE0_NAK mode */
    MUSB_HSET_PORT_TEST_SE0_NAK,
    /** TEST_J mode */
    MUSB_HSET_PORT_TEST_J,
    /** TEST_K mode */
    MUSB_HSET_PORT_TEST_K,
    /** TEST_PACKET mode */
    MUSB_HSET_PORT_TEST_PACKET,
    /** TEST_FORCE_ENABLE mode */
    MUSB_HSET_PORT_TEST_FORCE_ENABLE,
    /** TEST_SUSPEND_RESUME */
    MUSB_HSET_PORT_TEST_SUSPEND_RESUME,
    /** TEST_GET_DESC1 */
    MUSB_HSET_PORT_TEST_GET_DESC1,
    /** TEST_GET_DESC2 */
    MUSB_HSET_PORT_TEST_GET_DESC2,
    /** Reset */
    MUSB_HSET_PORT_RESET,
    /** Suspend */
    MUSB_HSET_PORT_SUSPEND,
    /** Resume */
    MUSB_HSET_PORT_RESUME,
    /** Perform SETUP based on FIFO contents */
    MUSB_HSET_PORT_SETUP_START,
    /** Perform IN data phase */
    MUSB_HSET_PORT_SETUP_IN,
    /** Perform status phase of SETUP */
    MUSB_HSET_PORT_SETUP_STATUS
} MUSB_HsetPortMode;

/**
 * MUSB_HsetDeviceTest.
 * An HSET device test.
 */
typedef enum
{
    /** No special device test mode */
    MUSB_HSET_DEVICE_TEST_NONE,
    /** put device in TEST_J mode */
    MUSB_HSET_DEVICE_TEST_J,
    /** put device in TEST_K mode */
    MUSB_HSET_DEVICE_TEST_K,
    /** put device in TEST_SE0_NAK mode */
    MUSB_HSET_DEVICE_TEST_SE0_NAK,
    /** put device in TEST_PACKET mode */
    MUSB_HSET_DEVICE_TEST_PACKET,
    /** perform SET_ADDRESS to device */
    MUSB_HSET_DEVICE_SET_ADDRESS,
    /** perform GET_DESCRIPTOR to device */
    MUSB_HSET_DEVICE_GET_DESCRIPTOR,
    /** perform disable remote wakeup to device */
    MUSB_HSET_DEVICE_DISABLE_WAKEUP,
    /** perform enable remote wakeup to device */
    MUSB_HSET_DEVICE_ENABLE_WAKEUP,
    /** perform SET_FEATURE to device */
    MUSB_HSET_DEVICE_SET_FEATURE
} MUSB_HsetDeviceTest;

extern int mtk_read_testmode(void);
extern void mtk_set_testmode(int value);
extern int mtk_usb_in_testmode(struct usb_hcd *hcd);

int mtk_debugfs_init(MUSB_LinuxController *p);
void mtk_debugfs_exit(MUSB_LinuxController *p);

#endif  //__MTK_DEBUGFS_H__
