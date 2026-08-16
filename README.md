# Hexic HD - PC Native Recompilation
# Xbox 360 XBLA title Hexic HD (Title ID: 584107D1) statically recompiled to a native x86-64 PC executable using the ReXGlue SDK.

No emulator. No interpreter. No JIT. The original PowerPC code is converted to C++ at build time and compiled natively with Clang.

Status
Feature	Status
Boot & main menu	Working
Gameplay	Working
Rendering (D3D12)	Working (1280x720)
Input (keyboard/mouse)	Working
Input (gamepad)	Working (SDL3)
Audio	Not working (see Audio)
Achievements	Loaded (12 entries), display not tested
Localization	Working (fallback copies created)
Requirements
Build Tools
Tool	Minimum Version	Tested With
CMake	3.25+	4.4.2
Clang/LLVM	20+	22.1.8
Ninja	1.10+	1.13.2
Python	3.8+	3.13.14
Visual Studio 2022	17.0+ (Windows SDK + D3D12)	VS 18 Community
Runtime
Windows 10/11 with DirectX 12 support
GPU with D3D12 feature level 11_0+
Game assets extracted from the original Hexic HD XBLA package
Source Dependencies
ReXGlue SDK v0.9.0+ (cloned with --recursive for submodules)
360tools (optional, for extraction utilities and templates)
Building
1. Clone and prepare the ReXGlue SDK
git clone --recursive https://github.com/rexglue/rexglue-sdk.git
Windows symlink fix (required): Git symlinks in thirdparty/libmspack/cabextract/mspack/ are stored as text files on Windows. Replace them with actual copies from thirdparty/libmspack/libmspack/mspack/:

$symlinkDir = "rexglue-sdk/thirdparty/libmspack/cabextract/mspack"
foreach ($f in Get-ChildItem $symlinkDir -File) {
    $target = Join-Path $symlinkDir (Get-Content $f.FullName -Raw).Trim()
    $resolved = [System.IO.Path]::GetFullPath($target)
    if (Test-Path $resolved) { Copy-Item $resolved $f.FullName -Force }
}
2. Build the ReXGlue SDK
# Ensure Clang is on PATH
cmake --preset win-amd64 -DREXGLUE_ENABLE_TRACY=OFF
cmake --build out/build/win-amd64 --config Release
Note: Tracy is disabled to avoid a fileno linker error with recent Clang/MSVC combinations. An oldnames library addition is also needed in src/system/CMakeLists.txt (see SDK Patches).

3. Extract game assets
Place the extracted Hexic HD content (from the XBLA package) in a directory. You should have at minimum:

default.xex
HexicDeluxe.swf, HexicHDLoading.swf
g.xml, KeyMap_HXIC.xml
.wma audio files, .png textures, .ttf fonts
Create localized copies for the files the game requests with an en_ prefix:

Copy-Item HexicHDLoading.swf en_HexicHDLoading.swf
Copy-Item HexicDeluxe.swf en_HexicDeluxe.swf
Copy-Item g.xml en_g.xml
Copy-Item KeyMap_HXIC.xml en_KeyMap_HXIC.xml
4. Generate recompiled code
# From the hexic-hd-pc project root
path/to/rexglue codegen
This converts ~7500 PowerPC functions to C++ source files (~35 MB total).

5. Build the game
cmake --preset win-amd64-release -DREXSDK_DIR="path/to/rexglue-sdk" -DREXGLUE_ENABLE_TRACY=OFF
cmake --build out/build/win-amd64-release
6. Run
./out/build/win-amd64-release/hexic_hd.exe \
    --game_data_root "path/to/extracted/assets" \
    --gpu_plugin xenos \
    --protect_zero false \
    --mnk_mode true
Controls
Key	Xbox Button
Space	A
Shift	B
R	X
E	Y
W / A / S / D	Left Stick
Arrow Keys	D-pad
Q	LB
F	RB
Left Click	RT
Right Click	LT
Escape	Start
Tab	Back
Gamepad input is also supported natively through SDL3 (no --mnk_mode needed).

SDK Patches
The following patches were applied to the ReXGlue SDK to support Hexic HD's non-standard binary layout and runtime requirements.

1. High base address support (src/system/xmemory.cpp)
Hexic HD's XEX uses base address 0x92000000 (standard XBLA is 0x82000000). The SDK's InitializeFunctionTable hardcoded heaps_.v80000000 for the function table allocation, which doesn't cover the 0x90000000-0x9FFFFFFF range.

Fix: Use LookupHeap(table_base) instead of the hardcoded heap, and align to the heap's native page size:

- if (!heaps_.v80000000.AllocFixed(
-         table_base, table_size, 0x10000,
+ BaseHeap* table_heap = LookupHeap(table_base);
+ if (!table_heap) {
+   REXSYS_ERROR("No heap covers function table base {:08X}", table_base);
+   return false;
+ }
+ if (!table_heap->AllocFixed(
+         table_base, table_size, table_heap->page_size(),
2. POSIX symbol linking (src/system/CMakeLists.txt)
Clang 22+ with MSVC runtime requires oldnames.lib for POSIX symbols (isatty, itoa) used by FFmpeg and SDL3.

- target_link_libraries(rexruntime PRIVATE ws2_32)
+ target_link_libraries(rexruntime PRIVATE ws2_32 oldnames)
3. Tracy profiler disabled
Tracy's fileno symbol is unavailable in recent Clang/MSVC. Disabled via -DREXGLUE_ENABLE_TRACY=OFF.

Project-Level Fixes
Floating point exception handler (src/fp_fix.cpp)
Xbox 360 PowerPC masks all floating point exceptions by default. x86-64 raises STATUS_FLOAT_INEXACT_RESULT (0xC000008F) for operations that are silently handled on PPC. A Vectored Exception Handler catches FP exceptions and masks them via _controlfp, allowing execution to continue.

Dispatch tolerance (src/dispatch_tolerance.cpp)
From 360tools/templates. Overrides the SDK's indirect call resolver to return a no-op trampoline instead of fataling on unregistered function targets. Required because some indirect call targets (vtable slots, function pointers) aren't detected by static analysis.

Crash diagnostics (src/crash_diag.cpp)
From 360tools/templates. Installs a low-priority VEH that logs unhandled access violations with symbolized stack traces to crash_diag.log. Build with RelWithDebInfo for useful output.

Function hints (hexic_hd_manifest.toml)
Eight function entry points that codegen's static analysis couldn't detect automatically:

[entrypoint.functions]
0x92194DC0 = {}
0x9224E768 = {}
0x92145A48 = {}
0x92194B90 = {}
0x92053598 = {}
0x9204E188 = {}
0x9204E170 = {}
0x9204E1D0 = {}
Audio Implementation Guide
Audio is not yet functional. The infrastructure is partially in place (the XMA decoder thread and audio worker thread both start successfully, and SDL audio frames are being submitted), but no audible output is produced. Here's what needs to happen:

Current state
The runtime initializes:

XMA Decoder thread (hardware XMA decompression emulation)
Audio Worker thread (dispatches audio callbacks)
SDL audio driver (receives PCM frames via SubmitFrame)
The game's audio callback (0x92195CD8) is being invoked and frames are queued, but the audio data is likely silence or malformed because the XMA decode pipeline isn't fully processing the .wma containers.

What needs to be implemented
WMA container parsing: Hexic HD ships audio as .wma (Windows Media Audio) files, not raw XMA streams. The ReXGlue runtime's XMA decoder expects raw XMA packets delivered via MMIO registers at 0x7FEA0000. The .wma files need to be demuxed (strip the ASF container) to feed raw audio packets to the XMA decoder. The SDK uses FFmpeg internally (thirdparty/FFmpeg) which can handle WMA demuxing.

XMA MMIO register emulation: Verify that the guest code's writes to the XMA hardware registers (XMAContextArray at physical memory) are being intercepted and translated correctly. The XMA decoder thread watches these registers for context updates. Key registers:

Context array base (set via XAudioRegisterRenderDriverClient)
Packet buffer pointers
Loop/state fields
Audio callback chain: The game registers an audio render callback via XAudioRegisterRenderDriverClient. The callback is invoked by the audio worker, fills a buffer with mixed samples, and XAudioSubmitRenderDriverFrame pushes it to the SDL output. Verify the callback receives valid context data (the 0x30033000 argument in the logs) and that the XMA-decoded PCM is making it into the mix buffer.

Sample format conversion: Xbox 360 XMA outputs interleaved 16-bit PCM at 48kHz. SDL's audio subsystem may be configured for a different format. Check SDLAudioDriver initialization for sample rate and channel count matching.

Implementation approach
The most practical approach is to override OnPostSetup() in HexicHdApp to:

Scan the game root for .wma files
Use FFmpeg's avformat_open_input / avcodec_send_packet to decode them to PCM
Cache the decoded PCM buffers
Hook the game's file-open paths for .wma to return the decoded PCM, or patch the XMA context setup to point at pre-decoded buffers
Alternatively, study how other recompiled titles handle audio (e.g., Crazy Taxi XBLA which lists "XMA WIP" status) for patterns that can be adapted.

Reference files
rexglue-sdk/src/audio/ - XMA decoder, audio system, SDL driver
rexglue-sdk/src/system/kernel/xam/xam_audio.cpp - XAudio API stubs
rexglue-sdk/include/rex/audio/ - Audio subsystem headers
Binary Details
Property	Value
Title ID	584107D1
Base Address	0x92000000 (non-standard)
Image Size	2.6 MB (0x2A5000)
Code Range	0x9204E000 - 0x9224F91C
Entry Point	0x00000004
Imports	xboxkrnl.exe (142), xam.xex (33)
Recompiled Functions	~7500
Generated C++	~35 MB (18 source files)
License
This project is for educational and preservation purposes only. It does not include any copyrighted game assets. You must own a legitimate copy of Hexic HD to use this.

The recompilation tooling and runtime code are subject to their respective licenses:

ReXGlue SDK - See its LICENSE
360tools - MIT
Acknowledgments
ReXGlue - The recompiler and runtime
360tools - Toolkit and templates
Xenia - GPU backend foundation
XenonRecomp - Pioneered the static recompilation approach
