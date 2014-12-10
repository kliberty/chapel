/*
 * Copyright 2004-2014 Cray Inc.
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

#include "ParamForLoop.h"

#include "AstVisitor.h"
#include "build.h"
#include "resolution.h"

/************************************ | *************************************
*                                                                           *
* Factory methods for the Parser                                            *
*                                                                           *
************************************* | ************************************/

BlockStmt* ParamForLoop::buildParamForLoop(VarSymbol* indexVar,
                                           Expr*      range,
                                           BlockStmt* stmts)
{
  VarSymbol*   lowVar     = newParamVar();
  VarSymbol*   highVar    = newParamVar();
  VarSymbol*   strideVar  = newParamVar();

  LabelSymbol* breakLabel = new LabelSymbol("_breakLabel");

  CallExpr*    call       = toCallExpr(range);
  Expr*        low        = NULL;
  Expr*        high       = NULL;
  Expr*        stride     = NULL;

  BlockStmt*   outer      = new BlockStmt();

  if (call && call->isNamed("by"))
  {
    stride = call->get(2)->remove();
    call   = toCallExpr(call->get(1));
  }
  else
  {
    stride = new SymExpr(new_IntSymbol(1));
  }

  if (call && call->isNamed("_build_range"))
  {
    low    = call->get(1)->remove();
    high   = call->get(1)->remove();
  }
  else
  {
    USR_FATAL(range, "iterators for param-for-loops must be literal ranges");
  }

  outer->insertAtTail(new DefExpr(indexVar, new_IntSymbol((int64_t) 0)));

  outer->insertAtTail(new DefExpr(lowVar));
  outer->insertAtTail(new CallExpr(PRIM_MOVE, lowVar,    low));

  outer->insertAtTail(new DefExpr(highVar));
  outer->insertAtTail(new CallExpr(PRIM_MOVE, highVar,   high));

  outer->insertAtTail(new DefExpr(strideVar));
  outer->insertAtTail(new CallExpr(PRIM_MOVE, strideVar, stride));

  outer->insertAtTail(new ParamForLoop(indexVar,
                                       lowVar,
                                       highVar,
                                       strideVar,
                                       breakLabel,
                                       stmts));

  outer->insertAtTail(new DefExpr(breakLabel));

  return buildChapelStmt(outer);
}

VarSymbol* ParamForLoop::newParamVar()
{
  VarSymbol* retval = newTemp();

  retval->addFlag(FLAG_MAYBE_PARAM);

  return retval;
}

/************************************ | *************************************
*                                                                           *
* Instance methods                                                          *
*                                                                           *
************************************* | ************************************/

ParamForLoop::ParamForLoop() : LoopStmt(0)
{

}

ParamForLoop::ParamForLoop(VarSymbol*   indexVar,
                           VarSymbol*   lowVar,
                           VarSymbol*   highVar,
                           VarSymbol*   strideVar,
                           LabelSymbol* breakLabel,
                           BlockStmt*   initBody) : LoopStmt(initBody)
{
  mIndexVariable  = indexVar;
  mLowVariable    = lowVar;
  mHighVariable   = highVar;
  mStrideVariable = strideVar;

  breakLabelSet(breakLabel);

  BlockStmt::blockInfoSet(new CallExpr(PRIM_BLOCK_PARAM_LOOP,
                                       indexVar,
                                       lowVar,
                                       highVar,
                                       strideVar));
}

ParamForLoop::~ParamForLoop()
{

}

ParamForLoop* ParamForLoop::copy(SymbolMap* mapRef, bool internal)
{
  SymbolMap     localMap;
  SymbolMap*    map       = (mapRef != 0) ? mapRef : &localMap;
  CallExpr*     blockInfo = paramInfoGet();
  ParamForLoop* retval    = new ParamForLoop();

  retval->astloc        = astloc;
  retval->blockTag      = blockTag;
  retval->breakLabel    = breakLabel;
  retval->continueLabel = continueLabel;

  if (blockInfo != 0)
    retval->BlockStmt::blockInfoSet(blockInfo->copy(map, true));

  for_alist(expr, body)
    retval->insertAtTail(expr->copy(map, true));

  if (internal == false)
    update_symbols(retval, map);

  return retval;
}

bool ParamForLoop::isParamForLoop() const
{
  return true;
}

CallExpr* ParamForLoop::paramInfoGet() const
{
  return BlockStmt::blockInfoGet();
}

CallExpr* ParamForLoop::blockInfoGet() const
{
  printf("Migration: ParamForLoop   %12d Unexpected call to blockInfoGet()\n", id);

  return BlockStmt::blockInfoGet();
}

CallExpr* ParamForLoop::blockInfoSet(CallExpr* expr)
{
  printf("Migration: ParamForLoop   %12d Unexpected call to blockInfoSet()\n", id);

  return BlockStmt::blockInfoSet(expr);
}

BlockStmt* ParamForLoop::copyBody(SymbolMap* map)
{
  BlockStmt* retval = new BlockStmt();

  retval->astloc        = astloc;
  retval->blockTag      = blockTag;

  retval->breakLabelSet   (breakLabel);
   retval->continueLabelSet(continueLabel);

   if (modUses   != 0)
     retval->modUses = modUses->copy(map, true);

   if (byrefVars != 0)
     retval->byrefVars = byrefVars->copy(map, true);

   for_alist(expr, body)
     retval->insertAtTail(expr->copy(map, true));

   update_symbols(retval, map);

   return retval;
}

void ParamForLoop::accept(AstVisitor* visitor)
{
  if (visitor->enterParamForLoop(this) == true) {
    for_alist(next_ast, body)
      next_ast->accept(visitor);

    if (paramInfoGet() != 0)
      paramInfoGet()->accept(visitor);

    if (modUses)
      modUses->accept(visitor);

    if (byrefVars)
      byrefVars->accept(visitor);

    visitor->exitParamForLoop(this);
  }
}

void ParamForLoop::verify()
{
  BlockStmt::verify();

  if (BlockStmt::blockInfoGet() == 0)
    INT_FATAL(this, "ParamForLoop::verify. blockInfo is NULL");

  if (modUses   != 0)
    INT_FATAL(this, "ParamForLoop::verify. modUses   is not NULL");

  if (byrefVars != 0)
    INT_FATAL(this, "ParamForLoop::verify. byrefVars is not NULL");
}

GenRet ParamForLoop::codegen()
{
  GenRet ret;

  INT_FATAL(this, "ParamForLoop::codegen This should be unreachable");

  return ret;
}

//
// The following functions support function resolution.

// The first two functions support a post-order iteration over the AST.
// It is important that the "loop header" is traversed before the body
// could be visited.
//
// The second two functions are used to unroll the body of the loop
// and then replace the loop with a NOP.
//

Expr* ParamForLoop::getFirstExpr()
{
  Expr* retval = 0;

  if (paramInfoGet() != 0)
    retval = paramInfoGet()->getFirstExpr();

  else if (body.head      != 0)
    retval = body.head->getFirstExpr();

  else
    retval = this;

  return retval;
}

Expr* ParamForLoop::getNextExpr(Expr* expr)
{
  Expr* retval = this;

  if (expr == paramInfoGet() && body.head != 0)
    retval = body.head->getFirstExpr();

  return retval;
}

CallExpr* ParamForLoop::foldForResolve()
{
  CallExpr*  loopInfo = paramInfoGet();

  SymExpr*   idxExpr  = toSymExpr(loopInfo->get(1));
  SymExpr*   lse      = toSymExpr(loopInfo->get(2));
  SymExpr*   hse      = toSymExpr(loopInfo->get(3));
  SymExpr*   sse      = toSymExpr(loopInfo->get(4));

  if (!lse || !hse || !sse)
    USR_FATAL(loopInfo, "param for loop must be defined over a param range");

  VarSymbol* lvar    = toVarSymbol(lse->var);
  VarSymbol* hvar    = toVarSymbol(hse->var);
  VarSymbol* svar    = toVarSymbol(sse->var);

  CallExpr*  noop    = new CallExpr(PRIM_NOOP);

  if (!lvar            || !hvar            || !svar)
    USR_FATAL(loopInfo, "param for loop must be defined over a param range");

  if (!lvar->immediate || !hvar->immediate || !svar->immediate)
    USR_FATAL(loopInfo, "param for loop must be defined over a param range");

  Symbol*      idxSym  = idxExpr->var;
  Type*        idxType = indexType();
  IF1_int_type idxSize = (get_width(idxType) == 32) ? INT_SIZE_32 : INT_SIZE_64;

  // Insert an "insertion marker" for loop unrolling
  insertAfter(noop);

  if (is_int_type(idxType)) {
    int64_t low    = lvar->immediate->int_value();
    int64_t high   = hvar->immediate->int_value();
    int64_t stride = svar->immediate->int_value();

    if (stride <= 0) {
      for (int64_t i = high; i >= low; i += stride) {
        SymbolMap map;

        map.put(idxSym, new_IntSymbol(i, idxSize));

        noop->insertBefore(copyBody(&map));
      }
    } else {
      for (int64_t i = low; i <= high; i += stride) {
        SymbolMap map;

        map.put(idxSym, new_IntSymbol(i, idxSize));

        noop->insertBefore(copyBody(&map));
      }
    }
  } else {
    INT_ASSERT(is_uint_type(idxType) || is_bool_type(idxType));

    uint64_t low    = lvar->immediate->uint_value();
    uint64_t high   = hvar->immediate->uint_value();
    int64_t  stride = svar->immediate->int_value();

    if (stride <= 0) {
      for (uint64_t i = high; i >= low; i += stride) {
        SymbolMap map;

        map.put(idxSym, new_UIntSymbol(i, idxSize));

        noop->insertBefore(copyBody(&map));
      }
    } else {
      for (uint64_t i = low; i <= high; i += stride) {
        SymbolMap map;

        map.put(idxSym, new_UIntSymbol(i, idxSize));

        noop->insertBefore(copyBody(&map));
      }
    }
  }

  // Remove the "insertion marker"
  noop->remove();

  // Replace the paramLoop with the NO-OP
  replace(noop);

  return noop;
}

//
// Determine the index type for a ParamForLoop.
//
// This implementation creates a range with low/high values and then
// asks for its type.
//
Type* ParamForLoop::indexType() {
  CallExpr* loopInfo = paramInfoGet();
  SymExpr*  lse      = toSymExpr(loopInfo->get(2));
  SymExpr*  hse      = toSymExpr(loopInfo->get(3));
  CallExpr* range    = new CallExpr("_build_range", lse->copy(), hse->copy());
  Type*     idxType  = 0;

  insertBefore(range);

  resolveCall(range);

  if (FnSymbol* sym = range->isResolved()) {

    resolveFormals(sym);

    DefExpr* formal = toDefExpr(sym->formals.get(1));

    if (toArgSymbol(formal->sym)->typeExpr) {
      idxType = toArgSymbol(formal->sym)->typeExpr->body.tail->typeInfo();
    } else {
      idxType = formal->sym->type;
    }

    range->remove();

  } else {
    INT_FATAL("unresolved range");
  }

  return idxType;
}








