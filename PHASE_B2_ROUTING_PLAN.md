# Phase B.2: Per-App Audio Routing Implementation Plan

## Overview
Phase B.2 implements **actual audio routing** — making audio from apps with custom output device UIDs actually go to those devices, not just mixing everything together.

## Current State (Phase A + B Scaffolding)
- ✅ App-side UI: Per-app output device selector in BGMApp menu
- ✅ Protocol: App sends output device UID via `kBGMAppVolumesKey_OutputDeviceUID` in app-volumes CFArray
- ✅ Driver storage: `BGM_Client::mOutputDeviceUID` field stores per-client routing hint
- ✅ Driver parsing: `BGM_Clients::SetClientsRelativeVolumes()` parses `outuid` and sets `mOutputDeviceUID`
- ✅ Placeholder manager: `BGMPlayThroughManager` created as skeleton for future routing

## Problem to Solve
Currently, audio from **all clients** mixes into a single `mLoopbackRingBuffer` in `BGM_Device` and goes out one output device. Even if clients have custom `mOutputDeviceUID` set, that routing information is not used.

## Two Possible Solutions

### Solution A: App-Side Routing (Recommended for Phase B.2)
**Concept**: Route audio at the app level using multiple `BGMPlayThrough` instances.

**How it works**:
1. Maintain one `BGMPlayThrough` per output device in `BGMPlayThroughManager`
2. For each client I/O operation that has a custom output device UID:
   - Instead of writing to the default playthrough's ring buffer
   - Write to the playthrough's ring buffer corresponding to the client's `mOutputDeviceUID`
3. Each playthrough reads from its dedicated ring buffer and writes to its output device

**Pros**:
- Non-invasive to the driver (driver remains mostly unchanged)
- Easier to test incrementally
- Can be implemented entirely on the app side
- Multiple output devices already technically supported

**Cons**:
- Requires app-side filtering of client audio into per-device buffers
- Higher CPU usage (multiple IOProcs running)
- Requires coordination between app and driver

**Implementation steps**:
1. Modify `BGMBackgroundMusicDevice` to accept multiple output ring buffers (one per device UID)
2. Extend `BGM_Device::DoIOOperation()` to route client audio to the appropriate ring buffer based on `GetClientOutputDeviceUIDRT()`
3. Implement multi-playthrough in `BGMPlayThroughManager`:
   - Create playthrough on-demand when a client requests a new output device
   - Destroy playthrough when no clients use it anymore
4. Dynamically create/destroy playthroughs as clients request new output devices
5. Test with multiple apps routing to different devices

### Solution B: Driver-Side Routing (Advanced, not recommended for Phase B.2)
**Concept**: Route audio inside the driver using multiple output mix buses.

**Pros**:
- Cleaner architecture
- More efficient (single IOProc per app)

**Cons**:
- Complex driver changes
- Affects real-time critical path
- Requires careful synchronization
- Harder to debug and test

## Recommended Approach: App-Side (Solution A)

### Key Components to Implement

#### 1. Multiple Ring Buffers in BGMBackgroundMusicDevice
Currently `BGM_Device` has a single `mLoopbackRingBuffer`. Phase B.2 needs:
- Map of output device UID → ring buffer
- Fallback ring buffer for clients without custom routing

#### 2. Routing Logic in BGM_Device::DoIOOperation()
In `kAudioServerPlugInIOOperationProcessOutput`:
```cpp
case kAudioServerPlugInIOOperationProcessOutput:
    CACFString outputUID = GetClientOutputDeviceUIDRT(inClientID);
    
    if (outputUID.IsValid()) {
        // Route to device-specific ring buffer
        WriteOutputDataToDevice(outputUID, inIOBufferFrameSize, ioMainBuffer);
    } else {
        // Route to default ring buffer (existing behavior)
        WriteOutputData(inIOBufferFrameSize, inIOCycleInfo.mOutputTime.mSampleTime, ioMainBuffer);
    }
```

#### 3. BGMPlayThroughManager Expansion
Extend `BGMPlayThroughManager` to:
- Dynamically create/destroy playthroughs per output device
- Manage lifecycle of playthrough instances
- Synchronize with device enumeration changes
- Handle clients switching output devices

#### 4. Integration with BGMAudioDeviceManager
Modify `BGMAudioDeviceManager` to:
- Replace single `playThrough` with `BGMPlayThroughManager`
- Pass app volume updates (including output UID) to driver
- Handle device enumeration for per-app routing UI

### Testing Strategy

1. **Unit tests** (can run on any platform):
   - Test `BGMPlayThroughManager` create/destroy/start/stop
   - Mock input/output devices and verify routing

2. **Integration tests** (macOS only):
   - Create test apps that output to different devices
   - Verify audio reaches correct output devices
   - Test dynamic device/app changes

3. **Manual testing**:
   - Play audio from multiple apps
   - Set each app to different output device via menu
   - Verify audio routes correctly
   - Check CPU usage doesn't spike

### Timeline & Effort Estimate
- **Phase B.2a** (2-3 days): Implement ring buffer routing in driver + test
- **Phase B.2b** (2-3 days): Expand BGMPlayThroughManager + integration
- **Phase B.2c** (1-2 days): Test & refinement on macOS

## Fallback / Minimal Viable Routing
If full Phase B.2 is too complex:
1. Keep current single-playthrough architecture
2. Add logging when client has custom output device UID set
3. Document design as "Phase B scaffolding" for future contributors
4. Mark as "feature not yet implemented" in UI

## Future Enhancements
- Persistence of output device mappings in UserDefaults
- UI to auto-route by app bundle ID patterns
- Performance optimization via driver-side routing (Solution B)
- Aggregate device support when CoreAudio adds control support
