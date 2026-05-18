#include "parser.h"

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), current(0) {}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    errorMessage.clear();
    std::vector<std::unique_ptr<Stmt>> statements;

    while (!isAtEnd()) {
        statements.push_back(statement());
    }

    runSemanticChecks(statements);
    return statements;
}

const std::string& Parser::getErrorMessage() const { return errorMessage; }

// ─── Statements ──────────────────────────────────────────────────────────────

std::unique_ptr<Stmt> Parser::statement() {
    int stmtLine = peek().line;

    if (match(TokenType::If))         return ifStatement(stmtLine);
    if (match(TokenType::While))      return whileStatement(stmtLine);
    if (match(TokenType::For))        return forStatement(stmtLine);
    if (match(TokenType::Break))      return breakStatement(stmtLine);
    if (match(TokenType::Continue))   return continueStatement(stmtLine);
    if (match(TokenType::LeftBrace))  return blockStatement(stmtLine);
    if (match(TokenType::Print))      return printStatement(stmtLine);

    if (match(TokenType::Let)) {
        // `let` infers the declared type from the initialiser expression.
        // We peek at the first meaningful token(s) to choose:
        //   true / false          → Bool
        //   Number with LL/ll     → LongLong
        //   Number containing '.' or 'e'/'E' → Float
        //   otherwise             → Int
        // The full expression is still parsed normally after the '='.
        // We determine the type just before parsing the expression.

        consume(TokenType::Identifier, "Expected variable name after 'let'");
        Token nameToken = previous();
        consume(TokenType::Equal, "Expected '=' after variable name");

        // Peek to infer type (don't consume — expression() will do that)
        ValueType inferredType = ValueType::Int;
        std::string inferredKeyword = "let";

        // Look past a leading unary minus if present
        std::size_t peekAt = current;
        if (peekAt < tokens.size() && tokens[peekAt].type == TokenType::Minus)
            peekAt++;

        if (peekAt < tokens.size()) {
            const Token& t = tokens[peekAt];
            if (t.type == TokenType::True || t.type == TokenType::False) {
                inferredType    = ValueType::Bool;
                inferredKeyword = "let(bool)";
            } else if (t.type == TokenType::Number) {
                const std::string& lex = t.lexeme;
                bool isFloat = lex.find('.') != std::string::npos ||
                               lex.find('e') != std::string::npos ||
                               lex.find('E') != std::string::npos;
                if (isFloat) {
                    inferredType    = ValueType::Float;
                    inferredKeyword = "let(float)";
                } else {
                    // Check if followed by LL/ll suffix
                    std::size_t next = peekAt + 1;
                    if (next < tokens.size() && tokens[next].type == TokenType::LL) {
                        inferredType    = ValueType::LongLong;
                        inferredKeyword = "let(long long)";
                    } else {
                        inferredType    = ValueType::Int;
                        inferredKeyword = "let(int)";
                    }
                }
            }
            // For any other expression (identifier, cast, input…) default to Int.
            // The user can always use an explicit cast to set the type they want.
        }

        std::unique_ptr<Expr> value = expression();

        if (!check(TokenType::Semicolon)) {
            parseError("Expected ';' after variable declaration. Found " + describeCurrentToken(),
                       peek().line, peek().lexeme);
        }
        consume(TokenType::Semicolon, "Expected ';' after variable declaration");

        auto node = std::make_unique<LetStmt>(inferredType, inferredKeyword,
                                              nameToken.lexeme, std::move(value));
        node->line = stmtLine;
        return node;
    }

    if (match(TokenType::IntKeyword)) {
        return declarationStatement(stmtLine, ValueType::Int, "int");
    }

    if (match(TokenType::BoolKeyword)) {
        return declarationStatement(stmtLine, ValueType::Bool, "bool");
    }

    if (match(TokenType::FloatKeyword)) {
        return declarationStatement(stmtLine, ValueType::Float, "float");
    }

    // 'long' optionally followed by 'long' and/or 'int'
    // accepted forms: long  /  long int  /  long long  /  long long int
    if (match(TokenType::LongKeyword)) {
        std::string keywordText = "long";
        if (match(TokenType::LongKeyword)) {
            keywordText = "long long";
            if (match(TokenType::IntKeyword)) keywordText = "long long int";
        } else if (match(TokenType::IntKeyword)) {
            keywordText = "long int";
        }
        return declarationStatement(stmtLine, ValueType::LongLong, keywordText);
    }

    if (check(TokenType::Identifier) && checkNext(TokenType::Equal)) {
        return assignmentStatement(stmtLine);
    }

    // Only reject a bare identifier with no following '('. An identifier
    // followed by '(' is a valid expression (e.g. size(int)), so let it
    // fall through to expressionStatement.
    if (check(TokenType::Identifier) && !checkNext(TokenType::LeftParen)) {
        parseError("Unexpected identifier '" + peek().lexeme + "'",
                   peek().line, peek().lexeme);
    }

    return expressionStatement(stmtLine);
}

std::unique_ptr<Stmt> Parser::declarationStatement(int stmtLine,
                                                   ValueType declaredType,
                                                   const std::string& keywordText) {
    consume(TokenType::Identifier, "Expected variable name after '" + keywordText + "'");
    Token nameToken = previous();

    consume(TokenType::Equal, "Expected '=' after variable name");

    std::unique_ptr<Expr> value = expression();

    if (!check(TokenType::Semicolon)) {
        parseError("Expected ';' after variable declaration. Found " + describeCurrentToken(),
                   peek().line, peek().lexeme);
    }
    consume(TokenType::Semicolon, "Expected ';' after variable declaration");

    auto node = std::make_unique<LetStmt>(declaredType, keywordText,
                                          nameToken.lexeme, std::move(value));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::blockStatement(int stmtLine) {
    std::vector<std::unique_ptr<Stmt>> stmts;

    while (!check(TokenType::RightBrace) && !isAtEnd()) {
        stmts.push_back(statement());
    }

    if (isAtEnd()) {
        parseError("Expected '}'", previous().line, previous().lexeme);
    }

    consume(TokenType::RightBrace, "Expected '}' after block");

    auto node = std::make_unique<BlockStmt>(std::move(stmts));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::ifStatement(int stmtLine) {
    consume(TokenType::LeftParen, "Expected '(' after 'if'");

    if (check(TokenType::RightParen)) {
        parseError("Expected expression inside condition", peek().line, peek().lexeme);
    }

    auto condition = expression();
    consume(TokenType::RightParen, "Expected ')' after if condition");

    auto thenBranch = statement();

    std::unique_ptr<Stmt> elseBranch;
    if (match(TokenType::Else)) elseBranch = statement();

    auto node = std::make_unique<IfStmt>(std::move(condition),
                                          std::move(thenBranch),
                                          std::move(elseBranch));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::whileStatement(int stmtLine) {
    consume(TokenType::LeftParen, "Expected '(' after 'while'");

    if (check(TokenType::RightParen)) {
        parseError("Expected expression inside condition", peek().line, peek().lexeme);
    }

    auto condition = expression();
    consume(TokenType::RightParen, "Expected ')' after while condition");

    loopDepth++;
    auto body = statement();
    loopDepth--;

    auto node = std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    node->line = stmtLine;
    return node;
}

// for ( [init] ; [condition] ; [update] ) body
//
// init   : type-decl  |  assignment  |  (empty)
// update : assignment (no trailing ';')  |  (empty)
// All three parts are optional; omitting condition makes an infinite loop.
std::unique_ptr<Stmt> Parser::forStatement(int stmtLine) {
    consume(TokenType::LeftParen, "Expected '(' after 'for'");

    // ── init ──────────────────────────────────────────────────────────────────
    std::unique_ptr<Stmt> init;
    if (!check(TokenType::Semicolon)) {
        int initLine = peek().line;

        // Declaration: int / bool / float / long [long [int]]
        auto tryDecl = [&]() -> ValueType {
            if (check(TokenType::IntKeyword))   return ValueType::Int;
            if (check(TokenType::BoolKeyword))  return ValueType::Bool;
            if (check(TokenType::FloatKeyword)) return ValueType::Float;
            if (check(TokenType::LongKeyword))  return ValueType::LongLong;
            return ValueType::Unknown;
        };

        if (tryDecl() != ValueType::Unknown) {
            // Re-use declarationStatement logic by parsing type keyword(s) first
            ValueType    declType;
            std::string  kwText;
            if (match(TokenType::IntKeyword))        { declType = ValueType::Int;      kwText = "int"; }
            else if (match(TokenType::BoolKeyword))  { declType = ValueType::Bool;     kwText = "bool"; }
            else if (match(TokenType::FloatKeyword)) { declType = ValueType::Float;    kwText = "float"; }
            else {
                // long [long [int]]
                advance(); // consume first 'long'
                kwText = "long";
                if (match(TokenType::LongKeyword)) {
                    kwText = "long long";
                    if (match(TokenType::IntKeyword)) kwText = "long long int";
                } else if (match(TokenType::IntKeyword)) {
                    kwText = "long int";
                }
                declType = ValueType::LongLong;
            }
            consume(TokenType::Identifier, "Expected variable name after '" + kwText + "'");
            Token nameTok = previous();
            consume(TokenType::Equal, "Expected '=' after variable name");
            auto val = expression();
            // Note: no semicolon consumed here — the for-loop ';' separator follows
            auto letNode = std::make_unique<LetStmt>(declType, kwText, nameTok.lexeme, std::move(val));
            letNode->line = initLine;
            init = std::move(letNode);
        } else if (check(TokenType::Identifier) && checkNext(TokenType::Equal)) {
            // Assignment
            advance(); // identifier
            Token nameTok = previous();
            advance(); // '='
            auto val = expression();
            auto assignNode = std::make_unique<AssignStmt>(nameTok.lexeme, std::move(val));
            assignNode->line = initLine;
            init = std::move(assignNode);
        } else {
            parseError("Expected declaration or assignment in 'for' init clause", peek().line, peek().lexeme);
        }
    }
    consume(TokenType::Semicolon, "Expected ';' after 'for' init clause");

    // ── condition ─────────────────────────────────────────────────────────────
    std::unique_ptr<Expr> condition;
    if (!check(TokenType::Semicolon)) {
        condition = expression();
    }
    consume(TokenType::Semicolon, "Expected ';' after 'for' condition");

    // ── update ────────────────────────────────────────────────────────────────
    std::unique_ptr<Stmt> update;
    if (!check(TokenType::RightParen)) {
        int updLine = peek().line;
        if (check(TokenType::Identifier) && checkNext(TokenType::Equal)) {
            // i = expr
            advance();
            Token nameTok = previous();
            advance(); // '='
            auto val = expression();
            auto assignNode = std::make_unique<AssignStmt>(nameTok.lexeme, std::move(val));
            assignNode->line = updLine;
            update = std::move(assignNode);
        } else if (check(TokenType::Identifier) &&
                   current + 1 < tokens.size() &&
                   (tokens[current + 1].type == TokenType::PlusPlus ||
                    tokens[current + 1].type == TokenType::MinusMinus)) {
            // i++  /  i--  →  i = i + 1  /  i = i - 1
            advance();
            Token nameTok = previous();
            bool isInc = (peek().type == TokenType::PlusPlus);
            advance(); // consume ++ or --
            auto ident = std::make_unique<IdentifierExpr>(nameTok.lexeme);
            ident->line = updLine;
            auto one = std::make_unique<NumberExpr>("1");
            one->line = updLine;
            auto rhs = std::make_unique<BinaryExpr>(std::move(ident),
                                                    isInc ? "+" : "-",
                                                    std::move(one));
            rhs->line = updLine;
            auto assignNode = std::make_unique<AssignStmt>(nameTok.lexeme, std::move(rhs));
            assignNode->line = updLine;
            update = std::move(assignNode);
        } else if ((check(TokenType::PlusPlus) || check(TokenType::MinusMinus)) &&
                   current + 1 < tokens.size() &&
                   tokens[current + 1].type == TokenType::Identifier) {
            // ++i  /  --i  →  i = i + 1  /  i = i - 1
            bool isInc = (peek().type == TokenType::PlusPlus);
            advance(); // consume ++ or --
            Token nameTok = peek(); advance(); // consume identifier
            auto ident = std::make_unique<IdentifierExpr>(nameTok.lexeme);
            ident->line = updLine;
            auto one = std::make_unique<NumberExpr>("1");
            one->line = updLine;
            auto rhs = std::make_unique<BinaryExpr>(std::move(ident),
                                                    isInc ? "+" : "-",
                                                    std::move(one));
            rhs->line = updLine;
            auto assignNode = std::make_unique<AssignStmt>(nameTok.lexeme, std::move(rhs));
            assignNode->line = updLine;
            update = std::move(assignNode);
        } else {
            parseError("Expected assignment or increment/decrement in 'for' update clause",
                       peek().line, peek().lexeme);
        }
    }
    consume(TokenType::RightParen, "Expected ')' after 'for' clauses");

    // ── body ──────────────────────────────────────────────────────────────────
    loopDepth++;
    auto body = statement();
    loopDepth--;

    auto node = std::make_unique<ForStmt>(std::move(init), std::move(condition),
                                          std::move(update), std::move(body));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::breakStatement(int stmtLine) {
    if (loopDepth == 0)
        parseError("'break' used outside of a loop", stmtLine, "break");
    consume(TokenType::Semicolon, "Expected ';' after 'break'");
    auto node = std::make_unique<BreakStmt>();
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::continueStatement(int stmtLine) {
    if (loopDepth == 0)
        parseError("'continue' used outside of a loop", stmtLine, "continue");
    consume(TokenType::Semicolon, "Expected ';' after 'continue'");
    auto node = std::make_unique<ContinueStmt>();
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::printStatement(int stmtLine) {
    if (check(TokenType::Semicolon) || isAtEnd()) {
        parseError("Expected expression after 'print'", peek().line, peek().lexeme);
    }
    if (check(TokenType::Identifier) && checkNext(TokenType::Equal)) {
        parseError("Invalid assignment inside print statement", peek().line, peek().lexeme);
    }

    auto value = expression();

    if (check(TokenType::Equal)) {
        std::string ul = value ? value->toString() : peek().lexeme;
        parseError("Invalid assignment inside print statement", peek().line, ul);
    }

    consume(TokenType::Semicolon, "Expected ';' after print value");

    auto node = std::make_unique<PrintStmt>(std::move(value));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::assignmentStatement(int stmtLine) {
    consume(TokenType::Identifier, "Expected variable name in assignment");
    Token nameToken = previous();

    consume(TokenType::Equal, "Expected '=' in assignment");

    auto value = expression();
    consume(TokenType::Semicolon, "Expected ';' after assignment");

    auto node = std::make_unique<AssignStmt>(nameToken.lexeme, std::move(value));
    node->line = stmtLine;
    return node;
}

std::unique_ptr<Stmt> Parser::expressionStatement(int stmtLine) {
    auto expr = expression();

    if (check(TokenType::Equal)) {
        std::string ul = expr ? expr->toString() : previous().lexeme;
        parseError("Invalid assignment target", peek().line, ul);
    }

    consume(TokenType::Semicolon, "Expected ';' after expression");

    auto node = std::make_unique<ExprStmt>(std::move(expr));
    node->line = stmtLine;
    return node;
}

// ─── Expressions ─────────────────────────────────────────────────────────────

std::unique_ptr<Expr> Parser::expression() { return logicalOr(); }

std::unique_ptr<Expr> Parser::logicalOr() {
    auto expr = logicalAnd();

    while (true) {
        Token op;
        if      (match(TokenType::OrOr))                                op = previous();
        else if (match(TokenType::OrKw))                                op = previous();
        else break;

        auto right = logicalAnd();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), "or", std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::logicalAnd() {
    auto expr = bitwiseOr();

    while (true) {
        Token op;
        if      (match(TokenType::AndAnd))                               op = previous();
        else if (match(TokenType::AndKw))                                op = previous();
        else break;

        auto right = bitwiseOr();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), "and", std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::bitwiseOr() {
    auto expr = bitwiseXor();
    while (match(TokenType::Pipe)) {
        Token op = previous();
        auto right = bitwiseXor();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), "|", std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::bitwiseXor() {
    auto expr = bitwiseAnd();
    while (match(TokenType::Caret)) {
        Token op = previous();
        auto right = bitwiseAnd();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), "^", std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::bitwiseAnd() {
    auto expr = equality();
    while (match(TokenType::Ampersand)) {
        Token op = previous();
        auto right = equality();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), "&", std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::shift() {
    auto expr = term();
    while (match(TokenType::LessLess) || match(TokenType::GreaterGreater)) {
        Token op = previous();
        auto right = term();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    auto expr = comparison();
    while (match(TokenType::EqualEqual) || match(TokenType::NotEqual)) {
        Token op = previous();
        auto right = comparison();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    auto expr = shift();
    while (match(TokenType::Less)    || match(TokenType::LessEqual) ||
           match(TokenType::Greater) || match(TokenType::GreaterEqual)) {
        Token op = previous();
        auto right = shift();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    auto expr = factor();
    while (match(TokenType::Plus) || match(TokenType::Minus)) {
        Token op = previous();
        auto right = factor();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    auto expr = unary();
    while (match(TokenType::Star) || match(TokenType::Slash) || match(TokenType::Percent)) {
        Token op = previous();
        auto right = unary();
        auto node = std::make_unique<BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
        node->line = op.line;
        expr = std::move(node);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match(TokenType::Minus)) {
        Token op = previous();
        auto right = unary();
        auto node = std::make_unique<UnaryExpr>("-", std::move(right));
        node->line = op.line;
        return node;
    }

    if (match(TokenType::Tilde)) {
        Token op = previous();
        auto right = unary();
        auto node = std::make_unique<UnaryExpr>("~", std::move(right));
        node->line = op.line;
        return node;
    }

    if (match(TokenType::Bang) || match(TokenType::NotKw)) {
        Token op = previous();
        auto right = unary();
        auto node = std::make_unique<UnaryExpr>("not", std::move(right));
        node->line = op.line;
        return node;
    }

    return power();
}

std::unique_ptr<Expr> Parser::power() {
    auto expr = primary();

    if (match(TokenType::CaretCaret)) {
        Token op = previous();
        auto right = unary();   // right-associative
        auto node = std::make_unique<BinaryExpr>(std::move(expr), "^^", std::move(right));
        node->line = op.line;
        return node;
    }

    return expr;
}

// ─── tryCastType ─────────────────────────────────────────────────────────────
//
// Called when we see '(' and want to check if it's a C-style cast.
// Peeks at the token(s) after '(' without consuming anything.
// Returns {type, text} if it looks like a cast keyword, else {Unknown, ""}.
//
// The '(' has already been consumed by the caller when this is invoked.

Parser::CastType Parser::tryCastType() {
    // int
    if (check(TokenType::IntKeyword)) {
        // peek ahead: must be followed by ')'
        if (current + 1 < tokens.size() &&
            tokens[current + 1].type == TokenType::RightParen) {
            advance(); // consume 'int'
            return {ValueType::Int, "int"};
        }
    }

    // bool
    if (check(TokenType::BoolKeyword)) {
        if (current + 1 < tokens.size() &&
            tokens[current + 1].type == TokenType::RightParen) {
            advance();
            return {ValueType::Bool, "bool"};
        }
    }

    // float
    if (check(TokenType::FloatKeyword)) {
        if (current + 1 < tokens.size() &&
            tokens[current + 1].type == TokenType::RightParen) {
            advance();
            return {ValueType::Float, "float"};
        }
    }

    // long  /  long int  /  long long  /  long long int
    if (check(TokenType::LongKeyword)) {
        std::size_t saved = current;
        advance(); // consume 'long'

        std::string text = "long";

        if (!isAtEnd() && check(TokenType::LongKeyword)) {
            advance(); // consume second 'long'
            text = "long long";
            if (!isAtEnd() && check(TokenType::IntKeyword)) {
                advance();
                text = "long long int";
            }
        } else if (!isAtEnd() && check(TokenType::IntKeyword)) {
            advance();
            text = "long int";
        }

        if (!isAtEnd() && check(TokenType::RightParen)) {
            return {ValueType::LongLong, text};
        }

        // Not a cast — rewind
        current = saved;
    }

    return {ValueType::Unknown, ""};
}

// ─── primary ─────────────────────────────────────────────────────────────────

std::unique_ptr<Expr> Parser::primary() {

    // ── Number literal (integer or float) ────────────────────────────────────
    if (match(TokenType::Number)) {
        Token numTok = previous();
        const std::string& lex = numTok.lexeme;

        bool isFloat = (lex.find('.') != std::string::npos ||
                        lex.find('e') != std::string::npos ||
                        lex.find('E') != std::string::npos);

        if (isFloat) {
            auto node = std::make_unique<FloatExpr>(lex);
            node->line = numTok.line;
            return node;
        }

        // Integer — check for LL suffix
        if (match(TokenType::LL)) {
            auto node = std::make_unique<LongLongExpr>(lex);
            node->line = numTok.line;
            return node;
        }

        auto node = std::make_unique<NumberExpr>(lex);
        node->line = numTok.line;
        return node;
    }

    // ── Boolean literals ─────────────────────────────────────────────────────
    if (match(TokenType::True)) {
        auto node = std::make_unique<BoolExpr>(true);
        node->line = previous().line;
        return node;
    }
    if (match(TokenType::False)) {
        auto node = std::make_unique<BoolExpr>(false);
        node->line = previous().line;
        return node;
    }

    // ── input ─────────────────────────────────────────────────────────────────
    if (match(TokenType::Input)) {
        auto node = std::make_unique<InputExpr>();
        node->line = previous().line;
        return node;
    }

    // ── size(type)  →  bit-width of the type ─────────────────────────────────
    if (check(TokenType::Identifier) && peek().lexeme == "size") {
        int sizeLine = peek().line;
        advance(); // consume 'size'
        consume(TokenType::LeftParen, "Expected '(' after 'size'");

        // Parse the type name: bool | int | float | long [long] [int]
        ValueType   targetType = ValueType::Unknown;
        std::string typeName;
        int         bits = 0;

        if (match(TokenType::BoolKeyword)) {
            targetType = ValueType::Bool;     typeName = "bool";         bits = 1;
        } else if (match(TokenType::IntKeyword)) {
            targetType = ValueType::Int;      typeName = "int";          bits = 32;
        } else if (match(TokenType::FloatKeyword)) {
            targetType = ValueType::Float;    typeName = "float";        bits = 64;
        } else if (match(TokenType::LongKeyword)) {
            typeName = "long";
            if (match(TokenType::LongKeyword)) {
                typeName = "long long";
                if (match(TokenType::IntKeyword)) typeName = "long long int";
            } else if (match(TokenType::IntKeyword)) {
                typeName = "long int";
            }
            targetType = ValueType::LongLong; bits = 64;
        } else {
            parseError("Expected a type name inside 'size(...)'", peek().line, peek().lexeme);
        }

        consume(TokenType::RightParen, "Expected ')' after type in 'size(...)'");

        auto node = std::make_unique<SizeOfExpr>(targetType, typeName, bits);
        node->line = sizeLine;
        return node;
    }

    // ── Identifier ────────────────────────────────────────────────────────────
    if (match(TokenType::Identifier)) {
        auto node = std::make_unique<IdentifierExpr>(previous().lexeme);
        node->line = previous().line;
        return node;
    }

    // ── Parenthesised expression  OR  C-style cast ────────────────────────────
    if (match(TokenType::LeftParen)) {
        int parenLine = previous().line;

        // Try to parse as cast: (type)expr
        CastType ct = tryCastType();
        if (ct.type != ValueType::Unknown) {
            consume(TokenType::RightParen, "Expected ')' after cast type");

            auto operand = unary();  // cast binds like unary — applies to immediately following expr
            auto node = std::make_unique<CastExpr>(ct.type, ct.text, std::move(operand));
            node->line = parenLine;
            return node;
        }

        // Normal grouped expression
        auto expr = expression();
        consume(TokenType::RightParen, "Expected ')' after expression");
        return expr;
    }

    if (check(TokenType::Equal)) {
        parseError("Invalid assignment target", peek().line, peek().lexeme);
    }

    parseError("Unexpected token " + describeCurrentToken() + " in expression",
               peek().line, peek().lexeme);
}

// ─── Semantic checks ─────────────────────────────────────────────────────────

// ── ScopeStack helpers ────────────────────────────────────────────────────────

bool Parser::isDeclared(const ScopeStack& scopes, const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
        if (it->count(name)) return true;
    return false;
}

bool Parser::isDeclaredInCurrentScope(const ScopeStack& scopes, const std::string& name) const {
    if (scopes.empty()) return false;
    return scopes.back().count(name) > 0;
}

void Parser::declare(ScopeStack& scopes, const std::string& name, int line) const {
    if (isDeclaredInCurrentScope(scopes, name))
        semanticError("Variable '" + name + "' is already declared in this scope", line, name);
    scopes.back().insert(name);
}

Parser::ScopeStack Parser::pushScope(ScopeStack scopes) const {
    scopes.push_back(ScopeLevel{});
    return scopes;
}

Parser::ScopeStack Parser::popScope(ScopeStack scopes) const {
    if (!scopes.empty()) scopes.pop_back();
    return scopes;
}

void Parser::runSemanticChecks(const std::vector<std::unique_ptr<Stmt>>& statements) const {
    ScopeStack scopes;
    scopes.push_back(ScopeLevel{});  // global scope
    checkStatements(statements, std::move(scopes));
}

Parser::ScopeStack Parser::checkStatements(
    const std::vector<std::unique_ptr<Stmt>>& statements,
    ScopeStack scopes) const
{
    for (const auto& s : statements)
        scopes = checkStatement(s.get(), std::move(scopes));
    return scopes;
}

Parser::ScopeStack Parser::checkStatement(const Stmt* stmt, ScopeStack scopes) const {
    if (const auto* letStmt = dynamic_cast<const LetStmt*>(stmt)) {
        // Check RHS before declaring name (prevents `int x = x + 1` from passing)
        checkExpression(letStmt->value.get(), scopes);
        declare(scopes, letStmt->name, letStmt->line);
        return scopes;
    }
    if (const auto* printStmt = dynamic_cast<const PrintStmt*>(stmt)) {
        checkExpression(printStmt->value.get(), scopes);
        return scopes;
    }
    if (const auto* assignStmt = dynamic_cast<const AssignStmt*>(stmt)) {
        if (!isDeclared(scopes, assignStmt->name))
            semanticError("Undefined variable '" + assignStmt->name + "'",
                          assignStmt->line, assignStmt->name);
        checkExpression(assignStmt->value.get(), scopes);
        return scopes;
    }
    if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(stmt)) {
        // Push a new scope, process block, pop — variables declared inside don't leak out
        ScopeStack inner = pushScope(scopes);
        inner = checkStatements(blockStmt->statements, std::move(inner));
        return popScope(std::move(inner));
    }
    if (const auto* ifStmt = dynamic_cast<const IfStmt*>(stmt)) {
        checkExpression(ifStmt->condition.get(), scopes);
        // Each branch gets its own child scope
        ScopeStack thenScopes = checkStatement(ifStmt->thenBranch.get(), pushScope(scopes));
        thenScopes = popScope(std::move(thenScopes));
        if (!ifStmt->elseBranch) return scopes;  // no else → outer scope unchanged
        ScopeStack elseScopes = checkStatement(ifStmt->elseBranch.get(), pushScope(scopes));
        elseScopes = popScope(std::move(elseScopes));
        return intersectScopes(scopes, thenScopes, elseScopes);
    }
    if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(stmt)) {
        checkExpression(whileStmt->condition.get(), scopes);
        // Body gets its own scope; declarations inside don't escape
        ScopeStack bodyScopes = checkStatement(whileStmt->body.get(), pushScope(scopes));
        (void)popScope(std::move(bodyScopes));
        return scopes;
    }
    if (const auto* forStmt = dynamic_cast<const ForStmt*>(stmt)) {
        // The for-loop introduces a new scope for the init variable
        ScopeStack forScopes = pushScope(scopes);
        if (forStmt->init)      forScopes = checkStatement(forStmt->init.get(), std::move(forScopes));
        if (forStmt->condition) checkExpression(forStmt->condition.get(), forScopes);
        if (forStmt->update)    forScopes = checkStatement(forStmt->update.get(), std::move(forScopes));
        ScopeStack bodyScopes = checkStatement(forStmt->body.get(), pushScope(forScopes));
        (void)popScope(std::move(bodyScopes));
        return scopes;  // init variable doesn't escape the for-loop
    }
    // break/continue are syntactically validated in the parser (loopDepth check)
    if (dynamic_cast<const BreakStmt*>(stmt))    return scopes;
    if (dynamic_cast<const ContinueStmt*>(stmt)) return scopes;
    if (const auto* exprStmt = dynamic_cast<const ExprStmt*>(stmt)) {
        checkExpression(exprStmt->expression.get(), scopes);
        return scopes;
    }
    return scopes;
}

void Parser::checkExpression(const Expr* expr, const ScopeStack& scopes) const {
    if (!expr) return;

    if (const auto* identExpr = dynamic_cast<const IdentifierExpr*>(expr)) {
        if (!isDeclared(scopes, identExpr->name))
            semanticError("Undefined variable '" + identExpr->name + "'",
                          identExpr->line, identExpr->name);
        return;
    }
    if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(expr)) {
        checkExpression(unaryExpr->right.get(), scopes);
        return;
    }
    if (const auto* binaryExpr = dynamic_cast<const BinaryExpr*>(expr)) {
        checkExpression(binaryExpr->left.get(), scopes);
        checkExpression(binaryExpr->right.get(), scopes);
        return;
    }
    if (const auto* castExpr = dynamic_cast<const CastExpr*>(expr)) {
        checkExpression(castExpr->operand.get(), scopes);
        return;
    }
    if (dynamic_cast<const SizeOfExpr*>(expr)) return;
    // NumberExpr, LongLongExpr, FloatExpr, BoolExpr, InputExpr — no identifiers to check
}

Parser::ScopeStack Parser::intersectScopes(const ScopeStack& base,
                                            const ScopeStack& left,
                                            const ScopeStack& right) const {
    // Start from the base (pre-branch) scope, then add names that were declared
    // in both branches so they're visible after the if/else.
    ScopeStack result = base;
    if (result.empty()) return result;

    // Collect all names added in the left branch (beyond base)
    std::unordered_set<std::string> leftNew, rightNew;
    for (const auto& level : left)
        for (const auto& name : level)
            if (!isDeclared(base, name)) leftNew.insert(name);
    for (const auto& level : right)
        for (const auto& name : level)
            if (!isDeclared(base, name)) rightNew.insert(name);

    // Only names declared in both branches are visible after the if/else
    for (const auto& name : leftNew)
        if (rightNew.count(name)) result.back().insert(name);

    return result;
}

// ─── Token helpers ────────────────────────────────────────────────────────────

bool Parser::match(TokenType type) {
    if (check(type)) { advance(); return true; }
    return false;
}

void Parser::consume(TokenType type, const std::string& message) {
    if (match(type)) return;
    int errLine  = isAtEnd() ? previous().line : peek().line;
    std::string tok = isAtEnd() ? previous().lexeme : peek().lexeme;
    parseError(message + ". Found " + describeCurrentToken(), errLine, tok);
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return type == TokenType::EndOfFile;
    return peek().type == type;
}

bool Parser::checkNext(TokenType type) const {
    if (current + 1 >= tokens.size()) return false;
    return tokens[current + 1].type == type;
}

const Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

const Token& Parser::peek()     const { return tokens[current]; }
const Token& Parser::previous() const { return tokens[current - 1]; }
bool         Parser::isAtEnd()  const { return peek().type == TokenType::EndOfFile; }

void Parser::parseError(const std::string& message, int line, const std::string& token) const {
    const_cast<Parser*>(this)->errorMessage =
        "[Parse Error] [Line " + std::to_string(line) + "]:\n" + message;
    throw ParseError(message, line, token);
}

void Parser::parseError(const std::string& message) const {
    int errLine = isAtEnd() ? previous().line : peek().line;
    std::string tok = isAtEnd() ? previous().lexeme : peek().lexeme;
    parseError(message, errLine, tok);
}

void Parser::semanticError(const std::string& message, int line,
                            const std::string& token) const {
    const_cast<Parser*>(this)->errorMessage =
        "[Semantic Error] [Line " + std::to_string(line) + "]:\n" + message;
    throw SemanticError(message, line, token);
}

std::string Parser::describeToken(const Token& token) const {
    if (token.type == TokenType::EndOfFile) return "end of file";
    if (token.lexeme.empty())
        return std::string("token ") + tokenTypeToString(token.type);
    return "'" + token.lexeme + "'";
}

std::string Parser::describeCurrentToken() const { return describeToken(peek()); }