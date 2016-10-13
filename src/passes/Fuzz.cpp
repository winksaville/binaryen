/*
 * Copyright 2016 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
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

#include <algorithm>

#include <pass.h>
#include <wasm.h>
#include <wasm-builder.h>
#include <ast_utils.h>

//
// Fuzzes the function bodies, adding random noise by replacing some of them.
// This operation is deterministic.
//

namespace wasm {

using namespace std;

// A limit on how many new nodes to create per function
static const int LIMIT = 1000;

struct Fuzz : public WalkerPass<ControlFlowWalker<Fuzz, UnifiedExpressionVisitor<Fuzz>>> {
  std::mt19937 noise;
  Index limit; // hard limit on how many new nodes to create, to avoid an infinite loop
  NameCollector names;
  Index nextName;

  Fuzz() : noise(42), limit(LIMIT) {}

  void visitExpression(Expression* curr) {
    // with small probability, replace the node with a random one of the same type
    if (chance(5)) {
      replaceCurrent(make(curr->type));
    }
  }

  void doWalkFunction(Function* func) {
    names.scan(func->body);
    nextName = 0;
    walk(func->body);
    assert(controlFlowStack.size() == 0);
  }

private:
  bool chance(int percentage) {
    return (noise() % 100) < percentage;
  }

  bool pick(int max) {
    return noise() % max;
  }

  // random node creators

  Expression* make(WasmType type) {
    Builder builder(*getModule());
    // if we hit the limit, stop making new nodes
    if (--limit == 0) return builder.makeUnreachable();
    // small chance to create an unreachable node no matter the type, it can fit anywhere
    if (chance(5)) return builder.makeUnreachable();
    switch (type) {
      case i32: replaceCurrent(makeInt32()); break;
      case i64: replaceCurrent(makeInt64()); break;
      case f32: replaceCurrent(makeFloat32()); break;
      case f64: replaceCurrent(makeFloat64()); break;
      case none: replaceCurrent(makeNone()); break;
      case unreachable: replaceCurrent(makeUnreachable()); break;
      default: WASM_UNREACHABLE();
    }
  }

  Expression* makeInt32() {
    Builder builder(*getModule());
    if (--limit == 0) return builder.makeUnreachable();
    switch (pick(Expression::NumExpressionIds)) {
      case Expression::Id::InvalidId: return builder.makeUnreachable();
      case Expression::Id::BlockId: return makeBlock(i32);
      case Expression::Id::IfId: return builder.makeIf(makeInt32(), makeInt32(), makeInt32()); // XXX order of operations
      case Expression::Id::LoopId: return makeLoop(i32);
      case Expression::Id::BreakId: return makeBreak(makeInt32(), makeInt32());
      case Expression::Id::SwitchId: return builder.makeUnreachable();
      case Expression::Id::CallId: return makeCall(i32);
      case Expression::Id::CallImportId: return makeCallImport(i32);
      case Expression::Id::CallIndirectId: return makeCallIndirect(i32);
      case Expression::Id::GetLocalId: return "get_local";
      case Expression::Id::SetLocalId: return "set_local";
      case Expression::Id::GetGlobalId: return "get_global";
      case Expression::Id::SetGlobalId: return "set_global";
      case Expression::Id::LoadId: return "load";
      case Expression::Id::StoreId: return "store";
      case Expression::Id::ConstId: return "const";
      case Expression::Id::UnaryId: return "unary";
      case Expression::Id::BinaryId: return "binary";
      case Expression::Id::SelectId: return "select";
      case Expression::Id::DropId: return "drop";
      case Expression::Id::ReturnId: return "return";
      case Expression::Id::HostId: return "host";
      case Expression::Id::NopId: return "nop";
      case Expression::Id::UnreachableId: return builder.makeUnreachable();
      default: WASM_UNREACHABLE();
    }
  }

  Expression* makeBlock(WasmType type) {
    Builder builder(*getModule());
    auto* ret = builder.makeBlock();
    ret->name = getNewName();
    ret->type = type;
    controlFlowStack.push_back(ret->name);
    Index size = pick(5) + 1;
    ret->list.resize(size);
    for (Index i = 0; i < size - 1; i++) {
      ret->list[i] = makeNone();
    }
    ret->list[size - 1] = make(type);
    controlFlowStack.pop_back();
    return ret;
  }

  Expression* makeLoop(WasmType type) {
    Builder builder(*getModule());
    auto name = getNewName();
    controlFlowStack.push_back(ret->name);
    auto* ret = builder.makeLoop(name, make(type));
    controlFlowStack.pop_back();
    return ret;
  }

  Expression* makeBreak(Expression* value, Expression* condition) {
    Builder builder(*getModule());
    auto target = getBreakTarget(value);
    if (target.is()) return builder.makeBreak(target, value, condition);
    return builder.makeUnreachable();
  }

  Expression* makeCall(WasmType type) {
    std::vector<Function*> funcs;
    for (auto& func : getModule()->functions) {
      if (func->result == type) {
        funcs.push_back(func.get());
      }
    }
    Builder builder(*getModule());
    if (funcs.empty()) return builder.makeUnreachable();
    auto* func = funcs[pick(funcs.size())];
    std::vector<Expression*> args;
    for (auto param : func->params) {
      args.push_back(make(param));
    }
    return builder.makeCall(func->name, args, type);
  }

  Expression* makeCallImport(WasmType type) {
    std::vector<Function*> imports;
    for (auto& import : getModule()->imports) {
      if (import->kind != Import::Function) continue;
      if (import->functionType->result == type) {
        imports.push_back(import.get());
      }
    }
    Builder builder(*getModule());
    if (imports.empty()) return builder.makeUnreachable();
    auto* import = imports[pick(imports.size())];
    std::vector<Expression*> args;
    for (auto param : import->functionType->params) {
      args.push_back(make(param));
    }
    return builder.makeCallImport(import->name, args, type);
  }

  Expression* makeCallIndirect(WasmType type) {
    auto& table = getModule()->table;
    Builder builder(*getModule());
    if (!table.exists) return builder.makeUnreachable();
    std::vector<Function*> funcs;
    for (auto& segment : table.segments) {
      for (auto name : segment.data) {
        auto* func = getModule()->getFunction(name);
        if (func->result == type && func->type.is()) {
          funcs.push_back(func.get());
        }
      }
    }
    if (funcs.empty()) return builder.makeUnreachable();
    auto* func = funcs[pick(funcs.size())];
    std::vector<Expression*> args;
    for (auto param : func->params) {
      args.push_back(make(param));
    }
    return builder.makeCallIndirect(func->type, makeInt32(), args, type);
  }

  // helpers

  Name getBreakTarget(Expression* value) {
    // find possible targets (including type, if we are breaking with a value), and pick one
    std::vector<Block*> blocks;
    std::vector<Loop*> loops;
    for (auto* parent : controlFlowStack) {
      if (auto* block = parent->dynCast<Block>()) {
        if (block->name.is() && (value ? value->type == block->type : true)) {
          blocks.push_back(block);
        }
      } else if (auto* loop = parent->dynCast<Loop>()) {
        if (loop->name.is() && !value) {
          loops.push_back(loop);
        }
      }
    }
    if (blocks.empty() && loops.empty()) return Name();
    auto choice = pick(blocks.size() + loops.size());
    if (choice < blocks.size()) return blocks[choice]->name;
    else return loops[choice - blocks.size()]->name;
  }

  Name getNewName() {
    while (1) {
      Name name = Name(std::string("fuzz$") + std::from_string(nextName++));
      if (names.names.count(name) == 0) {
        names.names.insert(name);
        return name;
      }
    }
  }
};

Pass *createFuzzPass() {
  return new Fuzz();
}

} // namespace wasm
