#include <Include.h>
#include <sdk/sdk.h>

#include "tools/console/console.h"

namespace {
    using namespace std::string_view_literals;

    // clang-format off
    constexpr std::initializer_list<std::string_view> kRequiredGameModules = {
        // @note: @es3n1n: modules that we'll use in our code
        "client.dll"sv,
        "engine2.dll"sv,
        "schemasystem.dll"sv,
        
        // @note: @soufiw: latest modules that gets loaded in the main menu
        "matchmaking.dll"sv, 
        "navsystem.dll"sv
    };
    // clang-format on

    std::atomic_bool is_finished = false;
} // namespace

namespace source2_gen {
    void Setup() try {
        // @note: @es3n1n: Waiting for game init
        //
        const auto required_modules_present = []() [[msvc::forceinline]] -> bool {
            bool result = true;

            for (auto& name : kRequiredGameModules)
                result &= static_cast<bool>(GetModuleHandleA(name.data()));

            return result;
        };

        while (!required_modules_present())
            sleep_for(std::chrono::seconds(5));

        fmt::print("{}: Starting\n", __FUNCTION__);

        // @note: @es3n1n: Capture interfaces
        //
        sdk::g_schema = CSchemaSystem::GetInstance();
        if (!sdk::g_schema)
            throw std::runtime_error(fmt::format("Unable to obtain Schema interface"));

        // @note: @es3n1n: Obtaining type scopes and generating sdk
        //
        const auto type_scopes = sdk::g_schema->GetTypeScopes();
        for (auto i = 0; i < type_scopes.Count(); ++i)
            sdk::GenerateTypeScopeSdk(type_scopes.m_pElements[i]);

        // @note: @es3n1n: Generating sdk for global type scope
        //
        sdk::GenerateTypeScopeSdk(sdk::g_schema->GlobalTypeScope());

        // @note: @es3n1n: We are done here
        //
        is_finished = true;
    } catch (std::runtime_error& err) {
        fmt::print("{} :: ERROR :: {}", __FUNCTION__, err.what());
        is_finished = true;
    }

    void WINAPI main(const HMODULE module) {
        auto console = std::make_unique<DebugConsole>();
        console->start("Source 2 Generator");

        std::jthread setup_thread(&Setup);

        while (!is_finished) {
            console->update();
            sleep_for(std::chrono::milliseconds(1));
        }

        fmt::print("Successfuly dumped Source 2 SDK, now you can safely close this console.\n");

        console->stop();
        console.reset();

        FreeLibraryAndExitThread(module, EXIT_SUCCESS);
    }
} // namespace source2_gen
