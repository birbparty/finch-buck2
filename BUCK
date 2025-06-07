# Root BUCK file for finch-buck2

# Generate version header
genrule(
    name = "generate_version_header",
    srcs = ["include/finch/version.h.in"],
    out = "include/finch/version.h",
    cmd = """
        sed -e 's/@PROJECT_VERSION_MAJOR@/0/g' \
            -e 's/@PROJECT_VERSION_MINOR@/1/g' \
            -e 's/@PROJECT_VERSION_PATCH@/0/g' \
            -e 's/@PROJECT_VERSION@/0.1.0/g' \
            $SRCS > $OUT
    """,
    visibility = ["PUBLIC"],
)

# Main finch-core library
cxx_library(
    name = "finch-core",
    srcs = glob(["src/**/*.cpp"]),
    headers = glob([
        "include/**/*.h",
        "include/**/*.hpp",
    ]),
    compiler_flags = ["-std=c++20"],
    preprocessor_flags = ["-DFINCH_PLATFORM_MACOS"],
    visibility = ["PUBLIC"],
)

# Main finch executable
cxx_binary(
    name = "finch",
    srcs = ["tools/main.cpp"],
    headers = glob([
        "include/**/*.h",
        "include/**/*.hpp",
    ]),
    compiler_flags = ["-std=c++20"],
    preprocessor_flags = [
        "-DCLI11_HEADER_ONLY",
        "-DFMT_HEADER_ONLY",
    ],
    deps = [
        ":finch-core",
    ],
)

# Test executable
cxx_test(
    name = "finch-tests",
    srcs = glob(["test/**/*_test.cpp"]),
    compiler_flags = ["-std=c++20"],
    deps = [
        ":finch-core",
    ],
)
