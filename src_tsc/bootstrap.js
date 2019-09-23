/* CJS-Like Module Loader */

function Module(path, opt) {
	opt = opt || {};
	this.path = path;
	this.dirpath = getDirpath(path);
	this.exports = either(opt.exports, {});
	this.loaded = false;
	this.cache = either(opt.cache, {});
	this.private = either(opt.private, false);
	this.loadPrivate = either(opt.loadPrivate, true);
	this.scope = assign({}, either(opt.scope, {}));
	this.paths = opt.paths ? opt.paths.slice(0) : [];
}

Module.prototype.require = function(path) {
	if (!(this instanceof Module))
		throw new TypeError("invalid receiver");
	if (!path) 
		throw new TypeError("expected valid path argument");
	var paths = [];
	var isRelativePath = (path[0] == '.' && (startsWith(path, "./") || startsWith(path, "../"))); 
	if (isRelativePath) {
		var resolvedPath = resolvePath(this.dirpath, path);
		var cachedModule = this.__requireFromCache(resolvedPath);
		if (cachedModule)
			return cachedModule;
		paths.push(resolvedPath);
	} else {
		var cachedModule = this.__requireFromCache(path);
		if (cachedModule)
			return cachedModule;
		var moduleName = path + ".js";
		for (var i = 0; i < this.paths.length; ++i) {
			var searchDir = this.paths[i];
			var finalPath = joinPaths(searchDir, moduleName);
			paths.push(finalPath);
		}
	}
	for (var i = 0; i < paths.length; ++i) {
		var modulePath = paths[i];
		var newModule = this.__tryRequireNewModule(modulePath);
		if (newModule) {
			if (isRelativePath)
				this.cache[modulePath] = newModule;
			else
				this.cache[path] = newModule;
			return newModule.exports;
		}
	}
	throw new Error(
		"cannot find specified module '" + path + "'\n" +
		"tried next search paths: " + __repr(paths) 
	);
}

Module.prototype.__requireFromCache = function(path) {
	var cachedModule = this.cache[path];
	if (cachedModule) {
		if (cachedModule.isPrivate && !this.loadPrivate)
			throw new Error("'" + this.path + "'' cannot load private module '" + path + "'");
		return cachedModule.exports;
	}
}

// loads internal modules only!
Module.prototype.__tryRequireNewModule = function(path) {
	if (!__existsInternal(path))
		return;
	var newModule = new Module(path, { 
		cache: this.cache,
		private: this.private,
		loadPrivate: this.loadPrivate,
		scope: this.scope,
		paths: this.paths
	});
	if (endsWith(path, ".json")) {
		var jsonFile = __readInternalFile(path);
		if (!jsonFile)
			return;
		newModule.exports = JSON.parse(jsonFile); 
	} else {
		var newModuleWrapper = __loadInternal(path, 
			assign({
				exports: newModule.exports,
				module:  newModule,
				require: newModule.require.bind(newModule),
				__filename: newModule.path,
				__dirname: newModule.dirpath
			}, this.scope));
		if (!newModuleWrapper) 
			return;
		newModuleWrapper();
	}
	return newModule;
}

Module.prototype.resolve = function(path) {
	if (!(this instanceof Module))
		throw new TypeError("invalid receiver");
	if (!path) 
		throw new TypeError("expected valid path argument");
	return resolvePath(this.dirpath, path);
}

/* Typescript Compiler Ignition */

var commonScope = {
	console: { log: println },
	process: { argv: __scriptArgs }
};
var jsSystem;
var tsObject = {
	set sys (item) {
		if (!jsSystem)
			jsSystem = getJSSystem(commonScope.ts);
	},
	get sys() {
		return jsSystem;
	}
};
// predefine ts variable in scope
Object.defineProperty(commonScope, "ts", {
	enumerable: true,
	configurable: false,
	writable: false,
	value: tsObject
});

var rootModule = new Module(__filename, { 
	hidden: true, 
	loadHidden: true, 
	scope: commonScope,
	cache: {
		// export io bindings, used by JSSystem
		'jshost': new Module('jshost', {
			scope: commonScope,
			exports: {
				scriptArgs: __scriptArgs,
				platform: __platform,
				realpath: __realpath,
				readdir: __readdir,
				stat: __stat,
				utimes: __utimes,
				print: __print,
				readFile: function(path) {
					if (__existsInternal(path))
						return __readInternalFile(path);
					return __readFile(path);
				},
				writeFile: __writeFile,
				remove: __remove,
				mkdir: __mkdir,
				getcwd: __getcwd,
				getenv: __getenv,
				exit: __exit,
				exists: __exists
			}
		})
	}
});

// load js system for tsc
var getJSSystem = rootModule.require("./tsc_jssystem.js");
// compile!
rootModule.require("./tsc.js");

/* Helpers */

function println() {
	for (var i = 0; i < arguments.length; ++i)
		__print(__repr(arguments[i]), "");
	__print('\n');
}

function getDirpath(path) {
	return path.replace(/(\/)*\w+\.\w+$/, "");
}

function either(a, b) {
	return a === undefined ? b : a;
}

function assign(objA) {
	if (!arguments.length)
		return {};
	for (var i = 1; i < arguments.length; ++i) {
		var objB = arguments[i];
		var keys = Object.keys(objB);
		for (var j = 0; j < keys.length; ++j) {
			var descriptor = Object.getOwnPropertyDescriptor(objB, keys[j]);
			Object.defineProperty(objA, keys[j], descriptor);
		}
	}
	return objA;
}

function isTruthy(item) {
	return Boolean(item);
}

function resolvePath(relto, path) {
	if (/^\w/.test(path)) return path;
	var resultedPath = relto.split(/[/\\]/).filter(isTruthy);
	var pathComponents = path.split(/[/\\]/).filter(isTruthy);
	var length = pathComponents.length;
	for (var i = 0; i < length; ++i) {
		if (pathComponents[i] == '..')
			resultedPath.pop();
		else if (pathComponents[i] != '.')
			resultedPath.push(pathComponents[i]);
	}
	return resultedPath.join("/");
}

function joinPaths(path1, path2) {
	var resultedPath = path1.split(/[/\\]/).filter(isTruthy);
	Array.prototype.push.apply(resultedPath, path2.split(/[/\\]/).filter(isTruthy));
	return resultedPath.join("/");
}

function startsWith(str, search, rawPos) {
    var pos = rawPos > 0 ? rawPos | 0 : 0;
    return str.substring(pos, pos + search.length) === search;
}

function endsWith(str, search, this_len) {
	if (this_len === undefined || this_len > str.length) {
		this_len = str.length;
	}
	return str.substring(this_len - search.length, this_len) === search;
};
