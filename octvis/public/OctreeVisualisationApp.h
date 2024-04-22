//
// Date       : 14/02/2024
// Project    : octvis
// Author     : -Ry
//

#ifndef OCTVIS_OCTREEVISUALISATIONAPP_H
#define OCTVIS_OCTREEVISUALISATIONAPP_H

#include "Application.h"
#include "Octree.h"

namespace octvis {

    class OctreeVisualisationApp : public Application {

      private:
        Octree<entt::entity> m_Octree;

      public:
        OctreeVisualisationApp();

      public:
        virtual void on_start() noexcept override;
        virtual void on_update() noexcept override;
        virtual void on_fixed_update() noexcept override;
        virtual void on_finish() noexcept override;
    };

} // octvis

#endif
