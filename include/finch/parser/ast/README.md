# CMake AST Design

This directory contains the Abstract Syntax Tree (AST) implementation for the Finch CMake parser.

## Overview

The AST is designed with the following principles:

- **Immutable nodes** - Once constructed, AST nodes cannot be modified
- **Visitor pattern** - For extensible traversal and analysis
- **Source location tracking** - Every node carries its source location for error reporting
- **String interning** - Command names, identifiers, and other strings are interned for performance
- **Error recovery** - ErrorNode type allows partial AST construction even with parse errors
- **Performance optimized** - Generator expressions kept as opaque strings, macros unexpanded

## Node Types

### Base (`node.hpp`)

- `ASTNode` - Abstract base class for all nodes
- `StringInterner` - String interning for memory efficiency
- `ErrorNode` - Error recovery node

### Literals (`literals.hpp`)

- `StringLiteral` - Quoted and unquoted strings
- `NumberLiteral` - Integer and floating-point numbers
- `BooleanLiteral` - TRUE/FALSE/ON/OFF/YES/NO
- `Variable` - Variable references ${VAR}, $ENV{VAR}, $CACHE{VAR}
- `Identifier` - Command names, function names, etc.

### Commands (`commands.hpp`)

- `CommandCall` - Generic command invocation (add_library, set, etc.)
- `FunctionDef` - function() definitions
- `MacroDef` - macro() definitions (kept unexpanded for performance)

### Control Flow (`control_flow.hpp`)

- `IfStatement` - if/elseif/else chains
- `ForEachStatement` - foreach loops with multiple loop types
- `WhileStatement` - while loops
- `ElseIfStatement` - Intermediate node for elseif
- `ElseStatement` - Intermediate node for else

### Expressions (`expressions.hpp`)

- `ListExpression` - Space or semicolon separated lists
- `GeneratorExpression` - $<...> expressions (kept opaque for performance)
- `BracketExpression` - [[...]] bracket arguments
- `BinaryOp` - Binary operators (AND, OR, EQUAL, etc.)
- `UnaryOp` - Unary operators (NOT, EXISTS, DEFINED, etc.)
- `FunctionCall` - Function call expressions

### Structure (`structure.hpp`)

- `Block` - Sequence of statements
- `File` - Entire CMake file representation
- `Include` - Include statements (single-file AST)

## Visitor Pattern (`visitor.hpp`)

Three visitor types are provided:

1. `ASTVisitor` - Pure virtual interface
2. `ASTVisitorBase` - Default empty implementations
3. `RecursiveASTVisitor` - Automatically visits children

## Builder (`builder.hpp`)

The `ASTBuilder` class provides factory methods for constructing AST nodes with:

- Automatic string interning
- Convenient list building
- Type-safe node construction

## Usage Example

```cpp
// Create a builder
ASTBuilder builder;

// Build an add_library command
auto ast = builder.makeCommand(
    SourceLocation{"CMakeLists.txt", 1, 1},
    "add_library",
    ASTBuilder::makeList(
        builder.makeString(loc, "mylib"),
        builder.makeString(loc, "STATIC"),
        builder.makeVariable(loc, "SOURCES")
    )
);

// Visit the AST
class PrintVisitor : public ASTVisitorBase {
public:
    void visit(const CommandCall& cmd) override {
        std::cout << "Command: " << cmd.name() << "\n";
    }
};

PrintVisitor visitor;
ast->accept(visitor);
```

## Performance Considerations

1. **String Interning**: All identifiers and command names are interned to reduce memory usage and speed up comparisons
2. **Generator Expressions**: Kept as opaque strings to avoid parsing complexity and performance overhead
3. **Macros**: Stored unexpanded for better performance - expansion happens in a separate phase
4. **Include Handling**: Single-file AST representation - multi-file projects handled at a higher level
5. **Smart Pointers**: Using unique_ptr for clear ownership and automatic memory management

## Integration with Error Handling

The AST integrates with Finch's Result<T,E> error handling:

- ParseError types are used for construction errors
- ErrorNode allows partial AST construction
- Source locations enable precise error reporting
