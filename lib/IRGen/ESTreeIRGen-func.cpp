/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "ESTreeIRGen.h"

#include "hermes/IR/Analysis.h"
#include "hermes/IR/IRUtils.h"
#include "hermes/IR/Instrs.h"
#include "llvh/ADT/SmallString.h"

namespace hermes {
namespace irgen {

//===----------------------------------------------------------------------===//
// FunctionContext

FunctionContext::FunctionContext(
    ESTreeIRGen *irGen,
    Function *function,
    sema::FunctionInfo *semInfo)
    : irGen_(irGen),
      semInfo_(semInfo),
      oldContext_(irGen->functionContext_),
      builderSaveState_(irGen->Builder),
      function(function) {
  irGen->functionContext_ = this;

  // Initialize it to LiteralUndefined by default to avoid corner cases.
  this->capturedNewTarget = irGen->Builder.getLiteralUndefined();

  if (semInfo_) {
    // Allocate the label table. Each label definition will be encountered in
    // the AST before it is referenced (because of the nature of JavaScript), at
    // which point we will initialize the GotoLabel structure with basic blocks
    // targets.
    labels_.resize(semInfo_->numLabels);
  }
}

FunctionContext::~FunctionContext() {
  irGen_->functionContext_ = oldContext_;
}

Identifier FunctionContext::genAnonymousLabelName(llvh::StringRef hint) {
  llvh::SmallString<16> buf;
  llvh::raw_svector_ostream nameBuilder{buf};
  nameBuilder << "?anon_" << anonymousLabelCounter++ << "_" << hint;
  return function->getContext().getIdentifier(nameBuilder.str());
}

//===----------------------------------------------------------------------===//
// ESTreeIRGen

void ESTreeIRGen::genFunctionDeclaration(
    ESTree::FunctionDeclarationNode *func) {
  if (func->_async && func->_generator) {
    Builder.getModule()->getContext().getSourceErrorManager().error(
        func->getSourceRange(), Twine("async generators are unsupported"));
    return;
  }

  // Find the name of the function.
  auto *id = llvh::cast<ESTree::IdentifierNode>(func->_id);
  Identifier functionName = Identifier::getFromPointer(id->_name);
  LLVM_DEBUG(llvh::dbgs() << "IRGen function \"" << functionName << "\".\n");

  sema::Decl *decl = getIDDecl(id);
  if (decl->generic) {
    // Skip generics that aren't specialized.
    return;
  }

  Value *funcStorage = resolveIdentifier(id);
  assert(funcStorage && "Function declaration storage must have been resolved");

  auto *newFuncParentScope = curFunction()->curScope->getVariableScope();
  Function *newFunc = func->_async
      ? genAsyncFunction(functionName, func, newFuncParentScope)
      : func->_generator
      ? genGeneratorFunction(functionName, func, newFuncParentScope)
      : genBasicFunction(functionName, func, newFuncParentScope);

  // Store the newly created closure into a frame variable with the same name.
  auto *newClosure =
      Builder.createCreateFunctionInst(curFunction()->curScope, newFunc);

  emitStore(newClosure, funcStorage, true);
}

Value *ESTreeIRGen::genFunctionExpression(
    ESTree::FunctionExpressionNode *FE,
    Identifier nameHint,
    ESTree::Node *superClassNode,
    Function::DefinitionKind functionKind) {
  if (FE->_async && FE->_generator) {
    Builder.getModule()->getContext().getSourceErrorManager().error(
        FE->getSourceRange(), Twine("async generators are unsupported"));
    return Builder.getLiteralUndefined();
  }

  // This is the possibly empty scope containing the function expression name.
  emitScopeDeclarations(FE->getScope());

  auto *id = llvh::cast_or_null<ESTree::IdentifierNode>(FE->_id);
  Identifier originalNameIden =
      id ? Identifier::getFromPointer(id->_name) : nameHint;

  auto *parentScope = curFunction()->curScope->getVariableScope();
  Function *newFunc = FE->_async
      ? genAsyncFunction(originalNameIden, FE, parentScope)
      : FE->_generator ? genGeneratorFunction(originalNameIden, FE, parentScope)
                       : genBasicFunction(
                             originalNameIden,
                             FE,
                             parentScope,
                             superClassNode,
                             /*isGeneratorInnerFunction*/ false,
                             functionKind);

  Value *closure =
      Builder.createCreateFunctionInst(curFunction()->curScope, newFunc);

  if (id)
    emitStore(closure, resolveIdentifier(id), true);

  return closure;
}

Value *ESTreeIRGen::genArrowFunctionExpression(
    ESTree::ArrowFunctionExpressionNode *AF,
    Identifier nameHint) {
  // Check if already compiled.
  if (Value *compiled = findCompiledEntity(AF)) {
    return Builder.createCreateFunctionInst(
        curFunction()->curScope, llvh::cast<Function>(compiled));
  }

  LLVM_DEBUG(
      llvh::dbgs()
      << "Creating arrow function. "
      << Builder.getInsertionBlock()->getParent()->getInternalName() << ".\n");

  if (AF->_async) {
    Builder.getModule()->getContext().getSourceErrorManager().error(
        AF->getSourceRange(), Twine("async functions are unsupported"));
    return Builder.getLiteralUndefined();
  }

  auto *newFunc = Builder.createFunction(
      nameHint,
      Function::DefinitionKind::ES6Arrow,
      ESTree::isStrict(AF->strictness),
      AF->getSemInfo()->customDirectives,
      AF->getSourceRange());

  if (auto *functionType = llvh::dyn_cast<flow::TypedFunctionType>(
          flowContext_.getNodeTypeOrAny(AF)->info)) {
    newFunc->getAttributesRef(Mod).typed = true;
  }

  auto compileFunc = [this,
                      newFunc,
                      AF,
                      capturedThis = curFunction()->capturedThis,
                      capturedNewTarget = curFunction()->capturedNewTarget,
                      capturedArguments = curFunction()->capturedArguments,
                      parentScope =
                          curFunction()->curScope->getVariableScope()] {
    FunctionContext newFunctionContext{this, newFunc, AF->getSemInfo()};

    // Propagate captured "this", "new.target" and "arguments" from parents.
    curFunction()->capturedThis = capturedThis;
    curFunction()->capturedNewTarget = capturedNewTarget;
    curFunction()->capturedArguments = capturedArguments;

    emitFunctionPrologue(
        AF,
        Builder.createBasicBlock(newFunc),
        InitES5CaptureState::No,
        DoEmitDeclarations::Yes,
        parentScope);

    genStatement(AF->_body);
    emitFunctionEpilogue(Builder.getLiteralUndefined());
  };

  enqueueCompilation(AF, ExtraKey::Normal, newFunc, compileFunc);

  // Emit CreateFunctionInst after we have restored the builder state.
  return Builder.createCreateFunctionInst(curFunction()->curScope, newFunc);
}

NormalFunction *ESTreeIRGen::genBasicFunction(
    Identifier originalName,
    ESTree::FunctionLikeNode *functionNode,
    VariableScope *parentScope,
    ESTree::Node *superClassNode,
    bool isGeneratorInnerFunction,
    Function::DefinitionKind functionKind) {
  assert(functionNode && "Function AST cannot be null");

  // Check if already compiled.
  if (Value *compiled = findCompiledEntity(functionNode))
    return llvh::cast<NormalFunction>(compiled);

  auto *body = ESTree::getBlockStatement(functionNode);
  assert(body && "body of ES5 function cannot be null");

  NormalFunction *newFunction = isGeneratorInnerFunction
      ? Builder.createFunction(
            originalName,
            Function::DefinitionKind::GeneratorInner,
            ESTree::isStrict(functionNode->strictness),
            // TODO(T84292546): change to 'Sensitive' once the outer gen fn name
            // is used in the err stack trace instead of the inner gen fn name.
            CustomDirectives{
                .sourceVisibility = SourceVisibility::HideSource,
                .alwaysInline = false},
            functionNode->getSourceRange(),
            /* insertBefore */ nullptr)
      : (Builder.createFunction(
            originalName,
            functionKind,
            ESTree::isStrict(functionNode->strictness),
            functionNode->getSemInfo()->customDirectives,
            functionNode->getSourceRange(),
            /* insertBefore */ nullptr));

  if (auto *functionType = llvh::dyn_cast<flow::TypedFunctionType>(
          flowContext_.getNodeTypeOrAny(functionNode)->info)) {
    newFunction->getAttributesRef(Mod).typed = true;
  }

  auto compileFunc = [this,
                      functionKind,
                      newFunction,
                      functionNode,
                      classNode = curClassNode(),
                      classType = curClassType(),
                      isGeneratorInnerFunction,
                      superClassNode,
                      body,
                      parentScope] {
    FunctionContext newFunctionContext{
        this, newFunction, functionNode->getSemInfo()};
    newFunctionContext.superClassNode_ = superClassNode;
    ClassContext newClassContext{this, classNode, classType};

    if (isGeneratorInnerFunction) {
      // StartGeneratorInst
      // ResumeGeneratorInst
      // at the beginning of the function, to allow for the first .next()
      // call.
      auto *initGenBB = Builder.createBasicBlock(newFunction);
      Builder.setInsertionBlock(initGenBB);
      Builder.createStartGeneratorInst();
      auto *prologueBB = Builder.createBasicBlock(newFunction);
      auto *prologueResumeIsReturn = Builder.createAllocStackInst(
          genAnonymousLabelName("isReturn_prologue"), Type::createBoolean());
      genResumeGenerator(GenFinally::No, prologueResumeIsReturn, prologueBB);

      if (hasSimpleParams(functionNode)) {
        // If there are simple params, then we don't need an extra
        // yield/resume. They can simply be initialized on the first call to
        // `.next`.
        Builder.setInsertionBlock(prologueBB);
        emitFunctionPrologue(
            functionNode,
            prologueBB,
            InitES5CaptureState::Yes,
            DoEmitDeclarations::Yes,
            parentScope);
      } else {
        // If there are non-simple params, then we must add a new
        // yield/resume. The `.next()` call will occur once in the outer
        // function, before the iterator is returned to the caller of the
        // `function*`.
        auto *entryPointBB = Builder.createBasicBlock(newFunction);
        auto *entryPointResumeIsReturn = Builder.createAllocStackInst(
            genAnonymousLabelName("isReturn_entry"), Type::createBoolean());

        // Initialize parameters.
        Builder.setInsertionBlock(prologueBB);
        emitFunctionPrologue(
            functionNode,
            prologueBB,
            InitES5CaptureState::Yes,
            DoEmitDeclarations::Yes,
            parentScope);
        Builder.createSaveAndYieldInst(
            Builder.getLiteralUndefined(),
            Builder.getLiteralBool(false),
            entryPointBB);

        // Actual entry point of function from the caller's perspective.
        Builder.setInsertionBlock(entryPointBB);
        genResumeGenerator(
            GenFinally::No,
            entryPointResumeIsReturn,
            Builder.createBasicBlock(newFunction));
      }
    } else {
      emitFunctionPrologue(
          functionNode,
          Builder.createBasicBlock(newFunction),
          InitES5CaptureState::Yes,
          DoEmitDeclarations::Yes,
          parentScope);
    }

    if (functionKind == Function::DefinitionKind::ES6Constructor) {
      assert(
          classNode && classType &&
          "Class should be set for constructor function.");
      // If we're compiling a constructor with no superclass, emit the
      // field inits at the start.
      if (classNode->_superClass == nullptr) {
        emitFieldInitCall(classType);
      }
    }

    genStatement(body);
    if (functionNode->getSemInfo()->mayReachImplicitReturn) {
      emitFunctionEpilogue(Builder.getLiteralUndefined());
    } else {
      // Don't implicitly return any value.
      emitFunctionEpilogue(nullptr);
    }
  };

  enqueueCompilation(functionNode, ExtraKey::Normal, newFunction, compileFunc);

  return newFunction;
}

Function *ESTreeIRGen::genGeneratorFunction(
    Identifier originalName,
    ESTree::FunctionLikeNode *functionNode,
    VariableScope *parentScope) {
  assert(functionNode && "Function AST cannot be null");

  if (Value *compiled =
          findCompiledEntity(functionNode, ExtraKey::GeneratorOuter))
    return llvh::cast<Function>(compiled);

  if (!Builder.getModule()->getContext().isGeneratorEnabled()) {
    Builder.getModule()->getContext().getSourceErrorManager().error(
        functionNode->getSourceRange(), "generator compilation is disabled");
  }

  // Build the outer function which creates the generator.
  // Does not have an associated source range.
  auto *outerFn = Builder.createGeneratorFunction(
      originalName,
      Function::DefinitionKind::ES5Function,
      ESTree::isStrict(functionNode->strictness),
      functionNode->getSemInfo()->customDirectives,
      functionNode->getSourceRange(),
      /* insertBefore */ nullptr);

  auto compileFunc = [this,
                      outerFn,
                      functionNode,
                      originalName,
                      parentScope]() {
    FunctionContext outerFnContext{this, outerFn, functionNode->getSemInfo()};

    emitFunctionPrologue(
        functionNode,
        Builder.createBasicBlock(outerFn),
        InitES5CaptureState::Yes,
        DoEmitDeclarations::No,
        parentScope);

    // Build the inner function. This must be done in the parentScope since
    // generator functions don't create a scope.
    auto *innerFn = genBasicFunction(
        genAnonymousLabelName(originalName.isValid() ? originalName.str() : ""),
        functionNode,
        parentScope,
        /* classNode */ nullptr,
        true);

    // Generator functions do not create their own scope, so use the parent's
    // scope.
    GetParentScopeInst *parentScopeInst = Builder.createGetParentScopeInst(
        parentScope, curFunction()->function->getParentScopeParam());
    // Create a generator function, which will store the arguments.
    auto *gen = Builder.createCreateGeneratorInst(parentScopeInst, innerFn);

    if (!hasSimpleParams(functionNode)) {
      // If there are non-simple params, step the inner function once to
      // initialize them.
      Value *next = Builder.createLoadPropertyInst(gen, "next");
      Builder.createCallInst(
          next, /* newTarget */ Builder.getLiteralUndefined(), gen, {});
    }

    emitFunctionEpilogue(gen);
  };

  enqueueCompilation(
      functionNode, ExtraKey::GeneratorOuter, outerFn, compileFunc);

  return outerFn;
}

Function *ESTreeIRGen::genAsyncFunction(
    Identifier originalName,
    ESTree::FunctionLikeNode *functionNode,
    VariableScope *parentScope) {
  assert(functionNode && "Function AST cannot be null");

  if (auto *compiled = findCompiledEntity(functionNode, ExtraKey::AsyncOuter))
    return llvh::cast<Function>(compiled);

  if (!Builder.getModule()->getContext().isGeneratorEnabled()) {
    Builder.getModule()->getContext().getSourceErrorManager().error(
        functionNode->getSourceRange(),
        "async function compilation requires enabling generator");
  }

  auto *asyncFn = Builder.createAsyncFunction(
      originalName,
      Function::DefinitionKind::ES5Function,
      ESTree::isStrict(functionNode->strictness),
      functionNode->getSemInfo()->customDirectives,
      functionNode->getSourceRange(),
      /* insertBefore */ nullptr);

  auto compileFunc = [this,
                      asyncFn,
                      functionNode,
                      originalName,
                      parentScope]() {
    FunctionContext asyncFnContext{this, asyncFn, functionNode->getSemInfo()};

    // The outer async function need not emit code for parameters.
    // It would simply delegate `arguments` object down to inner generator.
    // This avoid emitting code e.g. destructuring parameters twice.
    emitFunctionPrologue(
        functionNode,
        Builder.createBasicBlock(asyncFn),
        InitES5CaptureState::Yes,
        DoEmitDeclarations::No,
        parentScope);

    // Build the inner generator. This must be done in the outerFnContext
    // since it's lexically considered a child function.
    auto *gen = genGeneratorFunction(
        genAnonymousLabelName(originalName.isValid() ? originalName.str() : ""),
        functionNode,
        curFunction()->curScope->getVariableScope());

    auto *genClosure =
        Builder.createCreateFunctionInst(curFunction()->curScope, gen);
    auto *thisArg = curFunction()->jsParams[0];
    auto *argumentsList = curFunction()->createArgumentsInst;

    auto *spawnAsyncClosure = Builder.createGetBuiltinClosureInst(
        BuiltinMethod::HermesBuiltin_spawnAsync);

    auto *res = Builder.createCallInst(
        spawnAsyncClosure,
        /* newTarget */ Builder.getLiteralUndefined(),
        /* thisValue */ Builder.getLiteralUndefined(),
        {genClosure, thisArg, argumentsList});

    emitFunctionEpilogue(res);
  };

  enqueueCompilation(functionNode, ExtraKey::AsyncOuter, asyncFn, compileFunc);

  return asyncFn;
}

void ESTreeIRGen::initCaptureStateInES5FunctionHelper() {
  // Capture "this", "new.target" and "arguments" if there are inner arrows.
  if (!curFunction()->getSemInfo()->containsArrowFunctions)
    return;

  auto *scope = curFunction()->curScope->getVariableScope();

  // "this".
  auto *th = Builder.createVariable(
      scope, genAnonymousLabelName("this"), Type::createAnyType());
  curFunction()->capturedThis = th;
  emitStore(curFunction()->jsParams[0], th, true);

  // "new.target".
  curFunction()->capturedNewTarget = Builder.createVariable(
      scope,
      genAnonymousLabelName("new.target"),
      curFunction()->function->getNewTargetParam()->getType());
  emitStore(
      Builder.createGetNewTargetInst(
          curFunction()->function->getNewTargetParam()),
      curFunction()->capturedNewTarget,
      true);

  // "arguments".
  if (curFunction()->getSemInfo()->containsArrowFunctionsUsingArguments) {
    auto *args = Builder.createVariable(
        scope, genAnonymousLabelName("arguments"), Type::createObject());
    curFunction()->capturedArguments = args;
    emitStore(curFunction()->createArgumentsInst, args, true);
  }
}

void ESTreeIRGen::emitFunctionPrologue(
    ESTree::FunctionLikeNode *funcNode,
    BasicBlock *entry,
    InitES5CaptureState doInitES5CaptureState,
    DoEmitDeclarations doEmitDeclarations,
    VariableScope *parentScope) {
  auto *newFunc = curFunction()->function;
  auto *semInfo = curFunction()->getSemInfo();

  Builder.setLocation(newFunc->getSourceRange().Start);

  // Start pumping instructions into the entry basic block.
  Builder.setInsertionBlock(entry);

  // Always insert a CreateArgumentsInst. We will delete it later if it is
  // unused.
  curFunction()->createArgumentsInst = newFunc->isStrictMode()
      ? (CreateArgumentsInst *)Builder.createCreateArgumentsStrictInst()
      : Builder.createCreateArgumentsLooseInst();

  // If "arguments" is declared in the current function, bind it to its value.
  if (semInfo->argumentsDecl.hasValue()) {
    setDeclData(
        semInfo->argumentsDecl.getValue(), curFunction()->createArgumentsInst);
  }

  // Always create the "this" parameter. It needs to be created before we
  // initialized the ES5 capture state.
  JSDynamicParam *thisParam = newFunc->addJSThisParam();
  if (flow::TypedFunctionType *ftype = llvh::dyn_cast<flow::TypedFunctionType>(
          flowContext_.getNodeTypeOrAny(funcNode)->info);
      ftype && ftype->getThisParam()) {
    thisParam->setType(flowTypeToIRType(ftype->getThisParam()));
  }

  // Save the "this" parameter. We will delete it later if unused.
  // In strict mode just use param 0 directly. In non-strict, we must coerce
  // it to an object.
  {
    Instruction *thisVal = Builder.createLoadParamInst(thisParam);
    assert(
        curFunction()->jsParams.empty() &&
        "jsParams must be empty in new function");
    curFunction()->jsParams.push_back(
        newFunc->isStrictMode() ? thisVal
                                : Builder.createCoerceThisNSInst(thisVal));
  }

  // Create the function level scope for this function. If a parent scope is
  // provided, use it, otherwise, this function does not have a lexical parent.
  Value *baseScope;
  if (parentScope) {
    baseScope = Builder.createGetParentScopeInst(
        parentScope, newFunc->getParentScopeParam());
  } else {
    baseScope = Builder.getEmptySentinel();
  }
  // GeneratorFunctions should not have a scope created. It will be created
  // later during a lowering pass.
  if (!llvh::isa<GeneratorFunction>(curFunction()->function)) {
    curFunction()->curScope = Builder.createCreateScopeInst(
        Builder.createVariableScope(parentScope), baseScope);
  }

  if (doInitES5CaptureState != InitES5CaptureState::No)
    initCaptureStateInES5FunctionHelper();

  // Construct the parameter list. Create function parameters and register
  // them in the scope.
  newFunc->setExpectedParamCountIncludingThis(
      countExpectedArgumentsIncludingThis(funcNode));

  if (doEmitDeclarations == DoEmitDeclarations::No)
    return;

  emitParameters(funcNode);
  emitScopeDeclarations(semInfo->getFunctionScope());

  // Generate the code for import declarations before generating the rest of the
  // body.
  for (auto importDecl : semInfo->imports) {
    genImportDeclaration(importDecl);
  }
}

void ESTreeIRGen::emitScopeDeclarations(sema::LexicalScope *scope) {
  if (!scope)
    return;

  for (sema::Decl *decl : scope->decls) {
    Variable *var = nullptr;
    bool init = false;
    switch (decl->kind) {
      case sema::Decl::Kind::Let:
      case sema::Decl::Kind::Const:
      case sema::Decl::Kind::Class:
        assert(
            ((curFunction()->debugAllowRecompileCounter != 0) ||
             (decl->customData == nullptr)) &&
            "customData can be bound only if recompiling AST");

        if (!decl->customData) {
          bool tdz = Mod->getContext().getCodeGenerationSettings().enableTDZ;
          var = Builder.createVariable(
              curFunction()->curScope->getVariableScope(),
              decl->name,
              tdz ? Type::unionTy(Type::createAnyType(), Type::createEmpty())
                  : Type::createAnyType());
          var->setObeysTDZ(tdz);
          var->setIsConst(decl->kind == sema::Decl::Kind::Const);
          setDeclData(decl, var);
        } else {
          var = llvh::cast<Variable>(getDeclData(decl));
        }
        init = true;
        break;

      case sema::Decl::Kind::Var:
        // 'arguments' must have already been bound in the function prologue.
        if (decl->special == sema::Decl::Special::Arguments) {
          assert(
              decl->customData &&
              "'arguments', if it exists, must be bound in the function prologue");
          continue;
        }
      case sema::Decl::Kind::Import:
      case sema::Decl::Kind::ES5Catch:
      case sema::Decl::Kind::FunctionExprName:
      case sema::Decl::Kind::ClassExprName:
      case sema::Decl::Kind::ScopedFunction:
        // NOTE: we are overwriting the decl's customData, even if it is already
        // set. Ordinarily we shouldn't be evaluating the same declarations
        // twice, except when we are compiling the body of a "finally" handler.
        assert(
            ((curFunction()->debugAllowRecompileCounter != 0) ||
             (decl->customData == nullptr)) &&
            "customData can be bound only if recompiling AST");

        if (!decl->customData) {
          var = Builder.createVariable(
              curFunction()->curScope->getVariableScope(),
              decl->name,
              Type::createAnyType());
          var->setIsConst(decl->kind == sema::Decl::Kind::Import);
          setDeclData(decl, var);
        } else {
          var = llvh::cast<Variable>(getDeclData(decl));
        }
        // Var declarations must be initialized to undefined at the beginning
        // of the scope.
        //
        // Loose mode scoped function decls also need to be initialized. In
        // strict mode, the scoped function is declared and created in the
        // same scope, so there is no need to initialize before that. In loose
        // mode however, the declaration may be promoted to function scope while
        // the function creation happens later in the scope. The variable needs
        // to be initialized meanwhile.
        init = decl->kind == sema::Decl::Kind::Var ||
            (decl->kind == sema::Decl::Kind::ScopedFunction &&
             !curFunction()->function->isStrictMode());
        break;

      case sema::Decl::Kind::Parameter:
        // Skip parameters, they are handled separately.
        continue;

      case sema::Decl::Kind::GlobalProperty:
      case sema::Decl::Kind::UndeclaredGlobalProperty: {
        assert(
            ((curFunction()->debugAllowRecompileCounter != 0) ||
             (decl->customData == nullptr)) &&
            "customData can be bound only if recompiling AST");

        if (!decl->customData) {
          bool declared = decl->kind == sema::Decl::Kind::GlobalProperty;
          auto *prop = Builder.createGlobalObjectProperty(decl->name, declared);
          setDeclData(decl, prop);
          if (declared)
            Builder.createDeclareGlobalVarInst(prop->getName());
        }
      } break;
    }

    if (init) {
      assert(var);
      Builder.createStoreFrameInst(
          curFunction()->curScope,
          var->getObeysTDZ() ? (Literal *)Builder.getLiteralEmpty()
                             : (Literal *)Builder.getLiteralUndefined(),
          var);
    }
  }

  // Generate and initialize the code for the hoisted function declarations
  // before generating the rest of the body.
  for (auto funcDecl : scope->hoistedFunctions) {
    genFunctionDeclaration(funcDecl);
  }
}

void ESTreeIRGen::emitParameters(ESTree::FunctionLikeNode *funcNode) {
  auto *newFunc = curFunction()->function;
  sema::FunctionInfo *semInfo = funcNode->getSemInfo();

  LLVM_DEBUG(llvh::dbgs() << "IRGen function parameters.\n");

  // Create a variable for every parameter.
  for (sema::Decl *decl : semInfo->getFunctionScope()->decls) {
    if (decl->kind != sema::Decl::Kind::Parameter)
      break;

    LLVM_DEBUG(llvh::dbgs() << "Adding parameter: " << decl->name << "\n");

    // If not simple parameter list, enable TDZ and init every param.
    bool tdz = !semInfo->simpleParameterList &&
        Mod->getContext().getCodeGenerationSettings().enableTDZ;
    Variable *var = Builder.createVariable(
        curFunction()->curScope->getVariableScope(),
        decl->name,
        tdz ? Type::unionTy(Type::createAnyType(), Type::createEmpty())
            : Type::createAnyType());
    setDeclData(decl, var);

    // If not simple parameter list, enable TDZ and init every param.
    if (!semInfo->simpleParameterList) {
      var->setObeysTDZ(tdz);
      Builder.createStoreFrameInst(
          curFunction()->curScope,
          tdz ? (Literal *)Builder.getLiteralEmpty()
              : (Literal *)Builder.getLiteralUndefined(),
          var);
    }
  }

  uint32_t paramIndex = uint32_t{0} - 1;
  for (auto &elem : ESTree::getParams(funcNode)) {
    ESTree::Node *param = &elem;
    ESTree::Node *init = nullptr;
    ++paramIndex;

    if (auto *rest = llvh::dyn_cast<ESTree::RestElementNode>(param)) {
      createLRef(rest->_argument, true)
          .emitStore(genBuiltinCall(
              BuiltinMethod::HermesBuiltin_copyRestArgs,
              Builder.getLiteralNumber(paramIndex)));
      break;
    }

    // Unpack the optional initialization.
    if (auto *assign = llvh::dyn_cast<ESTree::AssignmentPatternNode>(param)) {
      param = assign->_left;
      init = assign->_right;
    }

    Identifier formalParamName = llvh::isa<ESTree::IdentifierNode>(param)
        ? getNameFieldFromID(param)
        : genAnonymousLabelName("param");

    size_t jsParamIndex = newFunc->getJSDynamicParams().size();
    if (jsParamIndex > UINT32_MAX) {
      Mod->getContext().getSourceErrorManager().error(
          param->getSourceRange(), "too many parameters");
      break;
    }
    auto *jsParam = newFunc->addJSDynamicParam(formalParamName);
    if (flow::TypedFunctionType *ftype =
            llvh::dyn_cast<flow::TypedFunctionType>(
                flowContext_.getNodeTypeOrAny(funcNode)->info);
        ftype && paramIndex < ftype->getParams().size()) {
      jsParam->setType(flowTypeToIRType(ftype->getParams()[paramIndex].second));
    }
    Instruction *formalParam = Builder.createLoadParamInst(jsParam);
    curFunction()->jsParams.push_back(formalParam);
    createLRef(param, true)
        .emitStore(
            emitOptionalInitialization(formalParam, init, formalParamName));
  }
}

uint32_t ESTreeIRGen::countExpectedArgumentsIncludingThis(
    ESTree::FunctionLikeNode *funcNode) {
  // Start at 1 to account for "this".
  uint32_t count = 1;
  // Implicit functions, whose funcNode is null, take no arguments.
  if (funcNode) {
    for (auto &param : ESTree::getParams(funcNode)) {
      if (llvh::isa<ESTree::AssignmentPatternNode>(param)) {
        // Found an initializer, stop counting expected arguments.
        break;
      }
      ++count;
    }
  }
  return count;
}

void ESTreeIRGen::emitFunctionEpilogue(Value *returnValue) {
  Builder.setLocation(SourceErrorManager::convertEndToLocation(
      Builder.getFunction()->getSourceRange()));
  if (returnValue) {
    Builder.createReturnInst(returnValue);
  } else {
    Builder.createUnreachableInst();
  }

  // Delete CreateArgumentsInst if it is unused.
  if (!curFunction()->createArgumentsInst->hasUsers())
    curFunction()->createArgumentsInst->eraseFromParent();

  // Delete the load of "this" if unused.
  if (!curFunction()->jsParams.empty()) {
    Instruction *I = curFunction()->jsParams[0];
    if (!I->hasUsers()) {
      // If the instruction is CoerceThisNSInst, we may have to delete its
      // operand too.
      Instruction *load = nullptr;
      if (auto *CT = llvh::dyn_cast<CoerceThisNSInst>(I)) {
        load = llvh::dyn_cast<Instruction>(CT->getSingleOperand());
      }
      I->eraseFromParent();
      if (load && !load->hasUsers())
        load->eraseFromParent();
    }
  }

  curFunction()->function->clearStatementCount();

  onCompiledFunction(curFunction()->function);
}

Function *ESTreeIRGen::genFieldInitFunction() {
  ESTree::ClassDeclarationNode *classNode = curClass()->getClassNode();
  sema::FunctionInfo *initFuncInfo =
      ESTree::getDecoration<ESTree::ClassLikeDecoration>(classNode)
          ->fieldInitFunctionInfo;
  if (initFuncInfo == nullptr) {
    return nullptr;
  }

  auto initFunc = llvh::cast<Function>(Builder.createFunction(
      (llvh::Twine("<instance_members_initializer:") +
       curClass()->getClassType()->getClassName().str() + ">")
          .str(),
      Function::DefinitionKind::ES5Function,
      true /*strictMode*/));

  auto compileFunc = [this,
                      initFunc,
                      initFuncInfo,
                      classNode = classNode,
                      classType = curClass()->getClassType(),
                      parentScope =
                          curFunction()->curScope->getVariableScope()] {
    FunctionContext newFunctionContext{this, initFunc, initFuncInfo};
    ClassContext newClassContext{this, classNode, classType};

    auto *prologueBB = Builder.createBasicBlock(initFunc);
    Builder.setInsertionBlock(prologueBB);

    emitFunctionPrologue(
        nullptr,
        prologueBB,
        InitES5CaptureState::No,
        DoEmitDeclarations::No,
        parentScope);

    auto *classBody = llvh::dyn_cast<ESTree::ClassBodyNode>(classNode->_body);
    for (ESTree::Node &it : classBody->_body) {
      if (auto *prop = llvh::dyn_cast<ESTree::ClassPropertyNode>(&it)) {
        if (prop->_value) {
          Value *value = genExpression(prop->_value);
          emitFieldStore(classType, prop->_key, genThisExpression(), value);
        }
      }
    }

    emitFunctionEpilogue(Builder.getLiteralUndefined());
    initFunc->setReturnType(Type::createUndefined());
  };
  enqueueCompilation(
      classNode, ExtraKey::ImplicitFieldInitializer, initFunc, compileFunc);
  return initFunc;
}

void ESTreeIRGen::emitCreateFieldInitFunction() {
  Function *initFunc = genFieldInitFunction();
  if (initFunc == nullptr) {
    return;
  }

  ClassFieldInitInfo &classInfo =
      classFieldInitInfo_[curClass()->getClassType()];

  classInfo.fieldInitFunction = initFunc;

  CreateFunctionInst *createFieldInitFunc =
      Builder.createCreateFunctionInst(curFunction()->curScope, initFunc);
  Variable *fieldInitFuncVar = Builder.createVariable(
      curFunction()->curScope->getVariableScope(),
      (llvh::Twine("<fieldInitFuncVar:") +
       curClass()->getClassType()->getClassName().str() + ">"),
      Type::createObject());
  Builder.createStoreFrameInst(
      curFunction()->curScope, createFieldInitFunc, fieldInitFuncVar);
  classInfo.fieldInitFunctionVar = fieldInitFuncVar;
}

void ESTreeIRGen::emitFieldInitCall(flow::ClassType *classType) {
  auto iter = classFieldInitInfo_.find(classType);
  if (iter == classFieldInitInfo_.end()) {
    return;
  }
  Function *fieldInitFunc = iter->second.fieldInitFunction;
  Variable *fieldInitFuncVar = iter->second.fieldInitFunctionVar;
  assert(
      fieldInitFuncVar &&
      "If entry is in classFieldInitInfo_, var should be set");
  auto *scope = emitResolveScopeInstIfNeeded(fieldInitFuncVar->getParent());
  Value *funcVal = Builder.createLoadFrameInst(scope, fieldInitFuncVar);

  funcVal->setType(Type::createObject());
  Builder
      .createCallInst(
          funcVal,
          fieldInitFunc,
          Builder.getEmptySentinel(),
          /*newTarget*/ Builder.getLiteralUndefined(),
          genThisExpression(),
          /*args*/ {})
      ->setType(Type::createUndefined());
}

void ESTreeIRGen::genDummyFunction(Function *dummy) {
  IRBuilder builder{dummy};

  dummy->addJSThisParam();
  BasicBlock *firstBlock = builder.createBasicBlock(dummy);
  builder.setInsertionBlock(firstBlock);
  builder.createUnreachableInst();
}

/// Generate a function which immediately throws the specified SyntaxError
/// message.
Function *ESTreeIRGen::genSyntaxErrorFunction(
    Module *M,
    Identifier originalName,
    SMRange sourceRange,
    llvh::StringRef error) {
  IRBuilder::SaveRestore saveRestore(Builder);

  Function *function = Builder.createFunction(
      originalName,
      Function::DefinitionKind::ES5Function,
      true,
      CustomDirectives{
          .sourceVisibility = SourceVisibility::Sensitive,
          .alwaysInline = false,
      },
      sourceRange);

  function->addJSThisParam();
  BasicBlock *firstBlock = Builder.createBasicBlock(function);
  Builder.setInsertionBlock(firstBlock);

  Builder.createThrowInst(Builder.createCallInst(
      emitLoad(Builder.createGlobalObjectProperty("SyntaxError", false), false),
      /* newTarget */ Builder.getLiteralUndefined(),
      /* thisValue */ Builder.getLiteralUndefined(),
      Builder.getLiteralString(error)));

  return function;
}

void ESTreeIRGen::onCompiledFunction(hermes::Function *F) {
  // Delete any unreachable blocks produced while emitting this function.
  deleteUnreachableBasicBlocks(curFunction()->function);

  fixupCatchTargets(F);
}

} // namespace irgen
} // namespace hermes
