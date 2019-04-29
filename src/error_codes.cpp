#include <common/util/error_codes.h>

#include <stdio.h>

std::string StatusToString(int error_code) {
  switch (error_code) {
    case STATUS_OK:
      return "OK";
    case STATUS_CANCELLED:
      return "CANCELLED";
    case STATUS_UNKNOWN:
      return "UNKNOWN";
    case STATUS_INVALID_ARGUMENT:
      return "INVALID_ARGUMENT";
    case STATUS_DEADLINE_EXCEEDED:
      return "DEADLINE_EXCEEDED";
    case STATUS_NOT_FOUND:
      return "NOT_FOUND";
    case STATUS_ALREADY_EXISTS:
      return "ALREADY_EXISTS";
    case STATUS_PERMISSION_DENIED:
      return "PERMISSION_DENIED";
    case STATUS_RESOURCE_EXHAUSTED:
      return "RESOURCE_EXHAUSTED";
    case STATUS_FAILED_PRECONDITION:
      return "FAILED_PRECONDITION";
    case STATUS_ABORTED:
      return "ABORTED";
    case STATUS_OUT_OF_RANGE:
      return "OUT_OF_RANGE";
    case STATUS_UNIMPLEMENTED:
      return "UNIMPLEMENTED";
    case STATUS_INTERNAL:
      return "INTERNAL";
    case STATUS_UNAVAILABLE:
      return "UNAVAILABLE";
    case STATUS_DATA_LOSS:
      return "DATA_LOSS";
  }
  char buf[12];
  snprintf(buf, sizeof(buf), "%d", error_code);
  return std::string(buf);
}
