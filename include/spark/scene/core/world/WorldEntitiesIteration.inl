
    template<typename Fn>
    void ForEach(Fn&& fn) {
        world_.GetEntityGraph().ForEach(Forward<Fn>(fn));
    }

    template<typename Fn>
    void ForEach(Fn&& fn) const {
        world_.GetEntityGraph().ForEach(Forward<Fn>(fn));
    }

    template<typename Fn>
    void ForEach(Fn&& fn, const GameObjectQueryFilter& filter) {
        world_.GetEntityGraph().ForEach(Forward<Fn>(fn), filter);
    }

    template<typename Fn>
    void ForEach(Fn&& fn, const GameObjectQueryFilter& filter) const {
        world_.GetEntityGraph().ForEach(Forward<Fn>(fn), filter);
    }

    template<typename Fn>
    void ForEachActive(Fn&& fn) {
        world_.GetEntityGraph().ForEachActive(Forward<Fn>(fn));
    }

    template<typename Fn>
    void ForEachActive(Fn&& fn) const {
        world_.GetEntityGraph().ForEachActive(Forward<Fn>(fn));
    }

    template<typename... ComponentTypes, typename Fn>
    void ForEachWithComponents(Fn&& fn, const GameObjectQueryFilter& filter = {}) {
        world_.GetEntityGraph().ForEachWithComponents<ComponentTypes...>(Forward<Fn>(fn), filter);
    }

    template<typename... ComponentTypes, typename Fn>
    void ForEachWithComponents(Fn&& fn, const GameObjectQueryFilter& filter = {}) const {
        world_.GetEntityGraph().ForEachWithComponents<ComponentTypes...>(Forward<Fn>(fn), filter);
    }

    template<typename T, typename Fn>
    void ForEachComponent(Fn&& fn, const GameObjectQueryFilter& filter = {}) {
        world_.GetEntityGraph().ForEachComponent<T>(Forward<Fn>(fn), filter);
    }

    template<typename T, typename Fn>
    void ForEachComponent(Fn&& fn, const GameObjectQueryFilter& filter = {}) const {
        world_.GetEntityGraph().ForEachComponent<T>(Forward<Fn>(fn), filter);
    }

    template<typename... ComponentTypes>
    [[nodiscard]] GameObject* FindFirstWithComponents(const GameObjectQueryFilter& filter = {}) {
        return world_.GetEntityGraph().FindFirstWithComponents<ComponentTypes...>(filter);
    }

    template<typename... ComponentTypes>
    [[nodiscard]] GameObject* FindFirstWithComponents(const GameObjectQueryFilter& filter = {}) const {
        return world_.GetEntityGraph().FindFirstWithComponents<ComponentTypes...>(filter);
    }
