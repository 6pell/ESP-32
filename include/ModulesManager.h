#pragma once
#include <initializer_list>
#include "Module.h"
#include "SimpleMap.h"

/**
 * A manager of modules: logical components that could be added, removed, enabled
 * and disabled in runtime. Modules are identified by it's name (string). The
 * module name is limited by str_switch library (9 characters).
 *
 * https://github.com/spacehuhn/SimpleMap
 */

typedef std::function<void(Module*)> ModuleCallback;

class ModulesManager {
private:
  SimpleMap<String, Module*> *modules;

public:
  ModulesManager();

  void add( const String& moduleId );
  void add( std::initializer_list<String> moduleIds );
  int  count()  { return modules->size(); }
  void execute( const String& moduleId, ModuleCallback callback );
  Module* get( const String& moduleId );
  Module* get( int index )  { return modules->getData(index); }
  bool isCustomModule( const String& moduleId );
  void iterator( ModuleCallback callback );
  void remove( const String& moduleId );

  void dispatchCommand( const String& command );

private:
  static Module* create( const String& moduleId );
};

extern ModulesManager Modules;
