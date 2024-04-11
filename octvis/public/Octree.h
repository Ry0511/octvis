//
// Date       : 13/02/2024
// Project    : octvis
// Author     : -Ry
//

#ifndef OCTVIS_OCTREE_H
#define OCTVIS_OCTREE_H

#include "Logging.h"

#include "glm/glm.hpp"

#include <vector>
#include <unordered_set>
#include <type_traits>

namespace octvis {

    //############################################################################//
    // | COLLISION DETECTION FUNCTIONS |
    //############################################################################//

    namespace collision {

        // - NOTE -
        // This does not take orientation into account, that is, if the object is rotated then the
        // collision functions may be wrong.
        //

        inline bool sphere_intersects_sphere(
                const glm::vec3& lhs_centre, const float lhs_size,
                const glm::vec3& rhs_centre, const float rhs_size
        ) noexcept {
            return glm::distance(lhs_centre, rhs_centre) < (lhs_size + rhs_size);
        };

        inline bool sphere_intersects_cube(
                const glm::vec3& sphere_centre, const float radius,
                const glm::vec3& cube_centre, const float cube_size
        ) noexcept {

            const float size = cube_size * 0.5F;

            glm::vec3 nearest_point{
                    glm::clamp(sphere_centre.x, cube_centre.x - size, cube_centre.x + size),
                    glm::clamp(sphere_centre.y, cube_centre.y - size, cube_centre.y + size),
                    glm::clamp(sphere_centre.z, cube_centre.z - size, cube_centre.z + size)
            };

            float distance_squared = glm::dot(
                    sphere_centre - nearest_point,
                    sphere_centre - nearest_point
            );
            return distance_squared < (radius * radius);
        };

        inline bool cube_intersects_cube(
                const glm::vec3& lhs_centre, const float lhs_size,
                const glm::vec3& rhs_centre, const float rhs_size
        ) noexcept {

            const float l_size = lhs_size * 0.5F;
            const float r_size = rhs_size * 0.5F;

            glm::vec3 l_left = lhs_centre - l_size;
            glm::vec3 l_right = lhs_centre + l_size;
            glm::vec3 r_left = rhs_centre - r_size;
            glm::vec3 r_right = rhs_centre + r_size;

            return l_right.x >= r_left.x && l_left.x <= r_right.x
                   && l_right.y >= r_left.y && l_left.y <= r_right.y
                   && l_right.z >= r_left.z && l_left.z <= r_right.z;
        };

        inline bool point_intersects_cube(
                const glm::vec3& point,
                const glm::vec3& centre,
                float size
        ) noexcept {

            const float half_size = size * 0.5F;
            glm::vec3 left = centre - half_size;
            glm::vec3 right = centre + half_size;

            return point.x >= left.x && point.x <= right.x
                   && point.y >= left.y && point.y <= right.y
                   && point.z >= left.z && point.z <= right.z;
        }

        inline bool box_intersects_sphere(
                const glm::vec3& min, const glm::vec3& max,
                const glm::vec3& centre, const float radius
        ) noexcept {
            float dist_squared = 0.0F;
            for (unsigned int i = 0; i < 3; i++) {
                float v = centre[i];

                if (v < min[i]) {
                    float delta = min[i] - v;
                    dist_squared += delta * delta;
                } else if (v > max[i]) {
                    float delta = v - max[i];
                    dist_squared += delta * delta;
                }
            }
            return dist_squared <= (radius * radius);
        }

        inline bool box_intersects_box(
                const glm::vec3& min0, const glm::vec3& max0,
                const glm::vec3& min1, const glm::vec3& max1
        ) noexcept {
            if (max0.x < min1.x || min0.x > max1.x) return false;
            if (max0.y < min1.y || min0.y > max1.y) return false;
            if (max0.z < min1.z || min0.z > max1.z) return false;
            return true;
        }
    }

    //############################################################################//
    // | OCTREE |
    //############################################################################//

    template<class T>
    class Octree {

        // TODO: Implement
        // TODO: Create a Test Application to visualise the Tree
        // TODO: Implement Basic Collision Detection & Resolution

      public:
        struct Node { // @off
            Node*                 parent;
            glm::vec3             centre;
            float                 size;
            int                   depth;
            std::vector<Node>     children;
            std::unordered_set<T> data;

          public:
            inline Node(
                    const glm::vec3& centre_, const float size_
            ) : centre(centre_), size(size_), depth(), children(), data() {
                // Important we don't initialise here because otherwise it will infinitely allocate.
                children.reserve(CHILD_COUNT);
            };

          public:
            inline bool is_inside(const glm::vec3& centre_) const noexcept {
                return collision::point_intersects_cube(centre_, centre, size * 2.0F);
            }

            inline bool is_inside(const glm::vec3& centre_, float size_) const noexcept {
                return collision::cube_intersects_cube(centre_, size_, centre, size * 2.0F);
            }

            template<class Function>
            inline void for_each(Function fn) {

                struct {
                    void recurse(Function fn, Node* begin, Node* end) {
                        for (Node* it = begin; it != end; ++it) {
                            for (auto&& elem : it->data) {
                                std::invoke(fn, elem);
                            }
                            if (it->children.empty()) continue;
                            recurse(
                                    fn,
                                    &it->children[0],
                                    &it->children[it->children.size()-1]
                            );
                        }
                    }
                } impl;

                if (children.empty()) {
                    for (auto&& elem : data) {
                        std::invoke(fn, elem);
                    }
                    return;
                }

                impl.recurse(
                        fn,
                        &children[0],
                        &children[children.size() - 1]
                );
            }

          public:
            inline Node& operator[](size_t index) {
                return children[index];
            }
            inline const Node& operator[](size_t index) const {
                return children[index];
            }
            inline void clear() {
                data.clear();
            }
            inline void divide(const glm::vec3& centre_, const float size_) {

                centre = centre_;
                size = size_;
                children.clear();

                const float half_size = size * 0.5F;

                for (int i = 0; i < CHILD_COUNT; ++i) {
                    glm::vec3 child_centre = centre;
                    child_centre.x += (i & OCTANT_FLAG_X) ? half_size : -half_size;
                    child_centre.y += (i & OCTANT_FLAG_Y) ? half_size : -half_size;
                    child_centre.z += (i & OCTANT_FLAG_Z) ? half_size : -half_size;
                    children.emplace_back(child_centre, half_size);
                }
            }
        }; // @on

      public: // @off
        static constexpr int CHILD_COUNT   = 8;
        static constexpr int MIN_DEPTH     = 1; // 8    Nodes; from f x = sum [8 ^ x | x <- [0..x]]
        static constexpr int MAX_DEPTH     = 4; // 4681 Nodes
        static constexpr int DEFAULT_DEPTH = 2;

        static constexpr int OCTANT_FLAG_X = 1;
        static constexpr int OCTANT_FLAG_Y = 2;
        static constexpr int OCTANT_FLAG_Z = 4;
        // @on

      private:
        Node m_Root;
        int m_Depth;

      public:
        Octree(const glm::vec3& centre, float size, int depth = DEFAULT_DEPTH);

        ~Octree() = default;

      public:
        Octree(const Octree<T>&) = default;

        Octree(Octree<T>&&) = default;

        Octree<T>& operator =(const Octree<T>&) = default;

        Octree<T>& operator =(Octree<T>&&) = default;

      public:
        void rebuild(const glm::vec3& centre, const float size, int depth = DEFAULT_DEPTH);

      public:
        inline bool insert(const T& elem, const glm::vec3& point, float size) {
            Node* node = search(
                    [&point, size](Node& node) {
                        return collision::box_intersects_box(
                                (point - size),
                                (point + size),
                                (node.centre - node.size),
                                (node.centre + node.size)
                        );
                    }
            );
            if (node == nullptr) return false;
            node->children.push_back(elem);
            return true;
        }

        inline bool insert(const T& elem, const glm::vec3& min, const glm::vec3& max) {
            Node* node = search(
                    [&elem, &min, &max](Node& node) {
                        if (!collision::box_intersects_box(
                                min, max,
                                node.centre - node.size,
                                node.centre + node.size
                        )) {
                            return false;
                        }

                        node.data.insert(elem);
                        return true;
                    }
            );
            return node != nullptr;
        }

      public:
        template<class Function>
        inline void for_each(Function&& fn, bool skip_root = false) noexcept {
            std::vector<Node>& children = m_Root.children;
            if (children.empty()) return;

            struct {
                void recurse(
                        Function&& fn,
                        Node* begin,
                        Node* end,
                        int depth,
                        int max_depth
                ) const noexcept {

                    if (depth >= max_depth) return;

                    for (Node* it = begin; it != end; ++it) {
                        if (!it->children.empty()) {
                            std::vector<Node>& children = it->children;
                            recurse(
                                    std::forward<Function>(fn),
                                    children.data(),
                                    children.data() + CHILD_COUNT,
                                    depth + 1, max_depth
                            );
                        }
                        std::invoke(std::forward<Function>(fn), *it);
                    }
                }
            } impl;

            if (!skip_root) {
                std::invoke(std::forward<Function>(fn), m_Root);
            }

            impl.recurse(
                    std::forward<Function>(fn),
                    children.data(),
                    children.data() + CHILD_COUNT,
                    0,
                    m_Depth
            );

        }

        template<class Predicate>
        inline Node* search(Predicate fn, int max_depth = -1) noexcept {

            struct {
                inline Node* search(
                        Predicate fn,
                        Node* begin,
                        Node* end,
                        int depth,
                        int max_depth,
                        Node* found = nullptr
                ) const noexcept {

                    if (depth >= max_depth) return found;

                    for (Node* it = begin; it != end; ++it) {

                        Node& node = *it;

                        if (!std::invoke(fn, node)) continue;
                        if (node.children.empty()) return &node;

                        return search(
                                fn,
                                &node.children[0],
                                &node.children[node.children.size() - 1],
                                depth + 1,
                                max_depth,
                                &node
                        );

                    }

                    return found;
                }
            } impl;

            return impl.search(
                    fn,
                    &m_Root.children[0],
                    &m_Root.children[m_Root.children.size() - 1],
                    0,
                    (max_depth == -1) ? m_Depth : max_depth
            );
        }

      public:
    };

    //############################################################################//
    // | IMPLEMENTATIONS |
    //############################################################################//

    template<class T>
    Octree<T>::Octree(
            const glm::vec3& centre,
            float size,
            int depth
    ) : m_Root(centre, size), m_Depth(depth) {
        rebuild(centre, size, std::clamp(depth, MIN_DEPTH, MAX_DEPTH));
    }

    template<class T>
    void Octree<T>::rebuild(const glm::vec3& centre, const float size, int depth) {

        struct {
            void recurse(Node* begin, Node* end, int depth, int max_depth) noexcept {
                for (Node* it = begin; it != end; ++it) {
                    it->children.clear();
                    it->depth = depth;

                    if (depth + 1 < max_depth) {
                        it->divide(it->centre, it->size);
                        Node* data = it->children.data();
                        recurse(data, data + CHILD_COUNT, depth + 1, max_depth);
                    }
                }
            };
        } impl;

        // Initialise Root and then walk the tree
        m_Root.centre = centre;
        m_Root.size = size;
        m_Root.parent = nullptr;
        m_Depth = depth;

        // Initialise Root & Initialise Children
        m_Root.children.clear();
        m_Root.divide(m_Root.centre, m_Root.size);
        Node* data = m_Root.children.data();
        impl.recurse(data, data + CHILD_COUNT, 0, m_Depth);

        for_each(
                [](Node& node) {
                    for (Node& child : node.children) {
                        child.parent = &node;
                    }
                }
        );

    }


} // octvis

#endif
