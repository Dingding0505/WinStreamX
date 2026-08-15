#include "diagnostics/logger.h"

namespace winstreamx {

std::string_view Logger::name() const {
    return "Diagnostics";
}

std::string_view Logger::responsibility() const {
    return "logging and error context";
}

}  // namespace winstreamx
