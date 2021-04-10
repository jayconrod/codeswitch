// Copyright Jay Conrod. All rights reserved.

// This file is part of CodeSwitch. Use of this source code is governed by
// the 3-clause BSD license that can be found in the LICENSE.txt file.

#include "asm.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <iostream>
#include <string>
#include <vector>
#include "common/file.h"
#include "common/str.h"
#include "function.h"
#include "memory/handle.h"
#include "package.h"
#include "type.h"

namespace codeswitch {
namespace internal {

enum class TokenKind {
  LPAREN,
  RPAREN,
  LBRACE,
  RBRACE,
  RARROW,
  COMMA,
  SEMI,
  IDENT,
  INT,
};

static const char* tokenKindName(TokenKind kind);

struct Token {
  TokenKind kind;
  std::size_t begin, end;
};

struct Position {
  std::string filename;
  int line, column;
};

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

class AsmError : public std::exception {
 public:
  AsmError(const Position& pos, const std::string& message) {
    text_ = strprintf("%s:%d.%d: %s", pos.filename.c_str(), pos.line, pos.column, message.c_str());
  }

  virtual const char* what() const noexcept { return text_.c_str(); }

 private:
  std::string text_;
};

static void lexPackageAsm(const std::string& filename, const std::vector<uint8_t>& data, TokenSet* tset,
                          std::deque<Token>* tokens);
static bool isIdentFirst(char b);
static bool isIdent(char b);
static bool isDigit(char b);

class PackageAsmParser {
 public:
  PackageAsmParser(const std::vector<uint8_t>& data, const TokenSet& tset, const std::deque<Token>& tokens) :
      data_(data), tset_(tset), tokens_(tokens), it_(tokens_.begin()) {}

  Handle<Package> parseFile();

 private:
  Handle<Function> parseFunction();
  Handle<List<Inst>> parseFunctionBody();

  Handle<List<Ptr<Type>>> parseTypeList();
  Handle<Type> parseType();

  void parseInst(Assembler* s);

  std::string text(Token t);
  TokenKind peek();
  Token next();
  int64_t nextInt(int size);
  Token expect(TokenKind kind);
  Token expectIdent(const std::string& want);

  const std::vector<uint8_t>& data_;
  const TokenSet& tset_;
  const std::deque<Token>& tokens_;
  std::deque<Token>::const_iterator it_;
};

Handle<Package> readPackageAsm(const std::string& filename) {
  auto data = readFile(filename);
  TokenSet tset(filename);
  std::deque<Token> tokens;
  lexPackageAsm(filename, data, &tset, &tokens);
  PackageAsmParser parser(data, tset, tokens);
  return parser.parseFile();
}

const char* tokenKindName(TokenKind kind) {
  switch (kind) {
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
    case TokenKind::SEMI:
      return ";";
    case TokenKind::IDENT:
      return "identifier";
    case TokenKind::INT:
      return "integer";
  }
  UNREACHABLE();
  return "";
}

void lexPackageAsm(const std::string& filename, const std::vector<uint8_t>& data, TokenSet* tset,
                   std::deque<Token>* tokens) {
  std::size_t i = 0;
  int line = 1, column = 0;
  while (i < data.size()) {
    auto b = data[i++];
    switch (b) {
      case ' ':
      case '\t':
        // Whitespace
        continue;
      case '\n':
        // Newline
        line++;
        column = 0;
        tset->addLine(i);
        continue;

      case '(':
        tokens->push_back(Token{TokenKind::LPAREN, i - 1, i});
      case ')':
        tokens->push_back(Token{TokenKind::RPAREN, i - 1, i});
      case '{':
        tokens->push_back(Token{TokenKind::LBRACE, i - 1, i});
      case '}':
        tokens->push_back(Token{TokenKind::RBRACE, i - 1, i});
      case ',':
        tokens->push_back(Token{TokenKind::COMMA, i - 1, i});
      case ';':
        tokens->push_back(Token{TokenKind::SEMI, i - 1, i});

      case '/':
        // Comment
        if (i + 1 < data.size() && data[i + 1] == '/') {
          auto j = i + 1;
          while (j < data.size() && data[j] != '\n') j++;
          column += j - i;
          i = j;
          continue;
        }
        throw AsmError(Position{filename, line, column}, "unexpected character '/'");

      default:
        if (b == '-' && i + 1 < data.size() && data[i + 1] == '>') {
          i++;
          tokens->push_back(Token{TokenKind::RARROW, i - 2, i});
          continue;
        }
        if (isIdentFirst(b)) {
          // Identifier
          auto j = i;
          while (j < data.size() && isIdent(data[j])) j++;
          tokens->push_back(Token{TokenKind::IDENT, i - 1, j});
          column += j - i;
          i = j;
          continue;
        }
        // Digit
        auto j = i;
        if (b == '-' || b == '+') {
          j++;
          if (j == data.size()) {
            throw AsmError(Position{filename, line, column}, strprintf("unexpected character '%c'", b));
          }
          b = data[j];
          column++;
        }
        if (b == '0') {
          j++;
          if (j < data.size() && isDigit(data[j])) {
            throw AsmError(Position{filename, line, column}, "integer may not start with 0");
          }
          tokens->push_back(Token{TokenKind::INT, i - 1, j});
          column += j - i;
          i = j;
          continue;
        }
        if (!isDigit(b)) {
          throw AsmError(Position{filename, line, column}, strprintf("unexpected character '%c'", b));
        }
        while (j < data.size() && isDigit(data[j])) j++;
        tokens->push_back(Token{TokenKind::INT, i - 1, j});
        column += j - i;
        i = j;
        continue;
    }
  }
}

bool isIdentFirst(char b) {
  return ('A' <= b && b <= 'Z') || ('a' <= b && b <= 'z') || b == '_';
}

bool isIdent(char b) {
  return isIdentFirst(b) || isDigit(b);
}

bool isDigit(char b) {
  return '0' <= b && b <= '9';
}

Handle<Package> PackageAsmParser::parseFile() {
  std::deque<Handle<Function>> functions;

  while (it_ != tokens_.end()) {
    auto t = *it_;
    auto s = text(t);
    if (s == "function") {
      functions.emplace_back(parseFunction());
    } else {
      throw AsmError(tset_.position(t.begin), "expected definition");
    }
  }

  auto functionArray = handle(Array<Ptr<Function>>::make(functions.size()));
  length_t i = 0;
  for (auto& f : functions) {
    (**functionArray)[i] = *f;
  }
  return handle(Package::make(functionArray.get()));
}

Handle<Function> PackageAsmParser::parseFunction() {
  expectIdent("function");
  auto t = expect(TokenKind::IDENT);
  auto name = text(t);
  auto paramTypes = parseTypeList();
  Handle<List<Ptr<Type>>> returnTypes;
  if (peek() == TokenKind::RARROW) {
    next();
    returnTypes = parseTypeList();
  } else {
    returnTypes = handle(List<Ptr<Type>>::make(0));
  }

  expect(TokenKind::LBRACE);
  Assembler s;
  while (peek() != TokenKind::RBRACE) {
    parseInst(&s);
  }
  next();
  auto insts = s.finish();
  return handle(Function::make(**paramTypes, **returnTypes, **insts, 0));
}

Handle<List<Ptr<Type>>> PackageAsmParser::parseTypeList() {
  auto types = handle(List<Ptr<Type>>::make(0));
  expect(TokenKind::LPAREN);
  auto first = true;
  while (peek() != TokenKind::RPAREN) {
    if (!first) {
      expect(TokenKind::COMMA);
    }
    first = false;
    types->append(*parseType());
  }
  return types;
}

Handle<Type> PackageAsmParser::parseType() {
  auto t = expect(TokenKind::IDENT);
  auto s = text(t);
  Type::Kind kind;
  if (s == "unit") {
    kind = Type::Kind::UNIT;
  } else if (s == "bool") {
    kind = Type::Kind::BOOL;
    return handle(Type::make(Type::Kind::BOOL));
  } else if (s == "int64") {
    kind = Type::Kind::INT64;
  } else {
    throw AsmError(tset_.position(t.begin), "expected type");
  }
  return handle(Type::make(kind));
}

void PackageAsmParser::parseInst(Assembler* s) {
  auto t = next();
  auto name = text(t);
  if (name == "false") {
    s->fals();
  } else if (name == "int64") {
    auto n = nextInt(64);
    s->int64(n);
  } else if (name == "ret") {
    s->ret();
  } else if (name == "tru") {
    s->tru();
  } else if (name == "unit") {
    s->unit();
  } else {
    throw AsmError(tset_.position(t.begin), "expected instruction");
  }
  expect(TokenKind::SEMI);
}

std::string PackageAsmParser::text(Token t) {
  auto s = reinterpret_cast<const char*>(data_.data() + t.begin);
  return std::string(s, t.end - t.begin);
}

TokenKind PackageAsmParser::peek() {
  return it_->kind;
}

Token PackageAsmParser::next() {
  return *it_++;
}

int64_t PackageAsmParser::nextInt(int size) {
  auto t = expect(TokenKind::INT);
  auto s = text(t);
  int64_t n = 0;
  auto ok = true;
  try {
    n = std::stoll(s, nullptr, 0);
  } catch (std::out_of_range& ex) {
    ok = false;
  }
  if (ok) {
    switch (size) {
      case 64:
        break;
      case 32:
        ok = static_cast<int64_t>(static_cast<int32_t>(n)) == n;
      case 16:
        ok = static_cast<int64_t>(static_cast<int16_t>(n)) == n;
      case 8:
        ok = static_cast<int64_t>(static_cast<int8_t>(n)) == n;
      default:
        UNREACHABLE();
    }
  }
  if (!ok) {
    throw AsmError(tset_.position(t.begin), strprintf("expected %d-bit integer", size));
  }
  return n;
}

Token PackageAsmParser::expect(TokenKind kind) {
  auto t = next();
  if (t.kind != kind) {
    throw AsmError(tset_.position(t.begin), strprintf("expected %s", tokenKindName(kind)));
  }
  return t;
}

Token PackageAsmParser::expectIdent(const std::string& want) {
  auto t = expect(TokenKind::IDENT);
  auto got = text(t);
  if (got != want) {
    throw AsmError(tset_.position(t.begin), strprintf("expected identifier %s", want));
  }
  return t;
}

}  // namespace internal
}  // namespace codeswitch
