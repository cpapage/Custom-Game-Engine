#pragma once
#include "Types.h"
#include <unordered_map>
#include <typeindex>
#include <functional>

typedef long EntityID;

const string entityArray[] = { "Child Brontosaurus", "Triceratops", "Platform", "Platform2", "Platform3", "Platform4", "Platform5", "Platform6",
                                "Platform7", "Platform8", "Platform9", "Platform10", "Platform11", "Platform12", "Platform13", "Platform14", "Platform15", "Platform16",
                                "Platform17", "Platform18", "Platform19", "Platform20", "Platform21", "Platform22", "Platform23", "Platform24", "Platform25", "Platform26",
                                "Platform27", "Platform28", "Platform29", "Obstacle1", "Obstacle2", "Obstacle3", "Start", "Goal", "Life1", "Life2", "Life3", "Sign",
                                "Game Over", "Title"};

struct EntityMap
{
    std::unordered_map<string, EntityID> entities;

    EntityMap()
    {
        EntityID l = 0L;
        for(string s : entityArray)
        {
            entities[s] = l;
            l += 1;
        }
    }

    EntityID GetID(string s)
    {
        return entities[s];
    }
};

class SparseSetHolder {
public:
    // A virtual destructor, since subclasses need their destructors to run to free memory.
    virtual ~SparseSetHolder() = default;
    virtual bool Has( EntityID ) const = 0;
    virtual void Drop( EntityID ) = 0;
};

// Subclasses are templated on the component type they hold.
template< typename T > class SparseSet : public SparseSetHolder {
public:
    std::unordered_map< EntityID, T > data;
    bool Has( EntityID e ) const override { return data.count( e ) > 0; };
    void Drop( EntityID e ) override { data.erase( e ); };
};

typedef std::type_index ComponentIndex;

struct EntityComponentSystem
{
    std::unordered_map< ComponentIndex, std::unique_ptr< SparseSetHolder > > m_components;
    EntityMap emap;

    template< typename T >
    std::unordered_map< EntityID, T >&
    GetAppropriateSparseSet() {
        // Get the index for T's SparseSet
        const ComponentIndex index = std::type_index(typeid(T));
        // Create the actual sparse set if needed.
        if( m_components[ index ] == nullptr ) m_components[index] = std::make_unique< SparseSet<T> >();
        // It's safe to cast the SparseSetHolder to its subclass and return the std::unordered_map< EntityID, T > inside.
        return static_cast< SparseSet<T>& >( *m_components[ index ] ).data;
    }

    template< typename T >
    T& Get( EntityID entity ) {
        return GetAppropriateSparseSet<T>()[ entity ];
    }

    template< typename T>
    EntityID Create(EntityID e, T target)
    {
        GetAppropriateSparseSet<T>()[e] = target;
        return e;
    }

    // Drop a component from an entity.
    template< typename T >
    void Drop( EntityID e ) {
        GetAppropriateSparseSet<T>().erase( e );
    }

    // Destroy the entity by removing all components.
    void Destroy( EntityID e ) {
        for( const auto& [index, comps] : m_components ) { comps->Drop( e ); }
    }

    typedef std::function<void( EntityID )> ForEachCallback;
    template<typename EntitiesThatHaveThisComponent, typename... AndAlsoTheseComponents>
    void ForEach( const ForEachCallback& callback ) {
        // Get a vector of ComponentIndex we can use with `m_components[ index ]->Has( entity )`.
        std::vector<ComponentIndex> also{ std::type_index(typeid(AndAlsoTheseComponents))... };
        // Iterate over entities in the first container.
        // If the entity has all components in `also`, call `callback( entity )`.
        // ... your code goes here ...
        std::unordered_map<EntityID, EntitiesThatHaveThisComponent>& map = GetAppropriateSparseSet<EntitiesThatHaveThisComponent>();
        bool hasAll = true;
        for(const auto& [entity, val] : map)
        {
            for(ComponentIndex i : also)
            {
                if(!m_components[i]->Has(entity))
                {
                    hasAll = false;
                    break;
                }
            }

            if(hasAll)
                callback(entity);
        }
    }
};