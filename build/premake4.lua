function args_contains(element)
  for _, value in pairs(_ARGS) do
    if value == element then
      return true
    end
  end
  return false
end

newoption {
    trigger = "prefix",
    value = "/usr",
    description = "Installation prefix"
}

if not _OPTIONS["prefix"] then
    _OPTIONS["prefix"] = "/usr"
end

newaction {
    trigger = "install",
    description = "Install binary files to system location(s) [you have to build the project first]",
    execute = function()
        if not os.is("windows") then
            prefix = _OPTIONS["prefix"]
            print("Installing newsoul binary")
            binpath = path.join(prefix, "bin")
            os.mkdir(binpath)
            os.copyfile("bin/newsoul", binpath)
            print("Installing systemd service file")
            syspath = path.join(prefix, "lib/systemd/system")
            os.mkdir(syspath)
            os.copyfile("../scripts/newsoul@.service", syspath)
            print("Installing default configuration")
            os.mkdir("/etc/newsoul")
            os.copyfile("../scripts/config.json", "/etc/newsoul")
        end
    end
}

function include()
    if os.is("bsd") then
        includedirs {"/usr/local/include/db5"}
    end
end

function link(tests)
    include()
    links {"z", "event", "nettle", "json-c", "efsw"}
    if not tests then
        links {"db_cxx", "tag"}
    else
        includedirs {"../tests/mocks"}
        links {"CppUTest", "CppUTestExt"}
    end

    if os.is("bsd") then
        libdirs {"/usr/local/lib/db5"}
        links {"iconv"}
    end

    if args_contains("kqueue") then
        links {"kqueue"}
    end
    if not os.is("windows") and not os.is("haiku") then
        links {"pthread"}
    end
    if os.is("macosx") then
        links {"CoreFoundation.framework", "CoreServices.framework"}
    end
end

solution "newsoul"
    targetdir "bin"
    configurations {"release", "debug"}
    language "C++"
    flags {"ExtraWarnings"}
    buildoptions {"-std=c++11"}

    configuration "debug"
        defines {"DEBUG"}
        flags {"Symbols"}
        buildoptions {"-ggdb3", "-O0", "-fno-inline"}

    configuration "release"
        defines {"NDEBUG"}
        flags {"Optimize"}

    project "newsoul"
        kind "ConsoleApp"
        files {"../src/**.cpp"}
        excludes {"../src/efsw/**.cpp"}
        link()

    project "newsoul-tests"
        kind "ConsoleApp"
        files {"../src/**.cpp", "../tests/**.cpp"}
        excludes {"../src/efsw/**.cpp", "../src/main.cpp"}
        link(true)
        buildoptions {
            "-include CppUTest/MemoryLeakDetectorNewMacros.h",
            "-include CppUTest/MemoryLeakDetectorMallocMacros.h"
        }

    project "efsw"
        if os.is("windows") then
            osfiles = "../src/efsw/src/efsw/platform/win/*.cpp"
        else
            osfiles = "../src/efsw/src/efsw/platform/posix/*.cpp"
        end

        -- This is for testing in other platforms that don't support kqueue
        if args_contains("kqueue") then
            defines {"EFSW_KQUEUE"}
            printf("Forced Kqueue backend build.")
        end

        -- Activates verbose mode
        if args_contains("verbose") then
            defines {"EFSW_VERBOSE"}
        end

        if os.is("macosx") then
            -- Premake 4.4 needed for this
            if not string.match(_PREMAKE_VERSION, "^4.[123]") then
                local ver = os.getversion();

                if not (ver.majorversion >= 10 and ver.minorversion >= 5) then
                    defines {"EFSW_FSEVENTS_NOT_SUPPORTED"}
                end
            end
        end

		kind "StaticLib"
		targetdir "lib"
		includedirs {"../src/efsw/include", "../src/efsw/src"}
		files {"../src/efsw/src/efsw/*.cpp", osfiles}

		configuration "debug"
			buildoptions {"-pedantic -Wno-long-long"}
