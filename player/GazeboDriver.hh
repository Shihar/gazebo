/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003  
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* Desc: Gazebo Driver for Player
 * Author: Nate Koenig
 * Date: 2 March 2006
 * CVS: $Id$
 */

#ifndef GAZEBODRIVER_HH
#define GAZEBODRIVER_HH

#include <unistd.h>
#include <string.h>

#include <libplayercore/playercore.h>

#include "GazeboClient.hh"


namespace libgazebo
{

/// \addtogroup player
/// \brief Gazebo player driver
/// \{

// Forward declarations
class GazeboInterface;

/// \brief Gazebo player driver
class GazeboDriver : public Driver
{
  /// \brief Constructor 
  public: GazeboDriver(ConfigFile* cf, int section);

  /// \brief Destructor 
  public: virtual ~GazeboDriver();

  /// \brief Set up the device.  Return 0 if things go well, and -1 otherwise.
  public: virtual int Setup();

  /// \brief Shutdown the device
  public: virtual int Shutdown();

  /// \brief Process all messages for this driver. 
  public: virtual int ProcessMessage(QueuePointer &respQueue, 
                                     player_msghdr * hdr, 
                                     void * data);

  /// \brief Subscribe an device to this driver
  public: virtual int Subscribe(player_devaddr_t addr);

  /// \brief Remove a device from this driver
  public: virtual int Unsubscribe(player_devaddr_t addr);

  /// \brief The server thread calls this method frequently. We use it to check  for new commands and configs
  private: virtual void Update();

  /// \brief Helper function to load all devices on startup
  private: int LoadDevices(ConfigFile* cf, int section);

  /// \brief Find a device according to a player_devaddr
  private: GazeboInterface *LookupDevice(player_devaddr_t addr);

  /// Array of interfaces associated with this driver
  protected: GazeboInterface **devices;

  /// \brief Number of devices
  protected: int deviceCount;

  /// \brief Max device count
  protected: int deviceMaxCount;
};

/// \}


}
#endif
