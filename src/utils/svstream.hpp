#include <string_view>

#include <cctype>
#include <cstring>

namespace std
{
    class string_view_stream
    {
    public:
        explicit string_view_stream(std::string_view sv) noexcept : view(sv), pos(0)
        {
        }

        [[nodiscard]] bool eof() const noexcept
        {
            return pos >= view.size();
        }

        [[nodiscard]] char peek() const noexcept
        {
            return eof() ? '\0' : view[pos];
        }

        [[nodiscard]] char get() noexcept
        {
            return eof() ? '\0' : view[pos++];
        }

        void unget() noexcept
        {
            if (pos > 0)
                --pos;
        }

        void skipWhitespace() noexcept
        {
            while (!eof() && std::isspace(peek()))
                ++pos;
        }

        bool expect(char c) noexcept
        {
            if (peek() == c)
            {
                ++pos;
                return true;
            }
            return false;
        }

        // Extracts an identifier [a-zA-Z_][a-zA-Z0-9_]*
        std::string_view parseIdentifier() noexcept
        {
            skipWhitespace();
            size_t start = pos;

            if (eof() || !(std::isalpha(peek()) || peek() == '_'))
                return "";

            ++pos;
            while (!eof() && (std::isalnum(peek()) || peek() == '_'))
                ++pos;

            return view.substr(start, pos - start);
        }

        // Extract number (only unsigned for simplicity)
        int parseInteger() noexcept
        {
            skipWhitespace();
            int result = 0;
            while (!eof() && std::isdigit(peek()))
            {
                result = result * 10 + (get() - '0');
            }
            return result;
        }

        // Consume until next token boundary
        void skipUntil(char c) noexcept
        {
            while (!eof() && peek() != c)
                ++pos;
            if (!eof())
                ++pos;
        }

        [[nodiscard]] std::string_view remaining() const noexcept
        {
            return view.substr(pos);
        }

        void reset(std::string_view newView) noexcept
        {
            view = newView;
            pos  = 0;
        }

    private:
        std::string_view view;
        size_t           pos;
    };
} // namespace std
