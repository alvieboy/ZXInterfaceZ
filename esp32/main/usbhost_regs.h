// For read
#define USB_REG_STATUS (0x0000)
# define USB_STATUS_RESET (1<<4)
# define USB_STATUS_PWRON (1<<5)
# define USB_STATUS_CONN (1<<0)

#define USB_REG_INTPEND1 (0x0001)
# define USB_INTPEND_DISC (1<<0)
# define USB_INTPEND_CONN (1<<1)
# define USB_INTPEND_OVERCURRENT (1<<2)

# define USB_INTACK (1<<7)

#define USB_REG_INTPEND2 (0x02)

#define USB_REG_INTCONF  (0x02)
# define USB_INTCONF_GINTEN (1<<7)
# define USB_INTCONF_DISC (1<<0)
# define USB_INTCONF_CONN (1<<1)
# define USB_INTCONF_OVERCURRENT (1<<2)

#define USB_REG_INTCLR  (0x03)
# define USB_INTCLR_DISC (1<<0)
# define USB_INTCLR_CONN (1<<1)
# define USB_INTCLR_OVERCURRENT (1<<2)

#define USB_REG_CHAN_BASE (1U<<6)

#define USB_REG_CHAN_INTPEND(chan) ((USB_REG_CHAN_BASE) + (((uint16_t)chan)<<3) + 0x04)
#define USB_REG_CHAN_CONF1(chan) ((USB_REG_CHAN_BASE) + (((uint16_t)chan)<<3) + 0x00)
#define USB_REG_CHAN_CONF2(chan) ((USB_REG_CHAN_BASE) + (((uint16_t)chan)<<3) + 0x01)
#define USB_REG_CHAN_CONF3(chan) ((USB_REG_CHAN_BASE) + (((uint16_t)chan)<<3) + 0x02)
#define USB_REG_CHAN_INTCONF(chan) ((USB_REG_CHAN_BASE) + (((uint16_t)chan)<<3) + 0x03)
#define USB_REG_CHAN_INTCLR(chan) ((USB_REG_CHAN_BASE) + (((uint16_t)chan)<<3) + 0x04)
#define USB_REG_CHAN_TRANS1(chan) ((USB_REG_CHAN_BASE) + (((uint16_t)chan)<<3) + 0x05)
#define USB_REG_CHAN_TRANS2(chan) ((USB_REG_CHAN_BASE) + (((uint16_t)chan)<<3) + 0x06)
#define USB_REG_CHAN_TRANS3(chan) ((USB_REG_CHAN_BASE) + (((uint16_t)chan)<<3) + 0x07)

#define USB_REG_DATA(address) ((address) | 0x0400)
