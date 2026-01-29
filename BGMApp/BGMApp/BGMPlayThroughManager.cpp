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
//  BGMPlayThroughManager.cpp
//  BGMApp
//
//  Copyright © 2025 Kyle Neideck
//

// Self Include
#include "BGMPlayThroughManager.h"

// PublicUtility Includes
#include "CAException.h"
#include "CADebugMacros.h"


BGMPlayThroughManager::BGMPlayThroughManager(BGMAudioDevice inInputDevice, 
                                             BGMAudioDevice inDefaultOutputDevice)
    : mInputDevice(inInputDevice), mDefaultOutputDevice(inDefaultOutputDevice)
{
    CAMutex::Locker locker(mMutex);
    
    // Create the default playthrough instance
    CACFString defaultUID = mDefaultOutputDevice.GetUID();
    mPlayThroughsByOutputUID[defaultUID] = 
        std::make_unique<BGMPlayThrough>(mInputDevice, mDefaultOutputDevice);
}

void BGMPlayThroughManager::StartPlayThrough()
{
    CAMutex::Locker locker(mMutex);
    
    // Start all playthrough instances
    for (auto& entry : mPlayThroughsByOutputUID) {
        if (entry.second) {
            CATry {
                entry.second->Start();
            }
            CACatch {
                LogError("BGMPlayThroughManager::StartPlayThrough: Failed to start playthrough for device %s",
                         CFStringGetCStringPtr(entry.first.GetCFString(), kCFStringEncodingUTF8));
                throw;
            }
        }
    }
}

void BGMPlayThroughManager::StopPlayThrough()
{
    CAMutex::Locker locker(mMutex);
    
    // Stop all playthrough instances
    for (auto& entry : mPlayThroughsByOutputUID) {
        if (entry.second) {
            CATry {
                entry.second->Stop();
            }
            CACatch {
                DebugMsg("BGMPlayThroughManager::StopPlayThrough: Failed to stop playthrough");
            }
        }
    }
}

void BGMPlayThroughManager::SetDevices(const BGMAudioDevice* __nullable inInputDevice,
                                       const BGMAudioDevice* __nullable inDefaultOutputDevice)
{
    CAMutex::Locker locker(mMutex);
    
    bool shouldRestart = false;
    
    // Check if playthrough is currently running
    if (!mPlayThroughsByOutputUID.empty() && mPlayThroughsByOutputUID.begin()->second) {
        // We can't easily check if it's running without exposing more API, so we'll just
        // assume it might be and let SetDevices on each playthrough handle it.
    }
    
    // Update default output device and create new playthrough if needed
    if (inDefaultOutputDevice != nullptr) {
        mDefaultOutputDevice = *inDefaultOutputDevice;
        
        // For now, we only support the default output device
        // Phase B.2: This will handle creating multiple playthroughs
        CACFString newUID = mDefaultOutputDevice.GetUID();
        
        // Update the existing playthrough with the new output device
        if (!mPlayThroughsByOutputUID.empty()) {
            auto& defaultPlayThrough = mPlayThroughsByOutputUID.begin()->second;
            if (defaultPlayThrough) {
                defaultPlayThrough->SetDevices(inInputDevice, inDefaultOutputDevice);
            }
        }
    }
    
    // Update input device if provided
    if (inInputDevice != nullptr) {
        mInputDevice = *inInputDevice;
        
        for (auto& entry : mPlayThroughsByOutputUID) {
            if (entry.second) {
                entry.second->SetDevices(inInputDevice, nullptr);
            }
        }
    }
}

BGMAudioDevice BGMPlayThroughManager::GetDefaultOutputDevice() const
{
    CAMutex::Locker locker(mMutex);
    return mDefaultOutputDevice;
}
