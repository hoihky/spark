
    template<typename Fn>
    void ForEach(Fn&& fn) {
        hostWorld.GetEntityGraph().ForEach(Forward<Fn>(fn));
    }

    template<typename Fn>
    void ForEach(Fn&& fn) const {
        hostWorld.GetEntityGraph().ForEach(Forward<Fn>(fn));
    }

    template<typename Fn>
    void ForEach(Fn&& fn, const GameObjectQueryFilter& filter) {
        hostWorld.GetEntityGraph().ForEach(Forward<Fn>(fn), filter);
    }

    template<typename Fn>
    void ForEach(Fn&& fn, const GameObjectQueryFilter& filter) const {
        hostWorld.GetEntityGraph().ForEach(Forward<Fn>(fn), filter);
    }

    template<typename Fn>
    void ForEachActive(Fn&& fn) {
        hostWorld.GetEntityGraph().ForEachActive(Forward<Fn>(fn));
    }

    template<typename Fn>
    void ForEachActive(Fn&& fn) const {
        hostWorld.GetEntityGraph().ForEachActive(Forward<Fn>(fn));
    }

    template<typename... ComponentTypes, typename Fn>
    void ForEachWithComponents(Fn&& fn, const GameObjectQueryFilter& filter = {}) {
        hostWorld.GetEntityGraph().ForEachWithComponents<ComponentTypes...>(Forward<Fn>(fn), filter);
    }

    template<typename... ComponentTypes, typename Fn>
    void ForEachWithComponents(Fn&& fn, const GameObjectQueryFilter& filter = {}) const {
        hostWorld.GetEntityGraph().ForEachWithComponents<ComponentTypes...>(Forward<Fn>(fn), filter);
    }

    template<typename T, typename Fn>
    void ForEachComponent(Fn&& fn, const GameObjectQueryFilter& filter = {}) {
        hostWorld.GetEntityGraph().ForEachComponent<T>(Forward<Fn>(fn), filter);
    }

    template<typename T, typename Fn>
    void ForEachComponent(Fn&& fn, const GameObjectQueryFilter& filter = {}) const {
        hostWorld.GetEntityGraph().ForEachComponent<T>(Forward<Fn>(fn), filter);
    }

    template<typename... ComponentTypes>
    [[nodiscard]] GameObject* FindFirstWithComponents(const GameObjectQueryFilter& filter = {}) {
        return hostWorld.GetEntityGraph().FindFirstWithComponents<ComponentTypes...>(filter);
    }

    template<typename... ComponentTypes>
    [[nodiscard]] GameObject* FindFirstWithComponents(const GameObjectQueryFilter& filter = {}) const {
        return hostWorld.GetEntityGraph().FindFirstWithComponents<ComponentTypes...>(filter);
    }
