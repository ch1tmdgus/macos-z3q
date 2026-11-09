# Standalone Mach-O research helpers

These UniXNU-zeta-derived tools lower unsupported pointer-authentication instructions for the experimental non-PAC environment. Dependencies include Python, NumPy and Capstone. Retain the BSD-2-Clause-Patent license.

These files do **not** provide an end-to-end Ventura dyld shared-cache/root-image builder. The separate local cache adaptation and system-resource assembly still require additional preparation; see ../../docs/BUILD.md. Lowering authentication changes the security model.
