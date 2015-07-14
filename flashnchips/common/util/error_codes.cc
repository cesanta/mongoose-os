#include <common/util/error_codes.h>

namespace util {
namespace error {

std::string ToString(Code error_code) {
  switch (error_code) {
    case OK: return "OK";
    case CANCELLED: return "CANCELLED";
    case UNKNOWN: return "UNKNOWN";
    case INVALID_ARGUMENT: return "INVALID_ARGUMENT";
    case DEADLINE_EXCEEDED: return "DEADLINE_EXCEEDED";
    case NOT_FOUND: return "NOT_FOUND";
    case ALREADY_EXISTS: return "ALREADY_EXISTS";
    case PERMISSION_DENIED: return "PERMISSION_DENIED";
    case RESOURCE_EXHAUSTED: return "RESOURCE_EXHAUSTED";
    case FAILED_PRECONDITION: return "FAILED_PRECONDITION";
    case ABORTED: return "ABORTED";
    case OUT_OF_RANGE: return "OUT_OF_RANGE";
    case UNIMPLEMENTED: return "UNIMPLEMENTED";
    case INTERNAL: return "INTERNAL";
    case UNAVAILABLE: return "UNAVAILABLE";
    case DATA_LOSS: return "DATA_LOSS";
  }
  return "";
}

}  // namespace error
}  // namespace util
