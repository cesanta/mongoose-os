# -*- mode: gn -*-
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "common",
    srcs = [
        "cs_strtod.c",
        "cs_time.c",
        "mbuf.c",
        "mg_str.c",
        "str_util.c",
        "utf.c",
    ],
    hdrs = [
        "cs_strtod.h",
        "cs_time.h",
        "mbuf.h",
        "mg_str.h",
        "str_util.h",
        "utf.h",
        "platform.h",
    ] + glob(["platforms/platform_*.h"]),
    defines = ["c_strnlen=strnlen"],
)

cc_library(
    name = "test_util",
    srcs = ["test_util.c"],
    hdrs = ["test_util.h"],
    deps = [":common"],
)
