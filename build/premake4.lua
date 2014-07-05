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
            etcpath = path.join(prefix, "etc/newsoul")
            os.mkdir(etcpath)
            os.copyfile("../scripts/config.json", etcpath)
        end
    end
}

function link(tests)
    links {"z", "event", "nettle", "json-c", "sqlite3", "pcrecpp", "pcre"}
    if not tests then
        links {"tag"}
    else
        includedirs {"../tests/mocks"}
        links {"CppUTest", "CppUTestExt"}
    end

    if os.is("bsd") then
        links {"iconv"}
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
        link()

    project "newsoul-tests"
        kind "ConsoleApp"
        files {"../src/**.cpp", "../tests/**.cpp"}
        excludes {"../src/main.cpp"}
        link(true)
        buildoptions {
            "-include CppUTest/MemoryLeakDetectorNewMacros.h",
            "-include CppUTest/MemoryLeakDetectorMallocMacros.h"
        }
