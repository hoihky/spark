#include "spark/ecs/components/rendering/TextOverlayComponent.hpp"

#include "spark/core/Utility.hpp"
#include "spark/ecs/GameObject.hpp"

namespace Spark {

void TextOverlayComponent::OnSignal(GameObject& /*owner*/, SignalId /*id*/, const SignalPayload& /*payload*/) {}

void TextOverlayComponent::SetText(Utf8String value) {
    text = MoveTemp(value);
}

void TextOverlayComponent::SetScreenPosition(float x, float y) {
    screenX = x;
    screenY = y;
}

void TextOverlayComponent::SetFontSizePixels(float px) {
    fontSizePixels = px > 0.0F ? px : 1.0F;
}

void TextOverlayComponent::SetColor(const Vector3& rgb) {
    color = rgb;
}

void TextOverlayComponent::SetAlpha(float a) {
    alpha = a;
}

void TextOverlayComponent::SetVisible(bool v) {
    visible = v;
}

}  // namespace Spark
