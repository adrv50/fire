import os
import sys
import glob
import subprocess
import json
import argparse

import concurrent.futures

COL_DEFAULT     = "\033[0m"
COL_BOLD        = "\033[1m"

COL_BLACK       = "\033[30m"
COL_RED         = "\033[31m"
COL_GREEN       = "\033[32m"
COL_YELLOW      = "\033[33m"
COL_BLUE        = "\033[34m"
COL_MAGENTA     = "\033[35m"
COL_CYAN        = "\033[36;5m"
COL_WHITE       = "\033[37m"

COL_BK_BLACK    = "\033[40m"
COL_BK_RED      = "\033[41m"
COL_BK_GREEN    = "\033[42m"
COL_BK_YELLOW   = "\033[43m"
COL_BK_BLUE     = "\033[44m"
COL_BK_MAGENTA  = "\033[45m"
COL_BK_CYAN     = "\033[46;5m"
COL_BK_WHITE    = "\033[47m"

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

        self.mtime = 0 if not os.path.exists(self.objout) \
            else os.path.getmtime(self.objout)

        self.depends = self.get_depends()

        # print(f"{self.path}, {self.depends}")

        # for result
        self.is_failed = False
        self.result = None

        self.is_compile_needed = \
            not os.path.exists(self.objout) or self.depends == [ ]

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
        if not self.is_compile_needed:
            for d in self.depends:
                if os.path.getmtime(d) > self.mtime:
                    self.is_compile_needed = True
                    break

        if not self.is_compile_needed:
            return False

        is_cpp = self.ext != 'c'

        print(f"{COL_BOLD}{COL_WHITE}{'CXX ' if is_cpp else 'CC  '} {self.path}{COL_DEFAULT}")
        
        self.result = subprocess.run(
            f"{'clang++' if is_cpp else 'clang'} -MP -MMD -MF {self.dpath} {self.flags.COMMONFLAGS} {self.flags.CXXFLAGS if is_cpp else self.flags.CFLAGS} -c -o {self.objout} {self.path}",
            shell=True,
            capture_output=True,
            text=True
        )

        self.is_failed = (self.result.returncode != 0)

        if self.is_failed:
            print(COL_BOLD + COL_RED, "failed: ", self.path, COL_DEFAULT)

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

        self.sources = [ ]
        self.err_sources = [ ]

    def is_completed(self, target: str, incldir: str, sources: list[Source]):
        if not os.path.exists(target):
            return False

        mtime = os.path.getmtime(target)
        headers = glob.glob(f"{incldir}/**/*.h", recursive=True)

        for s in sources:
            if mtime < s.mtime:
                return False

        for h in headers:
            if mtime < os.path.getmtime(h):
                return False

        return True

    def compile_all(self):
        self.flags.COMMONFLAGS  += f" -I{self.flags.INCLUDE} "

        self.sources = glob.glob(f"{self.flags.SRCDIR}/**/*.c", recursive=True)
        self.sources += glob.glob(f"{self.flags.SRCDIR}/**/*.cpp", recursive=True)
        self.sources = [Source(self.flags, s) for s in self.sources]

        if self.is_completed(self.flags.TARGET, self.flags.INCLUDE, self.sources):
            print(f"'{COL_BOLD}{COL_YELLOW}{self.flags.TARGET}' is up to date.{COL_DEFAULT}")
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
            COL_BOLD,
            COL_RED,
            f"=== detected {COL_YELLOW}{errcount}{COL_RED} errors ===\n",
            COL_DEFAULT
        )

        for src in self.sources:
            if not src.is_failed:
                continue

            border = "=" * 15
            longborder = "=" * (32 + len(src.path))

            xprint(COL_BOLD, COL_WHITE, border, " " + 
                COL_CYAN + src.path + COL_WHITE + " ", border, COL_DEFAULT)

            xprint(src.result.stderr)
            xprint(COL_BOLD, COL_WHITE, longborder, COL_DEFAULT, "\n")

        exit(1)


    # Linking
    def link(self):
        xprint(COL_BOLD, COL_WHITE, "\nCREATE ",
            COL_GREEN, self.flags.TARGET, COL_WHITE, " ...", COL_DEFAULT)

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
            xprint(COL_BOLD, COL_RED, "\nlink failed:\n", COL_WHITE, "=" * 40, COL_DEFAULT)
            print(ld_result.stderr)
            xprint(COL_BOLD, COL_WHITE, "=" * 40, COL_DEFAULT)

            exit(1)
        else:
            print(COL_BOLD + COL_WHITE + "\nDone." + COL_DEFAULT)

    def clean_up(self):
        os.system(f"rm -rf {self.flags.WORKDIR_ROOT}")

    def do_build(self):
        if self.args.clean:
            self.clean_up()
            return 0

        if self.args.run:
            os.system("clear")
            cmd = f"./{self.flags.TARGET} {' '.join(self.args.run)}"
            print(f"{COL_GREEN}{cmd}\n{'='*50}{COL_DEFAULT}")
            return os.system(cmd)

        os.system(f"mkdir -p {self.flags.OBJDIR}")

        self.compile_all()
        self.check_error()

        self.link()

        return 0

def __main__():
    parser = argparse.ArgumentParser()

    parser.add_argument('-c', '--clean', action='store_true')
    parser.add_argument('-d', '--debug', action='store_true')
    parser.add_argument('-J', action='store_true')
    parser.add_argument('--run', nargs='*')

    return Builder(parser.parse_args()).do_build()

if __name__ == "__main__":
    exit(__main__())