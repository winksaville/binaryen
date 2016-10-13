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
    if (--limit == 0) return builder.makeUnreachable();
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
    switch (pick()) {
      case 0: return makeBlock(makeInt32());
      case 1: return builder.makeIf(makeInt32(), makeInt32(), makeInt32());
      case 2: return makeLoop(makeInt32());
      default: WASM_UNREACHABLE();
    }
  }

  Expression* makeBlock(WasmType type) {
    Builder builder(*getModule());
    auto* ret = builder.makeBlock();
    ret->name = getNewName();
    controlFlowStack.push_back(ret->name);
    Index size = pick(5) + 1;
    ret->list.resize(size);
    for (Index i = 0; i < size - 1; i++) {
      ret->list[i] = makeNone();
    }
    ret->list[size - 1] = make(type);
    ret->finalize();
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
