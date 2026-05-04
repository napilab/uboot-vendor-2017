# NapiLab Napi-C — Vendor U-Boot 2017.09

This fork of Rockchip's vendor U-Boot 2017.09 is adapted for the **NapiLab Napi-C** board (RK3308) and supports:

- ✅ Boot from **eMMC**
- ✅ Dual boot **OpenWrt / Armbian** from SD/eMMC
- ✅ **USB host** for booting from a USB stick

Based on the upstream `evb_rk3308` defconfig from Rockchip; the `rockpi-s-rk3308` board reuses the same header (`evb_rk3308.h` → `rk3308_common.h`) but ships its own DTS — `rockpi-s-linux.dts`.

## Build

```bash
./build.sh
```

The script runs `make rockpi-s-rk3308_defconfig` and builds with `KCFLAGS="-fcommon"` (required by modern GCC, where `-fno-common` became the default).

The resulting `u-boot-dtb.bin` is consumed by [napi-vendor-uboot-tool](https://github.com/napilab/napi-vendor-uboot-tool), which patches the bootloader into Armbian/OpenWrt images.

## Release history

### `napi-c-v1` — vendor U-Boot with saveenv, MBR, fixed memory layout

Initial port of Rockchip's evb_rk3308 to rockpi-s-rk3308:

- Working `saveenv` into the env partition on eMMC
- MBR partition table support
- Fixed memory layout for U-Boot

### `napi-c-v2` — clean boot flow and consistent MMC numbering

- Removed redundant/conflicting boot scenarios
- `mmc 0` always means eMMC, `mmc 1` always means SD card (the default Rockchip ordering is unstable on RK3308)
- Clean `boot_targets` without noise

### `napi-c-v3` — fix `CONFIG_DOS_PARTITION` + add `build.sh`

While adding USB configuration options, `CONFIG_DOS_PARTITION` kept getting silently dropped from `.config` because of Kconfig default reshuffling. Now pinned explicitly in the defconfig.

Added `build.sh` — a wrapper that sets the correct `CROSS_COMPILE` and `KCFLAGS="-fcommon"`.

### `napi-c-v4` — USB host via DTS only

USB host now works **purely via DTS changes** — no C code touched. This is the cleanest possible patch:

#### Changes in `arch/arm/dts/rockpi-s-linux.dts`

**1. VBUS regulator on GPIO0_C5:**

```dts
vcc5v0_otg: vcc5v0-otg {
    compatible = "regulator-fixed";
    enable-active-high;
    gpio = <&gpio0 RK_PC5 GPIO_ACTIVE_HIGH>;
    regulator-name = "vcc5v0_otg";
    regulator-always-on;
    regulator-boot-on;
};
```

On Napi-C, VBUS for the physical USB-A port is gated by GPIO0_C5 (high = power ON). Without this regulator the port has no power and a USB stick will not be detected at all.

**2. Wire the regulator to u2phy_host:**

```dts
&u2phy_host {
    phy-supply = <&vcc5v0_otg>;
    status = "okay";
};
```

When the PHY probes, U-Boot enables the `phy-supply` regulator, which drives VBUS automatically.

**3. Disable OTG:**

```dts
&u2phy_otg { status = "disabled"; };
&usb20_otg { status = "disabled"; };
```

The DWC2 OTG controller was contending with EHCI host for the same physical port. Disabling DWC2 is fine — Napi-C only exposes a single USB-A host port externally.

#### Changes in `configs/rockpi-s-rk3308_defconfig`

```
CONFIG_USB_EHCI_HCD=y
CONFIG_USB_EHCI_GENERIC=y
CONFIG_USB_DWC2=y
CONFIG_CMD_USB=y
CONFIG_USB_STORAGE=y
CONFIG_DOS_PARTITION=y
```

OHCI is **not** enabled — USB 1.1 devices (low-speed mice/keyboards or hubs with low-speed downstream ports) are not used on Napi-C. Generic OHCI in this U-Boot also requires `#define CONFIG_USB_OHCI_NEW` and `CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS` in the board header — extra plumbing we don't need.

## DTS structure

`rockpi-s-linux.dts` includes `rk3308.dtsi` (SoC-wide) + `rk3308-u-boot.dtsi` (U-Boot-specific overrides shared across all RK3308 boards), then overrides nodes for Rock Pi S specifically.

Important: USB node `status = "okay" / "disabled"` is controlled in `rockpi-s-linux.dts` (the last definition wins).

## What was NOT needed

While bringing v4 up, several C-level changes were tried and reverted:

- Backporting `rk3308_phy_cfgs` from mainline U-Boot 2024.10. The vendor tree mistakenly used `rk3328_phy_cfgs` for RK3308 (`phy_sus = { 0x0104, 8, 0, 0, 0x1d1 }` instead of the correct `{ 0x0104, 1, 0, 2, 1 }`).
- Adding a `struct clk phyclk` field to `rockchip_usb2phy` + `clk_get_by_name` in probe + `clk_enable` in init.
- Adding `generic_phy_power_on/off` calls in `drivers/usb/host/ehci-generic.c`.

None of these are required — vendor U-Boot 2017.09 already contains a working PHY init and EHCI driver. Only the DT plumbing for VBUS was missing.

In fact these C changes were actively harmful — they introduced EHCI timeouts on large reads by perturbing initialization timing.

## Known quirks

### USB read speed
Sequential USB reads yield ~13 MiB/s (typical for USB 2.0 on EHCI). Small files are slow — every `ext4_read` under ~1 KB takes around 2 seconds (a known limitation of the U-Boot 2017 EHCI driver; newer versions added a block cache). This is most noticeable when applying DT overlays during boot — about 2 s per overlay.

Storage performance on Napi-C:

| Device | Buffered read |
|---|---|
| USB 2.0 stick | ~13 MiB/s |
| SD card | ~22 MiB/s |
| eMMC | ~44 MiB/s |

### UUID collision
If a USB stick was cloned from an SD card, both filesystems will share the same ext4 UUID. With `rootwait` Linux mounts whichever device wins enumeration first (usually the SD). Fix:

```bash
sudo tune2fs -U random /dev/sda1
sudo blkid /dev/sda1   # note the new UUID
# update rootdev=UUID=... in /boot/armbianEnv.txt
```

## Related repositories

- [napi-vendor-uboot-tool](https://github.com/napilab/napi-vendor-uboot-tool) — scripts that splice this U-Boot into ready-made Armbian (`run-vendor-uboot.sh`) and OpenWrt (`napiwrt-vendor-uboot.sh`) images.
