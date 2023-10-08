#include <string>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <fmt/core.h>
#include <fmt/ostream.h>

#include <Windows.h>

#pragma comment(lib, "imm32.lib")

#define IMC_GETCONVERSIONMODE 0x01
#define IMC_SETCONVERSIONMODE 0x02

namespace po = boost::program_options;
template <> struct fmt::formatter<po::options_description> : ostream_formatter {};

boost::optional<intptr_t> hwnd;

HWND getForegroundWindow()
{
    if (hwnd)
        return reinterpret_cast<HWND>(*hwnd);
    return GetForegroundWindow();
}

int GetCurrentInputMethod() {
    HWND hWnd = getForegroundWindow();
    DWORD threadId = GetWindowThreadProcessId(hWnd, nullptr);
    HKL hKL = GetKeyboardLayout(threadId);
    uintptr_t langId = reinterpret_cast<uintptr_t>(hKL) & 0xFFFF;
    return static_cast<int>(langId);
}

void SwitchInputMethod(int langId) {
    HWND hWnd = getForegroundWindow();
    PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, 0, static_cast<LPARAM>(langId));
}

int GetCurrentInputMethodMode() {
    HWND hWnd = getForegroundWindow();
    HWND hIMEWnd = ImmGetDefaultIMEWnd(hWnd);
    LRESULT mode = SendMessage(hIMEWnd, WM_IME_CONTROL, IMC_GETCONVERSIONMODE, 0);
    return static_cast<int>(mode);
}

void SwitchInputMethodMode(int mode) {
    HWND hWnd = getForegroundWindow();
    HWND hIMEWnd = ImmGetDefaultIMEWnd(hWnd);
    SendMessage(hIMEWnd, WM_IME_CONTROL, IMC_SETCONVERSIONMODE, static_cast<LPARAM>(mode));
}

int main(int argc, char** argv) {
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help,h", "produce help message")
            ("im", po::value<std::string>()->implicit_value("")->value_name("[INPUTMETHOD]"), "show the current input method if INPUTMETHOD is omitted, otherwise switch to the specified input method")
            ("imm", po::value<std::string>()->implicit_value("")->value_name("[INPUTMETHODMODE or switch]"), "show the current input method mode if INPUTMETHODMODE is omitted, otherwise switch to the specified input method mode. If switch, toggle between 1025 and 0")
            ("hwnd", po::value(&hwnd)->value_name("[HWND]"), "specify hwnd to send message, GetForegroundWindow if omitted")
            ;
    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            fmt::print("{}\n", desc);
            return -1;
        }

        if (vm.count("im")) {
            auto im = vm["im"].as<std::string>();
            try {
                auto lang_id = boost::lexical_cast<int>(im);
                SwitchInputMethod(lang_id);
            }
            catch(boost::bad_lexical_cast&) {
                if (im == "") {
                    return GetCurrentInputMethod();
                }
                else {
                    fmt::print("{}\n", desc);
                    return -1;
                }
            }
        }

        if (vm.count("imm")) {
            auto imm = vm["imm"].as<std::string>();
            try {
                auto mode = boost::lexical_cast<int>(imm);
                SwitchInputMethodMode(mode);
            }
            catch(boost::bad_lexical_cast&) {
                if (imm == "switch") {
                    auto mode = GetCurrentInputMethodMode();
                    if (mode == 0)
                        SwitchInputMethodMode(1025);
                    else
                        SwitchInputMethodMode(0);
                }
                else if (imm == "") {
                    return GetCurrentInputMethodMode();
                }
                else {
                    fmt::print("{}\n", desc);
                    return -1;
                }
            }
        }
    }
    catch (...) {
        fmt::print("{}\n", desc);
    }

    return 0;
}
