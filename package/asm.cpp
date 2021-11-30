// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "asm.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include "common/file.h"
#include "common/str.h"
#include "data/list.h"
#include "data/map.h"
#include "data/string.h"
#include "function.h"
#include "inst.h"
#include "memory/handle.h"
#include "package.h"
#include "type.h"

namespace filesystem = std::filesystem;

namespace codeswitch {

enum class TokenKind {
  NONE,
  NEWLINE,
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  RARROW,
  COMMA,
  COLON,
  IDENT,
  INT,
};

static const char* tokenKindName(TokenKind kind);

struct Token {
  TokenKind kind = TokenKind::NONE;
  std::size_t begin = 0, end = 0;
};

struct Position {
  std::string filename;
  int line, column;
};

/**
 * Token set maps byte offsets within a file to complete positions with line
 * and column numbers. We need a position on each token for error reporting,
 * but unless an error is actually reported, there's no need to store the
 * file name or line number.
 */
class TokenSet {
 public:
  explicit TokenSet(const std::string& filename) : filename_(filename) { addLine(0); }
  void addLine(std::size_t offset) { lines_.push_back(offset); }
  Position position(std::size_t offset) const {
    auto i = std::upper_bound(lines_.begin(), lines_.end(), offset);
    int line = i - lines_.begin();
    int column = offset - *(i - 1) + 1;
    return Position{filename_, line, column};
  }

 private:
  std::string filename_;
  std::vector<std::size_t> lines_;
};

class ParseError : public Error {
 public:
  ParseError(const Position& pos, const std::string& message) :
      Error(strprintf("%s:%d.%d: ", pos.filename.c_str(), pos.line, pos.column) + message) {}
};

/**
 * Base class for the various stages of the assembler. Provides common access
 * to file contents and the token set.
 */
class AsmPass {
 public:
  AsmPass(const std::vector<uint8_t>& data, TokenSet& tset) : data_(data), tset_(tset) {}

 protected:
  std::string_view text(Token t);
  ParseError parseErrorf(std::size_t offset, const char* fmt, ...);

  const std::vector<uint8_t>& data_;
  TokenSet& tset_;
};

class AsmLexer : public AsmPass {
 public:
  AsmLexer(const std::vector<uint8_t>& data, TokenSet& tset) : AsmPass(data, tset) {}

  std::deque<Token> lexFile();

 private:
  static bool isIdentFirst(char b);
  static bool isIdent(char b);
  static bool isDigit(char b);
};

struct AsmType {
  Token name;
};

struct AsmInst {
  Token label;
  Token mnemonic;
  std::vector<Token> operands;
};

struct AsmFunction {
  Token name;
  std::vector<AsmType> paramTypes;
  std::vector<AsmType> returnTypes;
  std::vector<AsmInst> insts;
};

struct AsmFile {
  std::vector<AsmFunction> functions;
};

class AsmParser : public AsmPass {
 public:
  AsmParser(const std::vector<uint8_t>& data, TokenSet& tset, const std::deque<Token>& tokens) :
      AsmPass(data, tset), tokens_(tokens), it_(tokens_.begin()) {}

  AsmFile parseFile();

 private:
  AsmFunction parseFunction();
  std::vector<AsmInst> parseFunctionBody();
  AsmInst parseInst();

  std::vector<AsmType> parseTypeList();
  AsmType parseType();

  TokenKind peek();
  std::string_view peekIdent();
  Token next();
  Token expect(TokenKind kind);
  Token expectIdent(const std::string& want);

  const std::deque<Token>& tokens_;
  std::deque<Token>::const_iterator it_;
};

class PackageBuilder : public AsmPass {
 public:
  PackageBuilder(const std::vector<uint8_t>& data, TokenSet& tset, AsmFile& file) : AsmPass(data, tset), file_(file) {}

  Handle<Package> build();

 private:
  Handle<Function> buildFunction(const AsmFunction& function);
  Handle<Type> buildType(const AsmType& type);
  uint8_t uint8Token(Token token);
  uint16_t uint16Token(Token token);
  int64_t int64Token(Token token);
  std::string_view identToken(Token token);

  Handle<String> tokenString(Token t);

  AsmFile& file_;
  std::unordered_map<std::string_view, int32_t> functionNameToIndex_;
};

Handle<Package> readPackageAsm(const filesystem::path& filename, std::istream& is) {
  auto data = readFile(filename);
  TokenSet tset(filename);
  auto tokens = AsmLexer(data, tset).lexFile();
  auto syntax = AsmParser(data, tset, tokens).parseFile();
  return PackageBuilder(data, tset, syntax).build();
}

const char* tokenKindName(TokenKind kind) {
  switch (kind) {
    case TokenKind::NONE:
      return "none";
    case TokenKind::NEWLINE:
      return "newline";
    case TokenKind::LPAREN:
      return "(";
    case TokenKind::RPAREN:
      return ")";
    case TokenKind::LBRACE:
      return "{";
    case TokenKind::RBRACE:
      return "}";
    case TokenKind::RARROW:
      return "->";
    case TokenKind::COMMA:
      return ",";
    case TokenKind::COLON:
      return ":";
    case TokenKind::IDENT:
      return "identifier";
    case TokenKind::INT:
      return "integer";
  }
  UNREACHABLE();
  return "";
}

std::string_view AsmPass::text(Token t) {
  return std::string_view(reinterpret_cast<const char*>(data_.data() + t.begin), t.end - t.begin);
}

ParseError AsmPass::parseErrorf(std::size_t offset, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  auto message = vstrprintf(fmt, args);
  va_end(args);
  auto pos = tset_.position(offset);
  return ParseError(pos, message);
}

std::deque<Token> AsmLexer::lexFile() {
  std::deque<Token> tokens;
  std::size_t i = 0;
  while (i < data_.size()) {
    auto b = data_[i++];
    switch (b) {
      case ' ':
      case '\t':
        // Whitespace
        continue;
      case '\n':
        // Newline
        tset_.addLine(i);
        if (!tokens.empty()) {
          auto prevKind = tokens.back().kind;
          if (prevKind == TokenKind::IDENT || prevKind == TokenKind::INT) {
            tokens.push_back(Token{TokenKind::NEWLINE, i - 1, i});
          }
        }
        continue;

      case '(':
        tokens.push_back(Token{TokenKind::LPAREN, i - 1, i});
        continue;
      case ')':
        tokens.push_back(Token{TokenKind::RPAREN, i - 1, i});
        continue;
      case '{':
        tokens.push_back(Token{TokenKind::LBRACE, i - 1, i});
        continue;
      case '}':
        tokens.push_back(Token{TokenKind::RBRACE, i - 1, i});
        continue;
      case ',':
        tokens.push_back(Token{TokenKind::COMMA, i - 1, i});
        continue;
      case ':':
        tokens.push_back(Token{TokenKind::COLON, i - 1, i});
        continue;

      case '/':
        // Comment
        if (i < data_.size() && data_[i] == '/') {
          auto j = i + 1;
          while (j < data_.size() && data_[j] != '\n') j++;
          i = j;
          continue;
        }
        throw parseErrorf(i, "unexpected character '/'");

      default:
        if (b == '-' && i < data_.size() && data_[i] == '>') {
          i++;
          tokens.push_back(Token{TokenKind::RARROW, i - 2, i});
          continue;
        }
        if (isIdentFirst(b)) {
          // Identifier
          auto j = i;
          while (j < data_.size() && isIdent(data_[j])) j++;
          tokens.push_back(Token{TokenKind::IDENT, i - 1, j});
          i = j;
          continue;
        }
        // Digit
        auto j = i;
        if (b == '-' || b == '+') {
          j++;
          if (j == data_.size()) {
            throw parseErrorf(i, "unexpected character '%c'; want digit", b);
          }
          b = data_[j];
        }
        if (b == '0') {
          if (j + 1 < data_.size() && isDigit(data_[j + 1])) {
            throw parseErrorf(i, "integer may not start with 0");
          }
          tokens.push_back(Token{TokenKind::INT, i - 1, j});
          continue;
        }
        if (!isDigit(b)) {
          throw parseErrorf(i, "unexpected character '%c'; want digit", b);
        }
        while (j < data_.size() && isDigit(data_[j])) j++;
        tokens.push_back(Token{TokenKind::INT, i - 1, j});
        i = j;
        continue;
    }
  }

  return tokens;
}

bool AsmLexer::isIdentFirst(char b) {
  return ('A' <= b && b <= 'Z') || ('a' <= b && b <= 'z') || b == '_';
}

bool AsmLexer::isIdent(char b) {
  return isIdentFirst(b) || isDigit(b);
}

bool AsmLexer::isDigit(char b) {
  return '0' <= b && b <= '9';
}

AsmFile AsmParser::parseFile() {
  std::vector<AsmFunction> functions;
  while (it_ != tokens_.end()) {
    auto s = peekIdent();
    if (s == "function") {
      functions.push_back(parseFunction());
    } else {
      throw parseErrorf(it_->begin, "unexpected token '%s'; want definition", it_->kind);
    }
  }
  return AsmFile{.functions = functions};
}

AsmFunction AsmParser::parseFunction() {
  expectIdent("function");
  auto name = expect(TokenKind::IDENT);
  auto paramTypes = parseTypeList();
  std::vector<AsmType> returnTypes;
  if (peek() == TokenKind::RARROW) {
    next();
    returnTypes = parseTypeList();
  }

  auto insts = parseFunctionBody();
  return AsmFunction{.name = name, .paramTypes = paramTypes, .returnTypes = returnTypes, .insts = insts};
}

std::vector<AsmInst> AsmParser::parseFunctionBody() {
  expect(TokenKind::LBRACE);
  std::vector<AsmInst> insts;
  while (peek() != TokenKind::RBRACE) {
    insts.push_back(parseInst());
  }
  next();
  return insts;
}

AsmInst AsmParser::parseInst() {
  Token label, mnemonic;
  mnemonic = expect(TokenKind::IDENT);
  if (peek() == TokenKind::COLON) {
    label = mnemonic;
    next();
    mnemonic = expect(TokenKind::IDENT);
  }
  std::vector<Token> operands;
  while (peek() != TokenKind::NEWLINE) {
    if (!operands.empty()) {
      expect(TokenKind::COMMA);
    }
    switch (peek()) {
      case TokenKind::IDENT:
      case TokenKind::INT:
        operands.push_back(next());
        break;
      default:
        throw parseErrorf(it_->begin, "unexpected token '%s'; want integer or identifier", tokenKindName(it_->kind));
    }
  }
  next();
  return AsmInst{.label = label, .mnemonic = mnemonic, .operands = operands};
}

std::vector<AsmType> AsmParser::parseTypeList() {
  expect(TokenKind::LPAREN);
  std::vector<AsmType> types;
  while (peek() != TokenKind::RPAREN) {
    if (!types.empty()) {
      expect(TokenKind::COMMA);
    }
    types.push_back(parseType());
  }
  next();
  return types;
}

AsmType AsmParser::parseType() {
  auto name = expect(TokenKind::IDENT);
  return AsmType{.name = name};
}

TokenKind AsmParser::peek() {
  return it_->kind;
}

std::string_view AsmParser::peekIdent() {
  if (it_->kind != TokenKind::IDENT) {
    return std::string_view();
  }
  return text(*it_);
}

Token AsmParser::next() {
  return *it_++;
}

Token AsmParser::expect(TokenKind kind) {
  auto t = next();
  if (t.kind != kind) {
    throw parseErrorf(t.begin, "unexpected token '%s'; want %s", std::string(text(t)).c_str(), tokenKindName(kind));
  }
  return t;
}

Token AsmParser::expectIdent(const std::string& want) {
  auto t = expect(TokenKind::IDENT);
  auto got = text(t);
  if (got != want) {
    throw parseErrorf(t.begin, "unexpected token '%s'; want identifier %s", got, want);
  }
  return t;
}

Handle<Package> PackageBuilder::build() {
  for (std::size_t i = 0; i < file_.functions.size(); i++) {
    functionNameToIndex_[text(file_.functions[i].name)] = i;
  }

  auto functions = List<Ptr<Function>>::create(file_.functions.size());
  for (auto& f : file_.functions) {
    functions->append(*buildFunction(f));
  }
  return handle(Package::make(**functions));
}

struct LabelInfo {
  Label label;
  Token use;
};

Handle<Function> PackageBuilder::buildFunction(const AsmFunction& function) {
  auto name = tokenString(function.name);
  auto returnTypes = List<Ptr<Type>>::create(function.returnTypes.size());
  for (auto& type : function.returnTypes) {
    returnTypes->append(*buildType(type));
  }
  auto paramTypes = List<Ptr<Type>>::create(function.paramTypes.size());
  for (auto& type : function.paramTypes) {
    paramTypes->append(*buildType(type));
  }

  Assembler a;
  std::unordered_map<std::string_view, LabelInfo> labels;
  length_t frameSize = 0;  // TODO: compute the actual frame size.
  for (auto& inst : function.insts) {
    if (inst.label.kind == TokenKind::IDENT) {
      auto name = text(inst.label);
      auto& l = labels[name];
      if (l.label.bound()) {
        auto ns = std::string(name);
        throw parseErrorf(inst.label.begin, "label %s bound multiple times", ns.c_str());
      }
      a.bind(&l.label);
    }

    auto m = text(inst.mnemonic);
    std::size_t wantOpCount = 0;
    if (m == "b" || m == "bif" || m == "call" || m == "int64" || m == "loadarg" || m == "loadlocal" ||
        m == "storearg" || m == "storelocal" || m == "sys") {
      wantOpCount = 1;
    }
    if (inst.operands.size() != wantOpCount) {
      auto ms = std::string(m);
      throw parseErrorf(inst.mnemonic.begin, "instruction %s must have %d operand(s); got %d", ms.c_str(), wantOpCount,
                        inst.operands.size());
    }

    if (m == "add") {
      a.add();
    } else if (m == "and") {
      a.and_();
    } else if (m == "asr") {
      a.asr();
    } else if (m == "b") {
      auto name = identToken(inst.operands[0]);
      if (labels.find(name) == labels.end()) {
        labels[name].use = inst.operands[0];
      }
      a.b(&labels[name].label);
    } else if (m == "bif") {
      auto name = identToken(inst.operands[0]);
      if (labels.find(name) == labels.end()) {
        labels[name].use = inst.operands[0];
      }
      a.bif(&labels[name].label);
    } else if (m == "call") {
      auto name = identToken(inst.operands[0]);
      auto it = functionNameToIndex_.find(name);
      if (it == functionNameToIndex_.end()) {
        std::string nameStr(name);
        throw parseErrorf(inst.operands[0].begin, "undefined function: %s", nameStr.c_str());
      }
      auto index = it->second;
      if (static_cast<decltype(index)>(static_cast<uint32_t>(index)) != index) {
        throw parseErrorf(inst.operands[0].begin, "cannot encode function index");
      }
      a.call(static_cast<uint32_t>(index));
    } else if (m == "div") {
      a.div();
    } else if (m == "eq") {
      a.eq();
    } else if (m == "false") {
      a.false_();
    } else if (m == "ge") {
      a.ge();
    } else if (m == "gt") {
      a.gt();
    } else if (m == "int64") {
      auto n = int64Token(inst.operands[0]);
      a.int64(n);
    } else if (m == "le") {
      a.le();
    } else if (m == "loadarg") {
      auto slot = uint16Token(inst.operands[0]);
      a.loadarg(slot);
    } else if (m == "loadlocal") {
      auto slot = uint16Token(inst.operands[0]);
      a.loadlocal(slot);
    } else if (m == "lt") {
      a.lt();
    } else if (m == "mod") {
      a.mod();
    } else if (m == "mul") {
      a.mul();
    } else if (m == "ne") {
      a.ne();
    } else if (m == "neg") {
      a.neg();
    } else if (m == "nop") {
      a.nop();
    } else if (m == "not") {
      a.not_();
    } else if (m == "or") {
      a.or_();
    } else if (m == "ret") {
      a.ret();
    } else if (m == "shl") {
      a.shl();
    } else if (m == "shr") {
      a.shr();
    } else if (m == "storearg") {
      auto slot = uint16Token(inst.operands[0]);
      a.storearg(slot);
    } else if (m == "storelocal") {
      auto slot = uint16Token(inst.operands[0]);
      a.storelocal(slot);
    } else if (m == "sub") {
      a.sub();
    } else if (m == "sys") {
      auto name = identToken(inst.operands[0]);
      Sys sys;
      if (name == "exit") {
        sys = Sys::EXIT;
      } else if (name == "println") {
        sys = Sys::PRINTLN;
      } else {
        std::string nameStr(name);
        throw parseErrorf(inst.operands[0].begin, "undefined system function: %s", nameStr.c_str());
      }
      a.sys(sys);
    } else if (m == "true") {
      a.true_();
    } else if (m == "unit") {
      a.unit();
    } else if (m == "xor") {
      a.xor_();
    } else {
      auto ms = std::string(m);
      throw parseErrorf(inst.mnemonic.begin, "unknown instruction '%s'", ms.c_str());
    }
  }

  for (auto kv : labels) {
    if (!kv.second.label.bound()) {
      auto ns = std::string(kv.first);
      throw parseErrorf(kv.second.use.begin, "use of unbound label '%s'", ns.c_str());
    }
  }

  auto insts = a.finish();
  return handle(Function::make(**name, **returnTypes, **paramTypes, **insts, frameSize));
}  // namespace codeswitch

Handle<Type> PackageBuilder::buildType(const AsmType& type) {
  auto name = identToken(type.name);
  Type::Kind kind;
  if (name == "unit") {
    kind = Type::UNIT;
  } else if (name == "bool") {
    kind = Type::BOOL;
  } else if (name == "int64") {
    kind = Type::INT64;
  } else {
    std::string nameStr(name);
    throw parseErrorf(type.name.begin, "unknown type: %s", nameStr.c_str());
  }
  return handle(Type::make(kind));
}

uint8_t PackageBuilder::uint8Token(Token token) {
  if (token.kind != TokenKind::INT) {
    throw parseErrorf(token.begin, "expected integer; found %s", tokenKindName(token.kind));
  }
  try {
    auto nwide = std::stoull(std::string(text(token)));
    auto n = static_cast<uint8_t>(nwide);
    if (static_cast<decltype(nwide)>(n) == nwide) {
      return n;
    }
  } catch (std::out_of_range&) {
  }
  throw parseErrorf(token.begin, "expected unsigned 8-bit integer");
}

uint16_t PackageBuilder::uint16Token(Token token) {
  if (token.kind != TokenKind::INT) {
    throw parseErrorf(token.begin, "expected integer; found %s", tokenKindName(token.kind));
  }
  try {
    auto nwide = std::stoull(std::string(text(token)));
    auto n = static_cast<uint16_t>(nwide);
    if (static_cast<decltype(nwide)>(n) == nwide) {
      return n;
    }
  } catch (std::out_of_range&) {
  }
  throw parseErrorf(token.begin, "expected unsigned 16-bit integer");
}

int64_t PackageBuilder::int64Token(Token token) {
  if (token.kind != TokenKind::INT) {
    throw parseErrorf(token.begin, "expected integer; found %s", tokenKindName(token.kind));
  }
  try {
    auto nwide = std::stoll(std::string(text(token)));
    auto n = static_cast<int64_t>(nwide);
    if (static_cast<decltype(nwide)>(n) == nwide) {
      return n;
    }
  } catch (std::out_of_range&) {
  }
  throw parseErrorf(token.begin, "expected 64-bit integer");
}

std::string_view PackageBuilder::identToken(Token token) {
  if (token.kind != TokenKind::IDENT) {
    throw parseErrorf(token.begin, "expected identifier; found %s", tokenKindName(token.kind));
  }
  return text(token);
}

Handle<String> PackageBuilder::tokenString(Token t) {
  return String::create(std::string(reinterpret_cast<const char*>(data_.data() + t.begin), t.end - t.begin));
}

Assembler::Assembler() {
  fragments_.emplace_back();
}

Handle<List<Inst>> Assembler::finish() {
  auto l = List<Inst>::create(size_);
  for (auto& f : fragments_) {
    l->append(reinterpret_cast<Inst*>(f.begin), f.end - f.begin);
  }
  return l;
}

void Assembler::bind(Label* label) {
  ASSERT(!label->bound_);
  auto labelOffset = static_cast<int32_t>(size_);
  auto useOffset = label->offset_;
  while (useOffset != 0) {
    auto useAddr = reinterpret_cast<int32_t*>(addrOf(useOffset));
    auto nextUseOffset = *useAddr;
    auto instOffset = useOffset - 1;
    *useAddr = labelOffset - instOffset;
    useOffset = nextUseOffset;
  }
  label->bound_ = true;
  label->offset_ = labelOffset;
}

void Assembler::add() {
  op(Op::ADD);
}

void Assembler::and_() {
  op(Op::AND);
}

void Assembler::asr() {
  op(Op::ASR);
}

void Assembler::b(Label* label) {
  op1_label(Op::B, label);
}

void Assembler::bif(Label* label) {
  op1_label(Op::BIF, label);
}

void Assembler::call(uint32_t index) {
  op1_32(Op::CALL, index);
}

void Assembler::div() {
  op(Op::DIV);
}

void Assembler::eq() {
  op(Op::EQ);
}

void Assembler::false_() {
  op(Op::FALSE);
}

void Assembler::ge() {
  op(Op::GE);
}

void Assembler::gt() {
  op(Op::GT);
}

void Assembler::int64(int64_t n) {
  op1_64(Op::INT64, n);
}

void Assembler::le() {
  op(Op::LE);
}

void Assembler::loadarg(uint16_t slot) {
  op1_16(Op::LOADARG, slot);
}

void Assembler::loadlocal(uint16_t slot) {
  op1_16(Op::LOADLOCAL, slot);
}

void Assembler::lt() {
  op(Op::LT);
}

void Assembler::mod() {
  op(Op::MOD);
}

void Assembler::mul() {
  op(Op::MUL);
}

void Assembler::ne() {
  op(Op::NE);
}

void Assembler::neg() {
  op(Op::NEG);
}

void Assembler::nop() {
  op(Op::NOP);
}

void Assembler::not_() {
  op(Op::NOT);
}

void Assembler::or_() {
  op(Op::OR);
}

void Assembler::ret() {
  op(Op::RET);
}

void Assembler::shl() {
  op(Op::SHL);
}

void Assembler::shr() {
  op(Op::SHR);
}

void Assembler::storearg(uint16_t slot) {
  op1_16(Op::STOREARG, slot);
}

void Assembler::storelocal(uint16_t slot) {
  op1_16(Op::STORELOCAL, slot);
}

void Assembler::sub() {
  op(Op::SUB);
}

void Assembler::sys(Sys sys) {
  op1_8(Op::SYS, static_cast<uint8_t>(sys));
}

void Assembler::true_() {
  op(Op::TRUE);
}

void Assembler::unit() {
  op(Op::UNIT);
}

void Assembler::xor_() {
  op(Op::XOR);
}

void Assembler::ensureSpace(length_t n) {
  if (size_ + n > kMaxFunctionSize) {
    throw errorf("maximum function size exceeded");
  }
  size_ += n;
  if (fragments_.back().end + n > fragments_.back().begin + sizeof(fragments_.back().begin)) {
    fragments_.emplace_back();
  }
}

void Assembler::op(Op op) {
  ensureSpace(sizeof(op));
  fragments_.back().push(static_cast<uint8_t>(op));
}

void Assembler::op1_8(Op op, uint8_t a) {
  ensureSpace(sizeof(op) + sizeof(a));
  fragments_.back().push(static_cast<uint8_t>(op));
  fragments_.back().push(a);
}

void Assembler::op1_16(Op op, uint16_t a) {
  ensureSpace(sizeof(op) + sizeof(a));
  fragments_.back().push(static_cast<uint8_t>(op));
  fragments_.back().push(a);
}

void Assembler::op1_32(Op op, uint32_t a) {
  ensureSpace(sizeof(op) + sizeof(a));
  fragments_.back().push(static_cast<uint8_t>(op));
  fragments_.back().push(a);
}

void Assembler::op1_64(Op op, uint64_t a) {
  ensureSpace(sizeof(op) + sizeof(a));
  fragments_.back().push(static_cast<uint8_t>(op));
  fragments_.back().push(a);
}

void Assembler::op1_label(Op op, Label* label) {
  auto offset = static_cast<int32_t>(size_);
  ensureSpace(sizeof(op) + sizeof(int32_t));
  fragments_.back().push(static_cast<uint8_t>(op));
  if (label->bound_) {
    // Label is bound. It contains the absolute offset where it was bound.
    fragments_.back().push(label->offset_ - offset);
  } else {
    // Label is unbound. It contains the absolute offset of the last place
    // where it was used. Save that here (like a linked list) and maintain
    // the invariant.
    fragments_.back().push(label->offset_);
    label->offset_ = offset + 1;  // offset of the offset, not the opcode.
  }
}

uint8_t* Assembler::addrOf(int32_t offset) {
  ASSERT(0 <= offset && static_cast<length_t>(offset) < size_);
  int32_t fragOffset = 0;
  for (auto& f : fragments_) {
    auto p = f.begin + offset - fragOffset;
    if (p < f.end) {
      return p;
    }
    fragOffset += f.end - f.begin;
  }
  UNREACHABLE();
  return nullptr;
}

static void writeFunction(std::ostream& os, Package* package, const Function* function);
static void writeTypeList(std::ostream& os, const List<Ptr<Type>>& types);
static void writeType(std::ostream& os, const Type* type);

void writePackageAsm(std::ostream& os, Package* package) {
  auto sep = "";
  for (length_t i = 0, n = package->functionCount(); i < n; i++) {
    os << sep;
    sep = "\n\n";
    writeFunction(os, package, package->functionByIndex(i));
  }
  os << '\n';
}

void writeFunction(std::ostream& os, Package* package, const Function* function) {
  os << "function " << function->name;
  writeTypeList(os, function->paramTypes);
  if (!function->returnTypes.empty()) {
    os << " -> ";
    writeTypeList(os, function->returnTypes);
  }
  os << " {\n";

  std::unordered_map<int32_t, int32_t> labelIndices;
  int32_t labelIndex = 1;
  for (auto begin = function->insts.begin(), inst = begin, end = function->insts.end(); inst < end;
       inst = inst->next()) {
    switch (inst->op) {
      case Op::B:
      case Op::BIF: {
        auto delta = *reinterpret_cast<const int32_t*>(inst + 1);
        auto instOffset = inst - begin;
        auto labelOffset = instOffset + delta;
        labelIndices[labelOffset] = labelIndex++;
        break;
      }
      default:
        break;
    }
  }

  auto sep = "";
  for (auto begin = function->insts.begin(), inst = begin, end = function->insts.end(); inst < end;
       inst = inst->next()) {
    os << sep;
    sep = "\n";
    auto instOffset = inst - begin;
    auto it = labelIndices.find(instOffset);
    if (it != labelIndices.end()) {
      os << "L" << it->second << ":\n";
    }

    os << "  " << inst->mnemonic();

    switch (inst->op) {
      case Op::B:
      case Op::BIF: {
        auto delta = *reinterpret_cast<const int32_t*>(inst + 1);
        auto labelOffset = instOffset + delta;
        os << " L" << labelIndices[labelOffset];
        break;
      }

      case Op::CALL: {
        auto index = *reinterpret_cast<const int32_t*>(inst + 1);
        auto callee = package->functionByIndex(index);
        os << " " << callee->name;
        break;
      }

      case Op::INT64: {
        auto n = *reinterpret_cast<const int64_t*>(inst + 1);
        os << " " << n;
        break;
      }

      case Op::LOADARG:
      case Op::LOADLOCAL:
      case Op::STOREARG:
      case Op::STORELOCAL: {
        auto n = *reinterpret_cast<const uint16_t*>(inst + 1);
        os << " " << n;
        break;
      }

      case Op::SYS: {
        auto sys = *reinterpret_cast<const Sys*>(inst + 1);
        os << " " << sysMnemonic(sys);
        break;
      }

      default:
        break;
    }
  }
  os << "\n}";
}

void writeTypeList(std::ostream& os, const List<Ptr<Type>>& types) {
  if (types.empty()) {
    return;
  }
  os.put('(');
  auto sep = "";
  for (auto& type : types) {
    os << sep;
    sep = ", ";
    writeType(os, type.get());
  }
  os.put(')');
}

void writeType(std::ostream& os, const Type* type) {
  switch (type->kind()) {
    case Type::Kind::UNIT:
      os << "unit";
      return;
    case Type::Kind::BOOL:
      os << "bool";
      return;
    case Type::Kind::INT64:
      os << "int64";
      return;
  }
  UNREACHABLE();
}

}  // namespace codeswitch
