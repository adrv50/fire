import os
import sys
import glob
import subprocess
import json
import argparse

import concurrent.futures

Color::Default     = "\033[0m"
Color::Bold        = "\033[1m"

Color::Black       = "\033[30m"
Color::Red         = "\033[31m"
Color::Green       = "\033[32m"
Color::Yellow      = "\033[33m"
Color::Blue        = "\033[34m"
Color::Magenta     = "\033[35m"
Color::Cyan        = "\033[36;5m"
Color::White       = "\033[37m"

Color::Bk_BLACK    = "\033[40m"
Color::Bk_RED      = "\033[41m"
Color::Bk_GREEN    = "\033[42m"
Color::Bk_YELLOW   = "\033[43m"
Color::Bk_BLUE     = "\033[44m"
Color::Bk_MAGENTA  = "\033[45m"
Color::Bk_CYAN     = "\033[46;5m"
Color::Bk_WHITE    = "\033[47m"

# -------------------------

# TODO:
#   argparse
#   debug-build

def xprint(*args):
    for v in args:
        print(v, end="")

    print()

def cwd(S):
    print(S)
    return os.system(S)

#
#  get only file name
#  (remove folder and extension)
#  
#  folder/aaa.cpp   -->   aaa
#  b.xx             -->   b
#
def get_file_name(s: str) -> str:
    s = os.path.basename(s)

    if '.' in s:
        s = s[:s.find('.')]

    return s

class Source:
    def __init__(self, flags, path: str):
        self.path = path
        self.pathnotdir = path[path.rfind("/") + 1:]
        self.pathnoext  = get_file_name(path)
        self.ext = path[path.rfind('.'):]

        self.flags = flags

        self.dpath = f"{self.flags.OBJDIR}/{self.pathnoext}.d"
        self.objout = f"{self.flags.OBJDIR}/{self.pathnoext}.o"

#        self.mtime = 0 if not os.path.exists(self.objout) \
#            else os.path.getmtime(self.objout)

        self.mtime = os.path.getmtime(self.path)

        self.depends = self.get_depends()

        # print(f"{self.path}, {self.depends}")

        # for result
        self.is_failed = False
        self.result = None

    def get_depends(self) -> list[str]:
        global CFLAGS

        if not os.path.exists(self.dpath):
            return [ ]

        # Get all dependencies
        with open(self.dpath, mode="r") as fs:
            depends = fs.read().replace("\\\n", "")
            depends = depends[depends.find(":") + 2: depends.find("\n")]

            while depends.find("  ") != -1:
                depends = depends.replace("  ", " ")

            return depends.split(" ")

    #
    # result = if compiled return True
    #
    def compile(self):
        if os.path.exists(self.objout):
            if os.path.getmtime(self.objout) > self.mtime:
                return False

        if os.path.exists(self.flags.TARGET):
            if os.path.getmtime(self.flags.TARGET) > self.mtime:
                return False

        for d in self.depends:
            if os.path.getmtime(d) > self.mtime:
                return False

        is_cpp = self.ext != 'c'

        print(f"{Color::Bold}{Color::White}{'CXX ' if is_cpp else 'CC  '} {self.path}{Color::Default}")
        
        self.result = subprocess.run(
            f"{'clang++' if is_cpp else 'clang'} -MP -MMD -MF {self.dpath} {self.flags.COMMONFLAGS} {self.flags.CXXFLAGS if is_cpp else self.flags.CFLAGS} -c -o {self.objout} {self.path}",
            shell=True,
            capture_output=True,
            text=True
        )

        self.is_failed = (self.result.returncode != 0)

        if self.is_failed:
            print(Color::Bold + Color::Red, "failed: ", self.path, Color::Default)

        return True

class BuilderFlags:

    def _flag_filter(F) -> str:
        if type(F) == str:
            return F

        if type(F) != list:
            raise NotImplementedError

        return " ".join(F)

    def __init__(self, args):
        config = json.load(open("build.json", mode="r"))

        self.ISDEBUG    = args.debug
        
        self.WORKDIR_ROOT   = ".pymake"
        self.WORKDIR    = self.WORKDIR_ROOT + "/" + ("debug" if self.ISDEBUG else "release") + "/"

        self.CC         = config["cc"]
        self.CXX        = config["cxx"]
        self.LD         = config["ld"]

        self.TARGET     = self.WORKDIR + config["output"]

        if self.ISDEBUG:
            self.TARGET += 'd'

        self.INCLUDE    = config["include"]
        self.SRCDIR     = config["src"]
        self.OBJDIR     = self.WORKDIR + config["objdir"]

        FLAGS       = config["flags"]["debug" if self.ISDEBUG else "default"]

        self.COMMONFLAGS     = BuilderFlags._flag_filter(FLAGS["common"])
        self.CFLAGS          = BuilderFlags._flag_filter(FLAGS["c"])
        self.CXXFLAGS        = BuilderFlags._flag_filter(FLAGS["cpp"])

        self.LDFLAGS        = FLAGS["ld"]

        if self.LDFLAGS == [ ]:
            self.LDFLAGS = ""
        else:
            self.LDFLAGS = "-Wl," + ",".join(self.LDFLAGS)

class Builder:
    def __init__(self, args):
        self.args = args
        self.flags = BuilderFlags(args)

        self.sources = \
            glob.glob(f"{self.flags.SRCDIR}/**/*.c", recursive=True) \
            + glob.glob(f"{self.flags.SRCDIR}/**/*.cpp", recursive=True)

        self.sources = [Source(self.flags, s) for s in self.sources]

    def is_completed(self):
        if not os.path.exists(self.flags.TARGET):
            return False

        mtime = os.path.getmtime(self.flags.TARGET)

        for s in self.sources:
            # updated source file
            if mtime < s.mtime:
                return False

        for h in glob.glob(f"{self.flags.INCLUDE}/**/*.h", recursive=True):
            # updated header file
            if mtime < os.path.getmtime(h):
                return False

        return True

    def compile_all(self):
        self.flags.COMMONFLAGS  += f" -I{self.flags.INCLUDE} "

        if self.is_completed():
            print(f"'{Color::Bold}{Color::Yellow}{self.flags.TARGET}' is up to date.{Color::Default}")
            exit(0)

        # Try compile all sources
        if self.args.J:
            with concurrent.futures.ThreadPoolExecutor() as executor:
                executor.map(Source.compile, self.sources)
        else:
            for src in self.sources:
                src.compile()

    def check_error(self) -> None:
        errcount = 0

        for s in self.sources:
            if s.is_failed:
                errcount += 1

        if errcount == 0:
            return

        xprint(
            Color::Bold,
            Color::Red,
            f"=== detected {Color::Yellow}{errcount}{Color::Red} errors ===\n",
            Color::Default
        )

        for src in self.sources:
            if not src.is_failed:
                continue

            border = "=" * 15
            longborder = "=" * (32 + len(src.path))

            xprint(Color::Bold, Color::White, border, " " + 
                Color::Cyan + src.path + Color::White + " ", border, Color::Default)

            xprint(src.result.stderr)
            xprint(Color::Bold, Color::White, longborder, Color::Default, "\n")

        exit(1)


    # Linking
    def link(self):
        xprint(Color::Bold, Color::White, "\nCREATE ",
            Color::Green, self.flags.TARGET, Color::White, " ...", Color::Default)

        ld_result = \
            subprocess.run(
                [
                    self.flags.LD,
                    self.flags.LDFLAGS,
                    "-pthread",
                    "-o",
                    self.flags.TARGET
                ] + [s.objout for s in self.sources],
                capture_output=True,
                text=True
            )

        if ld_result.returncode != 0:
            xprint(Color::Bold, Color::Red, "\nlink failed:\n", Color::White, "=" * 40, Color::Default)
            print(ld_result.stderr)
            xprint(Color::Bold, Color::White, "=" * 40, Color::Default)

            exit(1)
#        else:
#            print(Color::Bold + Color::White + "\nDone." + Color::Default)

    def clean_up(self):
        os.system(f"rm -rf {self.flags.WORKDIR_ROOT}")

    def build(self):
        os.system(f"mkdir -p {self.flags.OBJDIR}")

        self.compile_all()
        self.check_error()
        self.link()

        return 0

    def main(self):
        if self.args.clean:
            self.clean_up()
            return 0

        if self.args.run:
            if not self.is_completed():
                self.build()

            return os.system(f"./{self.flags.TARGET} {' '.join(self.args.run)}")

        if self.args.install:
            os.system(f"install {self.flags.TARGET} /usr/local/bin")
            return 0

        os.system(f"mkdir -p {self.flags.OBJDIR}")

        self.build()

        print(Color::Bold + Color::White + "\nDone." + Color::Default)

        return 0

def __main__():
    parser = argparse.ArgumentParser()

    parser.add_argument('-c', '--clean', action='store_true')
    parser.add_argument('-d', '--debug', action='store_true')
    parser.add_argument('-J', action='store_true')
    parser.add_argument('--run', nargs='*')
    parser.add_argument('--install', action='store_true')

    return Builder(parser.parse_args()).main()

if __name__ == "__main__":
    exit(__main__())