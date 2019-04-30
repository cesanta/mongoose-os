# Ethernet support

## ESP32

ESP32 includes an Ethernet MAC and requires an external PHY, connected over RMII interface.

Two PHY models are currently supported:

- Microchip LAN87x0 ([LAN8710](http://ww1.microchip.com/downloads/en/DeviceDoc/00002164B.pdf) supports both MII and RMII
- [LAN8720](http://ww1.microchip.com/downloads/en/DeviceDoc/00002165B.pdf) is RMII only) and [TI TLK110](http://www.ti.com/lit/ds/symlink/tlk110.pdf). PHY model selection is a compile-time option and is set [here](https://github.com/cesanta/mongoose-os/libs/ethernet/blob/master/mos_esp32.yml#L5).

There is a [detailed article](https://sautter.com/blog/ethernet-on-esp32-using-lan8720/)
on how to connect a PHY yourself. It is much easier to buy a dev board
which already has one, for example,
[Olimex ESP32-EVB](https://www.olimex.com/Products/IoT/ESP32-EVB/open-source-hardware).

Once wired, `mos config-set eth.enable=true` to enable Ethernet (see below).

## STM32

ESP32 includes an Ethernet MAC and requires an external PHY, connected over RMII interface.
Currently only LAN8742a PHY in RMII mode is supported.

### MAC pin configuration

Since ETH MAC functions can be assigned to different pins, you need to specify pin assignments.
These are provided as C macros and can be specified in the `cdefs` section.
See example definitions for NUCLEO and Discovery boards in `mos_stm32.yml`.

## Configuration

### Common settings

```javascript
"eth": {
  "enable": true       // Enable Ethernet support
}
```

### ESP32 specific

```javascript
"eth": {
  "phy_addr": 0,       // RMII PHY address
  "mdc_gpio": 23,      // GPIO to use for RMII MDC signal
  "mdio_gpio": 18      // GPIO to use for RMII MDIO signal
}
```

Note: the defaults match the EVB-ESP32 board, so if you use that,
you won't need to adjust anything except setting `eth.enable` to `true`.
