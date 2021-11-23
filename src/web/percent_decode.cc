#include "web/percent_decode.h"

#include <string.h>

namespace openmc
{
    namespace web
    {
        enum URLParserState
        {
            kIdle,
            kPercentFirstChar,
            kPercentSecondChar
        };

        static URLParserState state = kIdle;

        char *decoded_output;
        static char percent_first_char;

        static int HexToInt(const char hex);
        static void ProcessNextChar(const char c);
        static inline void PushChar(const char c);

        static int HexToInt(const char hex)
        {
            if (hex >= '0' && hex <= '9')
            {
                return hex - 48;
            }

            if (hex >= 'A' && hex <= 'F')
            {
                return hex - 55;
            }

            if (hex >= 'a' && hex <= 'f')
            {
                return hex - 87;
            }

            // Not a valid hex digit
            return -1;
        }

        static inline void PushChar(const char c)
        {
            // idk strncat spazzes if not properly formed
            char temp[2] = {c, '\0'};
            strncat(decoded_output, temp, 2);
        }

        static void ProcessNextChar(const char c)
        {
            switch (state)
            {
            case URLParserState::kIdle:
                if (c == '%')
                {
                    state = URLParserState::kPercentFirstChar;
                }
                else if (c == '+')
                {
                    PushChar(' ');
                }
                else
                {
                    PushChar(c);
                }
                break;

            case URLParserState::kPercentFirstChar:
                if (HexToInt(c) >= 0)
                {
                    percent_first_char = c;
                    state = URLParserState::kPercentSecondChar;
                }
                else
                {
                    PushChar('%');
                    PushChar(c);
                    state = URLParserState::kIdle;
                }
                break;

            case URLParserState::kPercentSecondChar:
                if (HexToInt(c) >= 0)
                {
                    PushChar((HexToInt(percent_first_char) * 16) + HexToInt(c));
                }
                else
                {
                    PushChar('%');
                    PushChar(percent_first_char);
                    PushChar(c);
                }

                state = URLParserState::kIdle;
                break;
            }
        }

        void PercentDecode(const char *encoded, char *decoded)
        {
            decoded_output = decoded;
            decoded_output[0] = '\0';
            char c;
            while ((c = *encoded++))
                ProcessNextChar(c);
        }
    } // web
} // openmc