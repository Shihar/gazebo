/*
 * Copyright (C) 2015 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#ifndef _GAZEBO_PHYSICS_ODE_ODEMESHPRIVATE_HH_
#define _GAZEBO_PHYSICS_ODE_ODEMESHPRIVATE_HH_

#include "gazebo/physics/ode/ODETypes.hh"
#include "gazebo/physics/ode/ode_inc.h"
#include "gazebo/util/system.hh"

namespace gazebo
{
  namespace physics
  {
    /// \internal
    /// \brief ODEMesh private data
    class ODEMeshPrivate
    {
      /// \brief Transform matrix.
      public: dReal transform[32];

      /// \brief Transform matrix index.
      public: int transformIndex;

      /// \brief Array of vertex values.
      public: float *vertices;

      /// \brief Array of index values.
      public: int *indices;

      /// \brief ODE trimesh data.
      public: dTriMeshDataID odeData;

      /// \brief The collision id that this mesh is attached to.
      public: dGeomID collisionId;
    };
  }
}
#endif