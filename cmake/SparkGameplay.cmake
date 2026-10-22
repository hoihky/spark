# Optional gameplay kit (combat, pickups, game flow, fog-of-war). Not part of core SparkEngine.

set(SPARK_GAMEPLAY_SOURCES
        src/spark/ecs/components/gameplay/HealthComponent.cpp
        src/spark/ecs/components/gameplay/DamageableComponent.cpp
        src/spark/ecs/components/gameplay/InteractableComponent.cpp
        src/spark/ecs/components/gameplay/PickupComponent.cpp
        src/spark/ecs/components/gameplay/GameStateComponent.cpp
        src/spark/ecs/components/gameplay/GameFlowTriggerComponent.cpp
        src/spark/ecs/components/gameplay/CombatHurtFlash3DComponent.cpp
        src/spark/ecs/components/gameplay/GameplayTeamComponent.cpp
        src/spark/ecs/components/gameplay/GameplayInventoryCollectorComponent.cpp
        src/spark/ecs/components/gameplay/DamageZone2DComponent.cpp
        src/spark/ecs/components/gameplay/FogOfWar2DComponent.cpp
        src/spark/gameplay/fog/FogOfWarExplorationMap.cpp
        src/spark/gameplay/fog/FogOfWarMaskTextureBuilder.cpp
        src/spark/gameplay/fog/FogOfWarScreenPresenter.cpp
        src/spark/gameplay/GameplayDataTable.cpp
        src/spark/scene/serialization/ComponentSnapshotHandlersGameFlow.cpp
        src/spark/scene/serialization/ComponentSnapshotHandlersGameplay.cpp
        src/spark/gameflow/GameFlowCoordinator.cpp
        src/spark/gameflow/GameFlowPersistentData.cpp
        src/spark/gameplay/Interaction2D.cpp
        src/spark/gameplay/Combat2D.cpp
        src/spark/gameplay/Combat3D.cpp
        src/spark/gameplay/GameplayDamageEvent.cpp
        src/spark/gameplay/GameplayDamageApplier.cpp
        src/spark/gameplay/GameplayDamageDeal.cpp
        src/spark/gameplay/GameplayInteractionQuery.cpp
        src/spark/gameplay/GameplayInteractionProcessor.cpp
        src/spark/gameplay/GameplayPickupDelivery.cpp
        src/spark/gameplay/GameplayTeamAffiliation.cpp
        src/spark/gameplay/GameplayTeamPolicy.cpp
        src/spark/gameplay/CameraBounds2DResolve.cpp
        src/spark/gameplay/SparkGameplayModule.cpp
        src/spark/ecs/components/animation/AnimationHitbox2DComponent.cpp
        src/spark/ecs/components/animation/AnimationMeleeHitComponent.cpp
)

function(spark_add_gameplay_library target_engine)
    if(TARGET SparkGameplay)
        return()
    endif()
    add_library(SparkGameplay STATIC ${SPARK_GAMEPLAY_SOURCES})
    # Spark may be add_subdirectory'd (CMAKE_SOURCE_DIR = parent game); use engine + repo includes.
    get_target_property(_spark_engine_includes ${target_engine} INTERFACE_INCLUDE_DIRECTORIES)
    if(_spark_engine_includes AND NOT _spark_engine_includes STREQUAL "_spark_engine_includes-NOTFOUND")
        target_include_directories(SparkGameplay PUBLIC ${_spark_engine_includes})
    endif()
    target_include_directories(SparkGameplay PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../include")
    # Do not PUBLIC-link SparkEngine here: shared SparkEngine also links SparkGameplay (see SparkExecutables.cmake).
    target_link_libraries(SparkGameplay INTERFACE ${target_engine})
    set_target_properties(SparkGameplay PROPERTIES POSITION_INDEPENDENT_CODE ON)
endfunction()
