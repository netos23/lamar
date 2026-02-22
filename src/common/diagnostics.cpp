// Diagnostic helpers implementation

#include "diagnostics.hpp"
#include "runtime.hpp"

#include <string>

namespace lamar::diagnostics {

void push_error_diagnostic(std::string_view message, uint32_t offset) {
    std::string formatted_message = std::string(message) + " At: %#08X\n";
    failure(formatted_message.data(), offset);
}

} // namespace lamar::diagnostics
