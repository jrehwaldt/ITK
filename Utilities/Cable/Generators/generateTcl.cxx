#include <iostream>
#include <list>
#include <map>
#include <cstdio>

#include "internalRep.h"

#define TCL_DIR_PREFIX      "Tcl/"
#define TCL_DIR_PREFIX_LEN  4

typedef std::map<String, std::vector<Function::Pointer> >  MethodMap;
typedef MethodMap::iterator  MethodMapIterator;

/**
 * Output the code that appears at the top of each generated wrapper file.
 * Some of this is dependent on the class being wrapped.
 */
void outputClassFileHeader(FILE* outFile, Class* aClass)
{
  fprintf(outFile,
          "/**\n"
          " * Automatically generated wrapper.  Do not edit!\n"
          " * Wrapper for class \"%s\".\n"
          " */\n"
          "\n",
          aClass->GetName().c_str());
  fprintf(outFile,
          "#include \"_wrap_Utils.h\"\n"
          "#include <tcl.h>\n"
          "#include <map>\n"
          "#include \"_wrap_TypeInfo.h\"\n"
          "#include \"_wrap_Calls.h\"\n"
          "#include \"_wrap_Wrappers.h\"\n"
          "\n");
  fprintf(outFile,
          "/**\n"
          " * Include type being wrapped.\n"
          " */\n"
          "#include \"%s\"\n"
          "\n",
          aClass->GetLocation()->GetFile().c_str());
  fprintf(outFile,
          "_wrap_NAMESPACE_BEGIN\n"
          "\n",
          aClass->GetName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetName().c_str());  
  
  fprintf(outFile,
          "/**\n"
          " * The method map for %s methods.\n"
          " */\n"
          "std::map<String, void (*)(%s&, Tcl_Interp*, int, Tcl_Obj*CONST*)>\n"
          "methodMap_%s;\n"
          "\n",
          aClass->GetName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetWrapName().c_str());
}


/**
 * Find all the type names need by the given function's prototype.
 */
template <typename OutputIterator>
void addTypeNames(Function* func, OutputIterator o)
{
  Type::Pointer t;
  if(func->GetReturns() && func->GetReturns()->GetType())
    {
    t = func->GetReturns()->GetType();
    *o++ = t->GetName();
    if(t->IsPointerType())
      {
      PointerType::Pointer pt = (PointerType*)t.RealPointer();
      *o++ = pt->GetPointedToType()->GetName();
      }
    else if(t->IsReferenceType())
      {
      ReferenceType::Pointer rt = (ReferenceType*)t.RealPointer();
      *o++ = rt->GetReferencedType()->GetName();
      }
    }
  for(ArgumentsIterator a = func->GetArguments().begin();
      a != func->GetArguments().end();
      ++a)
    {
    t = (*a)->GetType();
    if(t->IsPointerType())
      {
      PointerType::Pointer pt = (PointerType*)t.RealPointer();
      *o++ = pt->GetPointedToType()->GetName();
      }
    else if(t->IsReferenceType())
      {
      ReferenceType::Pointer rt = (ReferenceType*)t.RealPointer();
      *o++ = rt->GetReferencedType()->GetName();
      }
    }
}


/**
 * Output the type name definitions.
 */
template <typename InputIterator>
void outputTypeNames(FILE* outFile, InputIterator first, InputIterator last)
{
  fprintf(outFile,
          "\n");
  
  for(InputIterator t = first; t != last; ++t)
    {
    String type = *t;
    fprintf(outFile,
            "declare_TypeNameOf(%s);\n"
            "define_TypeNameOf(%s);\n",
            type.c_str(),
            type.c_str());
    }
  
  fprintf(outFile,
          "\n");
}


/**
 * Output the code that appears at the bottom of each generated wrapper file.
 * Some of this may be dependent on the class being wrapped.
 */
void outputClassFileFooter(FILE* outFile, Class*)
{
  fprintf(outFile,
          "\n"
          "_wrap_NAMESPACE_END\n"
          "\n");
}

//----------------------------------------------------------------------------
// Begin temporary area.  Code in this area must be changed later to handle
// overload resolution correctly.  Also must be updated to deal with
// default arguments.

/**
 * Output the test inside a method wrapper's if statement for
 * whether the input indends this function to be called.
 */
void outputMethodTest(FILE* outFile, Function* func, const char* indent)
{
  // Make sure argument count is correct.
  fprintf(outFile, "(objc == %d+2)", func->GetArgumentCount());
  // Check the argument types.
  int argNum = 0;
  for(ArgumentsIterator a = func->GetArguments().begin();
      a != func->GetArguments().end(); ++a)
    {
    fprintf(outFile, "\n%s&& ", indent);
    Type::Pointer t = (*a)->GetType();
    if(t->IsPointerType())
      {
      PointerType::Pointer pt = (PointerType*)t.RealPointer();
      String innerType = pt->GetPointedToType()->GetName();
      fprintf(outFile, "ObjectCanBePointerTo<%s>::Test(interp, objv[%d])",
              innerType.c_str(), argNum+2);
      }
    else if(t->IsReferenceType())
      {
      ReferenceType::Pointer rt = (ReferenceType*)t.RealPointer();
      String innerType = rt->GetReferencedType()->GetName();
      fprintf(outFile, "ObjectCanBeReferenceTo<%s>::Test(interp, objv[%d])",
              innerType.c_str(), argNum+2);
      }
    else
      {
      fprintf(outFile, "ObjectCanBe<%s>::Test(interp, objv[%d])",
              t->GetName().c_str(), argNum+2);
      }

    ++argNum;
    }
}


/**
 * Output the actual call to a method in its wrapper.
 */
void outputMethodCall(FILE* outFile, Function* func)
{
  Type::Pointer returnType;
  if(func->GetReturns() && func->GetReturns()->GetType())
    {
    if(func->GetReturns()->GetType()->GetName() != "void")
      returnType = func->GetReturns()->GetType();
    }
  
  fprintf(outFile, "    ");
  if(returnType)
    {
    if(returnType->IsPointerType())
      {
      PointerType::Pointer pt = (PointerType*)returnType.RealPointer();
      String innerType = pt->GetPointedToType()->GetName();
      fprintf(outFile, "ReturnPointerTo<%s>::From(interp,\n      ",
              innerType.c_str());
      }
    else if(returnType->IsReferenceType())
      {
      ReferenceType::Pointer rt = (ReferenceType*)returnType.RealPointer();
      String innerType = rt->GetReferencedType()->GetName();
      fprintf(outFile, "ReturnReferenceTo<%s>::From(interp,\n      ",
              innerType.c_str());
      }
    else
      {
      fprintf(outFile, "ReturnInstanceOf<%s>::From(interp,\n      ",
              returnType->GetName().c_str());
      }
    }

  fprintf(outFile, "instance.%s(",
          func->GetCallName().c_str());

  int argNum = 0;
  for(ArgumentsIterator a = func->GetArguments().begin();
      a != func->GetArguments().end();++a)
    {
    Type::Pointer t = (*a)->GetType();
    if(t->IsPointerType())
      {
      PointerType::Pointer pt = (PointerType*)t.RealPointer();
      String innerType = pt->GetPointedToType()->GetName();
      fprintf(outFile, "ObjectAsPointerTo<%s>::Get(interp, objv[%d])",
              innerType.c_str(), argNum+2);
      }
    else if(t->IsReferenceType())
      {
      ReferenceType::Pointer rt = (ReferenceType*)t.RealPointer();
      String innerType = rt->GetReferencedType()->GetName();
      fprintf(outFile, "ObjectAsReferenceTo<%s>::Get(interp, objv[%d])",
              innerType.c_str(), argNum+2);
      }
    else
      {
      fprintf(outFile, "ObjectAs<%s>::Get(interp, objv[%d])",
              t->GetName().c_str(), argNum+2);
      }

    ++argNum;
    if(argNum != func->GetArgumentCount())
      fprintf(outFile, ",\n        ");
    }
  
  if(returnType)
    {
    fprintf(outFile, "));\n");
    }
  else
    {
    fprintf(outFile, ");\n");
    }
}


/**
 * Wrap a group of methods with the same name.  Code to do
 * parameter-type-based overload resolution must be generated.
 *
 * Dereferencing an input iterator must produce a Function::Pointer.
 */
template <typename InputIterator>
void wrapMethodGroup(FILE* outFile, Class* aClass,
                     InputIterator first, InputIterator last)
{
  if(first == last) return;
  
  InputIterator currentFunctionIterator = first;
  Function::Pointer func = *currentFunctionIterator++;
  
  fprintf(outFile,
          "/**\n"
          " * Wrap %s::%s() methods.\n"
          " */\n",
          aClass->GetName().c_str(), func->GetCallName().c_str());
  fprintf(outFile,
          "void Method_%s__%s(\n"
          "  %s& instance, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])\n"
          "{\n",
          aClass->GetWrapName().c_str(),
          func->GetWrapName().c_str(),
          aClass->GetName().c_str());
  
  fprintf(outFile,
          "  if(");
  outputMethodTest(outFile, func, "     ");
  fprintf(outFile,
          ")\n");
  fprintf(outFile,
          "    {\n");
  outputMethodCall(outFile, func);
  fprintf(outFile,
          "    }\n");
  
  while(currentFunctionIterator != last)
    {
    func = *currentFunctionIterator++;
    
    fprintf(outFile,
            "  else if(");
    outputMethodTest(outFile, func, "          ");
    fprintf(outFile,
            ")\n");
    fprintf(outFile,
            "    {\n");
    outputMethodCall(outFile, func);
    fprintf(outFile,
            "    }\n");
    }
  
  fprintf(outFile,
          "  else\n"
          "    {\n"
          "    throw UnknownMethodError(interp, \"%s\", \"%s\", objv+2, objc-2);\n"
          "    }\n",
          aClass->GetName().c_str(),
          func->GetCallName().c_str());  
  
  fprintf(outFile,
          "}\n"
          "\n");
}

// End temporary area.
//----------------------------------------------------------------------------


/**
 * Generate the code for the main wrapper function of the class.  The function
 * generated will be called by the Tcl interpreter, and is responsible for
 * determining which actual method wrapper to call.
 */
void outputMethodWrapper(FILE* outFile, Class* aClass)
{
  fprintf(outFile,
          "/**\n"
          " * Main wrapper function for class:\n"
          " * %s\n"
          " */\n",
          aClass->GetName().c_str());
  fprintf(outFile,
          "int WrapperFor_%s(\n"
          "  ClientData clientData, Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[])\n"
          "{\n",
          aClass->GetWrapName().c_str());
  fprintf(outFile,
          "  // Get the command name.  This will be either an instance name,\n"
          "  // or the name of type %s.\n"
          "  String commandName = Tcl_GetStringFromObj(objv[0], NULL);\n"
          "  \n"
          "  if(commandName == \"%s\")\n"
          "    {\n"
          "    // The command is the name of type %s.\n"
          "    // Create a new instance.\n"
          "    char* instanceName = Tcl_GetStringFromObj(objv[1], NULL);\n"
          "    \n"
          "    InstanceSet(instanceName, NewObjectOf<%s>::Create(), \"%s\");\n"
          "    Tcl_CreateObjCommand(interp, instanceName, WrapperFor_%s,\n"
          "                         (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);\n"
          "    return TCL_OK;\n"
          "    }\n",
          aClass->GetName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetWrapName().c_str());
  fprintf(outFile,
          "  try\n"
          "    {\n"
          "    // The command is the name of an instance or reference.\n"
          "    String methodName = Tcl_GetStringFromObj(objv[1], NULL);\n"
          "    \n"
          "    if(methodMap_%s.count(methodName) > 0)\n"
          "      {\n"
          "      if(InstanceExists(commandName))\n"
          "        {\n"
          "        methodMap_%s[methodName](InstanceAs<%s>::Get(commandName), interp, objc, objv);\n"
          "        }\n"
          "      else if(ReferenceExists(commandName))\n"
          "        {\n"
          "        methodMap_%s[methodName](ReferenceAs<%s>::Get(commandName), interp, objc, objv);\n"
          "        }\n"
          "      else\n"
          "        {\n"
          "        throw _wrap_UnknownCommandNameException(commandName);\n"
          "        }\n"
          "      }\n"
          "    else\n"
          "      {\n"
          "      throw UnknownMethodError(interp, \"%s\", methodName, objv+2, objc-2);\n"
          "      }\n"
          "    }\n"
          "  catch(String error)\n"
          "    {\n"
          "    ReportErrorMessage(interp, error);\n"
          "    FreeTemporaries(interp, objv, objc);\n"
          "    return TCL_ERROR;\n"
          "    }\n"
          "  \n"
          "  FreeTemporaries(interp, objv, objc);\n"
          "  return TCL_OK;\n"
          "}\n"
          "\n",
          aClass->GetWrapName().c_str(),
          aClass->GetWrapName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetWrapName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetName().c_str());
}


/**
 * Generate wrappers for the given Class, Struct, or Union.
 */
void wrap(Class* aClass)
{
  /**
   * Prepare the output file for this class.
   */
  FILE* outFile;
  char outFileName[256+TCL_DIR_PREFIX_LEN];
  sprintf(outFileName, "%s%s.cxx", TCL_DIR_PREFIX,
          aClass->GetWrapName().c_str());
  outFile = fopen(outFileName, "wt");
  if(!outFile)
    {
    fprintf(stderr, "Error opening output file: %s\n", outFileName);
    exit(1);
    }

  outputClassFileHeader(outFile, aClass);

  std::set<String> typeNames;
  
  /**
   * Find all the class's public methods.
   * They will be put in groups of the same name in the method map.
   */
  MethodMap methodMap;  
  for(MethodsIterator methodItr = aClass->GetMethods().begin();
      methodItr != aClass->GetMethods().end();
      ++methodItr)
    {
    Method::Pointer method = *methodItr;
    if((method->IsMethod() || method->IsOperatorMethod())
       && (method->GetAccess() == Public)
       && !method->IsStatic())
      {
      methodMap[method->GetName()].push_back(method.RealPointer());
      addTypeNames(method, inserter(typeNames, typeNames.begin()));
      }
    }

  outputTypeNames(outFile, typeNames.begin(), typeNames.end());
  
  /**
   * Generate the wrapper functions for each method group.
   */
  for(MethodMapIterator m = methodMap.begin() ; m != methodMap.end() ; ++m)
    {
    wrapMethodGroup(outFile, aClass, m->second.begin(), m->second.end());
    }

  outputMethodWrapper(outFile, aClass);
  
  /**
   * Write out the method map initialization function.
   */
  fprintf(outFile,
          "/**\n"
          " * Initialize %s wrapper.\n"
          " */\n",
          aClass->GetName().c_str());
  
  fprintf(outFile,
          "void InitWrapper_%s(Tcl_Interp* interp)\n"
          "{\n",
          aClass->GetWrapName().c_str());
  
  for(MethodMapIterator m = methodMap.begin() ; m != methodMap.end() ; ++m)
    {
    fprintf(outFile,
            "  methodMap_%s[\"%s\"] = Method_%s__%s;\n",
            aClass->GetWrapName().c_str(),
            m->first.c_str(),
            aClass->GetWrapName().c_str(),
            GetValid_C_Identifier(m->first).c_str());
    }

  fprintf(outFile,
          "  \n"
          "  Tcl_CreateObjCommand(interp, \"%s\", WrapperFor_%s,\n"
          "                       (ClientData)NULL, (Tcl_CmdDeleteProc*)NULL);\n"
          "  \n"
          "  RegisterWrapperFunction(\"%s\", WrapperFor_%s);\n"
          "  RegisterDeleteFunction(\"%s\", OldObjectOf<%s>::Delete);\n",
          aClass->GetName().c_str(),
          aClass->GetWrapName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetWrapName().c_str(),
          aClass->GetName().c_str(),
          aClass->GetName().c_str());
  fprintf(outFile,
          "}\n"
          "\n");

  outputClassFileFooter(outFile, aClass);
  
  /**
   * Done with the output file.
   */
  fclose(outFile);
}


/**
 * Generate TCL wrappers for the classes defined in the given
 * global namespace.
 */
void generateTcl(Namespace* globalNamespace)
{
  for(ClassesIterator classItr = globalNamespace->GetClasses().begin();
      classItr != globalNamespace->GetClasses().end();
      ++classItr)
    {
    wrap(*classItr);
    }
}
