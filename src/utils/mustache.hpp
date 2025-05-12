
#pragma once

#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>

#include "msgvar.hpp"

namespace fin::mustache
{
    using pair        = std::pair<std::string_view, std::string_view>;
    using PartialList = std::span<std::pair<std::string_view, std::string_view>>;

     /*
     * This is a minimal implementation of the Mustache templating language.
     * It supports the following features:
     * - Variables: {{variable}}
     * - Sections: {{#section}}...{{/section}}
     * - Inverted Sections: {{^section}}...{{/section}}
     * - Partials: {{>partial}}
     * - Delimiters: {{=open close=}}
     * - Comments: {{!comment}}
     */
    std::string render(std::string_view templateContents, const msg::Var& data, const PartialList& partials = {});




    namespace intern
    {
        enum class TokenType
        {
            Text,
            Comment,
            Section,
            InvertedSection,
            EndSection,
            Partial,
            Delimiter,
        };

        struct Token
        {
            std::string_view name;
            TokenType        type;
            bool             htmlEscape;
        };

        struct DelimiterPair
        {
            std::string open;
            std::string close;
        };

        struct Range
        {
            size_t start;
            size_t end;
        };

        struct ExclusionRange
        {
            size_t start;
            size_t end;
        };

        static ExclusionRange getExclusionRangeForToken(std::string_view templateContents, const Range& range, TokenType tokenType)
        {
            ExclusionRange defaultResult{
                .start = range.start,
                .end   = range.end,
            };

            if (tokenType == TokenType::Text)
            {
                // Text tokens are never standalone
                return defaultResult;
            }

            auto lineStart = templateContents.find_last_of('\n', range.start) + 1; // 0 if this is the first line
            auto lineEnd   = templateContents.find('\n', range.end);               // -1 if this is the last line
            if (lineEnd == -1)
            {
                lineEnd = (templateContents.length());
            }
            bool standalone = true;
            for (size_t i = lineStart; i < (range.start); i++)
            {
                if (isspace(templateContents[i]) == 0)
                {
                    standalone = false;
                    break;
                }
            }
            for (size_t i = (range.end); i < lineEnd; i++)
            {
                if (isspace(templateContents[i]) == 0)
                {
                    standalone = false;
                    break;
                }
            }
            if (!standalone)
            {
                return defaultResult;
            }

            // If the token is on the very last line of the template, then remove the preceding newline,
            // but only if there's no leading whitespace before the token
            if (lineEnd == (templateContents.length()) && lineStart > 0 && lineStart == (range.start))
            {
                lineStart--;
                // Also remove any preceding carriage return
                if (lineStart > 0 && templateContents[lineStart] == '\r')
                {
                    lineStart--;
                }
            }
            else
            {
                // Remove the trailing newline
                lineEnd++;
            }
            return {
                .start = (lineStart),
                .end   = (lineEnd),
            };
        }

        static msg::Var lookupTokenInContextStack(const std::string_view& name, const std::vector<msg::Var>& contextStack)
        {
            if (name == ".")
            {
                return contextStack.back();
            }
            auto ctx = contextStack.back();
            return ctx.find(name);
        }

        static std::string renderToken(const Token& token, const std::vector<msg::Var>& contextStack)
        {
            auto        node = lookupTokenInContextStack(token.name, contextStack);
            std::string result;

            if (node.is_string())
            {
                if (token.htmlEscape)
                {
                    for (auto ch : node.str())
                    {
                        switch (ch)
                        {
                            case '&':
                                result.append("&amp;");
                                break;
                            case '"':
                                result.append("&quot;");
                                break;
                            case '<':
                                result.append("&lt;");
                                break;
                            case '>':
                                result.append("&gt;");
                                break;
                            default:
                                result.push_back(ch);
                        }
                    }
                }
                else
                {
                    result.append(node.str());
                }
            }
            else if (!node.is_null() && !node.is_undefined())
            {
                node.to_string(result);
            }

            return result;
        }

        // Given a const char * at the start of a token sequence "{{", extract the token name and return a
        // pair of the token name and the const char * at the end of the token sequence
        std::pair<Token, size_t> parseTokenAtPoint(std::string_view templateContents, size_t position, const DelimiterPair& delimiters)
        {
            assert(templateContents.substr(position, delimiters.open.length()) == delimiters.open);
            position += delimiters.open.length();
            std::string closeTag(delimiters.close);

            bool      escapeHtml = true;
            TokenType type       = TokenType::Text;
            auto      nextChar   = templateContents[position];
            switch (nextChar)
            {
                case '{':
                    escapeHtml = false;
                    position++;
                    closeTag = "}";
                    closeTag.append(delimiters.close);
                    break;
                case '!':
                    type = TokenType::Comment;
                    position++;
                    break;
                case '#':
                    type = TokenType::Section;
                    position++;
                    break;
                case '^':
                    type = TokenType::InvertedSection;
                    position++;
                    break;
                case '/':
                    type = TokenType::EndSection;
                    position++;
                    break;
                case '>':
                    type = TokenType::Partial;
                    position++;
                    break;
                case '=':
                    type = TokenType::Delimiter;
                    position++;
                    closeTag = "=";
                    closeTag.append(delimiters.close);
                    break;
                default:
                    break;
            }

            while (templateContents[position] == ' ')
            {
                position++;
            }

            if (templateContents[position] == '&')
            {
                escapeHtml = false;
                position++;
            }

            while (templateContents[position] == ' ')
            {
                position++;
            }

            // figure out the token name
            auto closeTagPosition = templateContents.find(closeTag, position);
            // printf("Close tag position: %lu\n", closeTagPosition);
            if (closeTagPosition == -1)
            {
                return std::make_pair(Token{.name = "", .type = type, .htmlEscape = escapeHtml}, templateContents.length());
            }
            size_t tokenEnd = closeTagPosition;
            while (tokenEnd > position && templateContents[tokenEnd - 1] == ' ')
            {
                tokenEnd--;
            }
            return std::make_pair(Token{.name       = templateContents.substr(position, tokenEnd - position),
                                        .type       = type,
                                        .htmlEscape = escapeHtml},
                                  closeTagPosition + closeTag.length());
        }

        inline bool isFalsy(msg::Var& value)
        {
            if (value.is_null() || value.is_undefined())
            {
                return true;
            }
            if (value.is_array())
            {
                return value.size() == 0;
            }
            if (value.is_bool())
            {
                return !value.get(false);
            }
            return false;
        }

        // TODO(floatplane) yeah, I know it's a mess
        // NOLINTNEXTLINE(readability-function-cognitive-complexity)
        static std::pair<std::string, size_t> renderWithContextStack(
            std::string_view       templateContents,
            size_t                 position,
            std::vector<msg::Var>& contextStack,
            const PartialList&     partials,
            bool                   renderingEnabled,
            const DelimiterPair&   initialDelimiters = {.open = "{{", .close = "}}"})
        {
            std::string   result;
            DelimiterPair delimiters = initialDelimiters;
            while (position < templateContents.length())
            {
                auto nextToken = templateContents.find(delimiters.open, position);
                if (nextToken == -1)
                {
                    // No more tokens, so just render the rest of the template (if necessary) and return
                    if (renderingEnabled)
                    {
                        // TODO(floatplane): I think it would be an error if renderingEnabled is false here -
                        // would indicate an unclosed tag
                        result.append(templateContents.substr(position, templateContents.length() - position));
                    }
                    break;
                }

                const auto parsedToken        = parseTokenAtPoint(templateContents, nextToken, delimiters);
                const auto tokenRenderExtents = getExclusionRangeForToken(templateContents,
                                                                          {.start = static_cast<size_t>(nextToken),
                                                                           .end   = parsedToken.second},
                                                                          parsedToken.first.type);
                if (renderingEnabled)
                {
                    result.append(templateContents.substr(position, tokenRenderExtents.start - position));
                }
                const Token token = parsedToken.first;
                switch (token.type)
                {
                    case TokenType::Text:
                        if (renderingEnabled)
                        {
                            result.append(renderToken(token, contextStack));
                        }
                        position = tokenRenderExtents.end;
                        break;

                    case TokenType::Section:
                    {
                        auto context = lookupTokenInContextStack(token.name, contextStack);
                        auto falsy   = isFalsy(context);
                        if (!falsy && context.is_array())
                        {
                            auto array = context;
                            for (uint32_t i = 0; i < array.size(); i++)
                            {
                                contextStack.push_back(array[i]);
                                auto sectionResult = renderWithContextStack(templateContents,
                                                                            tokenRenderExtents.end,
                                                                            contextStack,
                                                                            partials,
                                                                            renderingEnabled,
                                                                            delimiters);
                                if (renderingEnabled)
                                {
                                    result.append(sectionResult.first);
                                }
                                contextStack.pop_back();
                                position = sectionResult.second;
                            }
                        }
                        else
                        {
                            contextStack.push_back(context);
                            auto sectionResult = renderWithContextStack(templateContents,
                                                                        tokenRenderExtents.end,
                                                                        contextStack,
                                                                        partials,
                                                                        renderingEnabled && !falsy,
                                                                        delimiters);
                            if (renderingEnabled)
                            {
                                result.append(sectionResult.first);
                            }
                            contextStack.pop_back();
                            position = sectionResult.second;
                        }
                        break;
                    }

                    case TokenType::InvertedSection:
                    {
                        // check token for falsy value, render if falsy
                        auto context       = lookupTokenInContextStack(token.name, contextStack);
                        auto falsy         = isFalsy(context);
                        auto sectionResult = renderWithContextStack(templateContents,
                                                                    tokenRenderExtents.end,
                                                                    contextStack,
                                                                    partials,
                                                                    renderingEnabled && falsy,
                                                                    delimiters);
                        if (renderingEnabled)
                        {
                            result.append(sectionResult.first);
                        }
                        position = sectionResult.second;
                        break;
                    }

                    case TokenType::EndSection:
                        return std::make_pair(result, tokenRenderExtents.end);

                    case TokenType::Partial:
                        if (renderingEnabled)
                        {
                            // find partial in partials list and render with it
                            auto partial = std::find_if(partials.begin(),
                                                        partials.end(),
                                                        [&token](const auto& partial)
                                                        { return partial.first == token.name; });
                            if (partial != partials.end())
                            {
                                // Partials must match the indentation of the token that references them
                                //   std::string indentedPartial = indentLines(partial->second, tokenRenderExtents.indentation);
                                // NB: we don't pass custom delimiters down to a partial
                                std::string partialResult = renderWithContextStack(partial->second, 0, contextStack, partials, renderingEnabled)
                                                                .first;
                                result.append(partialResult);
                            }
                        }
                        position = tokenRenderExtents.end;
                        break;

                    case TokenType::Delimiter:
                        // The name will look like "<% %>" - we need to split it on whitespace, and
                        // assign the start and end delimiters
                        delimiters.open  = token.name.substr(0, token.name.find(' '));
                        position         = token.name.find_last_of(' ', token.name.length() - 1) + 1;
                        delimiters.close = token.name.substr(position);
                        position         = tokenRenderExtents.end;
                        break;

                    default:
                        position = tokenRenderExtents.end;
                        break;
                }
            }
            return std::make_pair(result, position);
        }
    }

    inline std::string render(std::string_view templateContents, const msg::Var& data, const PartialList& partials)
    {
        std::vector<msg::Var> contextStack;
        contextStack.push_back(data);
        const auto result = intern::renderWithContextStack(templateContents, 0, contextStack, partials, true);
        return result.first;
    }

} // namespace fin::mustache
