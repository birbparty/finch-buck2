#include <finch/parser/lexer/lexer.hpp>
#include <gtest/gtest.h>

using namespace finch::lexer;

class LexerTest : public ::testing::Test {
  protected:
    std::vector<Token> tokenize(const std::string& source) {
        Lexer lexer(source, "test.cmake");
        std::vector<Token> tokens;

        while (true) {
            auto result = lexer.next_token();
            EXPECT_TRUE(result.has_value()) << "Lexer error: " << result.error().message();

            if (!result.has_value()) {
                break;
            }

            Token token = std::move(result.value());
            bool is_eof = token.is(TokenType::Eof);
            tokens.push_back(std::move(token));

            if (is_eof) {
                break;
            }
        }

        return tokens;
    }
};

TEST_F(LexerTest, BasicTokens) {
    auto tokens = tokenize("add_library(mylib STATIC)");

    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0].type, TokenType::Identifier);
    EXPECT_EQ(*tokens[0].get_value<std::string>(), "add_library");

    EXPECT_EQ(tokens[1].type, TokenType::LeftParen);
    EXPECT_EQ(tokens[2].type, TokenType::Identifier);
    EXPECT_EQ(*tokens[2].get_value<std::string>(), "mylib");

    EXPECT_EQ(tokens[3].type, TokenType::Identifier);
    EXPECT_EQ(*tokens[3].get_value<std::string>(), "STATIC");

    EXPECT_EQ(tokens[4].type, TokenType::RightParen);
    EXPECT_EQ(tokens[5].type, TokenType::Eof);
}

TEST_F(LexerTest, StringLiterals) {
    auto tokens = tokenize("\"hello world\" \"escaped\\\"quote\\\"\"");

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::String);
    EXPECT_EQ(*tokens[0].get_value<std::string>(), "hello world");

    EXPECT_EQ(tokens[1].type, TokenType::String);
    EXPECT_EQ(*tokens[1].get_value<std::string>(), "escaped\"quote\"");

    EXPECT_EQ(tokens[2].type, TokenType::Eof);
}

TEST_F(LexerTest, Variables) {
    auto tokens = tokenize("${VAR} ${NESTED_${INNER}}");

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::Variable);
    EXPECT_EQ(*tokens[0].get_value<std::string>(), "VAR");

    EXPECT_EQ(tokens[1].type, TokenType::Variable);
    EXPECT_EQ(*tokens[1].get_value<std::string>(), "NESTED_${INNER}");

    EXPECT_EQ(tokens[2].type, TokenType::Eof);
}

TEST_F(LexerTest, GeneratorExpressions) {
    auto tokens = tokenize("$<CONFIG:Release> $<TARGET_FILE:mylib>");

    ASSERT_EQ(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, TokenType::GeneratorExpr);
    EXPECT_EQ(*tokens[0].get_value<std::string>(), "CONFIG:Release");

    EXPECT_EQ(tokens[1].type, TokenType::GeneratorExpr);
    EXPECT_EQ(*tokens[1].get_value<std::string>(), "TARGET_FILE:mylib");

    EXPECT_EQ(tokens[2].type, TokenType::Eof);
}

TEST_F(LexerTest, Numbers) {
    auto tokens = tokenize("123 45.67 1.23e-4");

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::Number);
    EXPECT_DOUBLE_EQ(*tokens[0].get_value<double>(), 123.0);

    EXPECT_EQ(tokens[1].type, TokenType::Number);
    EXPECT_DOUBLE_EQ(*tokens[1].get_value<double>(), 45.67);

    EXPECT_EQ(tokens[2].type, TokenType::Number);
    EXPECT_DOUBLE_EQ(*tokens[2].get_value<double>(), 1.23e-4);

    EXPECT_EQ(tokens[3].type, TokenType::Eof);
}

TEST_F(LexerTest, Comments) {
    auto tokens = tokenize("command # line comment\nother");

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::Identifier);
    EXPECT_EQ(*tokens[0].get_value<std::string>(), "command");

    EXPECT_EQ(tokens[1].type, TokenType::Newline);

    EXPECT_EQ(tokens[2].type, TokenType::Identifier);
    EXPECT_EQ(*tokens[2].get_value<std::string>(), "other");

    EXPECT_EQ(tokens[3].type, TokenType::Eof);
}

TEST_F(LexerTest, WhitespaceHandling) {
    auto tokens = tokenize("cmd   \t  arg");

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::Identifier);
    EXPECT_EQ(tokens[1].type, TokenType::Whitespace);
    EXPECT_EQ(tokens[2].type, TokenType::Identifier);
    EXPECT_EQ(tokens[3].type, TokenType::Eof);
}

TEST_F(LexerTest, UnquotedArguments) {
    auto tokens = tokenize("arg_with-dashes 123abc /path/to/file");

    ASSERT_EQ(tokens.size(), 4);
    EXPECT_EQ(tokens[0].type, TokenType::Identifier);
    EXPECT_EQ(*tokens[0].get_value<std::string>(), "arg_with-dashes");

    EXPECT_EQ(tokens[1].type, TokenType::Identifier);
    EXPECT_EQ(*tokens[1].get_value<std::string>(), "123abc");

    EXPECT_EQ(tokens[2].type, TokenType::Identifier);
    EXPECT_EQ(*tokens[2].get_value<std::string>(), "/path/to/file");

    EXPECT_EQ(tokens[3].type, TokenType::Eof);
}

TEST_F(LexerTest, ErrorRecovery) {
    Lexer lexer("\"unterminated string", "test.cmake");

    auto result = lexer.next_token();
    ASSERT_TRUE(result.has_error());
    EXPECT_NE(result.error().message().find("Unterminated"), std::string::npos);
}

TEST_F(LexerTest, SourceLocations) {
    auto tokens = tokenize("cmd\narg");

    ASSERT_EQ(tokens.size(), 4);

    // First token should be at line 1, column 1
    EXPECT_EQ(tokens[0].location.line, 1);
    EXPECT_EQ(tokens[0].location.column, 1);

    // Newline should be at line 1, column 4
    EXPECT_EQ(tokens[1].location.line, 1);
    EXPECT_EQ(tokens[1].location.column, 4);

    // Second identifier should be at line 2, column 1
    EXPECT_EQ(tokens[2].location.line, 2);
    EXPECT_EQ(tokens[2].location.column, 1);
}
