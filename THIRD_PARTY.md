# Source provenance

- Apple XNU **8796.121.2**, commit `c54f35ca767986246321eb901baf8f5ff7923f6a`: https://github.com/apple-oss-distributions/xnu/tree/xnu-8796.121.2
- Samsung bring-up base: https://github.com/leh0726/G986N-iOS/tree/946de0fed8e583606ca0470506c9e847f45e926f
- Original HackiOS work by OOSEX GANG: https://github.com/oosexgang/hackios-xnu-kernel and https://github.com/oosexgang/hackios-xnu-userland
- Reference port: https://github.com/CrossDarwin/macOS-ForSheng . This Ventura/SM8250 experiment is a separate adaptation.
- Standalone PAC compatibility tooling derived from UniXNU-zeta: https://github.com/none24smu/UniXNU-zeta/tree/eb489285a1d9153872f00c93ebb519a269b249b5 (BSD-2-Clause-Patent).
- libdeflate retains its COPYING file. libzbitmap identifies `GPL-2.0+ OR MIT`, used under its MIT option, in the retained source notices.

The kernel patch includes the pre-existing HackiOS/Samsung platform work as well as the Ventura adaptations; it is not all newly authored by this project. The source overlay and patch reconstruct the edited source tree on the pinned Apple base. Firmware and SDK/static-library inputs must be supplied separately.
