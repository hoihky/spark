
    template<typename Fn>
    void ForEachGameObject(Fn&& fn) {
        entityGraph_.ForEach(Forward<Fn>(fn));
    }

    template<typename Fn>
    void ForEachGameObject(Fn&& fn) const {
        entityGraph_.ForEach(Forward<Fn>(fn));
    }

    template<typename Fn>
    void ForEachGameObject(Fn&& fn, const GameObjectQueryFilter& filter) {
        entityGraph_.ForEach(Forward<Fn>(fn), filter);
    }

    template<typename Fn>
    void ForEachGameObject(Fn&& fn, const GameObjectQueryFilter& filter) const {
        entityGraph_.ForEach(Forward<Fn>(fn), filter);
    }

    template<typename Fn>
    void ForEachActiveGameObject(Fn&& fn) {
        entityGraph_.ForEachActive(Forward<Fn>(fn));
    }

    template<typename Fn>
    void ForEachActiveGameObject(Fn&& fn) const {
        entityGraph_.ForEachActive(Forward<Fn>(fn));
    }

    template<typename... ComponentTypes, typename Fn>
    void ForEachGameObjectWithComponents(Fn&& fn, const GameObjectQueryFilter& filter = {}) {
        entityGraph_.ForEachWithComponents<ComponentTypes...>(Forward<Fn>(fn), filter);
    }

    template<typename... ComponentTypes, typename Fn>
    void ForEachGameObjectWithComponents(Fn&& fn, const GameObjectQueryFilter& filter = {}) const {
        entityGraph_.ForEachWithComponents<ComponentTypes...>(Forward<Fn>(fn), filter);
    }

    template<typename T, typename Fn>
    void ForEachComponent(Fn&& fn, const GameObjectQueryFilter& filter = {}) {
        entityGraph_.ForEachComponent<T>(Forward<Fn>(fn), filter);
    }

    template<typename T, typename Fn>
    void ForEachComponent(Fn&& fn, const GameObjectQueryFilter& filter = {}) const {
        entityGraph_.ForEachComponent<T>(Forward<Fn>(fn), filter);
    }

    template<typename... ComponentTypes>
    [[nodiscard]] GameObject* FindFirstGameObjectWithComponents(const GameObjectQueryFilter& filter = {}) {
        return entityGraph_.FindFirstWithComponents<ComponentTypes...>(filter);
    }

    template<typename... ComponentTypes>
    [[nodiscard]] GameObject* FindFirstGameObjectWithComponents(const GameObjectQueryFilter& filter = {}) const {
        return entityGraph_.FindFirstWithComponents<ComponentTypes...>(filter);
    }
