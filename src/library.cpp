
#include "library.h"
#include "parser/parser.h"
#include "jit.h"
#include "value.h"
#include "interpreter.h"

#include <iostream>
#include <dirent.h>
#include <fstream>
#include <sys/stat.h>
#include <dlfcn.h>

void sourceFile(Thread& thread, std::string name) {
	//std::cout << "Sourcing " << name << std::endl;
	try {
		std::ifstream t(name.c_str());
		std::stringstream buffer;
		buffer << t.rdbuf();
		std::string code = buffer.str();

		Parser parser(thread.state);
		Value value;
		FILE* trace = NULL;//fopen((name+"_trace").c_str(), "w");
		parser.execute(code.c_str(), code.length(), true, value, trace);
		//fclose(trace);	
		thread.continueEval(JITCompiler::compile(thread, value));
	} catch(RiposteError& error) {
		_warning(thread, "unable to load library " + name + ": " + error.what().c_str());
	} catch(RuntimeError& error) {
		_warning(thread, "unable to load library " + name + ": " + error.what().c_str());
	} catch(CompileError& error) {
		_warning(thread, "unable to load library " + name + ": " + error.what().c_str());
	}
}

void openDynamic(Thread& thread, std::string path) {
	//std::string p = std::string("/Users/jtalbot/riposte/")+path;
	//void* lib = dlopen(p.c_str(), RTLD_LAZY);
	//if(lib == NULL) {
	//	_error(std::string("failed to open: ") + p + " (" + dlerror() + ")");
	//}
	// set lib in env...
}

void loadLibrary(Thread& thread, std::string path, std::string name) {
	thread.beginEval(thread.state.path.back(), 0);
	
	std::string p = path + "/" + name + ("/R/");

	dirent* file;
	struct stat info;

	// Load R files
	DIR* dir = opendir(p.c_str());
	if(dir != NULL) {
		while((file=readdir(dir))) {
			if(file->d_name[0] != '.') {
				stat(file->d_name, &info);
				std::string name = file->d_name;
				if(!S_ISDIR(info.st_mode) && 
						(name.length()>2 && name.substr(name.length()-2,2)==".R")) {
					sourceFile(thread, p+name);
				}
			}
		}
		closedir(dir);
	}

	// Load dynamic libraries
	p = path + "/" + name + "/libs/";
	dir = opendir(p.c_str());
	if(dir != NULL) {
		while((file=readdir(dir))) {
			if(file->d_name[0] != '.') {
				stat(file->d_name, &info);
				std::string name = file->d_name;
				if(!S_ISDIR(info.st_mode) && 
						(name.length()>2 && name.substr(name.length()-3,3)==".so")) {
					openDynamic(thread, p+name);
				}
			}
		}
		closedir(dir);
	}

	Environment* env = thread.endEval(true);	
	thread.state.path.push_back(env);
	thread.state.global->lexical = env;
}

