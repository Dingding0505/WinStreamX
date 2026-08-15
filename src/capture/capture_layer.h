#pragma once

#include <string_view>

namespace winstreamx {

class CaptureLayer {
public:
    std::string_view name() const;
    std::string_view responsibility() const;
};

}  // namespace winstreamx
