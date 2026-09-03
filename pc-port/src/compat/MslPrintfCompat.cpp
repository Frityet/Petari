#include "compat/MslPrintfCompat.hpp"

#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <limits>
#include <type_traits>

namespace {
    enum justification_options { left_justification,
                                 right_justification,
                                 zero_fill };

    enum sign_options { only_minus,
                        sign_always,
                        space_holder };

    enum argument_options {
        normal_argument,
        char_argument,
        short_argument,
        long_argument,
        long_long_argument,
        wchar_argument,
        intmax_argument,
        size_t_argument,
        ptrdiff_argument,
        long_double_argument
    };

    typedef struct {
        unsigned char justification_options;  // 0x0
        unsigned char sign_options;           // 0x1
        unsigned char precision_specified;    // 0x2
        unsigned char alternate_form;         // 0x3
        unsigned char argument_options;       // 0x4
        unsigned char conversion_char;        // 0x5
        int field_width;                      // 0x8
        int precision;                        // 0xC
    } print_format;

    struct NativeStars {
        int values[2];
        int count = 0;
    };

    static const char *parse_format(const char *format_string, va_list *arg, print_format *format, NativeStars *stars) {
        print_format f;
        const char *s = format_string;
        int c;
        int flag_found;
        f.justification_options = right_justification;
        f.sign_options = only_minus;
        f.precision_specified = 0;
        f.alternate_form = 0;
        f.argument_options = normal_argument;
        f.field_width = 0;
        f.precision = 0;

        if ((c = *++s) == '%') {
            f.conversion_char = c;
            *format = f;
            return ((const char *)s + 1);
        }

        for (;;) {
            flag_found = 1;

            switch (c) {
            case '-':
                f.justification_options = left_justification;
                break;
            case '+':
                f.sign_options = sign_always;
                break;
            case ' ':
                if (f.sign_options != sign_always) {
                    f.sign_options = space_holder;
                }
                break;
            case '#':
                f.alternate_form = 1;
                break;
            case '0':
                if (f.justification_options != left_justification) {
                    f.justification_options = zero_fill;
                }
                break;
            default:
                flag_found = 0;
                break;
            }

            if (flag_found) {
                c = *++s;
            } else {
                break;
            }
        }

        if (c == '*') {
            f.field_width = va_arg(*arg, int);
            stars->values[stars->count++] = f.field_width;
            if (f.field_width < 0) {
                if (f.field_width == INT_MIN) {
                    f.conversion_char = 0xFF;
                    *format = f;
                    return s + 1;
                }
                f.justification_options = left_justification;
                f.field_width = -f.field_width;
            }

            c = *++s;
        } else {
            while (isdigit(static_cast<unsigned char>(c))) {
                if (f.field_width > (INT_MAX - (c - '0')) / 10) {
                    f.conversion_char = 0xFF;
                    *format = f;
                    return s + 1;
                }
                f.field_width = (f.field_width * 10) + (c - '0');
                c = *++s;
            }
        }

        if (c == '.') {
            f.precision_specified = 1;

            if ((c = *++s) == '*') {
                f.precision = va_arg(*arg, int);
                stars->values[stars->count++] = f.precision;
                if (f.precision < 0) {
                    f.precision_specified = 0;
                }

                c = *++s;
            } else {
                while (isdigit(static_cast<unsigned char>(c))) {
                    if (f.precision > (INT_MAX - (c - '0')) / 10) {
                        f.conversion_char = 0xFF;
                        *format = f;
                        return s + 1;
                    }
                    f.precision = (f.precision * 10) + (c - '0');
                    c = *++s;
                }
            }
        }

        flag_found = 1;

        switch (c) {
        case 'h':
            f.argument_options = short_argument;

            if (s[1] == 'h') {
                f.argument_options = char_argument;
                c = *++s;
            }

            break;

        case 'l':
            f.argument_options = long_argument;

            if (s[1] == 'l') {
                f.argument_options = long_long_argument;
                c = *++s;
            }

            break;

        case 'L':
            f.argument_options = long_double_argument;
            break;
        case 'j':
            f.argument_options = intmax_argument;
            break;
        case 't':
            f.argument_options = ptrdiff_argument;
            break;
        case 'z':
            f.argument_options = size_t_argument;
            break;
        default:
            flag_found = 0;
            break;
        }

        if (flag_found) {
            c = *++s;
        }

        f.conversion_char = c;

        switch (c) {
        case 'd':
        case 'i':
        case 'u':
        case 'o':
        case 'x':
        case 'X':
            if (f.argument_options == long_double_argument) {
                f.conversion_char = 0xFF;
                break;
            }

            if (!f.precision_specified) {
                f.precision = 1;
            } else if (f.justification_options == zero_fill) {
                f.justification_options = right_justification;
            }
            break;

        case 'f':
        case 'F':
            if (f.argument_options == short_argument || f.argument_options == intmax_argument || f.argument_options == size_t_argument ||
                f.argument_options == ptrdiff_argument || f.argument_options == long_long_argument) {
                f.conversion_char = 0xFF;
                break;
            }

            if (!f.precision_specified) {
                f.precision = 6;
            }
            break;

        case 'a':
        case 'A':
            if (!f.precision_specified) {
                f.precision = 0xD;
            }

            if (f.argument_options == short_argument || f.argument_options == intmax_argument || f.argument_options == size_t_argument ||
                f.argument_options == ptrdiff_argument || f.argument_options == long_long_argument || f.argument_options == char_argument) {
                f.conversion_char = 0xFF;
            }

            break;

        case 'g':
        case 'G':
            if (!f.precision) {
                f.precision = 1;
            }

        case 'e':
        case 'E':
            if (f.argument_options == short_argument || f.argument_options == intmax_argument || f.argument_options == size_t_argument ||
                f.argument_options == ptrdiff_argument || f.argument_options == long_long_argument || f.argument_options == char_argument) {
                f.conversion_char = 0xFF;
                break;
            }

            if (!f.precision_specified) {
                f.precision = 6;
            }
            break;

        case 'p':
            f.conversion_char = 'x';
            f.alternate_form = 1;
            f.argument_options = long_argument;
            f.precision = 8;
            break;

        case 'c':
            if (f.argument_options == long_argument) {
                f.argument_options = wchar_argument;
            } else {
                if (f.precision_specified || f.argument_options != normal_argument) {
                    f.conversion_char = 0xFF;
                }
            }

            break;

        case 's':
            if (f.argument_options == long_argument) {
                f.argument_options = wchar_argument;
            } else {
                if (f.argument_options != normal_argument) {
                    f.conversion_char = 0xFF;
                }
            }

            break;

        case 'n':
            if (f.argument_options == long_double_argument) {
                f.conversion_char = 0xFF;
            }

            break;

        default:
            f.conversion_char = 0xFF;
            break;
        }

        if (f.conversion_char == 's' && f.field_width > 509)
            f.conversion_char = 0xFF;
        *format = f;
        return ((const char *)s + 1);
    }

    struct HostBuffer {
        char *data = nullptr;
        explicit HostBuffer(std::size_t size) : data(static_cast<char *>(std::malloc(size))) {
        }
        ~HostBuffer() {
            std::free(data);
        }
        HostBuffer(const HostBuffer &) = delete;
        HostBuffer &operator=(const HostBuffer &) = delete;
    };

    struct StringSink {
        char *output;
        std::size_t capacity;
        std::size_t copied = 0;
        int total = 0;

        bool write(const char *source, std::size_t size) {
            if (size > static_cast<std::size_t>(INT_MAX - total)) {
                errno = EOVERFLOW;
                return false;
            }
            const auto remaining = capacity - copied;
            const auto amount = size < remaining ? size : remaining;
            if (amount != 0) {
                std::memcpy(output + copied, source, amount);
                copied += amount;
            }
            total += static_cast<int>(size);
            return true;
        }
        void finish() {
            if (output && capacity)
                output[copied < capacity ? copied : capacity - 1] = '\0';
        }
    };

    template <class T>
    int host_conversion(char *output, std::size_t size, const char *specifier, const NativeStars &stars, T value) {
        switch (stars.count) {
        case 0:
            return std::snprintf(output, size, specifier, value);
        case 1:
            return std::snprintf(output, size, specifier, stars.values[0], value);
        default:
            return std::snprintf(output, size, specifier, stars.values[0], stars.values[1], value);
        }
    }

    template <class T>
    bool emit_host(StringSink &sink, const char *specifier, const NativeStars &stars, T value) {
        char local[512];
        const int needed = host_conversion(local, sizeof(local), specifier, stars, value);
        if (needed < 0)
            return false;
        if (static_cast<std::size_t>(needed) < sizeof(local))
            return sink.write(local, needed);
        HostBuffer full(static_cast<std::size_t>(needed) + 1);
        if (!full.data) {
            errno = ENOMEM;
            return false;
        }
        if (host_conversion(full.data, static_cast<std::size_t>(needed) + 1, specifier, stars, value) != needed)
            return false;
        return sink.write(full.data, needed);
    }

    bool emit_string(StringSink &sink, const char *value, const print_format &format) {
        // Original __pformatter string and padding behavior, including Pascal
        // strings, zero-fill, sign-prefix handling and byte precision.
        const char *buffer = value == nullptr ? "" : value;
        std::size_t length;
        if (format.alternate_form) {
            length = static_cast<unsigned char>(*buffer++);
            if (format.precision_specified && length > static_cast<std::size_t>(format.precision))
                length = format.precision;
        } else if (format.precision_specified) {
            length = 0;
            while (length < static_cast<std::size_t>(format.precision) && buffer[length])
                ++length;
        } else {
            length = std::strlen(buffer);
        }
        std::size_t field_width = length;
        if (format.justification_options != left_justification) {
            const char fill = format.justification_options == zero_fill ? '0' : ' ';
            if (length != 0 && (*buffer == '+' || *buffer == '-' || *buffer == ' ') && fill == '0') {
                if (!sink.write(buffer, 1))
                    return false;
                ++buffer;
                --length;
            }
            while (field_width < static_cast<std::size_t>(format.field_width)) {
                if (!sink.write(&fill, 1))
                    return false;
                ++field_width;
            }
        }
        if (!sink.write(buffer, length))
            return false;
        if (format.justification_options == left_justification) {
            const char blank = ' ';
            while (field_width < static_cast<std::size_t>(format.field_width)) {
                if (!sink.write(&blank, 1))
                    return false;
                ++field_width;
            }
        }
        return true;
    }

    // POSIX positional arguments and host-only conversion/locale extensions have
    // no MSL grammar. Preserve their existing host behavior as one native call.
    bool host_extension(const char *format) {
        for (const char *p = format; *p; ++p) {
            if (*p != '%')
                continue;
            if (p[1] == '%') {
                ++p;
                continue;
            }
            for (++p; *p; ++p) {
                if (*p == '$' || *p == '\'' || *p == 'q' || *p == 'I')
                    return true;
                if (std::strchr("diuoxXfFeEgGaAcsnp%", *p))
                    break;
                if (std::strchr("mBCDSOUb", *p))
                    return true;
                if (!std::strchr("-+ #0.*123456789hlLjzt", *p))
                    break;
            }
            if (!*p)
                break;
        }
        return false;
    }

    bool has_msl_string(const char *format) {
        for (const char *p = format; *p; ++p) {
            if (*p != '%')
                continue;
            if (p[1] == '%') {
                ++p;
                continue;
            }
            for (++p; *p; ++p) {
                if (*p == 's')
                    return true;
                if (std::strchr("diuoxXfFeEgGaAcnp%", *p))
                    break;
                if (!std::strchr("-+ #0.*123456789hlLjzt", *p))
                    break;
            }
            if (!*p)
                break;
        }
        return false;
    }

    int format_string(StringSink &sink, const char *format, va_list *args) {
        const char *cursor = format;
        while (*cursor) {
            const char *conversion = std::strchr(cursor, '%');
            if (!conversion)
                return sink.write(cursor, std::strlen(cursor)) ? sink.total : -1;
            if (!sink.write(cursor, conversion - cursor))
                return -1;
            print_format parsed;
            NativeStars stars;
            const char *end = parse_format(conversion, args, &parsed, &stars);
            if (parsed.conversion_char == 0xff)
                return sink.write(conversion, std::strlen(conversion)) ? sink.total : -1;
            cursor = end;
            const char original_conversion = end[-1];
            if (parsed.conversion_char == '%') {
                if (!sink.write("%", 1))
                    return -1;
                continue;
            }
            if (parsed.conversion_char == 's') {
                if (parsed.argument_options == wchar_argument) {
                    const wchar_t *value = va_arg(*args, const wchar_t *);
                    if (!value)
                        value = L"";
                    // MSL's only linked locale uses __wctomb_noconv: one low
                    // byte per wchar, independent of the native process locale.
                    const auto count = std::wcslen(value);
                    HostBuffer text(count + 1);
                    if (!text.data) {
                        errno = ENOMEM;
                        return -1;
                    }
                    for (std::size_t i = 0; i < count; ++i) {
                        text.data[i] = static_cast<char>(static_cast<unsigned char>(value[i]));
                    }
                    text.data[count] = '\0';
                    if (!emit_string(sink, text.data, parsed))
                        return -1;
                } else if (!emit_string(sink, va_arg(*args, const char *), parsed))
                    return -1;
                continue;
            }
            if (parsed.conversion_char == 'n') {
                switch (parsed.argument_options) {
                case char_argument:
                    *va_arg(*args, signed char *) = static_cast<signed char>(sink.total);
                    break;
                case short_argument:
                    *va_arg(*args, short *) = static_cast<short>(sink.total);
                    break;
                case long_argument:
                    *va_arg(*args, long *) = sink.total;
                    break;
                case long_long_argument:
                    *va_arg(*args, long long *) = sink.total;
                    break;
                case intmax_argument:
                    *va_arg(*args, std::intmax_t *) = sink.total;
                    break;
                case size_t_argument:
                    *va_arg(*args, std::make_signed_t<std::size_t> *) = sink.total;
                    break;
                case ptrdiff_argument:
                    *va_arg(*args, std::ptrdiff_t *) = sink.total;
                    break;
                default:
                    *va_arg(*args, int *) = sink.total;
                    break;
                }
                continue;
            }
            const auto specifier_size = static_cast<std::size_t>(end - conversion);
            HostBuffer specifier(specifier_size + 1);
            if (!specifier.data) {
                errno = ENOMEM;
                return -1;
            }
            std::memcpy(specifier.data, conversion, specifier_size);
            specifier.data[specifier_size] = '\0';
            bool okay = false;
            if (original_conversion == 'p') {
                okay = emit_host(sink, specifier.data, stars, va_arg(*args, void *));
            } else if (parsed.conversion_char == 'd' || parsed.conversion_char == 'i') {
                switch (parsed.argument_options) {
                case long_argument:
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, long));
                    break;
                case long_long_argument:
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, long long));
                    break;
                case intmax_argument:
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, std::intmax_t));
                    break;
                case size_t_argument:
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, std::make_signed_t<std::size_t>));
                    break;
                case ptrdiff_argument:
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, std::ptrdiff_t));
                    break;
                default:
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, int));
                    break;
                }
            } else if (std::strchr("uoxX", parsed.conversion_char)) {
                switch (parsed.argument_options) {
                case long_argument:
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, unsigned long));
                    break;
                case long_long_argument:
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, unsigned long long));
                    break;
                case intmax_argument:
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, std::uintmax_t));
                    break;
                case size_t_argument:
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, std::size_t));
                    break;
                case ptrdiff_argument:
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, std::make_unsigned_t<std::ptrdiff_t>));
                    break;
                default:
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, unsigned int));
                    break;
                }
            } else if (parsed.conversion_char == 'c') {
                if (parsed.argument_options == wchar_argument)
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, std::wint_t));
                else
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, int));
            } else {
                if (parsed.argument_options == long_double_argument)
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, long double));
                else
                    okay = emit_host(sink, specifier.data, stars, va_arg(*args, double));
            }
            if (!okay)
                return -1;
        }
        return sink.total;
    }
}  // namespace

extern "C" int smgpc_msl_vsnprintf(char *output, std::size_t size, const char *format, va_list input) {
    if ((!output && size != 0) || !format) {
        errno = EINVAL;
        return -1;
    }
    va_list args;
    va_copy(args, input);
    int result;
    if (host_extension(format) || !has_msl_string(format))
        result = std::vsnprintf(output, size, format, args);
    else {
        StringSink sink{output, size};
        result = format_string(sink, format, &args);
        sink.finish();
    }
    va_end(args);
    return result;
}
extern "C" int smgpc_msl_snprintf(char *output, std::size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const int result = smgpc_msl_vsnprintf(output, size, format, args);
    va_end(args);
    return result;
}
extern "C" int smgpc_msl_vsprintf(char *output, const char *format, va_list args) {
    return smgpc_msl_vsnprintf(output, std::numeric_limits<std::size_t>::max(), format, args);
}
extern "C" int smgpc_msl_sprintf(char *output, const char *format, ...) {
    va_list args;
    va_start(args, format);
    const int result = smgpc_msl_vsprintf(output, format, args);
    va_end(args);
    return result;
}
