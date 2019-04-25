/*
 * Copyright 2004-2019 Cray Inc.
 * Other additional copyright holders may be indicated within.
 *
 * The entirety of this work is licensed under the Apache License,
 * Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mli.h"
#include <cstring>
#include "library.h"
#include "ModuleSymbol.h"
#include "FnSymbol.h"
#include "beautify.h"
#include "codegen.h"
#include "driver.h"
#include "expr.h"
#include "stlUtil.h"
#include "stringutil.h"
#include <map>
#include <sstream>

const char* gen_mli_marshalling = "chpl_mli_marshalling";
const char* gen_mli_client_bundle = "chpl_mli_client_bundle";
const char* gen_mli_server_bundle = "chpl_mli_server_bundle";
static const char* client_main = "chpl_client.main";
static const char* client_arg = "chpl_client.arg";
static const char* client_res = "chpl_client.res";
static const char* server_arg = "chpl_server.arg";
static const char* server_res = "chpl_server.res";
static const char* marshal_push_prefix = "chpl_mli_mtpush_";
static const char* marshal_pull_prefix = "chpl_mli_mtpull_";
static const char* socket_push_name = "chpl_mli_push";
static const char* socket_pull_name = "chpl_mli_pull";
static const char* scope_begin = "{\n";
static const char* scope_end = "}\n";

class MLIContext {
private:

bool injectDebugPrintlines;
bool separateHeaders;
std::vector<FnSymbol*> exps;
std::vector<FnSymbol*> throws;
std::map<Type*, int64_t> typeMap;
fileinfo fiMarshalling;
fileinfo fiClientBundle;
fileinfo fiServerBundle;
GenInfo* info;

public:

MLIContext(bool injectDebugPrintlines=false, bool separateHeaders=false);
~MLIContext();

void emit(ModuleSymbol* md);
void emit(FnSymbol* fn);
void emitClientPrelude(void);
void emitServerPrelude(void);
void emitMarshalRoutines(void);
void emitServerDispatchRoutine(void);

private:

bool shouldEmit(ModuleSymbol* md);
bool shouldEmit(FnSymbol* fn);
void setOutput(fileinfo* fi);
void setOutputAndWrite(fileinfo* fi, const std::string& gen);
void write(const std::string& code);
int64_t assignUniqueTypeID(Type* t);
void emitClientWrapper(FnSymbol* fn);
void emitServerWrapper(FnSymbol* fn);
bool structContainsOnlyPrimitiveScalars(Type* t);
bool isSupportedType(Type* t);
void verifyPrototype(FnSymbol* fn);
Type* getTypeFromFormal(ArgSymbol* as);
Type* getTypeFromFormal(FnSymbol* fn, int i);
bool typeRequiresAllocation(Type* t);

private:

std::string genMarshalBodyPrimitiveScalar(Type* t, bool out);
std::string genMarshalBodyStringC(Type* t, bool out);
std::string genComment(const char* msg, const char* pfx="");
std::string genNote(const char* msg);
std::string genTodo(const char* msg);
std::string genHeaderInc(const char* header, bool system=false);
std::string genMarshalRoutine(Type* t, bool out);
std::string genMarshalPushRoutine(Type* t);
std::string genMarshalPullRoutine(Type* t);
std::string genServerDispatchSwitch(const std::vector<FnSymbol*>& fns);
std::string genFuncToSetServerGlobals(void);
std::string genDebugPrintCall(FnSymbol* fn, const char* pfx="");
std::string genDebugPrintCall(const char* msg, const char* pfx="");
std::string genFuncNumericID(FnSymbol* fn);
std::string genServerWrapperCall(FnSymbol* fn);
std::string genClientsideRPC(FnSymbol* fn);
std::string genServersideRPC(FnSymbol* fn);
std::string genMarshalCall(const char* s, const char* v, Type* t, bool out);
std::string genMarshalPushCall(const char* s, const char* v, Type* t);
std::string genMarshalPullCall(const char* s, const char* v, Type* t);
std::string genTypeName(Type* t);
std::string genSocketCall(const char* s, const char* v, const char* l,
                          bool out);
std::string genSocketCall(const char* s, const char* v, bool out);
std::string genSocketPushCall(const char* s, const char* v);
std::string genSocketPullCall(const char* s, const char* v);
std::string genAddressOf(const char* var);
std::string genAddressOf(std::string& var);
std::string genSizeof(const char* var);
std::string genSizeof(std::string& var);
std::string genNewDecl(const char* t, const char* n);
std::string genNewDecl(Type* t, const char* n);

};

//
// Generic helper method replacing C++11 std::to_string() method.
//
template <typename T>
std::string str(T value) {
  std::ostringstream tmp;
  tmp << value;
  return tmp.str();
}

//
// This is the main entrypoint for MLI code generation, call this if the
// necessary conditions are met in codegen().
//
void codegenMultiLocaleInteropWrappers(void) {
  Vec<ModuleSymbol*> &mds = allModules;

  // Insert all kinds of debug printlines into generated code for now.
  MLIContext mli(true);

  mli.emitClientPrelude();
  mli.emitServerPrelude();

  forv_Vec(ModuleSymbol, md, mds) {
    mli.emit(md);
  }

  mli.emitMarshalRoutines();
  mli.emitServerDispatchRoutine();

  return;
}

MLIContext::MLIContext(bool injectDebugPrintlines, bool separateHeaders) {

  // Yes, I know this isn't the most optimal way to initialize these!
  this->injectDebugPrintlines = injectDebugPrintlines;
  this->separateHeaders = separateHeaders;

  openCFile(&this->fiMarshalling, gen_mli_marshalling, "c");
  openCFile(&this->fiClientBundle, gen_mli_client_bundle, "c");
  openCFile(&this->fiServerBundle, gen_mli_server_bundle, "c");
  this->info = gGenInfo;

  return;
}

MLIContext::~MLIContext() {

  // Now, turn beautify ON!
  closeCFile(&this->fiMarshalling, true);
  closeCFile(&this->fiClientBundle, true);
  closeCFile(&this->fiServerBundle, true);

  return;
}

bool MLIContext::shouldEmit(ModuleSymbol* md) {
  return (md->modTag != MOD_INTERNAL);
}

bool MLIContext::shouldEmit(FnSymbol* fn) {
  return (fn->hasFlag(FLAG_EXPORT) && not fn->hasFlag(FLAG_GEN_MAIN_FUNC));
}

void MLIContext::emit(ModuleSymbol* md) {
  if (not this->shouldEmit(md)) { return; }

  const std::vector<FnSymbol*> fns = md->getTopLevelFunctions(true);

  for_vector(FnSymbol, fn, fns) {
    if (not this->shouldEmit(fn)) { continue; }

    this->exps.push_back(fn);
    this->emit(fn);
  }

  return;
}

void MLIContext::emit(FnSymbol* fn) {
  if (not this->shouldEmit(fn)) { return; }

  this->verifyPrototype(fn); 
  this->emitClientWrapper(fn);
  this->emitServerWrapper(fn);

  return;
}

void MLIContext::emitClientPrelude(void) {
  std::string gen;

  // TODO: Trying to work around the Travis malloc complaint.
  gen += "#define CHPL_MLI_IS_CLIENT_DEFINED\n";
  gen += this->genHeaderInc("chpl_mli_marshalling.c");
  gen += this->genNote("We use Makefile magic to make this visible!");
  gen += this->genHeaderInc("mli_client_runtime.c");
  gen += "\n";

  this->setOutputAndWrite(&this->fiClientBundle, gen);

  return;
}

std::string MLIContext::genHeaderInc(const char* header, bool system) {
  std::string gen;

  gen += "#include ";
  gen += system ? "<" : "\"";
  gen += header;
  gen += system ? ">" : "\"";
  gen += "\n";

  return gen;
}

std::string MLIContext::genComment(const char* msg, const char* pfx) {
  std::string gen;

  gen += "// ";
  gen += pfx;
  gen += strcmp(pfx, "") ? ": " : "";
  gen += msg;
  gen += "\n";

  return gen;
}

std::string MLIContext::genNote(const char* msg) {
  return this->genComment(msg, "NOTE");
}

std::string MLIContext::genTodo(const char* msg) {
  return this->genComment(msg, "TODO");
}

std::string MLIContext::genFuncToSetServerGlobals(void) {
  std::string gen;

  gen += "void chpl_mli_server_set_conf(void)";
  gen += scope_begin;
  gen += "chpl_server_conf.debug=";
  gen += this->injectDebugPrintlines ? "1" : "0";
  gen += ";\n";
  gen += scope_end;
  
  return gen;
}

void MLIContext::emitServerPrelude(void) {
  std::string gen;

  // TODO: Trying to work around the Travis malloc complaint.
  gen += "#define CHPL_MLI_IS_SERVER_DEFINED\n";
  gen += this->genHeaderInc("chpl_mli_marshalling.c");
  gen += this->genNote("We use Makefile magic to make this visible!");
  gen += this->genHeaderInc("mli_server_runtime.c");
  gen += this->genHeaderInc("_main.c");
  gen += "\n";
  
  // The server will call this function to set globals appropriately.
  gen += this->genFuncToSetServerGlobals();
  gen += "\n";

  this->setOutputAndWrite(&this->fiServerBundle, gen);

  return;
}

void MLIContext::emitMarshalRoutines(void) {
  std::map<Type*, int64_t>::iterator i;
  std::string gen;

  // Bunch of stuff included in generated header as bridge code.
  gen += this->genHeaderInc("stdlib.h", true);
  gen += this->genHeaderInc("stdio.h", true);
  gen += this->genHeaderInc("zmq.h", true);
  gen += this->genHeaderInc("chpl__header.h");
  gen += this->genNote("We use Makefile magic to make this visible!");
  gen += this->genHeaderInc("mli_common_code.c");
  gen += "\n";

  for (i = this->typeMap.begin(); i != this->typeMap.end(); ++i) {
    if (this->injectDebugPrintlines) {
      std::string tpn = this->genTypeName(i->first);
      gen += this->genComment(tpn.c_str());
    }

    gen += this->genMarshalPushRoutine(i->first);
    gen += this->genMarshalPullRoutine(i->first);
  }

  this->setOutputAndWrite(&this->fiMarshalling, gen);

  return;
}

//
//
//
std::string MLIContext::genMarshalBodyPrimitiveScalar(Type* t, bool out) {
  std::string gen;

  // On pack, target buffer is input parameter. On unpack, the temporary.
  const char* target = out ? "obj" : "result";

  // Move the raw bytes of the type to/from the wire.
  gen += "skt_err=";
  gen += this->genSocketCall("skt", target, out);

  // Generate a null frame in the opposite direction for the ACK.
  gen += this->genSocketCall("skt", NULL, not out);

  return gen;
}

//
//
//
std::string MLIContext::genMarshalBodyStringC(Type* t, bool out) {
  const char* target = out ? "obj" : "result";
  std::string gen;

  // Read the string length before pushing.
  if (out) { gen += "bytes = strlen(obj);\n"; }

  // Push/pull string length along with null frame for ACK.
  gen += this->genSocketCall("skt", "bytes", out);
  gen += this->genSocketCall("skt", NULL, not out);

  if (out) {
    // Next push this string across the wire.
    gen += this->genSocketCall("skt", "obj", "bytes", out);
  } else {
    // Allocate buffer for string (include NULL terminator).
    gen += "result = malloc(bytes + 1);\n";
    gen += this->genTodo("Assert should shutdown server or return NULL.");
    gen += "assert(result);\n";
  }

  // Move the string to/from the buffer.
  gen += this->genSocketCall("skt", target, "bytes", out);
  gen += this->genSocketCall("skt", NULL, not out);

  // Null terminate if receiving.
  if (not out) { gen += "result[bytes] = 0;\n"; }

  return gen;
}

std::string MLIContext::genMarshalRoutine(Type* t, bool out) {
  int64_t id = this->assignUniqueTypeID(t);
  std::string gen;

  // Push returns nothing, while pull returns the type being read in.
  if (out) {
    gen += "void ";
  } else {
    gen += this->genTypeName(t);
    gen += " ";
  }

  // Select appropriate prefix for function name based on direction.
  gen += out ? marshal_push_prefix : marshal_pull_prefix;
  gen += str(id);
  gen += "(void* skt";

  // Push routines expect the type as a parameter (named "obj").
  if (out) {
    gen += ",";
    gen += this->genTypeName(t);
    gen += " obj";
  }

  gen += ")";
  gen += scope_begin;

  // Always declare a variable to catch socket errors.
  gen += this->genNewDecl("int", "skt_err");

  // If unpacking, declare a temporary for the return value.
  if (not out) {
    gen += this->genNewDecl(t, "result");
  }

  // If allocating, declare a temporary for amount.
  if (this->typeRequiresAllocation(t)) {
    gen += this->genNewDecl("uint64_t", "bytes");
  }

  // Insert a debug message if appropriate.
  if (this->injectDebugPrintlines) {
    std::string msg;

    msg += out ? "Pushing type: " : "Pulling type: ";
    msg += this->genTypeName(t);

    gen += this->genDebugPrintCall(msg.c_str(), "[RPC]");
  }

  //
  // Handle translation of different type classes here. Note that right now
  // the only things we can translate are primitive scalars and cstrings.
  //
  if (isPrimitiveScalar(t)) {
    gen += this->genMarshalBodyPrimitiveScalar(t, out);
  } else if (t == dtStringC) {
    gen += this->genMarshalBodyStringC(t, out);
  } else {
    USR_FATAL("MLI does not support code generation for type", t);
  }

  // If we are unpacking, return our temporary.
  if (not out) { gen += "return result;\n"; }
  
  gen += scope_end;
  gen += "\n";

  return gen;
}

std::string MLIContext::genMarshalPushRoutine(Type* t) {
  return this->genMarshalRoutine(t, true);
}

std::string MLIContext::genMarshalPullRoutine(Type* t) {
  return this->genMarshalRoutine(t, false);
}

void MLIContext::emitServerDispatchRoutine(void) {
  std::string gen;

  gen += this->genServerDispatchSwitch(this->exps);
  gen += "\n";
  
  this->setOutputAndWrite(&this->fiServerBundle, gen);

  return;
}

void MLIContext::setOutput(fileinfo* fi) {
  this->info->cfile = fi->fptr;
  return;
}

void MLIContext::setOutputAndWrite(fileinfo* fi, const std::string& gen) {
  this->setOutput(fi);
  this->write(gen);
}

void MLIContext::write(const std::string& gen) {
  fprintf(this->info->cfile, "%s", gen.c_str());
  return;
}

//
// We can (as I understand it) cound on Type* being unique across the entire
// symbol table (IE, you'll never encounter two different Type objects that
// both end up describing the same concrete type).
//
int64_t MLIContext::assignUniqueTypeID(Type* t) {
  // Prepare a new ID based on the map size (will never overflow).
  int64_t result = (int64_t) this->typeMap.size();

  if (this->typeMap.find(t) != this->typeMap.end()) {
    result = this->typeMap[t];
  } else {
    this->typeMap[t] = result;
  }

  return result;
}

void MLIContext::emitClientWrapper(FnSymbol* fn) {
  std::string gen;

  this->setOutput(&this->fiClientBundle);
  fn->codegenHeaderC();

  gen += scope_begin;
  gen += this->genClientsideRPC(fn);
  gen += scope_end;
  gen += "\n";

  this->write(gen);
  
  return;
}

void MLIContext::emitServerWrapper(FnSymbol* fn) {
  std::string gen;

  // Big long, silly block of manual code generation.
  gen += this->genComment(toString(fn));
  gen += "int64_t chpl_mli_swrapper_";
  gen += this->genFuncNumericID(fn);
  gen += "(void)";
  gen += scope_begin;

  gen += this->genServersideRPC(fn);
  gen += "return 0;\n";
 
  gen += scope_end;
  gen += "\n";

  this->setOutputAndWrite(&this->fiServerBundle, gen);
 
  return;
}

std::string MLIContext::genDebugPrintCall(FnSymbol* fn, const char* pfx) {
  std::string msg;

  msg += "Calling: ";
  msg += toString(fn);

  return this->genDebugPrintCall(msg.c_str(), pfx);
}

std::string MLIContext::genDebugPrintCall(const char* msg, const char* pfx) {
  std::string gen;

  gen += "printf(\"";

  if (pfx && strcmp(pfx, "")) {
    gen += pfx;
    gen += " ";
  }

  gen += "%%s\\n\", \"";
  gen += msg;
  gen += "\");\n";

  return gen;
}

std::string MLIContext::genFuncNumericID(FnSymbol* fn) {
  return str((int64_t) fn->id);
}

std::string MLIContext::genServerWrapperCall(FnSymbol* fn) {
  std::string gen;

  gen += "chpl_mli_swrapper_";
  gen += this->genFuncNumericID(fn);
  gen += "();\n";

  return gen;
}
  
std::string
MLIContext::genServerDispatchSwitch(const std::vector<FnSymbol*>& fns) {
  std::string gen;

  gen += "int64_t chpl_mli_sdispatch";
  gen += "(int64_t function)";
  gen += scope_begin;
  gen += this->genNewDecl("int", "err");
  gen += "switch (function)";
  gen += scope_begin;

  for_vector(FnSymbol, fn, fns) {
    if (not fn->hasFlag(FLAG_EXPORT) || fn->hasFlag(FLAG_GEN_MAIN_FUNC)) {
      continue;
    }

    gen += "case ";
    gen += this->genFuncNumericID(fn);
    gen += ": ";

    gen += scope_begin;
    
    if (this->injectDebugPrintlines) {
      gen += this->genDebugPrintCall(fn, "[Server]");
    }
    
    gen += "err = ";
    gen += this->genServerWrapperCall(fn);
    gen += scope_end;
    gen += "break;\n";
  }

  gen += "default: return CHPL_MLI_ERROR_NOFUNC; break;\n";
  gen += scope_end;
  gen += "return err;\n";
  gen += scope_end;

  return gen;
}

//
// TODO: This is unused right now (no struct type support yet).
//
bool MLIContext::structContainsOnlyPrimitiveScalars(Type *t) {
  return false;
}

//
// TODO: This filter will change as we support more and more type classes.
//  - [ ] dtStringC
//  - [ ] Chapel `string` type
//  - [ ] ???
//
bool MLIContext::isSupportedType(Type* t) {
  return (isPrimitiveScalar(t));
}

void MLIContext::verifyPrototype(FnSymbol* fn) {

  if (fn->retType != dtVoid && not isSupportedType(fn->retType)) {
    USR_FATAL("MLI does not support code generation for type", fn->retType);
  }

  // Loop through all formals and error gen if a type is unsupported.
  for (int i = 1; i <= fn->numFormals(); i++) {
    ArgSymbol* as = fn->getFormal(i);
    if (isSupportedType(as->type)) { continue; }
    USR_FATAL("MLI does not support code generation for type", as->type);
  }

  return;
}

Type* MLIContext::getTypeFromFormal(ArgSymbol* as) {
  if (as == NULL) { return NULL; }
  return as->type;
}

Type* MLIContext::getTypeFromFormal(FnSymbol* fn, int i) {
  return getTypeFromFormal(fn->getFormal(i));
}

std::string MLIContext::genClientsideRPC(FnSymbol* fn) {
  bool hasVoidReturnType = fn->retType == dtVoid;
  bool hasFormals = fn->numFormals() != 0;
  std::string gen;

  // Declare the unique ID for this function.
  gen += "int64_t id = ";
  gen += this->genFuncNumericID(fn);
  gen += ";\n";

  // Declare a int64 for the server return value.
  gen += this->genNewDecl("int64_t", "st");

  // Declare a temporary for the return value, if necessary.
  if (not hasVoidReturnType) {
    gen += this->genNewDecl(fn->retType, "result");
  }

  if (this->injectDebugPrintlines) {
    gen += this->genDebugPrintCall(fn, "[Client]");
  }

  // Push function to call.
  gen += this->genSocketPushCall(client_main, "id");

  // Pull server confirmation.
  gen += this->genSocketPullCall(client_main, "st");

  // TODO: Handle server errors.
  gen += this->genTodo("Handle server errors.");
  gen += "if (st) { ;;; }\n";


  // If we are void/void, then there's nothing left to do.
  if (hasVoidReturnType and not hasFormals) {
    gen += this->genComment("Routine is void/void!");
    return gen;
  }

  // Issue pack call for each formal.
  for (int i = 1; i <= fn->numFormals(); i++) {
    ArgSymbol* as = fn->getFormal(i);
    Type* t = getTypeFromFormal(as);

    gen += this->genMarshalPushCall(client_arg, as->name, t);
  }

  // Pull and return result if applicable.
  if (not hasVoidReturnType) {
    gen += "result = ";
    gen += this->genMarshalPullCall(client_res, "result", fn->retType);
    gen += "return result;\n";
  }
 
  return gen;
}

std::string MLIContext::genServersideRPC(FnSymbol* fn) {
  std::map<int, std::string> formalTempNames;
  bool hasVoidReturnType = fn->retType == dtVoid;
  bool hasFormals = fn->numFormals() != 0;
  std::string gen;

  // Emit void/void calls immediately, then return.
  if (hasVoidReturnType and not hasFormals) {
    gen += fn->cname;
    gen += "();\n";
    return gen;
  }

  // Declare a temporary for the return value, if necessary.
  if (not hasVoidReturnType) {
    gen += this->genTypeName(fn->retType);
    gen += " result;\n";
  }

  // Declare temporaries, issue unpack call for each formal.
  for (int i = 1; i <= fn->numFormals(); i++) {
    std::string tmp;

    Type* t = this->getTypeFromFormal(fn, i);

    gen += this->genTypeName(t);
    gen += " ";

    // Map temp names to formal indices (shifted down one).
    tmp += "tmp_";
    tmp += str(i - 1);
    formalTempNames[i] = tmp;

    // Emit a unpack call to initialize each temporary.
    gen += tmp;
    gen += "=";
    gen += this->genMarshalPullCall(server_arg, tmp.c_str(), t);
  }

  // Only generate LHS target if necessary.
  if (not hasVoidReturnType) {
    gen += "result=";
  }

  // Make the unwrapped call.
  gen += fn->cname;
  gen += "(";

  // Pass in temporaries as arguments to call.
  if (hasFormals) {
    for (int i = 1; i <= fn->numFormals() - 1; i++) {
      gen += formalTempNames.at(i);
      gen += ",";
    }
    gen += formalTempNames.at(fn->numFormals());
  }

  gen += ");\n";

  // If there is a result, issue a pack call for it.
  if (not hasVoidReturnType) {
    gen += this->genMarshalPushCall(server_res, "result", fn->retType);
  }

  return gen;
}

std::string MLIContext::genMarshalCall(const char* s, const char* v, Type* t,
                                       bool out) {
  std::string gen;
  int64_t id = this->assignUniqueTypeID(t);

  gen += out ? marshal_push_prefix : marshal_pull_prefix;
  gen += str(id);
  gen += "(";
  gen += s;
  
  if (out) {
    gen += ",";
    gen += v;
  }

  gen += ");\n";

  return gen;
}

//
// TODO: These calls pass value types (cheaper to pass pointer).
//
std::string
MLIContext::genMarshalPushCall(const char* s, const char* v, Type* t) {
  return this->genMarshalCall(s, v, t, true);
}

//
// TODO: These calls pass value types (cheaper to pass pointer).
//
std::string
MLIContext::genMarshalPullCall(const char* s, const char* v, Type* t) {
  (void) v;
  return this->genMarshalCall(s, v, t, false);
}

std::string MLIContext::genTypeName(Type* t) {
  return t->codegen().c;
}

std::string
MLIContext::genSocketCall(const char* s, const char* v, const char* l,
                          bool out) {
  std::string gen;

  gen += out ? socket_push_name : socket_pull_name;
  gen += "(";
  gen += s;
  gen += ", ";
  gen += v ? this->genAddressOf(v) : "\"\"";
  gen += ", ";
  gen += l ? l : (v ? this->genSizeof(v) : "0");
  gen += ", 0);\n";

  return gen;
}

std::string
MLIContext::genSocketCall(const char* s, const char* v, bool out) {
  return this->genSocketCall(s, v, NULL, out);
}

std::string MLIContext::genSocketPushCall(const char* s, const char* v) {
  return this->genSocketCall(s, v, true);
}


std::string MLIContext::genSocketPullCall(const char* s, const char* v) {
  return this->genSocketCall(s, v, false);
}

std::string MLIContext::genAddressOf(const char* var) {
  std::string gen;

  gen += "&";
  gen += var;

  return gen;
}

std::string MLIContext::genAddressOf(std::string& var) {
  return this->genAddressOf(var.c_str());
}

std::string MLIContext::genSizeof(const char* var) {
  std::string gen;

  gen += "sizeof(";
  gen += var;
  gen += ")";

  return gen;
}

std::string MLIContext::genSizeof(std::string& var) {
  return this->genAddressOf(var.c_str());
}

bool MLIContext::typeRequiresAllocation(Type* t) {
  return (t == dtStringC);
}

std::string MLIContext::genNewDecl(const char* t, const char* v) {
  std::string gen;

  gen += t;
  gen += " ";
  gen += v;
  gen += ";\n";

  return gen;
}

std::string MLIContext::genNewDecl(Type* t, const char* v) {
  std::string gen = this->genTypeName(t);
  gen = this->genNewDecl(gen.c_str(), v);
  return gen;
}
