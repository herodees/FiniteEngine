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

        void skip_whitespace() noexcept
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
        std::string_view parse_identifier() noexcept
        {
            skip_whitespace();
            size_t start = pos;

            if (eof() || !(std::isalpha(peek()) || peek() == '_'))
                return "";

            ++pos;
            while (!eof() && (std::isalnum(peek()) || peek() == '_'))
                ++pos;

            return view.substr(start, pos - start);
        }

        // Extract number
        int parse_integer() noexcept
        {
            skip_whitespace();

            int sign = 1;
            if (peek() == '-')
            {
                sign = -1;
                ++pos;
            }
            else if (peek() == '+')
            {
                ++pos;
            }

            int result = 0;
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek())))
            {
                result = result * 10 + (get() - '0');
            }
            return result * sign;
        }

        double parse_real() noexcept
        {
            skip_whitespace();

            int sign = 1;
            if (peek() == '+')
            {
                ++pos;
            }
            else if (peek() == '-')
            {
                sign = -1;
                ++pos;
            }

            double result = 0.0;

            // Integer part
            while (!eof() && std::isdigit(static_cast<unsigned char>(peek())))
            {
                result = result * 10.0 + (get() - '0');
            }

            // Fractional part
            if (peek() == '.')
            {
                ++pos;
                double divisor = 10.0;

                while (!eof() && std::isdigit(static_cast<unsigned char>(peek())))
                {
                    result += (get() - '0') / divisor;
                    divisor *= 10.0;
                }
            }

            // Exponent part
            if (peek() == 'e' || peek() == 'E')
            {
                ++pos;

                int exp_sign = 1;
                if (peek() == '+')
                {
                    ++pos;
                }
                else if (peek() == '-')
                {
                    exp_sign = -1;
                    ++pos;
                }

                int exponent = 0;
                while (!eof() && std::isdigit(static_cast<unsigned char>(peek())))
                {
                    exponent = exponent * 10 + (get() - '0');
                }

                result *= std::pow(10.0, exp_sign * exponent);
            }

            return result * sign;
        }

        // Consume until next token boundary
        void skip_until(char c) noexcept
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
