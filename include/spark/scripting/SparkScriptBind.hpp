#pragma once

/**
 * Mark the next member for automatic <c>spark_*</c> interop + C# mirror codegen.
 *
 * @param suffix Interop name after <c>spark_&lt;componentPrefix&gt;_</c> (e.g. <c>get_enabled</c>).
 *
 * Example:
 * @code
 * SPARK_SCRIPT_BIND(get_enabled)
 * [[nodiscard]] bool IsEnabled() const noexcept;
 *
 * SPARK_SCRIPT_BIND(set_enabled)
 * void SetEnabled(bool value) noexcept;
 * @endcode
 *
 * Regenerate C# / interop from the spark-sharp repository (./tools/generate-csharp-bindings.sh).
 */
#define SPARK_SCRIPT_BIND(suffix) /* spark_script_bind:suffix */
