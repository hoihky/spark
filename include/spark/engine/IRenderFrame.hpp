#pragma once

namespace Spark {

/**
 * Per-frame rendering hook (extend later with command lists, pass data, etc.).
 */
class IRenderFrame {
public:
    virtual ~IRenderFrame() = default;
};

}  // namespace Spark
