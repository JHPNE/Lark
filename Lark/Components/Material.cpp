#include "Material.h"

namespace lark::material
{
    namespace
    {
        struct material_data
        {
            bool is_valid{false};
        };

        util::vector<material_data> material_components;
        util::vector<id::id_type> id_mapping;
        util::vector<id::generation_type> generations;
        std::deque<material_id> free_ids;

        bool exists(material_id id)
        {
            assert(id::is_valid(id));
            const id::id_type index{id::index(id)};
            assert(index < generations.size());
            return (id::is_valid(id_mapping[index]) && generations[index] == id::generation(id) &&
                    material_components[id_mapping[index]].is_valid);
        }
    } // anon namespace

    component create(init_info info, game_entity::entity entity)
    {
        assert(entity.is_valid());

        material_id id{};

        if (free_ids.size() > id::min_deleted_elements)
        {
            id = free_ids.front();
            assert(!exists(id));
            free_ids.pop_front();
            id = material_id{id::new_generation(id)};
            ++generations[id::index(id)];
        }
        else
        {
            id = material_id{(id::id_type)id_mapping.size()};
            id_mapping.emplace_back();
            generations.push_back(0);
        }
        assert(id::is_valid(id));
        const id::id_type index{(id::id_type)material_components.size()};

        material_components.emplace_back(material_data{true});
        id_mapping[id::index(id)] = index;
        return component{id};
    };

    void remove(component c)
    {
        if (!c.is_valid())
            return;
        if (!exists(c.get_id()))
            return;

        const material_id id{c.get_id()};
        const id::id_type index{id_mapping[id::index(id)]};
        const id::id_type last_index{(id::id_type)material_components.size() - 1};

        // Move last element to the removed position
        if (index != last_index)
        {
            material_components[index] = std::move(material_components[last_index]);
            // Update the id_mapping for the moved element
            const auto moved_id =
                std::find_if(id_mapping.begin(), id_mapping.end(),
                             [last_index](id::id_type mapping) { return mapping == last_index; });
            if (moved_id != id_mapping.end())
            {
                *moved_id = index;
            }
        }

        material_components.pop_back();
        id_mapping[id::index(id)] = id::invalid_id;

        if (generations[id::index(id)] < id::max_generation)
        {
            free_ids.push_back(id);
        }
    }

    void shutdown()
    {
        material_components.clear();
        id_mapping.clear();
        generations.clear();
        free_ids.clear();
    }
}