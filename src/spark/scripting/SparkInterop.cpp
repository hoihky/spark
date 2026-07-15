#include "spark/scripting/SparkInterop.h"

#include "spark/audio/SoundSubsystem.hpp"
#include "spark/ecs/GameComponent.hpp"
#include "spark/ecs/GameObject.hpp"
#include "spark/engine/FrameTiming.hpp"
#include "spark/engine/IEngineContext.hpp"
#include "spark/engine/IInput.hpp"
#include "spark/engine/IRenderFrame.hpp"
#include "spark/engine/SceneRenderParams.hpp"
#include "spark/scene/GameWorld.hpp"
#include "spark/scene/Scene.hpp"

#include <cstring>

namespace {

Spark::FrameTiming ToCpp(const SparkFrameTiming& t) {
    return Spark::FrameTiming{
            .deltaTimeSeconds = t.deltaTimeSeconds,
            .totalTimeSeconds = t.totalTimeSeconds,
            .frameIndex = t.frameIndex,
    };
}

Spark::ComponentKind ToCpp(SparkComponentKind k) {
    return static_cast<Spark::ComponentKind>(static_cast<std::uint32_t>(k));
}

}  // namespace

extern "C" {

void spark_context_get_framebuffer_size(
        const SparkEngineContext* context,
        int* outWidth,
        int* outHeight) {
    if (context == nullptr || outWidth == nullptr || outHeight == nullptr) {
        return;
    }
    const auto* ctx = reinterpret_cast<const Spark::IEngineContext*>(context);
    ctx->GetFramebufferSize(*outWidth, *outHeight);
}

SparkInput* spark_context_get_input(SparkEngineContext* context) {
    if (context == nullptr) {
        return nullptr;
    }
    auto* ctx = reinterpret_cast<Spark::IEngineContext*>(context);
    return reinterpret_cast<SparkInput*>(&ctx->GetInput());
}

SparkScene* spark_context_try_get_scene(SparkEngineContext* context) {
    if (context == nullptr) {
        return nullptr;
    }
    auto* ctx = reinterpret_cast<Spark::IEngineContext*>(context);
    Spark::Scene* scene = ctx->TryGetScene();
    return reinterpret_cast<SparkScene*>(scene);
}

void spark_context_set_scene_render_params(
        SparkEngineContext* context,
        const void* params,
        std::uint32_t paramsByteSize) {
    if (context == nullptr || params == nullptr) {
        return;
    }
    if (paramsByteSize != sizeof(Spark::SceneRenderParams)) {
        return;
    }
    auto* ctx = reinterpret_cast<Spark::IEngineContext*>(context);
    const auto* p = static_cast<const Spark::SceneRenderParams*>(params);
    ctx->SetSceneRenderParams(*p);
}

int spark_input_is_key_down(const SparkInput* input, int keyCode) {
    if (input == nullptr) {
        return 0;
    }
    const auto* in = reinterpret_cast<const Spark::IInput*>(input);
    return in->IsKeyDown(keyCode) ? 1 : 0;
}

int spark_input_is_key_pressed_this_frame(const SparkInput* input, int keyCode) {
    if (input == nullptr) {
        return 0;
    }
    const auto* in = reinterpret_cast<const Spark::IInput*>(input);
    return in->IsKeyPressedThisFrame(keyCode) ? 1 : 0;
}

float spark_input_get_mouse_delta_x(const SparkInput* input) {
    if (input == nullptr) {
        return 0.0F;
    }
    return reinterpret_cast<const Spark::IInput*>(input)->GetMouseDeltaX();
}

float spark_input_get_mouse_delta_y(const SparkInput* input) {
    if (input == nullptr) {
        return 0.0F;
    }
    return reinterpret_cast<const Spark::IInput*>(input)->GetMouseDeltaY();
}

int spark_input_is_mouse_button_down(const SparkInput* input, int button) {
    if (input == nullptr) {
        return 0;
    }
    return reinterpret_cast<const Spark::IInput*>(input)->IsMouseButtonDown(button) ? 1 : 0;
}

int spark_input_is_mouse_button_pressed_this_frame(const SparkInput* input, int button) {
    if (input == nullptr) {
        return 0;
    }
    return reinterpret_cast<const Spark::IInput*>(input)->IsMouseButtonPressedThisFrame(button) ? 1 : 0;
}

int spark_input_is_mouse_button_released_this_frame(const SparkInput* input, int button) {
    if (input == nullptr) {
        return 0;
    }
    return reinterpret_cast<const Spark::IInput*>(input)->IsMouseButtonReleasedThisFrame(button) ? 1 : 0;
}

float spark_input_get_scroll_delta_y(const SparkInput* input) {
    if (input == nullptr) {
        return 0.0F;
    }
    return reinterpret_cast<const Spark::IInput*>(input)->GetScrollDeltaY();
}

void spark_input_get_cursor_framebuffer_pixels(
        const SparkInput* input,
        float* outX,
        float* outY,
        int drawableWidth,
        int drawableHeight) {
    if (input == nullptr || outX == nullptr || outY == nullptr) {
        return;
    }
    reinterpret_cast<const Spark::IInput*>(input)->GetCursorFramebufferPixels(
            *outX, *outY, drawableWidth, drawableHeight);
}

void spark_input_set_cursor_captured(SparkInput* input, int capture) {
    if (input == nullptr) {
        return;
    }
    reinterpret_cast<Spark::IInput*>(input)->SetCursorCaptured(capture != 0);
}

int spark_input_is_cursor_captured(const SparkInput* input) {
    if (input == nullptr) {
        return 0;
    }
    return reinterpret_cast<const Spark::IInput*>(input)->IsCursorCaptured() ? 1 : 0;
}

SparkGameWorld* spark_scene_get_world(SparkScene* scene) {
    if (scene == nullptr) {
        return nullptr;
    }
    auto* s = reinterpret_cast<Spark::Scene*>(scene);
    return reinterpret_cast<SparkGameWorld*>(&s->GetWorld());
}

void spark_world_update_game_objects(
        SparkGameWorld* world,
        const SparkFrameTiming* timing,
        SparkEngineContext* context) {
    if (world == nullptr || timing == nullptr || context == nullptr) {
        return;
    }
    auto* w = reinterpret_cast<Spark::GameWorld*>(world);
    auto* ctx = reinterpret_cast<Spark::IEngineContext*>(context);
    w->UpdateGameObjects(ToCpp(*timing), *ctx);
}

void spark_world_process_sound_cues(SparkGameWorld* world, SparkEngineContext* context) {
    if (world == nullptr || context == nullptr) {
        return;
    }
    Spark::ProcessSoundCues(
            *reinterpret_cast<Spark::GameWorld*>(world),
            *reinterpret_cast<Spark::IEngineContext*>(context));
}

SparkGameObject* spark_world_create_game_object(SparkGameWorld* world, const char* utf8Name) {
    if (world == nullptr) {
        return nullptr;
    }
    auto* w = reinterpret_cast<Spark::GameWorld*>(world);
    Spark::GameObject* object = w->CreateGameObject();
    if (object != nullptr && utf8Name != nullptr) {
        object->GetName() = Spark::Utf8String(utf8Name);
    }
    return reinterpret_cast<SparkGameObject*>(object);
}

void spark_world_destroy_game_object(SparkGameWorld* world, SparkGameObject* object) {
    if (world == nullptr || object == nullptr) {
        return;
    }
    reinterpret_cast<Spark::GameWorld*>(world)->DestroyGameObject(
            reinterpret_cast<Spark::GameObject*>(object));
}

std::uint64_t spark_object_get_id(const SparkGameObject* object) {
    if (object == nullptr) {
        return 0;
    }
    return reinterpret_cast<const Spark::GameObject*>(object)->GetId();
}

SparkGameComponent* spark_object_try_get_component_by_kind(SparkGameObject* object, SparkComponentKind kind) {
    if (object == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<SparkGameComponent*>(
            reinterpret_cast<Spark::GameObject*>(object)->TryGetComponentByKind(ToCpp(kind)));
}

}  // extern "C"
