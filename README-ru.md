# NapiLab Napi-C — Vendor U-Boot 2017.09

Этот форк U-Boot 2017.09 от Rockchip адаптирован для платы **NapiLab Napi-C** (RK3308) с поддержкой:

- ✅ Загрузки с **eMMC**
- ✅ Dual boot **OpenWrt / Armbian** с SD/eMMC
- ✅ **USB host** для загрузки с USB-флешки

Базируется на родном `evb_rk3308` defconfig от Rockchip; плата `rockpi-s-rk3308` использует тот же header (`evb_rk3308.h` → `rk3308_common.h`), но свой DTS — `rockpi-s-linux.dts`.

## Сборка

```bash
./build.sh
```

Скрипт делает `make rockpi-s-rk3308_defconfig` и сборку с `KCFLAGS="-fcommon"` (нужен из-за нового GCC, в котором `-fno-common` стало дефолтом).

Результат — `u-boot-dtb.bin`. Используется в [napi-vendor-uboot-tool](https://github.com/napilab/napi-vendor-uboot-tool) для подмены U-Boot в Armbian/OpenWrt-образах.

## История изменений

### `napi-c-v1` — vendor U-Boot с saveenv, MBR, фиксированной памятью

Базовый порт от Rockchip evb_rk3308 на rockpi-s-rk3308:

- Устойчивый `saveenv` в env-партиции на eMMC
- Поддержка MBR partition table
- Фиксированный layout памяти под U-Boot

### `napi-c-v2` — чистый boot flow и единая нумерация MMC

- Убраны лишние/конфликтующие сценарии загрузки
- `mmc 0` всегда eMMC, `mmc 1` всегда SD-карта (в стоковом RK3308 порядок плавающий)
- Чистый `boot_targets` без шумов

### `napi-c-v3` — фикс `CONFIG_DOS_PARTITION` + `build.sh`

Когда добавляли USB-конфиги, `CONFIG_DOS_PARTITION` периодически слетал из `.config` из-за сброса дефолтов в Kconfig. Зафиксирован явно в defconfig.

Добавлен `build.sh` — обёртка для сборки с правильным `CROSS_COMPILE` и `KCFLAGS="-fcommon"`.

### `napi-c-v4` — USB host через DTS

USB host работает **только за счёт правок DTS** — без единой строки кода в C. Это самый чистый патч из возможных:

#### Изменения в `arch/arm/dts/rockpi-s-linux.dts`

**1. VBUS regulator на GPIO0_C5:**

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

На Napi-C VBUS физического USB-A разъёма управляется через GPIO0_C5 (high = power ON). Без этого regulator порт обесточен, и флешка не определяется.

**2. Привязка regulator к u2phy_host:**

```dts
&u2phy_host {
    phy-supply = <&vcc5v0_otg>;
    status = "okay";
};
```

Когда PHY probes, U-Boot enable'ит `phy-supply` regulator — VBUS подаётся автоматически.

**3. Отключение OTG:**

```dts
&u2phy_otg { status = "disabled"; };
&usb20_otg { status = "disabled"; };
```

DWC2 OTG конфликтовал с EHCI host за тот же физический порт. Отключение DWC2 не критично — у Napi-C наружу выведен только один USB-host разъём.

#### Изменения в `configs/rockpi-s-rk3308_defconfig`

```
CONFIG_USB_EHCI_HCD=y
CONFIG_USB_EHCI_GENERIC=y
CONFIG_USB_DWC2=y
CONFIG_CMD_USB=y
CONFIG_USB_STORAGE=y
CONFIG_DOS_PARTITION=y
```

OHCI не нужен — USB 1.1 устройства (мышки/клавиатуры/хабы с low-speed периферией) на Napi-C не используются. Generic OHCI требует ещё `#define CONFIG_USB_OHCI_NEW` + `CONFIG_SYS_USB_OHCI_MAX_ROOT_PORTS` в board header — лишние костыли.

## Структура DTS

`rockpi-s-linux.dts` инклюдит `rk3308.dtsi` (общий) + `rk3308-u-boot.dtsi` (u-boot-специфика для всех плат на этом SoC), затем переопределяет узлы для конкретно Rock Pi S.

Важно: USB-узлы статусом `okay`/`disabled` управляются именно в `rockpi-s-linux.dts` (последняя запись побеждает).

## Что НЕ потребовалось

В процессе разработки v4 были перепробованы разные варианты в C-коде (все откачены):

- Бэкпорт `rk3308_phy_cfgs` из mainline U-Boot 2024.10 (старый `rk3328_phy_cfgs` использовался для rk3308 — `phy_sus = { 0x0104, 8, 0, 0, 0x1d1 }` вместо правильного `{ 0x0104, 1, 0, 2, 1 }`)
- Добавление поля `struct clk phyclk` в `rockchip_usb2phy` + `clk_get_by_name` в probe + `clk_enable` в init
- Добавление `generic_phy_power_on/off` в `drivers/usb/host/ehci-generic.c`

Все эти правки **не нужны** — vendor U-Boot 2017.09 уже содержит работающий PHY init и EHCI driver, не хватало только DT-плумбинга для VBUS.

При этом C-правки **активно вредили** — добавляли EHCI timeouts на больших чтениях из-за изменения timing'а инициализации.

## Известные особенности

### Скорость USB
USB read даёт ~13 MiB/s (типично для USB 2.0 на EHCI). Маленькие файлы медленные — каждый `ext4_read` < 1 KB занимает ~2 секунды (особенность U-Boot 2017 EHCI driver, в новых версиях есть block cache). Заметно при загрузке overlay'ев — каждый ~2 сек.

Производительность носителей на Napi-C:

| Носитель | Buffered read |
|---|---|
| USB 2.0 флешка | ~13 MiB/s |
| SD карта | ~22 MiB/s |
| eMMC | ~44 MiB/s |

### UUID конфликт
Если флешка склонирована с SD-карты — у обеих ext4 одинаковый UUID. Linux при `rootwait` берёт первую попавшуюся (обычно SD). Лечится:

```bash
sudo tune2fs -U random /dev/sda1
sudo blkid /dev/sda1   # узнать новый UUID
# обновить rootdev=UUID=... в /boot/armbianEnv.txt
```

## Связанные репозитории

- [napi-vendor-uboot-tool](https://github.com/napilab/napi-vendor-uboot-tool) — скрипты подмены U-Boot в готовых Armbian (`run-vendor-uboot.sh`) и OpenWrt (`napiwrt-vendor-uboot.sh`) образах.
