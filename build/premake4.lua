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

newaction {
    trigger = "coverage",
    description = "Create HTML coverage report",
    execute = function()
        os.execute("lcov --directory . -c -o newsoul_cov")
        os.execute("lcov --remove newsoul_cov '/usr*' -o newsoul_cov")
        if os.getenv("config") == "release" then
            os.execute("lcov --remove newsoul_cov 'tests*' -o newsoul_cov")
        end
        os.execute("genhtml -o coverage -t 'newsoul' --num-spaces 4 newsoul_cov")
    end
}

function link(tests)
    links {
        "z", "event", "nettle", "json-c",
        "sqlite3", "pcrecpp", "pcre", "glog", "uv"
    }
    if not tests then
        links {"tag"}
    else
        includedirs {"../tests/mocks"}
        links {"CppUTest", "CppUTestExt", "gcov"}
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
        buildoptions {"-ggdb3", "-O0", "-fno-inline", "-Wfatal-errors"}

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
            "-include CppUTest/MemoryLeakDetectorMallocMacros.h",
            "--coverage"
        }
