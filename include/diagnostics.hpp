//
// Diagnostic helpers
//

#ifndef LAMAR_DIAGNOSTICS_HPP
#define LAMAR_DIAGNOSTICS_HPP

#include <cstdint>
#include <string_view>

namespace lamar::diagnostics {

void push_error_diagnostic(std::string_view message, uint32_t offset);

} // namespace lamar::diagnostics

#endif // LAMAR_DIAGNOSTICS_HPP
