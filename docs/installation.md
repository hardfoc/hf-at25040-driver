---
layout: default
title: Installation
parent: "📚 Documentation"
---

# Installation

Submodule under `hf-core-drivers/external/hf-at25040-driver`.

```bash
cd hf-core-drivers
git submodule add https://github.com/hardfoc/hf-at25040-driver.git external/hf-at25040-driver
```

C++17. Header-only. Include `inc/` and the CMake-generated `at25040_version.h`
directory from `cmake/hf_at25040_build_settings.cmake`.
