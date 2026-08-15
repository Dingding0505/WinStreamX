#pragma once

#include <string>
#include <utility>

namespace winstreamx {

class Status {
public:
    static Status Ok() { return Status(true, ""); }
    static Status Error(std::string message) { return Status(false, std::move(message)); }

    bool ok() const { return ok_; }
    const std::string& message() const { return message_; }

private:
    Status(bool ok, std::string message) : ok_(ok), message_(std::move(message)) {}

    bool ok_ = true;
    std::string message_;
};

template <typename T>
class Result {
public:
    Result(T value) : status_(Status::Ok()), value_(std::move(value)) {}
    Result(Status status) : status_(std::move(status)), value_() {}

    bool ok() const { return status_.ok(); }
    const Status& status() const { return status_; }
    const T& value() const { return value_; }
    T& value() { return value_; }

private:
    Status status_;
    T value_{};
};

}  // namespace winstreamx
