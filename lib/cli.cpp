// Copyright © 2026 Lucius Martius
// SPDX-License-Identifier: GPL-3.0-only

#include "cli.h"

#include "normalmapgenerator.h"
#include "specularmapgenerator.h"
#include "ssaogenerator.h"

#include <charconv>
#include <filesystem>
#include <print>

using namespace nmg;

template<typename T>
T parseNumber(std::span<const std::string_view>::iterator & it,
              const std::span<const std::string_view>::iterator end) {
    auto opt = *it;
    if (++it == end) {
        throw std::runtime_error{
            std::format("{} requires an argument of type '{}'", opt, typeid(T).name())};
    }

    T value;
    auto result = std::from_chars(it->data(), it->data() + it->size(), value);

    if (result.ec != std::errc{}) {
        throw std::runtime_error{std::format("Invalid value for {}: {}", opt, *it)};
    }

    return value;
}

template<>
bool parseNumber<bool>(std::span<const std::string_view>::iterator & it,
                       const std::span<const std::string_view>::iterator end) {
    auto opt = *it;
    if (++it == end) {
        throw std::runtime_error{std::format("--{} requires an argument of type 'bool'", opt)};
    }

    bool value{};
    if (*it == "true") {
        value = true;
    } else if (*it == "false") {
        value = false;
    } else {
    }

    return value;
}

void printGeneralHelp() {
    std::print(R"(nmg - Normal Map Generator

USAGE:
    nmg <command> [OPTIONS] <args>

COMMANDS:
    normals    Generate a normal map from a height/diffuse map
    specular   Generate a specular map
    ssao       Generate screen-space ambient occlusion
    help       Show help for a command
    version    Show version information

OPTIONS:
    -h, --help     Show this help message
    -v, --version  Show version information)");
}

void printNormalsHelp() {
    std::print(R"(nmg normals - Generate a normal map

USAGE:
    nmg normals [OPTIONS] <infile> <outfile>

OPTIONS:
    --strength <float>                Normal map strength (default: 2.0)
    --method <sobel|prewitt>          Convolution kernel method (default: sobel)
    --invert <true|false>             Invert height values (default: false)
    --tileable <true|false>           Make edges tileable (default: true)
    --keep-large-detail <true|false>  Preserve large scale details (default: true)
    --large-detail-scale <int>        Scale for large detail preservation (default: 25)
    --large-detail-height <float>     Height factor for large details (default: 1.0)
    --mode <average|max>              Intensity calculation mode (default: average)
    --red-mul <float>                 Red channel multiplier (default: 1.0)
    --green-mul <float>               Green channel multiplier (default: 1.0)
    --blue-mul <float>                Blue channel multiplier (default: 1.0)
    --alpha-mul <float>               Alpha channel multiplier (default: 0.0)
    -h, --help                        Show this help message)");
}

void printSpecularHelp() {
    std::print(R"(nmg specular - Generate a specular map

USAGE:
    nmg specular [OPTIONS] <infile> <outfile>

OPTIONS:
    --scale <float>              Scale factor (default: 1.0)
    --contrast <float>           Contrast adjustment (default: 1.0)
    --mode <average|max>         Intensity calculation mode (default: average)
    --red-mul <float>            Red channel multiplier (default: 1.0)
    --green-mul <float>          Green channel multiplier (default: 1.0)
    --blue-mul <float>           Blue channel multiplier (default: 1.0)
    --alpha-mul <float>          Alpha channel multiplier (default: 0.0)
    -h, --help                   Show this help message

EXAMPLE:
    nmg specular --scale 1.2 --contrast 0.8 input.png output.png)");
}

void printSsaoHelp() {
    std::print(R"(nmg ssao - Generate screen-space ambient occlusion

USAGE:
    nmg ssao [OPTIONS] <normalmap> <depthmap> <outfile>

OPTIONS:
    --radius <float>         SSAO sampling radius (default: 1.0)
    --samples <uint>         Number of kernel samples (default: 16)
    --noise-size <uint>      Noise texture size (default: 4)
    -h, --help               Show this help message)");
}

template<typename Fn>
auto parsePositionalArguments(std::size_t num,
                              std::span<const std::string_view> args,
                              Fn command) {
    if (args.size() < num) {
        throw std::runtime_error{"Too few positional arguments"};
    }

    if (args.size() > num) {
        throw std::runtime_error{"Too many positional arguments"};
    }

    for (auto arg : args) {
        if (arg.find_first_not_of('-' != 0)) {
            throw std::runtime_error{"No options after positional arguments allowed."};
        }
    }

    return [files = args | std::views::transform([](auto & arg) {
        return std::filesystem::path{arg};
    }) | std::ranges::to<std::vector>(),
            command = std::move(command)]() { return command(files); };
}

std::move_only_function<int()> parseNormalOptions(std::span<const std::string_view> range) {
    struct normal_options {
        double strength = 1.0;
        NormalmapGenerator::Kernel method = NormalmapGenerator::SOBEL;
        bool invert = false;
        bool tileable = false;
        bool keepLargeDetail = false;
        int largeDetailScale = 25;
        double largeDetailHeight = 1.0;
        IntensityMap::Mode mode = IntensityMap::AVERAGE;
        double redMul = 1.0;
        double greenMul = 1.0;
        double blueMul = 1.0;
        double alphaMul = 0.0;
    };
    normal_options opts{};

    for (auto it = range.begin(); it != range.end(); ++it) {
        if (*it == "--help" || *it == "-h") {
            return []() {
                printNormalsHelp();
                return 0;
            };
        } else if (*it == "--strength") {
            auto val = parseNumber<double>(it, range.end());
            opts.strength = val;
        } else if (*it == "--method") {
            if (++it == range.end()) {
                throw std::runtime_error{"--method requires an argument"};
            }

            if (*it == "sobel") {
                opts.method = NormalmapGenerator::SOBEL;
            } else if (*it == "prewitt") {
                opts.method = NormalmapGenerator::PREWITT;
            } else {
                throw std::runtime_error{
                    std::format("Invalid method: {} (must be 'sobel' or 'prewitt')", *it)};
            }
        } else if (*it == "--invert") {
            if (++it == range.end()) {
                throw std::runtime_error{"--invert requires an argument"};
            }
            opts.invert = true;
        } else if (*it == "--tileable") {
            auto val = parseNumber<bool>(it, range.end());
            opts.tileable = val;
        } else if (*it == "--keep-large-detail") {
            auto val = parseNumber<bool>(it, range.end());
            opts.keepLargeDetail = val;
        } else if (*it == "--large-detail-scale") {
            auto val = parseNumber<int>(it, range.end());
            opts.largeDetailScale = val;
        } else if (*it == "--large-detail-height") {
            auto val = parseNumber<double>(it, range.end());
            opts.largeDetailHeight = val;
        } else if (*it == "--mode") {
            if (++it == range.end()) {
                throw std::runtime_error{"--mode requires an argument"};
            }

            if (*it == "average") {
                opts.mode = IntensityMap::AVERAGE;
            } else if (*it == "max") {
                opts.mode = IntensityMap::MAX;
            } else {
                throw std::runtime_error{
                    std::format("Invalid mode: {} (must be 'average' or 'max')", *it)};
            }
        } else if (*it == "--red-mul") {
            auto val = parseNumber<double>(it, range.end());
            opts.redMul = val;
        } else if (*it == "--green-mul") {
            auto val = parseNumber<double>(it, range.end());
            opts.greenMul = val;
        } else if (*it == "--blue-mul") {
            auto val = parseNumber<double>(it, range.end());
            opts.blueMul = val;
        } else if (*it == "--alpha-mul") {
            auto val = parseNumber<double>(it, range.end());
            opts.alphaMul = val;
        } else if (it->find_first_not_of('-') == 0) {
            return parsePositionalArguments(2, std::span{it, range.end()},
                                            [opts](const auto & files) {
                QImage input(QString::fromStdString(files[0].string()));
                if (input.isNull()) {
                    std::println(stderr, "Failed to load input image: {}", files[0]);
                    return 1;
                }

                // Setup generator and calculate map
                NormalmapGenerator generator(opts.mode, opts.redMul, opts.greenMul,
                                             opts.blueMul, opts.alphaMul);

                QImage normalmap = generator.calculateNormalmap(input, opts.method,
                                                                opts.strength, opts.invert,
                                                                opts.tileable,
                                                                opts.keepLargeDetail,
                                                                opts.largeDetailScale,
                                                                opts.largeDetailHeight);

                if (!normalmap.save(QString::fromStdString(files[1].string()))) {
                    std::println(stderr, "Error: Failed to save output image: {}", files[1]);
                    return 1;
                }

                std::println("Normal map generated successfully: {}", files[1]);
                return 0;
            });
        } else {
            throw std::runtime_error{"Unknown option for normals command: " + *it};
        }
    }

    throw std::runtime_error{"Command missing <input> and <output> parameters."};
}

std::move_only_function<int()> parseSpecularOptions(std::span<const std::string_view> args) {
    struct SpecularOptions {
        double scale = 1.0;
        double contrast = 1.0;
        IntensityMap::Mode mode = IntensityMap::AVERAGE;
        double redMul = 1.0;
        double greenMul = 1.0;
        double blueMul = 1.0;
        double alphaMul = 0.0;
    };
    SpecularOptions opts;

    for (auto it = args.begin(); it != args.end(); ++it) {
        if (*it == "--help" || *it == "-h") {
            return []() {
                printSpecularHelp();
                return 0;
            };
        } else if (*it == "--scale") {
            auto val = parseNumber<double>(it, args.end());
            opts.scale = val;
        } else if (*it == "--contrast") {
            auto val = parseNumber<double>(it, args.end());
            opts.contrast = val;
        } else if (*it == "--mode") {
            if (++it != args.end()) {
                throw std::runtime_error{"--mode requires an argument"};
            }

            if (*it == "average") {
                opts.mode = IntensityMap::AVERAGE;
            } else if (*it == "max") {
                opts.mode = IntensityMap::MAX;
            } else {
                throw std::runtime_error{
                    std::format("Invalid mode: {} (must be 'average' or 'max')", *it)};
            }
        } else if (*it == "--red-mul") {
            auto val = parseNumber<double>(it, args.end());
            opts.redMul = val;
        } else if (*it == "--green-mul") {
            auto val = parseNumber<double>(it, args.end());
            opts.greenMul = val;
        } else if (*it == "--blue-mul") {
            auto val = parseNumber<double>(it, args.end());
            opts.blueMul = val;
        } else if (*it == "--alpha-mul") {
            auto val = parseNumber<double>(it, args.end());
            opts.alphaMul = val;
        } else if (it->find_first_not_of('-') == 0) {
            return parsePositionalArguments(2, std::span{it, args.end()},
                                            [opts](const auto & files) {
                // Load input image
                QImage input(QString::fromStdString(files[0].string()));
                if (input.isNull()) {
                    std::println(stderr, "Error: Failed to load input image: {}", files[0]);
                    return 1;
                }

                // Setup generator and calculate map
                SpecularmapGenerator generator(opts.mode, opts.redMul, opts.greenMul,
                                               opts.blueMul, opts.alphaMul);

                QImage specmap = generator.calculateSpecmap(input, opts.scale, opts.contrast);

                // Save output image
                if (!specmap.save(QString::fromStdString(files[1].string()))) {
                    std::println(stderr, "Error: Failed to save output image: {}", files[1]);
                    return 1;
                }

                std::println("Specular map generated successfully: {}", files[1]);
                return 0;
            });
        } else {
            throw std::runtime_error{"Unknown option for specular command: " + *it};
        }
    }

    throw std::runtime_error{"Missing positional arguments."};
}

std::move_only_function<int()> parseSsaoOptions(std::span<const std::string_view> args) {
    struct SsaoOptions {
        float radius = 1.0f;
        unsigned int samples = 16;
        unsigned int noiseSize = 4;
    };
    SsaoOptions opts;

    for (auto it = args.begin(); it != args.end(); ++it) {
        if (*it == "--help" || *it == "-h") {
            return []() {
                printSsaoHelp();
                return 0;
            };
        } else if (*it == "--radius") {
            auto val = parseNumber<double>(it, args.end());
            opts.radius = static_cast<float>(val);
        } else if (*it == "--samples") {
            auto val = parseNumber<unsigned int>(it, args.end());
            opts.samples = val;
        } else if (*it == "--noise-size") {
            auto val = parseNumber<unsigned int>(it, args.end());
            opts.noiseSize = val;
        } else if (it->find_first_not_of('-') == 0) {
            return parsePositionalArguments(3, std::span{it, args.end()},
                                            [opts](const auto & files) {
                QImage normalmap(QString::fromStdString(files[0].string()));
                if (normalmap.isNull()) {
                    std::println(stderr, "Error: Failed to load normalmap: {}", files[0]);
                    return 1;
                }

                QImage depthmap(QString::fromStdString(files[1].string()));
                if (depthmap.isNull()) {
                    std::println(stderr, "Error: Failed to load depthmap: {}", files[1]);
                    return 1;
                }

                // Setup generator and calculate map
                SsaoGenerator generator;
                QImage ssaomap = generator.calculateSsaomap(normalmap, depthmap, opts.radius,
                                                            opts.samples, opts.noiseSize);

                // Save output image
                if (!ssaomap.save(QString::fromStdString(files[2].string()))) {
                    std::println(stderr, "Error: Failed to save output image: {}", files[2]);
                    return 1;
                }

                std::println("SSAO map generated successfully: {}", files[2]);
                return 0;
            });
        } else {
            throw std::runtime_error{"Unknown option for ssao command: " + *it};
        }
    }

    throw std::runtime_error{"Missing positional arguments."};
}

cli::cli(const std::vector<std::string_view> & args) {
    if (args.empty()) {
        command = []() {
            printGeneralHelp();
            return 1;
        };
        return;
    }

    for (auto it = args.begin(); it != args.end(); ++it) {
        if (*it == "--help" || *it == "-h") {
            command = []() {
                printGeneralHelp();
                return 0;
            };
            return;
        } else if (*it == "--version" || *it == "-v") {
            command = []() {
                std::print("nmg (Normal Map Generator) version 1.0.0\n");
                return 0;
            };
            return;
        } else if (*it == "normals" || *it == "normal") {
            command = parseNormalOptions(std::span{it + 1, args.end()});
            break;
        } else if (*it == "specular" || *it == "spec") {
            command = parseSpecularOptions(std::span{it + 1, args.end()});
            break;
        } else if (*it == "ssao") {
            command = parseSsaoOptions(std::span{it + 1, args.end()});
            break;
        } else {
            throw std::runtime_error{"Unknown command or option: " + *it};
            return;
        }
    }
}
