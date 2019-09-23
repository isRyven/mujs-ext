var JSHost = require("jshost");

function getJSSystem(ts) {
    var executable_path = JSHost.scriptArgs[0];
    var args = JSHost.scriptArgs.slice(1);
    var useCaseSensitiveFileNames;
    var newLine;
    var realpath = function(path) {
        return JSHost.realpath(path);
    };
    if (JSHost.platform === "win32") {
        newLine = "\r\n";
        useCaseSensitiveFileNames = false;
    } else {
        newLine = "\n";
        useCaseSensitiveFileNames = true;
    }
    function getAccessibleFileSystemEntries(path) {
        try {
            var entries = JSHost.readdir(path || ".");
        } catch (err) {
            return ts.emptyFileSystemEntries;
        }
        entries = entries.sort();
        var files = [];
        var directories = [];
        for (var i = 0; i < entries.length; i++) {
            var entry = entries[i];
            if (entry === "." || entry === "..") {
                continue;
            }
            var name = ts.combinePaths(path, entry);
            try {
                var st = JSHost.stat(name);
            } catch (err) {
                continue;
            }
            if (st.isFile) {
                files.push(entry);
            } else if (st.isDirectory) {
                directories.push(entry);
            }
        }
        return { files: files, directories: directories };
    }
    function getDirectories(path) {
        try {
            var entries = JSHost.readdir(path);
        } catch (err) {
            return [];
        }
        var directories = [];
        for (var i = 0; i < entries.length; i++) {
            var entry = entries[i];
            var name = ts.combinePaths(path, entry);
            try {
                var st = JSHost.stat(name);
            } catch (err) {
                continue;
            }
            if (st.isDirectory) {
                directories.push(entry);
            }
        }
        return directories;
    }
    function directoryExists(path) {
        return JSHost.exists(path, 1);
    }
    return {
        newLine: newLine,
        args: args,
        useCaseSensitiveFileNames: useCaseSensitiveFileNames,
        write: JSHost.print,
        readFile: JSHost.readFile,
        writeFile: JSHost.writeFile,
        resolvePath: function (s) { return s; },
        fileExists: JSHost.exists,
        deleteFile: JSHost.remove,
        getModifiedTime: function(path) {
            var st = JSHost.stat(path);
            return st.mtime;
        },
        setModifiedTime: function(path, time) {
            JSHost.utimes(path, time, time);
        },
        directoryExists: directoryExists,
        createDirectory: function (dirpath) {
            if (!directoryExists(dirpath))
                JSHost.mkdir(dirpath);
        },
        getExecutingFilePath: function () {
            return executable_path;
        },
        getCurrentDirectory: JSHost.getcwd,
        getDirectories: getDirectories,
        getEnvironmentVariable: JSHost.getenv,
        readDirectory: function (path, extensions, excludes, includes, depth) {
            var cwd = JSHost.getcwd();
            return ts.matchFiles(path, extensions, excludes, includes, useCaseSensitiveFileNames, cwd, depth, getAccessibleFileSystemEntries, realpath);
        },
        exit: JSHost.exit,
        realpath: realpath,
    };
}

module.exports = getJSSystem;
