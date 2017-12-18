#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <deque>
#include <map>
#include <chrono>

enum LoggerType : unsigned int {
    log_none  = 0b000000,

    log_func  = 0b000001,
    log_arg   = 0b000010,
    log_stack = 0b000100,
    log_log   = 0b001000,
    log_warn  = 0b010000,
    log_error = 0b100000,

    log_std   = 0b110100, //standard logging level
    log_all   = 0b111111
};

enum LoggerColor : unsigned int{
    color_nocolor = 0,

    color_black   = 30,
    color_red     = 31,
    color_green   = 32,
    color_yellow  = 33,
    color_blue    = 34,
    color_magenta = 35,
    color_cyan    = 36,
    color_white   = 37
};

class LoggerSettings
{
    public:
        LoggerSettings(unsigned int mask = log_std, bool showColor = true, std::ostream & out = std::cout);

        inline std::ostream & out() const{return m_out;}

        inline void setMask(unsigned int mask){m_mask = mask;}
        inline unsigned int mask() const{return m_mask;}

        inline void setIndentWidth(unsigned int indentWidth){m_indentWidth = indentWidth;}
        inline unsigned int indentWidth() const{return m_indentWidth;}

        inline void toggleColor(){m_printColor = !m_printColor;}
        inline bool usingColor(){return m_printColor;}

        void beginFunction(std::string const& name);
        void endFunction();
        std::string indent() const;
        std::string indent(unsigned int indentLevel) const;

        void printStack() const;

    private:
        std::ostream & m_out;

        bool m_printColor;

        unsigned int m_mask;
        unsigned int m_indentWidth;
        std::deque<std::string> m_stack;
};

extern LoggerSettings defaultLoggerSettings;

class Logger
{
    protected:
        Logger(bool endl, LoggerSettings & settings = defaultLoggerSettings);

    public:
        Logger(LoggerSettings & settings = defaultLoggerSettings);
        ~Logger();
        Logger& operator=(Logger const& rhs) = delete;

        template <typename T>
        Logger& operator<<(T const& rhs)
        {
            m_line << rhs;
            return *this;
        }

    protected:
        inline std::ostream& setColorCode(std::ostream& out, unsigned int colorCode) const
        {
            if(m_settings.usingColor())
                out << "\x1b[" << colorCode << "m";
            return out;
        }

        inline std::ostream& setFGcolor(std::ostream& out, LoggerColor color) const {return setColorCode(out, (unsigned int) color);}
        inline std::ostream& setBGcolor(std::ostream& out, LoggerColor color) const {return setColorCode(out, (unsigned int) color + 10);}

        inline std::ostream& printColoredMessageHeader(std::ostream& out, LoggerColor msgColor, std::string const& header)
        {
            setFGcolor(out, LoggerColor::color_black);
            setBGcolor(out, msgColor);
            out << header << ":";
            setFGcolor(out, LoggerColor::color_nocolor);
            setFGcolor(out, msgColor);
            out << " ";
            return out;
        }

    private:
        LoggerSettings & m_settings;
        bool m_endl;

        LoggerType m_type;

        std::stringstream m_line;
};

template <>
inline Logger& Logger::operator<< <void*> (void* const& rhs)
{
    m_line << "0x" << std::hex << rhs << std::dec;
    return *this;
}

template <>
inline Logger& Logger::operator<< <bool> (bool const& rhs)
{
    m_line << (rhs? "true" : "false");
    return *this;
}

template <>
inline Logger& Logger::operator<< <std::stringstream> (std::stringstream const& rhs)
{
    m_line << rhs.str();
    return *this;
}

template <>
inline Logger& Logger::operator<< <LoggerType> (LoggerType const& rhs)
{
    m_type = rhs;
    return *this;
}

template <>
inline Logger& Logger::operator<< <LoggerColor> (LoggerColor const& rhs)
{
    setFGcolor(m_line, rhs);
    return *this;
}

class FuncTracer
{
    public:
        FuncTracer(std::string const& classname, std::string const& funcname, bool addParenthesis = true, LoggerSettings & settings = defaultLoggerSettings);
        FuncTracer(std::string const& funcname, bool addParenthesis = true, LoggerSettings & settings = defaultLoggerSettings);
        FuncTracer(bool addParenthesis = true, LoggerSettings & settings = defaultLoggerSettings);
        ~FuncTracer();

    private:
        bool m_addParenthesis;
        LoggerSettings & m_settings;
        std::string const m_name;
        std::chrono::high_resolution_clock::time_point m_startTime;
};

template <typename T>
void FuncArg(std::string const& argname, T const& argvalue, LoggerSettings & settings = defaultLoggerSettings)
{
    Logger(settings) << log_arg << argname << " = " << argvalue;
}

inline void _funcArgs(std::deque<std::string> & argnames) {}

template <typename T, typename ... other_T>
void _funcArgs(std::deque<std::string> & argnames, T const& argvalue, other_T const& ... argvalues)
{
    std::string argname = argnames.front();
    argnames.pop_front();

    FuncArg(argname, argvalue);
    _funcArgs(argnames, argvalues...);
}

template <typename ... T>
void FuncArgs(std::string const& argnames, T const& ... argvalues)
{
    std::stringstream argname;
    std::deque<std::string> t_argnames;
    for(unsigned int i = 0; i < argnames.size(); ++i)
    {
        char c = argnames[i];
        if(c == ',')
        {
            t_argnames.push_back(argname.str());
            argname = std::stringstream();
        }
        else if(c != ' ')
        {
            argname << c;
        }
    }
    t_argnames.push_back(argname.str());

    _funcArgs(t_argnames, argvalues...);
}

#ifdef NOLOG
    #define LOG(...)
    #define LOG_ARGS(...)
    #define LOG_WARN //
    #define LOG_ERROR //
#else
    #ifdef NOTRACE
        #define LOG(...)
        #define LOG_ARGS(...)
    #else
        #define LOG(...)\
            FuncTracer _log(BOOST_CURRENT_FUNCTION, false);\
            FuncArgs(#__VA_ARGS__, ##__VA_ARGS__)
        #define LOG_ARGS(...)\
            FuncArgs(#__VA_ARGS__, ##__VA_ARGS__)
    #endif

    #ifdef RELEASE
        #define LOG_WARN\
            Logger() << log_warn
        #define LOG_ERROR\
            Logger() << log_error
    #else
        #define LOG_WARN\
            Logger() << log_warn << __FILE__ << ":" << __LINE__ << " "
        #define LOG_ERROR\
            Logger() << log_error << __FILE__ << ":" << __LINE__ << " "
    #endif
#endif

template <>
void FuncArg(std::string const& argname, std::string const& argvalue, LoggerSettings & settings);
