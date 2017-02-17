# -*- mode: gn -*-
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "common",
    srcs = [
        "cs_dbg.c",
        "cs_file.c",
        "cs_strtod.c",
        "cs_time.c",
        "mbuf.c",
        "mg_str.c",
        "str_util.c",
        "utf.c",
    ],
    hdrs = [
        "cs_dbg.h",
        "cs_file.h",
        "cs_strtod.h",
        "cs_time.h",
        "mbuf.h",
        "mg_str.h",
        "platform.h",
        "queue.h",
        "str_util.h",
        "utf.h",
    ] + glob(["platforms/platform_*.h"]) + [
        "platforms/simplelink/cs_simplelink.h",
    ],
    defines = ["c_strnlen=strnlen"],
)

cc_library(
    name = "test_util",
    testonly = 1,
    srcs = ["test_util.c"],
    hdrs = ["test_util.h"],
    deps = [":common"],
)
