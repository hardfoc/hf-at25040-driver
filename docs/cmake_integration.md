---
layout: default
title: CMake
parent: "📚 Documentation"
---

# CMake integration

```cmake
set(HF_CORE_ENABLE_AT25040 ON)
include("${HF_CORE_DRIVER_EXT}/hf-at25040-driver/cmake/hf_at25040_build_settings.cmake")
```

Defines `HF_AT25040_PUBLIC_INCLUDE_DIRS` and generates `at25040_version.h`.
The hf-core handler compiles when the same flag is ON.
