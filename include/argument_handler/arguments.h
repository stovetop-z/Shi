#pragma once
#include "../common/common.h"

namespace arguments
{

    constexpr std::string_view exec = "shi";
    
    inline const std::unordered_set<std::string_view> valid_commands = {
        "init", "add", "sync", "cat-stage", "commit"
    };

    inline const std::unordered_set<std::string_view> valid_flags = {
        "-h", "--help", "-a", "-all"
    };

    enum class Argument
    {
        WORD,
        FLAG
    };

    struct Token
    {
        Argument type;
        std::string value;
    };

    struct Parsed
    {
        std::string command;
        std::vector<Token> flags;
        std::vector<fs::path> input;
    };

    inline std::vector<Token> lex(std::string_view command_line)
    {
        std::vector<Token> tokens;
        std::stringstream ss{std::string(command_line)};
        std::string token;

        while(ss >> token)
        {
            if(token.rfind('-', 0) == 0 && token.size() > 1)
            {
                tokens.push_back({ Argument::FLAG, std::move(token) });
            }
            else
            {
                tokens.push_back({ Argument::WORD, std::move(token) });
            }
        }
        return tokens;
    }

    inline std::vector<Token> lex(int argc, const char* argv[])
    {
        std::vector<Token> tokens;
        for (int i = 0; i < argc; ++i)
        {
            std::string token = argv[i];
            if(token.rfind('-', 0) == 0 && token.size() > 1)
            {
                tokens.push_back({ Argument::FLAG, std::move(token) });
            }
            else
            {
                tokens.push_back({ Argument::WORD, std::move(token) });
            }
        }
        return tokens;
    }

    class Parser
    {
    public:
        static Parsed parse(const std::vector<Token>& tokens, logger::ErrorCode& error)
        {
            Parsed parsed;
            size_t cursor = 0;

            auto hasMore = [&]() { return cursor < tokens.size(); };
            auto peek    = [&]() -> const Token& { return tokens[cursor]; };
            auto advance = [&]() -> const Token& { return tokens[cursor++]; };

            if(!hasMore())
            {
                setError(error, logger::ErrorCodes::INVALID_EXEC, "No arguments provided");
                return parsed;
            }

            const Token& first = advance();
            if(first.type != Argument::WORD || first.value != exec)
            {
                setError(error, logger::ErrorCodes::INVALID_EXEC, "Expected '" + std::string(exec) + "' got: " + first.value);
                return parsed;
            }

            if(!hasMore())
            {
                setError(error, logger::ErrorCodes::INVALID_ARG, "Expected command after '" + std::string(exec) + "'");
                return parsed;
            }

            const Token& cmdToken = advance();
            if(cmdToken.type != Argument::WORD || valid_commands.find(cmdToken.value) == valid_commands.end())
            {
                setError(error, logger::ErrorCodes::INVALID_ARG, "Unknown or invalid command: " + cmdToken.value);
                return parsed;
            }
            parsed.command = cmdToken.value;

            while(hasMore() && peek().type == Argument::FLAG)
            {
                const Token& flagToken = advance();
                if(valid_flags.find(flagToken.value) == valid_flags.end())
                {
                    setError(error, logger::ErrorCodes::INVALID_ARG, "Unknown flag: " + flagToken.value);
                    return parsed;
                }
                parsed.flags.push_back(flagToken);
            }

            if(!hasMore())
            {
                setError(error, logger::ErrorCodes::INVALID_ARG, "Expected <input> argument for command: " + parsed.command);
                return parsed;
            }

            while(hasMore() && peek().type == Argument::WORD)
            {
                parsed.input.emplace_back(advance().value);
            }

            if(hasMore())
            {
                setError(error, logger::ErrorCodes::INVALID_ARG, "Flags must precede inputs; unexpected token: " + peek().value);
                return parsed;
            }

            return parsed;
        }

    private:
        static void setError(logger::ErrorCode& error, logger::ErrorCodes code, const std::string& msg)
        {
            logger::log(logger::level::ERROR, msg);
            error = logger::ErrorCode{ .value = code, .message = msg };
        }
    };

    inline Parsed parse(const std::vector<Token>& tokens, logger::ErrorCode& error)
    {
        return Parser::parse(tokens, error);
    }
}