// This file is part of Background Music.
//
// Background Music is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 2 of the
// License, or (at your option) any later version.
//
// Background Music is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Background Music. If not, see <http://www.gnu.org/licenses/>.

//
//  BGMPlayThroughManager.h
//  BGMApp
//
//  Copyright © 2025 Kyle Neideck
//
//  Manages multiple BGMPlayThrough instances to support per-app output device routing.
//  
//  Phase B.2: Implements actual audio routing by maintaining one playthrough per output device.
//  When an app has a custom output device UID set via the app-volumes property, its audio is
//  routed to that specific playthrough instance instead of the default one.
//
//  Currently a design document / placeholder for future implementation.
//

#ifndef BGMApp__BGMPlayThroughManager
#define BGMApp__BGMPlayThroughManager

// Local Includes
#include "BGMAudioDevice.h"
#include "BGMPlayThrough.h"

// PublicUtility Includes
#include "CAMutex.h"

// STL Includes
#include <map>
#include <memory>

// System Includes
#include <CoreFoundation/CoreFoundation.h>


#pragma clang assume_nonnull begin

//==================================================================================================
//  BGMPlayThroughManager
//
//  Phase B.2 design (not yet implemented):
//
//  This class will manage multiple BGMPlayThrough instances, one per output device. When a client
//  has a custom output device UID set (via kBGMAppVolumesKey_OutputDeviceUID), the manager will
//  route the client's audio to the appropriate playthrough instance.
//
//  The manager will:
//  1. Maintain a map of output device UIDs to BGMPlayThrough instances
//  2. Create/destroy playthroughs as needed when output devices are added/removed
//  3. Provide a method to start/stop playthrough for a specific device
//  4. Handle the case where a client has no custom output device (use default)
//
//  Currently, audio routing still happens in the driver/BGMDevice, and all audio mixes to
//  a single output. Phase B.2 would change this to filter audio per playthrough.
//
//==================================================================================================

class BGMPlayThroughManager
{
    
public:
    /*! @brief Create a manager with a default playthrough for the given output device.
        @param inInputDevice The input device (usually BGMDevice).
        @param inDefaultOutputDevice The default output device to use for clients without custom routing.
        @throws CAException on error.
     */
    BGMPlayThroughManager(BGMAudioDevice inInputDevice, BGMAudioDevice inDefaultOutputDevice);
    ~BGMPlayThroughManager() = default;
    
    // Disallow copying
    BGMPlayThroughManager(const BGMPlayThroughManager&) = delete;
    BGMPlayThroughManager& operator=(const BGMPlayThroughManager&) = delete;

public:
    /*! @brief Start playthrough to all registered output devices.
        @throws CAException on error.
     */
    void                StartPlayThrough();
    
    /*! @brief Stop playthrough to all registered output devices.
     */
    void                StopPlayThrough();
    
    /*! @brief Change the default output device and/or input device.
        @param inInputDevice If non-null, change the input device.
        @param inDefaultOutputDevice If non-null, change the default output device.
        @throws CAException on error.
     */
    void                SetDevices(const BGMAudioDevice* __nullable inInputDevice,
                                   const BGMAudioDevice* __nullable inDefaultOutputDevice);

    /*! @brief Get the default output device.
        @return The current default output device.
     */
    BGMAudioDevice      GetDefaultOutputDevice() const;

private:
    CAMutex             mMutex { "PlayThroughManager" };
    
    BGMAudioDevice      mInputDevice;
    BGMAudioDevice      mDefaultOutputDevice;
    
    // Map from output device UID to playthrough instance.
    // Currently only contains one entry for the default output device.
    // Phase B.2: Will be expanded to support multiple output devices.
    std::map<CACFString, std::unique_ptr<BGMPlayThrough>> mPlayThroughsByOutputUID;
    
};

#pragma clang assume_nonnull end

#endif /* BGMApp__BGMPlayThroughManager */
