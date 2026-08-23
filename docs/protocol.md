---
layout: default
title: Protocol
parent: "📚 Documentation"
---

# Protocol

| Opcode | Value | Notes |
|--------|-------|-------|
| WREN | `0x06` | Write enable |
| WRDI | `0x04` | Write disable |
| RDSR | `0x05` | Status |
| WRSR | `0x01` | Status write |
| READ | `0x03` | A8 in opcode bit 3 |
| WRITE | `0x02` | A8 in opcode bit 3; 8-byte page |

Status: WIP `0x01`, WEL `0x02`, BP0/BP1 `0x04`/`0x08`.

`Probe()` requires two matching RDSR bytes that are not `0xFF`, not the `0x05`
echo, and have WIP clear.
