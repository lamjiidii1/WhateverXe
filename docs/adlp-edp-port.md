# WhateverXe 0.8.0-adlp: Alder Lake Internal eDP Port & Fixes

## 1. Overview

This document describes the architectural changes and fixes ported onto **WhateverXe 0.8.0** in the `adlp-0.8.0` branch to enable internal eDP/OLED panel display output and full QE/CI hardware acceleration on **Intel 12th Gen Alder Lake-U/P (Iris Xe `8086:46AA`)** systems, specifically verified on the **HP Envy x360 2-in-1 Laptop 13-bf0xxx**.

---

## 2. Problems in Upstream WhateverXe on Alder Lake eDP

When running upstream WhateverXe (0.6.x, 0.7.0, or 0.8.0) on an Alder Lake-U/P laptop with an internal OLED display, three independent issues prevent internal panel bringup:

### 2.1 VBT Digital Child Duplicate Conflict
In the OEM firmware VBT (Video BIOS Table) for boards like the HP `8A28`, the internal display is described with multiple child devices mapping to DDI 1 (eDP on Combo A `aux 0x40`, Combo B `aux 0x10`, plus an auxiliary DVO1 declaration).
- Upstream `VBT::parse()` treats any duplicate DDI declaration as `Status::Conflict` and discards all parsed ports.
- **Fix**: The `adlp-edp` patchset implements first-child-wins resolution (`firstWins = true`), keeping the first valid digital child per DDI, counting duplicates, and retaining the port map.

### 2.2 VBT Panel Record Selection (`panel_type = 0xFF`)
The firmware VBT block 40 defines `panel_type = 0xFF` ("select panel by PnP ID").
- Upstream's internal eDP path relies on a valid static VBT panel record to configure timings. With `panel_type = 0xFF`, no panel record is parsed, leaving `panel.valid = 0`.
- **Fix**: The `-wex-adlp-edp` boot argument instructs WhateverXe to add Combo A as a **fixed DP output** (`aux 0x40`), allowing Display 13 to query EDID and link parameters directly from the panel.

### 2.3 Upstream 0.8.0 Speculative Type-C Route Collision (Black Screen)
Upstream 0.8.0 introduced automatic DP-alt Type-C port routing in `display13Fixed()` (`kern_display13_firmware.hpp`).
- For laptops declaring Thunderbolt 4 / USB-C DDIs (DDIs 3–6 on the HP Envy), this added `0x78` to `board.dp` and `board.typeC`.
- In `kern_xe.cpp`, line 5598:
  ```cpp
  if (display13PanelDP && (adlnOutputWiring.dp & 3) && !(adlnOutputWiring.selected() & 0x78))
  ```
- Because `selected() & 0x78` was non-zero (`0x79`), `display13ComboDPReady` remained unset, `processDisplay13Training()` was skipped, and `AppleIntelFramebuffer::allocateBootDisplayResources` failed with error `0xe00002bc` (`FB0: Resources allocation failed` / `FB0: Failed to commit boot display resources`), resulting in a completely black screen on boot.
- **Fix**: In `display13BoardWiring`, when `-wex-adlp-edp` is active, speculative Type-C routes are explicitly masked:
  ```cpp
  if (display13PanelDP) {
      board.dp &= ~0x78U;
      board.typeC = 0;
  }
  ```
  This restores `selected() == 0x01` (Combo A OLED), allowing link training, backlight profile attachment (`F16T4166`), and framebuffer modeset to complete cleanly.

  **Trade-off:** because the speculative TC DP-alt routes are removed from the fixed wiring, external DisplayPort over USB-C is not offered while `-wex-adlp-edp` is active. USB data and USB-C charging are unaffected. Driving the internal panel is the priority for this variant; an external-sink-aware probe would be needed to keep both routes available. The VBT records for those TC ports are capability descriptions, not proof of a live DP-alt connection.

---

## 3. Improvements in WhateverXe 0.8.0-adlp

### 3.1 Native Dynamic MMIO Hook Resolution
In WhateverXe 0.6.0, `tglPrepareSequoiaMMIOUC()` required matching rigid 9-byte kernel sequences (`E8 71 CB 02 00` at `_IOProtectCacheMode+0xCA`). On macOS Sequoia 15.8.2 (`24H232`), this offset shifted by `+0x30`, requiring a manual binary patch (`E8 A1 CB 02 00`).

**In WhateverXe 0.8.0**:
- Dynamic kernel MMIO resolution dynamically resolves `_IOProtectCacheMode` and `pmap` at runtime across macOS versions and point updates.
- `"TGLMMIOStrongUCRouted" = Yes` is asserted automatically.
- **Zero binary patching or script execution is required** on 15.8.2 or future updates.

### 3.2 Display 13 Downscaler Bounds
Updated `display13_wm` watermark downscale bounds (`width > 3 * htotal` and `height > 3 * vtotal`) to support high-resolution panels scanning virtual surfaces up to 3200-wide (e.g. 2880×1800 HiDPI at Retina 1440×900).

### 3.3 Userspace Compatibility
WhateverXe 0.8.0 maintains backward ABI compatibility with `AppleIntelTGLCompat.kext` 0.6.0 (WhateverXe Graphics 0.3.1). Users with existing Graphics 0.3.1 installations do not need to reinstall or change userspace packages.

---

## 4. Hardware Verification Metrics

Tested on an **HP ENVY x360 2-in-1 Laptop 13-bf0xxx**:
- **CPU**: Intel Core i7-1250U (2P + 8E / 12 threads)
- **iGPU**: Intel Iris Xe (Alder Lake-UP4 GT2), `8086:46AA`, rev 0x0C
- **Display**: 2880×1800 OLED (`AppleBacklightDisplay`, EDID `4c83:4166`)
- **OS**: macOS Sequoia 15.8.2 (`24H232`, Darwin 24.6.0)
- **OpenCore**: 1.0.8

### IORegistry Output:
```text
"VersionInfo" = "REL-080-2026-11-07"
"TGLMMIOStrongUCRouted" = Yes
"WEXDisplay13ComboDPReady" = Yes
"WEXDisplay13TrainingReady" = Yes
"WhateverXeBacklightProfile" = "F16T4166"
"IntelAccelerator" = present (Metal 2, 8,000+ command submissions, 0 panics)
"Resolution" = 2880 x 1800 (Looks like 1440 x 900 @ 60 Hz, ARGB8888)
```

---

## 5. Required Boot Arguments

When using the `adlp-0.8.0` build on an Alder Lake laptop with an internal eDP/OLED panel:

```text
-allow3d -disablegfxfirmware -igfxdvmt -wex-adlp-edp -wex-reset-restore -wex-no-multicontext -wex-scanout-coherent -wex-pm
```

- `-wex-adlp-edp`: Enables first-child-wins VBT parsing and forces Combo A DP fixed route.
- `-wex-reset-restore`: Restores display/render state after a GPU reset.
- `-wex-no-multicontext`: Avoids second compute ring deadlock on recovery.
- `-wex-scanout-coherent`: Ensures coherent surface flips for high-resolution displays.

---

## 6. Building from Source

```sh
# Ensure MacKernelSDK and Lilu.kext are present in the repo root or passed as arguments:
python3 tools/build.py --release
python3 tools/test.py
```

Built artifacts will be placed in `build/`:
- `build/WhateverXe.kext`
- `build/AppleIntelTGLCompat.kext`

`python3 tools/test.py` runs the standalone CPU suite; the `adlp-0.8.0` branch is expected to pass all 210 tests.
